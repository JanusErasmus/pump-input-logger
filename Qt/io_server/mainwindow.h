#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QAbstractSocket>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

     QTcpServer * socket;

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
