import QtQuick

// PandaTheme - PandaIDE design-token system.
// Adapted from ReUI/shadcn conventions (semantic tokens, 4pt spacing
// scale, subtle motion) to this Qt Quick desktop shell. All UI
// surfaces must reference these tokens - never raw color literals.
// Supports dark (default, JetBrains-style) and light color schemes.
QtObject {
    id: theme

    property bool dark: true
    function toggle() { dark = !dark }

    // ---- semantic surfaces -------------------------------------------
    property color background: dark ? "#1e1f22" : "#f8f9fa"
    property color surface: dark ? "#23252a" : "#ffffff"
    property color surfaceElevated: dark ? "#2e3138" : "#ffffff"
    property color border: dark ? "#393b40" : "#e4e4e7"
    property color hover: dark ? "#35373d" : "#f4f4f5"
    property color selection: dark ? "#214283" : "#dbe7ff"

    // ---- outer chrome gradient (window, menu, toolbars) -------------------
    // Fully dark, top-lit: chrome fades from a lifted top edge into
    // a deeper bottom edge. Light mode gets the same treatment,
    // barely-there.
    property color chromeTop: dark ? "#2b2e35" : "#ffffff"
    property color chromeBottom: dark ? "#191b1f" : "#ececf0"
    property color railTop: dark ? "#26282e" : "#f4f4f6"
    property color railBottom: dark ? "#1b1d21" : "#e9e9ed"

    // ---- semantic text -------------------------------------------------
    property color textPrimary: dark ? "#bcbec4" : "#18181b"
    property color textSecondary: dark ? "#868a93" : "#71717a"
    property color textMuted: dark ? "#5a5e66" : "#a1a1aa"

    // ---- semantic accents ----------------------------------------------
    property color accent: "#3574f0"
    property color accentText: "#ffffff"
    property color success: dark ? "#499c54" : "#16a34a"
    property color warning: dark ? "#e8a33d" : "#d97706"
    property color error: dark ? "#e06c5b" : "#dc2626"
    property color info: dark ? "#4eade5" : "#0284c7"

    // ---- editor ----------------------------------------------------------
    property color editorBackground: dark ? "#1e1f22" : "#ffffff"
    property color editorCaret: dark ? "#bbbbbb" : "#18181b"
    property string codeFont: "Consolas"

    // ---- spacing scale (4pt rhythm: 4/8/12/16/20/24/32) -------------------
    readonly property int s1: 4
    readonly property int s2: 8
    readonly property int s3: 12
    readonly property int s4: 16
    readonly property int s5: 20
    readonly property int s6: 24
    readonly property int s8: 32

    // ---- shape -------------------------------------------------------------
    readonly property int radiusSm: 4
    readonly property int radiusMd: 6
    readonly property int radiusLg: 8

    // ---- type ----------------------------------------------------------------
    readonly property int fontSmall: 11
    readonly property int fontUi: 12
    readonly property int fontCode: 13
    readonly property int fontTitle: 13

    // ---- motion (subtle; skeleton shimmer only) ------------------------------
    readonly property int motionFast: 120
    readonly property int motionBase: 200
    readonly property int motionSlow: 320
    readonly property int motionShimmer: 700
}
