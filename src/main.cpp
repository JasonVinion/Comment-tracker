#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("CommentExplorer");
    QCoreApplication::setApplicationName("Comment Explorer & Cleaner");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow window;
    window.setWindowTitle(QCoreApplication::applicationName());
    window.resize(1400, 900);
    window.show();
    return app.exec();
}
