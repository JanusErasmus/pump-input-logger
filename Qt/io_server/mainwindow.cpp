#include <QDebug>
#include <QMessageBox>
#include <QTcpSocket>
#include <QDateTime>
#include <QRegExp>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "pump_report.h"
#include "user_variables.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mDB = new LoggerDB("localhost", 3306);

    setWindowTitle("Pump Logger Server");

    mStatus.setText("Not updated yet");
    ui->statusBar->addPermanentWidget(&mStatus);

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

int MainWindow::parseClient(QByteArray data)
{
    QString clientString(data);
    //qDebug() << clientString;

    int validEvents = 0;

    pumpReport report;

    QStringList args = clientString.split("\r\n");
    foreach(QString a, args)
    {
        QStringList event = a.split(",");
        if(event.length() > 2)
        {
            int timeStamp = event.at(0).toInt();
            QDateTime evtTime = QDateTime::fromTime_t(timeStamp);
            qDebug() << evtTime.toString("yyyy-MM-dd HH:mm:ss") << "Port " << event.at(1) << "State" << event.at(2);

            report.addEntry(evtTime, event.at(1).toInt(), event.at(2).toInt());

            if(mDB)
            {
                qDebug() << "MainWindow: DB insert event";
                mDB->insertEvent(evtTime, event.at(1).toInt(), event.at(2).toInt());
            }

            validEvents++;
        }
        else
        {
            qDebug() << "Status: " << event.at(0);
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

        QString configString;
        userVariables v;


        int value = v.getVarString("rate").toInt() * 60;
        configString = QString("Fp") + QString::number(value) + "\r\n";
        client->write(configString.toLocal8Bit());
        //client->write("Fp1800\r\n");

        value = v.getVarString("start").toInt() - 2;
        configString = QString("Fs") + QString::number(value) + "\r\n";
        client->write(configString.toLocal8Bit());
        //client->write("Fs10\r\n");

        value = v.getVarString("stop").toInt() - 2;
        configString = QString("Fe") + QString::number(value) + "\r\n";
        client->write(configString.toLocal8Bit());
        //client->write("Fe20\r\n");

        value = v.getVarString("on").toInt() * 60;
        configString = QString("Fu") + QString::number(value) + "\r\n";
        client->write(configString.toLocal8Bit());
        //client->write("Fu600\r\n");

        value = v.getVarString("rest").toInt() * 60;
        configString = QString("Fr") + QString::number(value) + "\r\n";
        client->write(configString.toLocal8Bit());
        //client->write("Fr60\r\n");

        int timeNow = QDateTime::currentDateTime().toTime_t();
        QString timeString = QString("T") + QString::number(timeNow) + QString("\r\n");
        client->write(timeString.toLocal8Bit());

        qDebug() << "Ack " << ack << " @ " << QDateTime::currentDateTime();

        mStatus.setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
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
    connect(clientConnection, SIGNAL(disconnected()), this, SLOT(disconnect()));
    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(readTCP()));

    if(!mDB->open())
    {
        qDebug() << "MainWindow: Could NOT open database";
    }

}

void MainWindow::disconnect()
{
    QTcpSocket *client = static_cast<QTcpSocket*>(sender());
    if(client)
        client->deleteLater();

    mDB->close();
}

MainWindow::~MainWindow()
{
    delete ui;
}
