#include <QApplication>
#include "mainwindow.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QApplication::setApplicationName("NetWatch Dashboard");
    QApplication::setApplicationVersion("0.1");
    QApplication::setOrganizationName("CapstoneTeam");

    MainWindow window;
    window.show();

    return app.exec();
}
