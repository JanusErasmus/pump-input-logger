#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QAbstractSocket>
#include <QLabel>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    QLabel mStatus;
    QTcpServer * socket;

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
};

#endif // MAINWINDOW_H
