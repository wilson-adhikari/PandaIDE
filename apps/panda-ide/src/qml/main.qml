import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
    title: qsTr("PandaIDE v0.1")

    PandaTheme { id: theme }

    // Outer window chrome: full dark gradient, top-lit.
    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: theme.chromeTop }
            GradientStop { position: 1.0; color: theme.chromeBottom }
        }
    }

    property string currentFile: "main.cpp"
    property string currentLanguage: "cpp"
    property bool internalChange: false
    property bool suppressOnce: false
    property bool sideVisible: true
    property bool panelVisible: true
    property bool explorerLoading: true
    property int errorCount: 0
    property int termLines: 0
    property var fileNames: ["main.cpp", "main.py", "fast_math.asm"]
    property var fileLang: ({"main.cpp": "cpp", "main.py": "python", "fast_math.asm": "asm"})
    property var openFiles: ({
        "main.cpp": "#include <iostream>\n\nclass Greeter {\npublic:\n    void hello() {\n        std::cout << \"Hello, PandaIDE\" << std::endl;\n    }\n};\n\nint main() {\n    Greeter g;\n    g.hello();\n    return 0;\n}\n",
        "main.py": "def greet(name):\n    print(f\"Hello, {name}\")\n\n\ndef main():\n    for i in range(3):\n        greet(\"PandaIDE\")\n\n\nif __name__ == \"__main__\":\n    main()\n",
        "fast_math.asm": "section .text\nglobal _start\n\n_start:\n    mov rax, 1\n    mov rdi, 1\n    syscall\n"
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
        openFiles[currentFile] = editor.text
        currentFile = fileNames[index]
        currentLanguage = fileLang[currentFile]
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
        terminalPrint("$ run " + currentFile)
        terminalService.runText(editor.text, currentLanguage, currentFile)
    }

    function runTerminalCommand(cmd) {
        var c = cmd.trim()
        terminalPrint("$ " + c)
        if (c === "") {
            return
        }
        if (c === "help") {
            terminalPrint("a live shell: type anything, e.g. python main.py, g++ --version, dir")
            terminalPrint("builtins: help, clear  |  demo: fail (sample Problems entry)")
            return
        }
        if (c === "clear") {
            terminalLog.clear()
            termLines = 0
            return
        }
        if (c === "fail") {
            terminalPrint("error: use of undeclared identifier 'x'")
            terminalPrint("Build finished: 1 error")
            problemsModel.clear()
            problemsModel.append({ text: "main.cpp:12: use of undeclared identifier 'x'", severity: "error" })
            problemsModel.append({ text: "main.cpp:9: unused variable 'tmp'", severity: "warning" })
            errorCount = 1
            updateProblemsLabel()
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
                text: qsTr("&New Project")
                shortcut: "Ctrl+N"
                onTriggered: projectManager.newProject("Untitled", "")
            }
            Action {
                text: qsTr("Open Project...")
                shortcut: "Ctrl+O"
                onTriggered: projectManager.openProject("")
            }
            MenuSeparator {}
            Action {
                text: qsTr("E&xit")
                shortcut: "Ctrl+Q"
                onTriggered: Qt.quit()
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
            Action {
                text: qsTr("&Debug")
                shortcut: "Ctrl+D"
                onTriggered: terminalPrint("Debugger attached (demo)")
            }
        }
        Menu {
            title: qsTr("&Help")
            Action {
                text: qsTr("&About")
                onTriggered: terminalPrint("PandaIDE v0.1 - word-based suggestions, UACE assembly engine")
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
            ComboBox {
                id: runConfig
                model: ["Debug", "Release"]
                Layout.preferredWidth: 120
                Accessible.name: qsTr("Run configuration")
            }
            Button {
                text: qsTr("Run")
                onClicked: runCurrentFile()
            }
            Button {
                text: qsTr("Debug")
                onClicked: terminalPrint("Debugger attached (demo)")
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
                    // Skeleton shimmer while the (demo) project index loads.
                    ColumnLayout {
                        visible: explorerLoading
                        Layout.fillWidth: true
                        spacing: theme.s2
                        Repeater {
                            model: 6
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 14
                                Layout.leftMargin: theme.s2
                                Layout.rightMargin: theme.s2
                                radius: theme.radiusSm
                                color: theme.hover
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    NumberAnimation { from: 0.35; to: 0.85; duration: theme.motionShimmer }
                                    NumberAnimation { from: 0.85; to: 0.35; duration: theme.motionShimmer }
                                }
                            }
                        }
                    }
                    ListView {
                        id: fileList
                        visible: !explorerLoading
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: fileNames
                        Accessible.name: qsTr("Project files")
                        delegate: ItemDelegate {
                            width: fileList.width
                            text: modelData
                            highlighted: ListView.isCurrentItem
                            onClicked: {
                                fileList.currentIndex = index
                                editorTabs.currentIndex = index
                            }
                        }
                    }
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
                            var q = text.trim().toLowerCase()
                            if (q === "") {
                                searchHint.visible = true
                            } else {
                                searchHint.visible = false
                                var hits = 0
                                for (var i = 0; i < fileNames.length; i++) {
                                    var body = openFiles[fileNames[i]].toLowerCase()
                                    if (body.indexOf(q) !== -1) {
                                        searchModel.append({ text: fileNames[i] + qsTr(": match") })
                                        hits++
                                    }
                                }
                                if (hits === 0) {
                                    searchModel.append({ text: qsTr("No matches for '%1'").arg(text) })
                                }
                            }
                        }
                    }
                    Label {
                        id: searchHint
                        text: qsTr("Type to search open files")
                        color: theme.textMuted
                        font.pixelSize: theme.fontSmall
                        leftPadding: theme.s2
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: ListModel { id: searchModel }
                        Accessible.name: qsTr("Search results")
                        delegate: Label { text: model.text; leftPadding: theme.s2; color: theme.textPrimary }
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("SOURCE CONTROL"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: ["M main.cpp", "M main.qml"]
                        Accessible.name: qsTr("Changed files")
                        delegate: Label { text: modelData; leftPadding: theme.s2; color: theme.textPrimary }
                    }
                    TextField {
                        id: commitBox
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        placeholderText: qsTr("Commit message")
                        Accessible.name: qsTr("Commit message")
                    }
                    Button {
                        text: qsTr("Commit")
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        onClicked: {
                            terminalPrint("Committed: " + (commitBox.text === "" ? qsTr("(empty message)") : commitBox.text))
                            commitBox.clear()
                        }
                    }
                }
                ColumnLayout {
                    spacing: theme.s2
                    Label { text: qsTr("RUN AND DEBUG"); font.bold: true; color: theme.textSecondary; font.pixelSize: theme.fontSmall; leftPadding: theme.s2 }
                    ComboBox {
                        Layout.fillWidth: true
                        Layout.leftMargin: theme.s2
                        Layout.rightMargin: theme.s2
                        model: ["Debug", "Release"]
                        Accessible.name: qsTr("Launch configuration")
                    }
                    Button {
                        text: qsTr("Start")
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
                    TabButton { text: modelData; width: implicitWidth + 24 }
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
                        function onOutputReceived(t) { appendOutput(t) }
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
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: ListModel { id: problemsModel }
                        Accessible.name: qsTr("Problems list")
                        delegate: Label {
                            text: model.text
                            leftPadding: theme.s2
                            color: model.severity === "error" ? theme.error
                                 : model.severity === "warning" ? theme.warning
                                 : theme.textSecondary
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
            Label { text: "main"; color: theme.textSecondary; font.pixelSize: theme.fontSmall }
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
        }
    }

    Timer {
        interval: 900
        running: true
        repeat: false
        onTriggered: explorerLoading = false
    }

    Component.onCompleted: {
        langLabel.text = currentLanguage
        dirLabel.text = terminalService.currentDirectory
        shellCombo.model = terminalService.availableShells()
        shellCombo.currentIndex = shellCombo.indexOfValue(terminalService.currentShell())
        updateCaret()
        updateProblemsLabel()
        refreshOutline()
        console.log("PandaIDE ready")
    }
}
