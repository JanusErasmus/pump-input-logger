#include <QCoreApplication>
#include <QTcpSocket>
#include <QDebug>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QHostAddress server("192.168.1.242");
     QTcpSocket * socket = new QTcpSocket();
     socket->connectToHost(server, 60000, QIODevice::ReadWrite);

     socket->waitForConnected();
     qDebug() << "Bound";

         qDebug() << socket->state();
//     if(socket->open(QIODevice::ReadWrite))
//         qDebug() << "OPened";

     int len = socket->write("RSSI: -83\n"
                             "78:c4:e:1:9c:5d\n"
                             "R3\n"
                             "R5\n"
                             "R6\n");

     socket->waitForBytesWritten();

     qDebug() << "wrote" << len;

     socket->waitForReadyRead();

     QByteArray rx = socket->readAll();

     qDebug() << "Received " << rx.length() << rx;

     socket->close();
     delete socket;
     qDebug() << "DONE";

     exit(0);

    return a.exec();
}
