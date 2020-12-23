#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTcpServer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void newMessage(QString);

private slots:
    //Functions for read/write to GUI over ethernet using TCP
    void newConnection();
    void appendToSocketList(QTcpSocket *socket);
    void readSocket();
    void discardSocket();
    void displayMessage(const QString& str);
    void sendMessage(QTcpSocket *socket, QByteArray seialData);

    //Functions for read/wrtie to VEX over USB using Serial Port
    void readSerial();
    void updateOnOff(QString);

private:
    Ui::MainWindow *ui;

    //GUI <-> RPi
    QTcpServer *m_TcpServer;
    QList<QTcpSocket*> connection_list;
    QString motor_speed;

    //RPi <-> VEX
    QSerialPort *vex;
    static const quint16 vex_vendor_id = 10376;
    static const quint16 vex_product_id = 1281;
    QString vex_port_name;
    bool vex_is_available;
};

#endif // MAINWINDOW_H
