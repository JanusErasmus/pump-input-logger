#include <QDebug>
#include <QMessageBox>
#include <QTcpSocket>
#include <QDateTime>
#include <QRegExp>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    socket = new QTcpServer(this);
    connect(socket, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(newConnection()),  this, SLOT(connected()));

    if(!socket->listen(QHostAddress::Any, 23))
    {
        qDebug() << "Unable to start server";
        return;
    }

    qDebug() << "Server started...";
}

int parseClient(QByteArray data)
{
    QString clientString(data);
    //qDebug() << clientString;

    int validEvents = 0;

    QStringList args = clientString.split("\r\n");
    foreach(QString a, args)
    {
        QStringList event = a.split(",");
        if(event.length() > 2)
        {
            int timeStamp = event.at(0).toInt();
            QDateTime evtTime = QDateTime::fromTime_t(timeStamp);
            qDebug() << evtTime.toString("yyyy-MM-dd HH:mm:ss") << "Port " << event.at(1) << "State" << event.at(2);

            validEvents++;
        }
    }

    return validEvents;
}

void MainWindow::readTCP()
{
    QTcpSocket *client = static_cast<QTcpSocket*>(sender());
    int len = client->bytesAvailable();
    if(len)
    {
        qDebug() << "RX: " << len;
        QByteArray data = client->readAll();
        int ack = parseClient(data);

        QString ackString("A");
        ackString += QString::number(ack) + QString("\r\n");
        client->write(ackString.toLocal8Bit());

        client->write("Fs8\r\n");
        client->write("Fe22\r\n");
        client->write("Fu30\r\n");
        client->write("Fr60\r\n");

        int timeNow = QDateTime::currentDateTime().toTime_t();
        QString timeString = QString("T") + QString::number(timeNow) + QString("\r\n");
        client->write(timeString.toLocal8Bit());

        qDebug() << "Ack " << ack << " @ " << QDateTime::currentDateTime();
    }
}

void MainWindow::displayError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "SocketError: " << socketError;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The host was not found. Please check the "
                                    "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The connection was refused by the peer. "
                                    "Make sure the fortune server is running, "
                                    "and check that the host name and port "
                                    "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Fortune Client"),
                                 tr("The following error occurred: %1.")
                                 .arg(socket->errorString()));
    }
}

void MainWindow::netState(QAbstractSocket::SocketError state)
{
    qDebug() << state;
}

void MainWindow::connected()
{
    QTcpSocket *clientConnection = socket->nextPendingConnection();
    if(!clientConnection)
    {
        qDebug() << "false connection";
        return;
    }

    qDebug() << "Cleint connected";
    connect(clientConnection, SIGNAL(disconnected()), clientConnection, SLOT(deleteLater()));
    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(readTCP()));

//    clientConnection->write("Hello Cleint!!!");
//    clientConnection->disconnectFromHost();
}

MainWindow::~MainWindow()
{
    delete ui;
}
