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

    // GPU vendors found in /sys/class/drm (PCI vendor ids)
    property bool hasIntel: false
    property bool hasNvidia: false
    property int amdCount: 0

    // Integrated GPU vendor: "intel" | "amd" | "generic".
    // With AMD integrated + AMD dedicated both present, integrated gets the generic icon.
    readonly property string igpu: hasIntel ? "intel"
        : (amdCount >= 2 ? "generic" : (amdCount === 1 ? "amd" : "intel"))
    // Dedicated GPU vendor: "nvidia" | "amd" | "generic".
    // EnvyControl hides the Nvidia card in integrated mode, so default to nvidia when nothing else is found.
    readonly property string dgpu: hasNvidia ? "nvidia"
        : ((hasIntel && amdCount >= 1) || amdCount >= 2 ? "amd" : "nvidia")

    function img(name) { return Qt.resolvedUrl("../images/" + name + ".png") }

    function iconFor(id) {
        if (id === "integrated")
            return img(igpu === "intel" ? "integrated" : "integrated-" + igpu)
        if (id === "nvidia")
            return img(dgpu === "nvidia" ? "nvidia" : "dedicated-" + dgpu)
        if (id === "hybrid") {
            if (igpu === "intel" && dgpu === "nvidia") return img("hybrid")
            return img("hybrid-" + igpu + "-" + dgpu)
        }
        return ""
    }

    readonly property var modes: [
        { id: "integrated", label: i18n("Integrated"), icon: iconFor("integrated"),
          cmd: "envycontrol -s integrated" },
        { id: "hybrid", label: i18n("Hybrid"), icon: iconFor("hybrid"),
          cmd: "envycontrol -s hybrid --rtd3 2" },
        { id: "nvidia", label: i18n("Nvidia"), icon: iconFor("nvidia"),
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
        executable.exec("gpus", "cat /sys/class/drm/card?/device/vendor")
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

            if (kind === "gpus") {
                const v = out.toLowerCase().split("\n").map(x => x.trim())
                root.hasIntel = v.includes("0x8086")
                root.hasNvidia = v.includes("0x10de")
                root.amdCount = v.filter(x => x === "0x1002").length
            } else if (kind === "query") {
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
