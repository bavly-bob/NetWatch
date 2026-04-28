#include <QApplication>
#include "mainwindow.hpp"
#include "qt_metatypes.hpp"   // Q_DECLARE_METATYPE for SystemStats / ProcessInfo

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Register our custom types so Qt can queue them across threads
    qRegisterMetaType<SystemStats>("SystemStats");
    qRegisterMetaType<ProcessInfo>("ProcessInfo");

    QApplication::setApplicationName("NetWatch Dashboard");
    QApplication::setApplicationVersion("0.1");
    QApplication::setOrganizationName("CapstoneTeam");

    MainWindow window;
    window.show();

    return app.exec();
}
