import QtQuick
// Fusion, not the native style: the Windows and macOS native styles ignore the theme's custom backgrounds.
import QtQuick.Controls.Fusion
import QtQuick.Layouts
import QtQuick.Dialogs

// PandaIDE v0.1 - VS Code / JetBrains style workbench.
// Design system: PandaTheme semantic tokens only (no raw colors),
// 4pt spacing rhythm, one focal point per panel (the editor),
// skeleton loading, empty and error states, subtle motion,
// keyboard-first controls with accessible names.
ApplicationWindow {
    id: window
    visible: true
    width: 1400
    height: 900
    minimumWidth: 1000
    minimumHeight: 640
    title: (projectManager.hasProject ? projectManager.projectName + " - " : "") + "PandaIDE " + Qt.application.version

    PandaTheme { id: theme }

    // Outer window chrome: full dark gradient, top-lit.
    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.chromeTop }
            GradientStop { position: 1.0; color: theme.chromeBottom }
        }
    }

    // Tabs are keyed by absolute path; a bare name ("main.cpp") is an unsaved scratch buffer.
    property string currentFile: "main.cpp"
    property string currentLanguage: "cpp"
    property bool internalChange: false
    property bool suppressOnce: false
    property bool sideVisible: true
    property bool panelVisible: true
    property bool quitting: false
    property int errorCount: 0
    property int termLines: 0
    property var fileNames: ["main.cpp", "main.py", "hello.asm"]
    property var dirty: ({})
    property var projectFiles: []
    property var gitChanges: []
    property string gitBranch: ""
    property bool gitRepo: false
    property var openFiles: ({
        "main.cpp": "#include <iostream>\n\nclass Greeter {\npublic:\n    void hello() {\n        std::cout << \"Hello, PandaIDE\" << std::endl;\n    }\n};\n\nint main() {\n    Greeter g;\n    g.hello();\n    return 0;\n}\n",
        "main.py": "def greet(name):\n    print(f\"Hello, {name}\")\n\n\ndef main():\n    for i in range(3):\n        greet(\"PandaIDE\")\n\n\nif __name__ == \"__main__\":\n    main()\n",
        "hello.asm": "; Exits with code 42. Run assembles it with NASM and links it with gcc,\n; so main is the entry point on Windows, Linux and macOS.\nglobal main\nglobal _main            ; macOS adds a leading underscore\n\nsection .text\nmain:\n_main:\n    mov eax, 42\n    ret\n"
    })

    // ---- editor helpers -------------------------------------------------
    function wordAtCursor() {
        var pos = editor.cursorPosition
        var t = editor.text
        var start = pos
        while (start > 0) {
            var ch = t.charAt(start - 1)
            if ((ch >= "a" && ch <= "z") || (ch >= "A" && ch <= "Z") ||
                (ch >= "0" && ch <= "9") || ch === "_") {
                start--
            } else {
                break
            }
        }
        return { start: start, word: t.substring(start, pos) }
    }

    function currentLineText() {
        var pos = editor.cursorPosition
        var t = editor.text
        var start = t.lastIndexOf("\n", pos - 1) + 1
        return t.substring(start, pos)
    }

    function currentLineIndent() {
        var line = currentLineText()
        var i = 0
        while (i < line.length && (line.charAt(i) === " " || line.charAt(i) === "\t")) {
            i++
        }
        return line.substring(0, i)
    }

    function updateCaret() {
        var parts = editor.text.substring(0, editor.cursorPosition).split("\n")
        caretLabel.text = "Ln " + parts.length + ", Col " + (parts[parts.length - 1].length + 1)
    }

    function updateProblemsLabel() {
        if (errorCount === 0) {
            problemsLabel.text = qsTr("0 problems")
            problemsLabel.color = theme.textSecondary
        } else {
            problemsLabel.text = errorCount + qsTr(" problems")
            problemsLabel.color = theme.error
        }
    }

    function updateSuggestions() {
        if (internalChange || suppressOnce || !editor.activeFocus) {
            suppressOnce = false
            return
        }
        var p = wordAtCursor()
        if (p.word.length < 2) {
            suggestPopup.close()
            return
        }
        var list = completionEngine.suggest(p.word, currentLanguage, editor.text)
        suggestModel.clear()
        for (var i = 0; i < list.length; i++) {
            suggestModel.append({
                word: list[i],
                detail: completionEngine.docFor(list[i], currentLanguage)
            })
        }
        if (suggestModel.count > 0) {
            suggestList.currentIndex = 0
            suggestPopup.x = Math.min(editor.cursorRectangle.x,
                                      Math.max(0, editor.width - suggestPopup.width))
            suggestPopup.y = editor.cursorRectangle.y + editor.cursorRectangle.height
            suggestPopup.open()
        } else {
            suggestPopup.close()
        }
    }

    function acceptSuggestion() {
        if (suggestList.currentIndex < 0 || suggestList.currentIndex >= suggestModel.count) {
            return
        }
        var word = suggestModel.get(suggestList.currentIndex).word
        var p = wordAtCursor()
        internalChange = true
        editor.remove(p.start, editor.cursorPosition)
        var body = completionEngine.snippetFor(word, currentLanguage)
        if (body !== "") {
            var indent = currentLineIndent()
            var out = body.split("\n").join("\n" + indent)
            var caret = out.indexOf("$0")
            out = out.split("$0").join("")
            var pos = editor.cursorPosition
            editor.insert(pos, out)
            editor.cursorPosition = (caret === -1) ? pos + out.length : pos + caret
        } else {
            editor.insert(editor.cursorPosition, word)
        }
        internalChange = false
        suppressOnce = true
        suggestPopup.close()
        editor.forceActiveFocus()
        updateCaret()
        refreshOutline()
    }

    function switchFile(index) {
        if (index < 0 || index >= fileNames.length) {
            return
        }
        openFiles[currentFile] = editor.text
        currentFile = fileNames[index]
        currentLanguage = languageOf(currentFile)
        internalChange = true
        editor.text = openFiles[currentFile]
        editor.cursorPosition = 0
        internalChange = false
        suggestPopup.close()
        langLabel.text = currentLanguage
        updateCaret()
        refreshOutline()
    }

    function refreshOutline() {
        outlineModel.clear()
        var lines = editor.text.split("\n")
        for (var n = 0; n < lines.length; n++) {
            var line = lines[n]
            var m = null
            if (currentLanguage === "python") {
                m = line.match(/^\s*(def|class)\s+([A-Za-z_][A-Za-z0-9_]*)/)
                if (m) {
                    outlineModel.append({ text: m[1] + " " + m[2] })
                }
            } else if (currentLanguage === "asm") {
                m = line.match(/^\s*([A-Za-z_.][\w.]*):/)
                if (m) {
                    outlineModel.append({ text: m[1] })
                }
            } else {
                if (/^\s*(if|for|while|switch|return)\b/.test(line)) {
                    continue
                }
                m = line.match(/([A-Za-z_]\w*)\s*\(/)
                if (m) {
                    outlineModel.append({ text: m[1] + "()" })
                }
            }
        }
        outlineEmpty.visible = outlineModel.count === 0
    }

    function terminalPrint(line) {
        terminalLog.append(line)
    }

    // ---- files and workspace --------------------------------------------------
    function languageOf(name) {
        var ext = name.toLowerCase().split(".").pop()
        if (ext === "py") return "python"
        if (["asm", "s", "nasm", "inc"].indexOf(ext) !== -1) return "asm"
        if (ext === "c") return "c"
        if (["cpp", "cc", "cxx", "h", "hpp", "hh", "hxx"].indexOf(ext) !== -1) return "cpp"
        return "text"
    }
    function baseName(path) { return path.split(/[\\/]/).pop() }
    function isScratch(key) { return key.indexOf("/") === -1 && key.indexOf("\\") === -1 }
    function fileUrl(path) { return "file://" + (path.charAt(0) === "/" ? "" : "/") + path }
    function relativeName(path) {
        var root = projectManager.projectPath
        return root !== "" && path.indexOf(root + "/") === 0 ? path.substring(root.length + 1) : path
    }
    function setDirty(key, value) {
        var d = Object.assign({}, dirty)
        if (value) d[key] = true; else delete d[key]
        dirty = d
    }
    function dirtyCount() { return Object.keys(dirty).length }

    function goToLine(line) {
        var lines = editor.text.split("\n")
        var pos = 0
        for (var n = 0; n < Math.min(line - 1, lines.length); n++) pos += lines[n].length + 1
        editor.cursorPosition = pos
        editor.forceActiveFocus()
    }

    // Open a file (or switch to its tab) and optionally jump to a line.
    function openPath(path, line) {
        var i = fileNames.indexOf(path)
        if (i === -1) {
            var text = projectManager.readFile(path)
            if (text === undefined) {
                terminalPrint("Cannot open " + path + " (missing, binary, or larger than 4 MB)")
                return
            }
            openFiles[currentFile] = editor.text
            openFiles[path] = text
            fileNames = fileNames.concat([path])
            i = fileNames.length - 1
        }
        editorTabs.currentIndex = i
        if (currentFile !== path) switchFile(i)
        if (line > 0) goToLine(line)
    }

    function newFile() {
        var n = 1
        while (fileNames.indexOf("untitled-" + n + ".txt") !== -1) n++
        var key = "untitled-" + n + ".txt"
        openFiles[currentFile] = editor.text
        openFiles[key] = ""
        fileNames = fileNames.concat([key])
        editorTabs.currentIndex = fileNames.length - 1
    }

    function saveCurrent() {
        openFiles[currentFile] = editor.text
        if (isScratch(currentFile)) {
            saveDialog.open()
        } else {
            writeTab(currentFile, currentFile)
        }
    }

    // Write a tab to `path`; a scratch buffer (or Save As) is renamed to the new path.
    function writeTab(key, path) {
        if (!projectManager.writeFile(path, openFiles[key])) {
            terminalPrint("Could not save " + path)
            return false
        }
        if (key !== path) {
            var i = fileNames.indexOf(key)
            openFiles[path] = openFiles[key]
            delete openFiles[key]
            setDirty(key, false)
            currentFile = path
            currentLanguage = languageOf(path)
            langLabel.text = currentLanguage
            var names = fileNames.slice()
            names[i] = path
            fileNames = names
            editorTabs.currentIndex = i
        }
        setDirty(path, false)
        terminalPrint("Saved " + path)
        refreshWorkspace()
        return true
    }

    function closeTab(force) {
        if (!force && dirty[currentFile]) {
            confirmDialog.ask(qsTr("Discard unsaved changes to %1?").arg(baseName(currentFile)), function() { closeTab(true) })
            return
        }
        if (fileNames.length === 1) newFile()
        var i = fileNames.indexOf(currentFile)
        var names = fileNames.slice()
        names.splice(i, 1)
        var next = names[Math.min(i, names.length - 1)]
        setDirty(currentFile, false)
        delete openFiles[currentFile]
        // Show the next tab before the model changes, so the closed text is never written into it.
        internalChange = true
        currentFile = next
        currentLanguage = languageOf(next)
        editor.text = openFiles[next]
        editor.cursorPosition = 0
        internalChange = false
        langLabel.text = currentLanguage
        fileNames = names
        editorTabs.currentIndex = names.indexOf(next)
    }

    function openWorkspace(path) {
        if (!projectManager.openProject(path)) {
            terminalPrint("Cannot open folder " + path)
            return
        }
        terminalService.setWorkingDirectory(path)
        refreshWorkspace()
        sideTabs.currentIndex = 0
        sideVisible = true
    }
    function refreshWorkspace() {
        projectFiles = projectManager.listFiles()
        refreshGit()
    }
    function refreshGit() {
        gitRepo = projectManager.hasProject && projectManager.isGitRepo()
        gitBranch = gitRepo ? projectManager.gitBranch() : ""
        gitChanges = gitRepo ? projectManager.gitStatus() : []
    }

    // gcc/clang/nasm "file:line[:col]: error|warning: message" lines become Problems entries.
    function collectProblems(t) {
        var lines = t.split("\n")
        for (var i = 0; i < lines.length; i++) {
            var m = lines[i].match(/^(.*?):(\d+):(?:(\d+):)?\s*(fatal error|error|warning):\s*(.*)$/)
            if (!m) continue
            var severity = m[4] === "warning" ? "warning" : "error"
            problemsModel.append({ text: baseName(m[1]) + ":" + m[2] + ": " + m[5], severity: severity, path: m[1], line: parseInt(m[2]) })
            if (severity === "error") errorCount++
        }
        updateProblemsLabel()
    }
    function openProblem(path, line) {
        // Scratch buffers run from a temporary copy; jump to their tab, not the copy.
        var name = baseName(path)
        openPath(fileNames.indexOf(path) === -1 && fileNames.indexOf(name) !== -1 ? name : path, line)
    }

    function startSignIn() {
        if (!account.configured) {
            terminalPrint("GitHub sign-in is not set up in this build (see README: GitHub sign-in).")
            return
        }
        signInDialog.code = ""
        signInDialog.open()
        account.signIn()
    }

    // Backend output arrives in chunks; keep the log line-oriented
    // and cap the buffer so a runaway process cannot grow it forever.
    function appendOutput(t) {
        var lines = t.split("\n")
        for (var i = 0; i < lines.length; i++) {
            if (i === lines.length - 1 && lines[i] === "") {
                continue
            }
            terminalPrint(lines[i])
            termLines++
        }
        if (termLines > 2000) {
            terminalLog.clear()
            termLines = 0
            terminalPrint("(output truncated)")
        }
    }

    // Run the current editor buffer for real (python/g++ via the
    // TerminalService backend) and reveal the terminal panel.
    function runCurrentFile() {
        openFiles[currentFile] = editor.text
        panelVisible = true
        panelTabs.currentIndex = 0
        problemsModel.clear()
        errorCount = 0
        updateProblemsLabel()
        terminalPrint("$ run " + baseName(currentFile))
        if (isScratch(currentFile)) {
            terminalService.runText(editor.text, currentLanguage, currentFile)
        } else if (!dirty[currentFile] || writeTab(currentFile, currentFile)) {
            terminalService.runFile(currentFile, currentLanguage) // saved first, so Run uses what is on screen
        }
    }

    function runTerminalCommand(cmd) {
        var c = cmd.trim()
        terminalPrint("$ " + c)
        if (c === "") {
            return
        }
        if (c === "help") {
            terminalPrint("a live shell: type anything, e.g. python main.py, g++ --version, dir")
            terminalPrint("builtins: help, clear")
            return
        }
        if (c === "clear") {
            terminalLog.clear()
            termLines = 0
            return
        }
        // Forward to the live shell (PowerShell/CMD/bash). `cd`
        // also nudges the directory label when the target exists.
        var low = c.toLowerCase()
        if (low === "cd" || low.indexOf("cd ") === 0) {
            var arg = c.substring(2).trim()
            if (arg.length >= 2 &&
                ((arg.charAt(0) === '"' && arg.charAt(arg.length - 1) === '"') ||
                 (arg.charAt(0) === "'" && arg.charAt(arg.length - 1) === "'"))) {
                arg = arg.substring(1, arg.length - 1)
            }
            terminalService.writeInput(c)
            terminalService.noteCd(arg)
            return
        }
        terminalService.writeInput(c)
    }

    // ---- menu / tool bars ------------------------------------------------
    menuBar: MenuBar {
        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: theme.chromeTop }
                GradientStop { position: 1.0; color: theme.chromeBottom }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: theme.border
            }
        }
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&New File")
                shortcut: "Ctrl+N"
                onTriggered: newFile()
            }
            Action {
                text: qsTr("&Open File...")
                shortcut: "Ctrl+O"
                onTriggered: openFileDialog.open()
            }
            Action {
                text: qsTr("Open &Folder...")
                shortcut: "Ctrl+Shift+O"
                onTriggered: folderDialog.open()
            }
            MenuSeparator {}
            Action {
                text: qsTr("&Save")
                shortcut: "Ctrl+S"
                onTriggered: saveCurrent()
            }
            Action {
                text: qsTr("Save &As...")
                shortcut: "Ctrl+Shift+S"
                onTriggered: {
                    openFiles[currentFile] = editor.text
                    saveDialog.open()
                }
            }
            Action {
                text: qsTr("&Close Tab")
                shortcut: "Ctrl+W"
                onTriggered: closeTab(false)
            }
            MenuSeparator {}
            Action {
                text: qsTr("E&xit")
                shortcut: "Ctrl+Q"
                onTriggered: window.close()
            }
        }
        Menu {
            title: qsTr("&Edit")
            Action {
                text: qsTr("Undo")
                shortcut: "Ctrl+Z"
                onTriggered: editor.undo()
            }
            Action {
                text: qsTr("Redo")
                shortcut: "Ctrl+Y"
                onTriggered: editor.redo()
            }
        }
        Menu {
            title: qsTr("&View")
            Action {
                text: qsTr("Toggle Sidebar")
                checkable: true
                checked: sideVisible
                onTriggered: sideVisible = checked
            }
            Action {
                text: qsTr("Toggle Panel")
                checkable: true
                checked: panelVisible
                onTriggered: panelVisible = checked
            }
            MenuSeparator {}
            Action {
                text: qsTr("Dark Mode")
                checkable: true
                checked: theme.dark
                onTriggered: theme.toggle()
            }
        }
        Menu {
            title: qsTr("&Run")
            Action {
                text: qsTr("&Run")
                shortcut: "Ctrl+R"
                onTriggered: runCurrentFile()
            }
        }
        Menu {
            title: qsTr("&Account")
            Action {
                text: account.signedIn ? qsTr("Signed in as @%1").arg(account.login) : qsTr("Sign in with &GitHub...")
                enabled: !account.signedIn && !account.busy
                onTriggered: startSignIn()
            }
            Action {
                text: qsTr("Sign &out")
                enabled: account.signedIn
                onTriggered: account.signOut()
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: terminalPrint("PandaIDE " + Qt.application.version + " - free and open source (MIT), https://github.com/wilson-adhikari/PandaIDE")
            }
        }
    }

    header: ToolBar {
        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: theme.chromeTop }
                GradientStop { position: 1.0; color: theme.surface }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: theme.border
            }
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: theme.s2
            anchors.rightMargin: theme.s2
            spacing: theme.s2
            Button {
                text: qsTr("Open Folder")
                onClicked: folderDialog.open()
            }
            Button {
                text: qsTr("Save")
                enabled: !!dirty[currentFile] || isScratch(currentFile)
                onClicked: saveCurrent()
            }
            Button {
                text: qsTr("Run")
                onClicked: runCurrentFile()
            }
            TextField {
                id: globalSearch
                placeholderText: qsTr("Search files (Ctrl+P)")
                Layout.fillWidth: true
                Accessible.name: qsTr("Global file search")
                onAccepted: {
                    sideTabs.currentIndex = 1
                    sideVisible = true
                    searchQuery.text = text
                    searchQuery.accepted()
                }
            }
        }
    }

    // ---- workbench body ---------------------------------------------------
    SplitView {
        anchors.fill: parent

        // Activity bar: single-purpose icon rail on gradient chrome.
        Item {
            SplitView.preferredWidth: 52
            SplitView.minimumWidth: 52
            SplitView.maximumWidth: 52
            Rectangle {
                anchors.fill: parent
                gradient: Gradient {
                    GradientStop { position: 0.0; color: theme.railTop }
                    GradientStop { position: 1.0; color: theme.railBottom }
                }
            }
            ColumnLayout {
            anchors.fill: parent
            spacing: 2
            Button { text: qsTr("Files"); checkable: true; checked: sideTabs.currentIndex === 0; Layout.fillWidth: true; Accessible.name: qsTr("Explorer"); Accessible.description: qsTr("Show project files and outline"); onClicked: { if (sideTabs.currentIndex === 0) sideVisible = !sideVisible; else { sideTabs.currentIndex = 0; sideVisible = true } } }
            Button { text: qsTr("Find"); checkable: true; checked: sideTabs.currentIndex === 1; Layout.fillWidth: true; Accessible.name: qsTr("Search"); Accessible.description: qsTr("Search across files"); onClicked: { if (sideTabs.currentIndex === 1) sideVisible = !sideVisible; else { sideTabs.currentIndex = 1; sideVisible = true } } }
            Button { text: qsTr("Git"); checkable: true; checked: sideTabs.currentIndex === 2; Layout.fillWidth: true; Accessible.name: qsTr("Source control"); Accessible.description: qsTr("Review changes and commit"); onClicked: { if (sideTabs.currentIndex === 2) sideVisible = !sideVisible; else { sideTabs.currentIndex = 2; sideVisible = true } } }
            Button { text: qsTr("Run"); checkable: true; checked: sideTabs.currentIndex === 3; Layout.fillWidth: true; Accessible.name: qsTr("Run and debug"); Accessible.description: qsTr("Launch configurations"); onClicked: { if (sideTabs.currentIndex === 3) sideVisible = !sideVisible; else { sideTabs.currentIndex = 3; sideVisible = true } } }
            Button { text: qsTr("Ext"); checkable: true; checked: sideTabs.currentIndex === 4; Layout.fillWidth: true; Accessible.name: qsTr("Extensions"); Accessible.description: qsTr("Installed language support"); onClicked: { if (sideTabs.currentIndex === 4) sideVisible = !sideVisible; else { sideTabs.currentIndex = 4; sideVisible = true } } }
            Item { Layout.fillHeight: true }
            }
        }

        // Sidebar on flat surface chrome with a separator edge.
        Item {
            visible: sideVisible
            SplitView.preferredWidth: 250
            SplitView.minimumWidth: 180
            Rectangle {
                anchors.fill: parent
                color: theme.surface
            }
            Rectangle {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                width: 1
                color: theme.border
            }
            ColumnLayout {
            anchors.fill: parent
            spacing: 0
            TabBar {
                id: sideTabs
                Layout.fillWidth: true
                TabButton { text: qsTr("Explorer") }
                TabButton { text: qsTr("Search") }
                TabButton { text: qsTr("Git") }
                TabButton { text: qsTr("Run") }
                TabButton { text: qsTr("Extensions") }
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sideTabs.currentIndex
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("EXPLORER"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    Label {
                        visible: !projectManager.hasProject
                        text: qsTr("No folder open. The tabs on the right are unsaved samples.")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        leftPadding: theme.s2
                        rightPadding: theme.s2
                    }
                    Button {
                        visible: !projectManager.hasProject
                        text: qsTr("Open Folder...")
                        Layout.leftMargin: theme.s2
                        onClicked: folderDialog.open()
                    }
                    ListView {
                        id: fileList
                        visible: projectManager.hasProject
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: projectFiles
                        Accessible.name: qsTr("Project files")
                        delegate: ItemDelegate {
                            width: fileList.width
                            text: relativeName(modelData)
                            highlighted: modelData === currentFile
                            onClicked: openPath(modelData, 0)
                        }
                    }
                    Item { visible: !projectManager.hasProject; Layout.fillHeight: true }
                    Label { text: qsTr("OUTLINE"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 140
                        model: ListModel { id: outlineModel }
                        Accessible.name: qsTr("Symbol outline")
                        delegate: Label { text: model.text; leftPadding: theme.s2; color: theme.textPrimary }
                    }
                    Label {
                        id: outlineEmpty
                        visible: false
                        text: qsTr("No symbols in this file")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        leftPadding: theme.s2
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    TextField {
                        id: searchQuery
                        Layout.fillWidth: true
                        Layout.topMargin: theme.s2
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        placeholderText: qsTr("Search text...")
                        Accessible.name: qsTr("Text search")
                        onAccepted: {
                            searchModel.clear()
                            var q = text.trim()
                            searchHint.visible = q === ""
                            if (q === "") {
                                return
                            }
                            openFiles[currentFile] = editor.text
                            var hits = projectManager.hasProject ? projectManager.searchFiles(q) : []
                            // Unsaved scratch buffers are not on disk; search them here.
                            for (var i = 0; i < fileNames.length; i++) {
                                if (!isScratch(fileNames[i])) continue
                                var lines = openFiles[fileNames[i]].split("\n")
                                for (var n = 0; n < lines.length; n++) {
                                    if (lines[n].toLowerCase().indexOf(q.toLowerCase()) !== -1) {
                                        hits.push({ path: fileNames[i], line: n + 1, text: lines[n].trim() })
                                    }
                                }
                            }
                            for (var h = 0; h < hits.length; h++) {
                                searchModel.append({ path: hits[h].path, line: hits[h].line,
                                                     text: relativeName(hits[h].path) + ":" + hits[h].line + "  " + hits[h].text })
                            }
                            if (hits.length === 0) {
                                searchModel.append({ path: "", line: 0, text: qsTr("No matches for '%1'").arg(q) })
                            }
                        }
                    }
                    Label {
                        id: searchHint
                        text: qsTr("Type and press Enter to search the folder")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        leftPadding: theme.s2
                    }
                    ListView {
                        id: searchList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel { id: searchModel }
                        Accessible.name: qsTr("Search results")
                        delegate: ItemDelegate {
                            width: searchList.width
                            text: model.text
                            enabled: model.path !== ""
                            onClicked: openPath(model.path, model.line)
                        }
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("SOURCE CONTROL"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    Label {
                        visible: !gitRepo
                        text: projectManager.hasProject ? qsTr("This folder is not a Git repository.") : qsTr("Open a folder to use Git.")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        leftPadding: theme.s2
                    }
                    Label {
                        visible: gitRepo && gitChanges.length === 0
                        text: qsTr("No changes on %1").arg(gitBranch)
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        leftPadding: theme.s2
                    }
                    ListView {
                        visible: gitRepo
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: gitChanges
                        Accessible.name: qsTr("Changed files")
                        delegate: Label { text: modelData; leftPadding: theme.s2; color: theme.textPrimary; font.family: theme.codeFont }
                    }
                    TextField {
                        id: commitBox
                        visible: gitRepo
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        placeholderText: qsTr("Commit message")
                        Accessible.name: qsTr("Commit message")
                    }
                    Button {
                        visible: gitRepo
                        text: qsTr("Commit all changes")
                        enabled: gitChanges.length > 0 && commitBox.text.trim() !== ""
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        onClicked: {
                            if (dirtyCount() > 0) terminalPrint("Note: unsaved editor changes are not included; save first to commit them.")
                            terminalPrint(projectManager.gitCommit(commitBox.text))
                            commitBox.clear()
                            refreshGit()
                        }
                    }
                    Button {
                        visible: gitRepo
                        text: qsTr("Refresh")
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        onClicked: refreshGit()
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("RUN"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    Label {
                        text: qsTr("Runs the current file: Python, C/C++ (g++ or clang++), or assembly (NASM .asm or GNU .s, linked with gcc).")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        leftPadding: theme.s2
                        rightPadding: theme.s2
                    }
                    Button {
                        text: qsTr("Run current file")
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        onClicked: runCurrentFile()
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("EXTENSIONS"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: ["C/C++ (built-in)", "Python (built-in)", "Assembly UACE (built-in)"]
                        Accessible.name: qsTr("Installed extensions")
                        delegate: Label { text: modelData; leftPadding: theme.s2; color: theme.textPrimary }
                    }
                }
            }
            }
        }

        // Main column: editor tabs, editor (the focal point), panel.
        ColumnLayout {
            SplitView.fillWidth: true
            SplitView.fillHeight: true
            SplitView.minimumWidth: 320
            spacing: 0
            TabBar {
                id: editorTabs
                Layout.fillWidth: true
                Repeater {
                    model: fileNames
                    TabButton {
                        text: baseName(modelData) + (dirty[modelData] ? "  ●" : "")
                        width: implicitWidth + 24
                        Accessible.name: baseName(modelData) + (dirty[modelData] ? qsTr(", unsaved") : "")
                    }
                }
                onCurrentIndexChanged: switchFile(currentIndex)
            }
            TextArea {
                id: editor
                Layout.fillWidth: true
                Layout.fillHeight: true
                text: openFiles[currentFile]
                font.family: theme.codeFont
                font.pixelSize: theme.fontCode
                color: theme.textPrimary
                selectionColor: theme.selection
                wrapMode: TextEdit.NoWrap
                selectByMouse: true
                tabStopDistance: 32
                Accessible.name: qsTr("Code editor")
                Accessible.description: qsTr("Edit code with word-based suggestions. Tab accepts, Enter keeps indentation.")
                background: Rectangle { color: theme.editorBackground }
                onTextChanged: {
                    // Typed edits differ from the buffer last loaded or saved; switching tabs does not.
                    if (!dirty[currentFile] && text !== openFiles[currentFile]) {
                        setDirty(currentFile, true)
                    }
                    updateCaret()
                    refreshOutline()
                    if (!internalChange) {
                        updateSuggestions()
                    }
                }
                onCursorPositionChanged: updateCaret()
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Escape && suggestPopup.visible) {
                        suggestPopup.close()
                        event.accepted = true
                        return
                    }
                    if (suggestPopup.visible &&
                        (event.key === Qt.Key_Down || event.key === Qt.Key_Up)) {
                        var n = suggestModel.count
                        if (n > 0) {
                            var i = suggestList.currentIndex + (event.key === Qt.Key_Down ? 1 : -1)
                            suggestList.currentIndex = (i + n) % n
                        }
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Tab) {
                        if (suggestPopup.visible && suggestModel.count > 0) {
                            acceptSuggestion()
                        } else {
                            editor.insert(editor.cursorPosition,
                                          completionEngine.tabString(currentLanguage))
                            updateCaret()
                        }
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                        if (suggestPopup.visible && suggestModel.count > 0) {
                            acceptSuggestion()
                            event.accepted = true
                            return
                        }
                        var indent = completionEngine.indentForNewLine(
                            currentLineText(), currentLanguage)
                        internalChange = true
                        editor.insert(editor.cursorPosition, "\n" + indent)
                        internalChange = false
                        updateCaret()
                        event.accepted = true
                        return
                    }
                }

                Popup {
                    id: suggestPopup
                    width: 340
                    height: Math.min(232, suggestModel.count * 30 + 8)
                    focus: false
                    modal: false
                    closePolicy: Popup.NoAutoClose
                    enter: Transition {
                        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: theme.motionFast }
                    }
                    exit: Transition {
                        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: theme.motionFast }
                    }
                    background: Rectangle {
                        color: theme.surfaceElevated
                        border.color: theme.border
                        radius: theme.radiusMd
                    }
                    contentItem: ListView {
                        id: suggestList
                        anchors.fill: parent
                        clip: true
                        model: ListModel { id: suggestModel }
                        Accessible.name: qsTr("Code suggestions")
                        highlight: Rectangle { color: theme.selection }
                        delegate: ItemDelegate {
                            width: suggestList.width
                            height: 30
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: theme.s2
                                anchors.rightMargin: theme.s2
                                spacing: theme.s2
                                Label {
                                    text: model.word
                                    font.family: theme.codeFont
                                    color: theme.textPrimary
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    text: model.detail
                                    color: theme.textSecondary
                                    font.pixelSize: theme.fontSmall
                                }
                            }
                            onClicked: {
                                suggestList.currentIndex = index
                                acceptSuggestion()
                            }
                        }
                    }
                }
            }
            TabBar {
                id: panelTabs
                Layout.fillWidth: true
                TabButton { text: qsTr("Terminal") }
                TabButton { text: qsTr("Problems") }
            }
            StackLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: panelVisible ? 170 : 0
                visible: panelVisible
                currentIndex: panelTabs.currentIndex
                ColumnLayout {
                    spacing: 0
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: theme.s2
                        ComboBox {
                            id: shellCombo
                            Layout.preferredWidth: 140
                            Accessible.name: qsTr("Terminal shell")
                            Accessible.description: qsTr("Pick PowerShell, Command Prompt, or a Unix shell")
                            onActivated: function(index) {
                                terminalService.startShell(model[index])
                            }
                        }
                        Label {
                            id: dirLabel
                            text: "..."
                            color: theme.textSecondary
                            font.pixelSize: theme.fontSmall
                            font.family: theme.codeFont
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                            Accessible.role: Accessible.StaticText
                            Accessible.name: qsTr("Terminal working directory")
                        }
                        Button {
                            text: qsTr("Stop")
                            Accessible.name: qsTr("Stop running process")
                            Accessible.description: qsTr("Terminate the process currently running in the terminal")
                            onClicked: terminalService.stop()
                        }
                    }
                    TextArea {
                        id: terminalLog
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        readOnly: true
                        font.family: theme.codeFont
                        font.pixelSize: theme.fontCode - 1
                        color: theme.textPrimary
                        text: "PandaIDE terminal - type 'help'\n"
                        Accessible.name: qsTr("Terminal output")
                        background: Rectangle { color: theme.background }
                    }
                    TextField {
                        id: terminalInput
                        Layout.fillWidth: true
                        placeholderText: qsTr("$ type a command, Enter to run (e.g. python main.py)")
                        font.family: theme.codeFont
                        Accessible.name: qsTr("Terminal input")
                        onAccepted: {
                            runTerminalCommand(text)
                            clear()
                        }
                    }
                    Connections {
                        target: terminalService
                        function onOutputReceived(t) {
                            appendOutput(t)
                            collectProblems(t)
                        }
                        function onFinished(code) { terminalPrint("[exit " + code + "]") }
                        function onCurrentDirectoryChanged(dir) { dirLabel.text = dir }
                        function onCurrentShellChanged(name) {
                            var i = shellCombo.indexOfValue(name)
                            if (i >= 0) {
                                shellCombo.currentIndex = i
                            }
                        }
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label {
                        id: problemsEmpty
                        visible: problemsModel.count === 0
                        text: qsTr("No problems")
                        color: theme.success
                        font.pixelSize: theme.fontSmall
                        leftPadding: theme.s2
                    }
                    ListView {
                        id: problemsList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel { id: problemsModel }
                        Accessible.name: qsTr("Problems list")
                        delegate: ItemDelegate {
                            width: problemsList.width
                            onClicked: openProblem(model.path, model.line)
                            contentItem: Label {
                                text: model.text
                                color: model.severity === "error" ? theme.error
                                     : model.severity === "warning" ? theme.warning
                                     : theme.textSecondary
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }

    footer: ToolBar {
        background: Rectangle {
            gradient: Gradient {
                GradientStop { position: 0.0; color: theme.surface }
                GradientStop { position: 1.0; color: theme.chromeBottom }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: theme.border
            }
        }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: theme.s2
            anchors.rightMargin: theme.s2
            spacing: theme.s3
            Label {
                visible: gitBranch !== ""
                text: "⎇ " + gitBranch
                color: theme.textSecondary
                font.pixelSize: theme.fontSmall
                Accessible.name: qsTr("Git branch %1").arg(gitBranch)
            }
            Label {
                id: problemsLabel
                text: qsTr("0 problems")
                color: theme.textSecondary
                font.pixelSize: theme.fontSmall
                Accessible.role: Accessible.StaticText
                Accessible.name: qsTr("Problem count")
            }
            Item { Layout.fillWidth: true }
            Label {
                id: langLabel
                text: currentLanguage
                color: theme.textSecondary
                font.pixelSize: theme.fontSmall
            }
            Label {
                id: caretLabel
                text: "Ln 1, Col 1"
                color: theme.textSecondary
                font.pixelSize: theme.fontSmall
            }
            Label { text: qsTr("Spaces: 4"); color: theme.textSecondary; font.pixelSize: theme.fontSmall }
            Label { text: "UACE " + uace.version; color: theme.textSecondary; font.pixelSize: theme.fontSmall }
            Image {
                visible: account.signedIn
                source: account.avatarUrl
                sourceSize.width: 16
                sourceSize.height: 16
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
            }
            Button {
                flat: true
                text: account.signedIn ? "@" + account.login : qsTr("Sign in with GitHub")
                font.pixelSize: theme.fontSmall
                Accessible.name: account.signedIn ? qsTr("GitHub account %1, click to sign out").arg(account.login) : qsTr("Sign in with GitHub")
                onClicked: account.signedIn ? accountMenu.popup() : startSignIn()
                Menu {
                    id: accountMenu
                    MenuItem { text: qsTr("Sign out"); onTriggered: account.signOut() }
                }
            }
        }
    }

    // ---- dialogs -----------------------------------------------------------
    FolderDialog {
        id: folderDialog
        title: qsTr("Open Folder")
        onAccepted: openWorkspace(projectManager.localPath(selectedFolder))
    }
    FileDialog {
        id: openFileDialog
        title: qsTr("Open File")
        currentFolder: projectManager.hasProject ? fileUrl(projectManager.projectPath) : ""
        onAccepted: openPath(projectManager.localPath(selectedFile), 0)
    }
    FileDialog {
        id: saveDialog
        title: qsTr("Save As")
        fileMode: FileDialog.SaveFile
        currentFolder: projectManager.hasProject ? fileUrl(projectManager.projectPath) : ""
        onAccepted: writeTab(currentFile, projectManager.localPath(selectedFile))
    }
    Dialog {
        id: confirmDialog
        property var onYes: null
        function ask(message, yes) {
            confirmText.text = message
            onYes = yes
            open()
        }
        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Unsaved changes")
        standardButtons: Dialog.Yes | Dialog.No
        Label { id: confirmText; wrapMode: Text.WordWrap }
        onAccepted: if (onYes) onYes()
    }
    Dialog {
        id: signInDialog
        property string code: ""
        property string uri: "https://github.com/login/device"
        anchors.centerIn: Overlay.overlay
        modal: true
        title: qsTr("Sign in with GitHub")
        standardButtons: Dialog.Cancel
        onRejected: account.cancel()
        ColumnLayout {
            spacing: theme.s2
            Label { text: signInDialog.code === "" ? qsTr("Contacting GitHub...") : qsTr("Enter this code at %1").arg(signInDialog.uri) }
            TextField {
                id: codeField
                visible: signInDialog.code !== ""
                text: signInDialog.code
                readOnly: true
                selectByMouse: true
                font.family: theme.codeFont
                font.pixelSize: 22
                Accessible.name: qsTr("Device code")
            }
            RowLayout {
                visible: signInDialog.code !== ""
                Button { text: qsTr("Copy code"); onClicked: { codeField.selectAll(); codeField.copy() } }
                Button { text: qsTr("Open GitHub"); onClicked: Qt.openUrlExternally(signInDialog.uri) }
            }
            Label {
                text: qsTr("PandaIDE only asks to read your public profile.")
                color: theme.textSecondary
                font.pixelSize: theme.fontSmall
            }
        }
    }
    Connections {
        target: account
        function onDeviceCodeReady(code, uri) {
            signInDialog.code = code
            signInDialog.uri = uri
        }
        function onAccountChanged() {
            if (account.signedIn && signInDialog.visible) {
                signInDialog.close()
                terminalPrint("Signed in to GitHub as @" + account.login)
            }
        }
        function onFailed(message) {
            signInDialog.close()
            terminalPrint("GitHub sign-in: " + message)
        }
    }

    onClosing: function(close) {
        openFiles[currentFile] = editor.text
        if (!quitting && dirtyCount() > 0) {
            close.accepted = false
            confirmDialog.ask(qsTr("%1 file(s) have unsaved changes. Quit and discard them?").arg(dirtyCount()), function() {
                quitting = true
                window.close()
            })
        }
    }

    Component.onCompleted: {
        langLabel.text = currentLanguage
        dirLabel.text = terminalService.currentDirectory
        shellCombo.model = terminalService.availableShells()
        shellCombo.currentIndex = shellCombo.indexOfValue(terminalService.currentShell)
        updateCaret()
        updateProblemsLabel()
        refreshOutline()
        var last = projectManager.lastProject()
        if (last !== "") {
            openWorkspace(last)
        }
    }
}
