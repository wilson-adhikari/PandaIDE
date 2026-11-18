#include "AccountService.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrlQuery>

#ifndef PANDAIDE_GITHUB_CLIENT_ID
#define PANDAIDE_GITHUB_CLIENT_ID ""
#endif

namespace Account {

namespace {
const QUrl kDeviceCodeUrl(QStringLiteral("https://github.com/login/device/code"));
const QUrl kTokenUrl(QStringLiteral("https://github.com/login/oauth/access_token"));
const QUrl kUserUrl(QStringLiteral("https://api.github.com/user"));

QNetworkRequest formRequest(const QUrl& url)
{
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/json");
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("PandaIDE"));
    return req;
}

QByteArray form(const QList<QPair<QString, QString>>& fields)
{
    QUrlQuery q;
    for (const auto& f : fields) {
        q.addQueryItem(f.first, f.second);
    }
    return q.query(QUrl::FullyEncoded).toUtf8();
}
} // namespace

AccountService::AccountService(QObject* parent)
    : QObject(parent)
    , m_clientId(qEnvironmentVariable("PANDAIDE_GITHUB_CLIENT_ID", QStringLiteral(PANDAIDE_GITHUB_CLIENT_ID)))
{
    m_poll.setSingleShot(true);
    connect(&m_poll, &QTimer::timeout, this, &AccountService::poll);

    // shortcut: the token (scope read:user, public profile only) lives in QSettings, not the OS keychain;
    // move it to a keychain before asking GitHub for broader scopes such as repo access.
    QSettings s;
    m_token = s.value(QStringLiteral("github/token")).toString();
    m_login = s.value(QStringLiteral("github/login")).toString();
    m_name = s.value(QStringLiteral("github/name")).toString();
    m_avatar = s.value(QStringLiteral("github/avatar")).toString();
    if (!m_token.isEmpty()) {
        fetchUser(); // refresh the profile; signs out if GitHub revoked the token
    }
}

void AccountService::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void AccountService::signIn()
{
    if (!configured()) {
        emit failed(tr("GitHub sign-in is not set up in this build (no OAuth client ID)."));
        return;
    }
    if (m_busy) {
        return;
    }
    setBusy(true);
    QNetworkReply* reply = m_net.post(formRequest(kDeviceCodeUrl),
                                      form({{QStringLiteral("client_id"), m_clientId},
                                            {QStringLiteral("scope"), QStringLiteral("read:user")}}));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
        if (o.value(QStringLiteral("device_code")).toString().isEmpty()) {
            setBusy(false);
            emit failed(o.value(QStringLiteral("error_description"))
                            .toString(tr("Could not reach GitHub: %1").arg(reply->errorString())));
            return;
        }
        m_deviceCode = o.value(QStringLiteral("device_code")).toString();
        m_intervalS = qMax(5, o.value(QStringLiteral("interval")).toInt(5));
        m_expiresAt = QDateTime::currentSecsSinceEpoch() + o.value(QStringLiteral("expires_in")).toInt(900);
        const QString uri = o.value(QStringLiteral("verification_uri")).toString(QStringLiteral("https://github.com/login/device"));
        emit deviceCodeReady(o.value(QStringLiteral("user_code")).toString(), uri);
        QDesktopServices::openUrl(QUrl(uri));
        m_poll.start(m_intervalS * 1000);
    });
}

void AccountService::poll()
{
    if (m_deviceCode.isEmpty()) {
        return;
    }
    if (QDateTime::currentSecsSinceEpoch() > m_expiresAt) {
        cancel();
        emit failed(tr("The sign-in code expired. Try again."));
        return;
    }
    QNetworkReply* reply = m_net.post(formRequest(kTokenUrl),
                                      form({{QStringLiteral("client_id"), m_clientId},
                                            {QStringLiteral("device_code"), m_deviceCode},
                                            {QStringLiteral("grant_type"),
                                             QStringLiteral("urn:ietf:params:oauth:grant-type:device_code")}}));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (m_deviceCode.isEmpty()) {
            return; // cancelled while the request was in flight
        }
        const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
        const QString token = o.value(QStringLiteral("access_token")).toString();
        const QString error = o.value(QStringLiteral("error")).toString();
        if (!token.isEmpty()) {
            m_token = token;
            m_deviceCode.clear();
            fetchUser();
        } else if (error == QStringLiteral("authorization_pending") || error.isEmpty()) {
            m_poll.start(m_intervalS * 1000); // still waiting for the user, or a network blip: ask again
        } else if (error == QStringLiteral("slow_down")) {
            m_intervalS = o.value(QStringLiteral("interval")).toInt(m_intervalS + 5);
            m_poll.start(m_intervalS * 1000);
        } else {
            cancel();
            emit failed(error == QStringLiteral("access_denied") ? tr("Sign-in was cancelled on GitHub.")
                        : error == QStringLiteral("expired_token") ? tr("The sign-in code expired. Try again.")
                        : o.value(QStringLiteral("error_description")).toString(tr("GitHub refused the sign-in (%1).").arg(error)));
        }
    });
}

void AccountService::fetchUser()
{
    QNetworkRequest req(kUserUrl);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("PandaIDE"));
    QNetworkReply* reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status == 401) {
            signOut();
            emit failed(tr("Your GitHub sign-in has ended. Sign in again."));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            setBusy(false); // offline: keep the remembered account
            return;
        }
        const QJsonObject u = QJsonDocument::fromJson(reply->readAll()).object();
        m_login = u.value(QStringLiteral("login")).toString();
        m_name = u.value(QStringLiteral("name")).toString();
        m_avatar = u.value(QStringLiteral("avatar_url")).toString();
        QSettings s;
        s.setValue(QStringLiteral("github/token"), m_token);
        s.setValue(QStringLiteral("github/login"), m_login);
        s.setValue(QStringLiteral("github/name"), m_name);
        s.setValue(QStringLiteral("github/avatar"), m_avatar);
        setBusy(false);
        emit accountChanged();
    });
}

void AccountService::cancel()
{
    m_poll.stop();
    m_deviceCode.clear();
    setBusy(false);
}

void AccountService::signOut()
{
    cancel();
    m_token.clear();
    m_login.clear();
    m_name.clear();
    m_avatar.clear();
    QSettings().remove(QStringLiteral("github"));
    emit accountChanged();
}

} // namespace Account
