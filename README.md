# GPU Switcher

Switch between **Integrated**, **Hybrid** and **Nvidia** GPU modes on Optimus laptops from your desktop, using [EnvyControl](https://github.com/bayasdev/envycontrol).

It comes in two flavours:

| | Location | Requires |
|---|---|---|
| **KDE Plasma widget** (recommended) | [plasmoid/](plasmoid) | Plasma 6 |
| Qt system tray app | [src/](src) | Qt 6, C++17, CMake |

The widget shows the current GPU mode in the panel (icon + tooltip), and lets you switch with one click. The mode is polled every 5 seconds.

## Requirements

- [EnvyControl](https://github.com/bayasdev/envycontrol) installed and on `PATH` (`envycontrol --query` must work)
- Either passwordless `sudo` for `envycontrol`, or `polkit` (`pkexec`). The widget tries `sudo -n` first and falls back to a graphical `pkexec` prompt.
- **Reboot after switching** – EnvyControl applies changes on the next boot.

Optional passwordless sudo (`sudo visudo -f /etc/sudoers.d/envycontrol`):

```
youruser ALL=(root) NOPASSWD: /usr/bin/envycontrol
```

## Plasma widget

### Install from the KDE Store

Right-click the desktop or panel → **Add Widgets…** → **Get New Widgets…** → **Download New Plasma Widgets** → search for *GPU Switcher* → **Install**.

### Install from a package file

Build the package and install it:

```bash
cd plasmoid
zip -r ../gpuswitcher.plasmoid metadata.json contents
cd ..
kpackagetool6 -t Plasma/Applet -i gpuswitcher.plasmoid
```

Or install directly from the source folder:

```bash
kpackagetool6 -t Plasma/Applet -i plasmoid
```

Update with `-u` instead of `-i`, remove with `-r org.kde.plasma.gpuswitcher`.

Then add **GPU Switcher** to a panel or the system tray (Add Widgets… → *GPU Switcher*).

### Develop / test

```bash
plasmawindowed org.kde.plasma.gpuswitcher     # standalone window
kpackagetool6 -t Plasma/Applet -u plasmoid     # reload after edits
```

### Publishing to the KDE Store

1. Build `gpuswitcher.plasmoid` as above (the zip must contain `metadata.json` and `contents/` at its top level).
2. Upload it at <https://store.kde.org> under *Plasma Widgets* → *Plasma 6 Extensions*.
3. Bump `KPlugin.Version` in [plasmoid/metadata.json](plasmoid/metadata.json) for each release.

### Layout

```
plasmoid/
├── metadata.json          # plugin id, name, version
└── contents/
    ├── ui/main.qml        # panel icon + popup
    └── images/            # integrated / hybrid / nvidia icons
```

## Qt tray app (alternative)

```bash
cmake -S . -B build
cmake --build build
./build/gpu_switcher_tray
```

## Modes

| Mode | Command run |
|---|---|
| Integrated | `envycontrol -s integrated` |
| Hybrid | `envycontrol -s hybrid --rtd3 2` |
| Nvidia | `envycontrol -s nvidia` |

## License

GPL-3.0-or-later (as declared in `metadata.json`).
