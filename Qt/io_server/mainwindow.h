#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QAbstractSocket>
#include <QLabel>

#include "logger_db.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QLabel mStatus;
    QTcpServer * socket;

    LoggerDB * mDB;

     int parseClient(QByteArray data);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void readTCP();
    void displayError(QAbstractSocket::SocketError socketError);
    void netState(QAbstractSocket::SocketError state);
    void connected();
    void disconnect();
};

#endif // MAINWINDOW_H
