#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Application metadata
    QCoreApplication::setOrganizationName("CommentExplorer");
    QCoreApplication::setApplicationName("Comment Explorer & Cleaner");
    QCoreApplication::setApplicationVersion("1.0.0");
    
    // Set modern style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.setWindowTitle("Comment Explorer & Cleaner");
    mainWindow.resize(1400, 900);
    mainWindow.show();
    
    return app.exec();
}
