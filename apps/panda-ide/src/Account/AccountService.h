#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QTimer>

namespace Account {

// "Sign in with GitHub" using the OAuth device flow: no client secret and no local web server, the user just
// enters a short code at github.com/login/device. Needs a GitHub OAuth App with device flow enabled; its public
// client ID comes from the build (-DPANDAIDE_GITHUB_CLIENT_ID=...) or the environment variable of the same name.
class AccountService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured CONSTANT)
    Q_PROPERTY(bool signedIn READ signedIn NOTIFY accountChanged)
    Q_PROPERTY(QString login READ login NOTIFY accountChanged)
    Q_PROPERTY(QString name READ name NOTIFY accountChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY accountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit AccountService(QObject* parent = nullptr);

    bool configured() const { return !m_clientId.isEmpty(); }
    bool signedIn() const { return !m_login.isEmpty(); }
    QString login() const { return m_login; }
    QString name() const { return m_name; }
    QString avatarUrl() const { return m_avatar; }
    bool busy() const { return m_busy; }

    Q_INVOKABLE void signIn();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void signOut();

signals:
    void accountChanged();
    void busyChanged();
    // Show userCode to the user; the browser is opened at verificationUri.
    void deviceCodeReady(const QString& userCode, const QString& verificationUri);
    void failed(const QString& message);

private:
    void poll();
    void fetchUser();
    void setBusy(bool busy);

    QNetworkAccessManager m_net;
    QTimer m_poll;
    QString m_clientId;
    QString m_deviceCode;
    QString m_token;
    QString m_login;
    QString m_name;
    QString m_avatar;
    int m_intervalS = 5;
    qint64 m_expiresAt = 0;
    bool m_busy = false;
};

} // namespace Account
