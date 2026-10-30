import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.plasma5support as Plasma5Support
import org.kde.kirigami as Kirigami

PlasmoidItem {
    id: root

    // "integrated" | "hybrid" | "nvidia" | "unknown"
    property string mode: "unknown"
    property bool busy: false
    property string message: ""

    readonly property var modes: [
        { id: "integrated", label: i18n("Integrated"), icon: Qt.resolvedUrl("../images/integrated.png"),
          cmd: "envycontrol -s integrated" },
        { id: "hybrid", label: i18n("Hybrid"), icon: Qt.resolvedUrl("../images/hybrid.png"),
          cmd: "envycontrol -s hybrid --rtd3 2" },
        { id: "nvidia", label: i18n("Nvidia"), icon: Qt.resolvedUrl("../images/nvidia.png"),
          cmd: "envycontrol -s nvidia" }
    ]

    function modeLabel(id) {
        for (const m of modes) if (m.id === id) return m.label
        return i18n("Unknown")
    }
    function modeIcon(id) {
        for (const m of modes) if (m.id === id) return m.icon
        return ""
    }

    function parseQuery(out) {
        const s = out.toLowerCase()
        if (s.includes("nvidia")) return "nvidia"
        if (s.includes("integrated") || s.includes("intel")) return "integrated"
        if (s.includes("hybrid") || s.includes("switchable")) return "hybrid"
        return "unknown"
    }

    function refresh() {
        executable.exec("query", "envycontrol --query")
    }

    function switchTo(m) {
        if (busy || m.id === mode) return
        busy = true
        message = ""
        // Prefer passwordless sudo, fall back to a graphical polkit prompt.
        executable.exec("switch:" + m.id,
            "sudo -n " + m.cmd + " || pkexec " + m.cmd)
    }

    Plasma5Support.DataSource {
        id: executable
        engine: "executable"
        connectedSources: []

        onNewData: (source, data) => {
            const kind = pending[source] || ""
            delete pending[source]
            const out = (data["stdout"] || "") + (data["stderr"] || "")
            const code = data["exit code"]
            disconnectSource(source)

            if (kind === "query") {
                if (code === 0) root.mode = root.parseQuery(out)
            } else if (kind.startsWith("switch:")) {
                root.busy = false
                if (code === 0) {
                    root.message = i18n("Switched to %1. Reboot to apply.", root.modeLabel(kind.substring(7)))
                } else {
                    root.message = i18n("Switch failed: %1", out.trim() || i18n("cancelled"))
                }
                root.refresh()
            }
        }

                // The source string IS the shell command, so uniqueness is added as a trailing comment.
        property int counter: 0
        property var pending: ({})
        function exec(kind, cmd) {
            const src = "LC_ALL=C " + cmd + " # " + (counter++)
            pending[src] = kind
            connectSource(src)
        }
    }

    Timer {
        interval: 5000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: if (!root.busy) root.refresh()
    }

    Plasmoid.icon: modeIcon(mode) !== "" ? modeIcon(mode) : "video-display"
    toolTipMainText: i18n("GPU Switcher")
    toolTipSubText: busy ? i18n("Switching…") : i18n("Current GPU: %1", modeLabel(mode))

    compactRepresentation: MouseArea {
        id: compact
        activeFocusOnTab: true
        hoverEnabled: true
        onClicked: root.expanded = !root.expanded

        Kirigami.Icon {
            anchors.fill: parent
            source: root.modeIcon(root.mode) !== "" ? root.modeIcon(root.mode) : "video-display"
            active: compact.containsMouse
            opacity: root.busy ? 0.5 : 1
        }
    }

    fullRepresentation: ColumnLayout {
        Layout.minimumWidth: Kirigami.Units.gridUnit * 16
        Layout.preferredWidth: Kirigami.Units.gridUnit * 18
        Layout.minimumHeight: Kirigami.Units.gridUnit * 14
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            level: 3
            text: i18n("Current GPU: %1", root.modeLabel(root.mode))
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
        }

        Repeater {
            model: root.modes
            delegate: PlasmaComponents3.ItemDelegate {
                required property var modelData
                Layout.fillWidth: true
                enabled: !root.busy
                highlighted: modelData.id === root.mode
                text: modelData.label
                icon.source: modelData.icon
                icon.width: Kirigami.Units.iconSizes.medium
                icon.height: Kirigami.Units.iconSizes.medium
                display: PlasmaComponents3.AbstractButton.TextBesideIcon
                onClicked: root.switchTo(modelData)
            }
        }

        PlasmaComponents3.BusyIndicator {
            visible: root.busy
            running: root.busy
            Layout.alignment: Qt.AlignHCenter
        }

        PlasmaComponents3.Label {
            visible: root.message !== ""
            text: root.message
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
        }

    }
}
