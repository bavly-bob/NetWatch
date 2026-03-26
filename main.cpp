#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QThread>

#include <boost/asio.hpp>

#include <chrono>
#include <memory>
#include <thread>

static void start_boost_asio_demo()
{
    auto io = std::make_shared<boost::asio::io_context>();
    auto timer = std::make_shared<boost::asio::steady_timer>(
        *io, std::chrono::seconds(1));

    timer->async_wait([io, timer](const boost::system::error_code &ec) {
        if (!ec) {
            qDebug() << "Boost.Asio timer fired on thread"
                     << reinterpret_cast<qulonglong>(QThread::currentThreadId());
        } else {
            qDebug() << "Boost.Asio timer error:" << QString::fromStdString(ec.message());
        }
        io->stop();
    });

    std::thread([io]() { io->run(); }).detach();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    start_boost_asio_demo();
    return a.exec();
}
