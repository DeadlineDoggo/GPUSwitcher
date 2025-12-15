#include "gpu_tray.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char **argv)
{
	QCoreApplication::setOrganizationName("GPUSwitcher");
	QCoreApplication::setOrganizationDomain("gpuswitcher.example");
	QCoreApplication::setApplicationName("GPU Switcher Tray");

	// Disable xdg-desktop-portal integration to avoid DBus portal registration
	qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");

	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	// Ensure the application has an icon (helps some desktop environments show tray icons)
	app.setWindowIcon(QIcon(":/icons/integrated.png"));
	// Do not set desktop file name here; avoid portal app registration requiring installed .desktop

	GPUTray tray;
	return app.exec();
}
