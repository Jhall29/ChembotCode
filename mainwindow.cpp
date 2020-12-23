//-----------------------------------------------
//     Module:     mainwindow.cpp
//     Author:     Justin Hall
//     Created:    3 October 2020
//-----------------------------------------------

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->label_2->setStyleSheet("font-weight: bold; color:red");
    ui->speedSlider->setRange(10,90);

    //create local instance of m_TcpSocket
    m_TcpSocket = new QTcpSocket(this);

    //connect functions to Transmission Control Protocol signals
    connect(this,SIGNAL(newMessage(QString)),this,SLOT(displayMessage(QString)));
    connect(m_TcpSocket,SIGNAL(readyRead()),this,SLOT(readData()));
    connect(m_TcpSocket,SIGNAL(disconnected()),this,SLOT(discardSocket()));
    //connect the IP address of the RPi
    m_TcpSocket->connectToHost("10.0.0.71",55555);

    //wait 3 sec to connect to RPi, close if not found
    if(m_TcpSocket->waitForConnected(3000))
        qDebug() << "Connected to Server";
    else{
        QMessageBox::critical(this,"QTCPClient", QString("The following error occurred: %1.").arg(m_TcpSocket->errorString()));
        exit(EXIT_FAILURE);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

//Get data form RPi with TCP
void MainWindow::readData()
{
    QByteArray ReceiveData = m_TcpSocket->readAll();
    QString temp = QString::fromStdString(ReceiveData.toStdString());
    ui->textEdit->setText(temp);

    //if data received from the RPi is 'hello', flash green for 2 sec
    if(temp == "hello")
    {
        ui->label_2->setStyleSheet("font-weight: bold; color:green");
        QTimer::singleShot(2000, [this](){
            this->ui->label_2->setStyleSheet("font-weight: bold; color:red)");
        });
    }
}

//delete a socket instance of m_TcpSocket
//Will become more important when more sockets are necesarry
void MainWindow::discardSocket()
{
    m_TcpSocket->deleteLater();
    m_TcpSocket=nullptr;
}


//Send data to RPi when GUI buttons pressed
void MainWindow::on_a_btn_clicked()
{
    if(m_TcpSocket)  //if m_TcpSocket has been created
    {
        if(m_TcpSocket->isOpen())
        {
            ui->a_btn->setEnabled(false);
            ui->b_btn->setEnabled(true);
            ui->a_btn->setStyleSheet("background-color: rgb(169, 169, 169)");
            ui->b_btn->setStyleSheet("background-color: rgb(40, 172, 152)");
            QByteArray data = "a";
            m_TcpSocket->write(data);
            qDebug() << "a sent to the Pi";
        }
    }
}

void MainWindow::on_b_btn_clicked()
{
    if(m_TcpSocket)
    {
        if(m_TcpSocket->isOpen())
        {
            ui->b_btn->setEnabled(false);
            ui->a_btn->setEnabled(true);
            ui->a_btn->setStyleSheet("background-color: rgb(40, 172, 152)");
            ui->b_btn->setStyleSheet("background-color: rgb(169, 169, 169)");
            QByteArray data = "b";
            m_TcpSocket->write(data);
            qDebug() << "b sent to the Pi";
        }
    }
}

void MainWindow::on_speedSlider_sliderReleased()
{
    int tick = ui->speedSlider->value();
    QString s_tick = QString::number(tick);
    ui->speed_text->setText(s_tick);
    QByteArray q_data;
    q_data.setNum(tick);

    if(m_TcpSocket)
    {
        if(m_TcpSocket->isOpen())
        {
            m_TcpSocket->write(q_data);
            qDebug() << "slider data sent to the Pi: " << q_data;
        }
    }
}


//When data is received from the RPi, update ui
void MainWindow::displayMessage(const QString& str)
{
    this->ui->textEdit->setText(str);
}


void MainWindow::on_speedSlider_sliderMoved()
{
    int tick = ui->speedSlider->value();
    QString s_tick = QString::number(tick);
    ui->speed_text->setText(s_tick);
}
