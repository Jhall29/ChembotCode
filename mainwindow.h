//-----------------------------------------------
//     Module:     mainwindow.h
//     Author:     Justin Hall
//     Created:    3 October 2020
//-----------------------------------------------

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void newMessage(QString);

private slots:
    void readData();
    void discardSocket();
    void displayMessage(const QString& str);

    void on_a_btn_clicked();
    void on_b_btn_clicked();
    void on_speedSlider_sliderReleased();

    void on_speedSlider_sliderMoved();

private:
    Ui::MainWindow *ui;
    QTcpSocket *m_TcpSocket;
};
#endif // MAINWINDOW_H
