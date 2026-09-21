#include "UI/MainWindow.h"
#include "App/WindowsRuntime.h"

#include <QApplication>
#include <QDir>
#include <QTimer>

int main(int argc, char* argv[]) {
    // Explorer activation can supply an untrusted current directory.
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) return 1;
    aidfame::WindowsRuntime runtime;
    if (!runtime.valid()) return 1;
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("Aidfame Player"));
    QCoreApplication::setOrganizationName(QStringLiteral("Aidfame"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    aidfame::MainWindow window;
    window.show();
    const auto arguments = app.arguments();
    if (arguments.size() == 2) {
        QTimer::singleShot(0, &window, [&window, path = arguments.at(1)] {
            window.openFile(QDir::isAbsolutePath(path) ? path : QDir::current().absoluteFilePath(path));
        });
    }
    return app.exec();
}
