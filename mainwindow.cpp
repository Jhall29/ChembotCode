//-----------------------------------------------
//      Module:     mainwindow.cpp
//      Author:     Justin Hall
//      Created:    3 October 2020
//-----------------------------------------------

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include <QtWidgets>
#include <QObject>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    //Desktop <-> RPi
        //create local instance of m_TcpServer
        m_TcpServer = new QTcpServer();
        //connect to GUI when available
        //connect functions to Transmission Control Protocol signals
        if(m_TcpServer->listen(QHostAddress::Any, 55555))
        {
           connect(this,SIGNAL(newMessage(QString)),this,SLOT(displayMessage(QString)));
           connect(m_TcpServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
        }
        //initialize motor speed
        motor_speed = "10";


    //RPi <-> VEX
        //create local instance of vex
        vex_is_available = false;
        vex_port_name = " ";
        vex = new QSerialPort;

    //------------Code to determine the serial port information of VEX V5 Brain----------------//
    /*
    qDebug() << "Number of available ports: " << QSerialPortInfo::availablePorts().length();
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
        qDebug() << "has name: " << serialPortInfo.portName();

        qDebug() << "Has vendor ID: " << serialPortInfo.hasVendorIdentifier();
        if(serialPortInfo.hasVendorIdentifier())
        {
            qDebug() << "Vendor ID: " << serialPortInfo.vendorIdentifier();
        }
        qDebug() << "Has product ID: " << serialPortInfo.hasProductIdentifier();
        if(serialPortInfo.hasProductIdentifier())
        {
            qDebug() << "Product ID: " << serialPortInfo.productIdentifier();
        }
    }*/
    //-------------------------------------------------------------------------------------//


    //establish vendor ID and product ID for connecting to VEX port
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts())
    {
        if(serialPortInfo.hasVendorIdentifier() && serialPortInfo.hasProductIdentifier()){
            if(serialPortInfo.vendorIdentifier() == vex_vendor_id){
                if(serialPortInfo.productIdentifier() == vex_product_id){
                    vex_port_name = serialPortInfo.portName();
                    vex_is_available = true;
                }
            }
        }
    }

    //setup Serial Port connection to VEX V5 Brain
    if(vex_is_available)  //if available, open and configure the port
    {
        vex->setPortName(vex_port_name);
        vex->open(QSerialPort::ReadWrite);   //allow two way communication (RPi <-> VEX)
        vex->setBaudRate(QSerialPort::Baud9600);
        vex->setDataBits(QSerialPort::Data8);
        vex->setParity(QSerialPort::NoParity);
        vex->setStopBits(QSerialPort::OneStop);
        vex->setFlowControl(QSerialPort::NoFlowControl);
        QObject::connect(vex, SIGNAL(readyRead()), this, SLOT(readSerial()));
    }
    else    //give error message if not available
    {
        QMessageBox::warning(this, "Port error", "Couldn't find the V5 Brain!");
    }
}

MainWindow::~MainWindow()
{
    foreach (QTcpSocket* socket, connection_list)
    {
        socket->close();
        socket->deleteLater();
    }
    m_TcpServer->close();
    m_TcpServer->deleteLater();

    if(vex->isOpen())
    {
        vex->close();
    }

    delete ui;
}

//create a new TCP connection to the GUI
void MainWindow::newConnection()
{
    //if new connction available, add socket
    while (m_TcpServer->hasPendingConnections())
        appendToSocketList(m_TcpServer->nextPendingConnection());
}

//add socket connection using TCP
void MainWindow::appendToSocketList(QTcpSocket* socket)
{
    //connect functions to Transmission Control Protocol signals
    connection_list.append(socket);
    connect(socket, SIGNAL(readyRead()), this , SLOT(readSocket()));
    connect(socket, SIGNAL(disconnected()), this , SLOT(discardSocket()));
}

//delete a socket instance of m_TcpServer
//Will become more important when more sockets are necesarry
void MainWindow::discardSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    for (int i=0;i<connection_list.size();i++)
    {
        if (connection_list.at(i) == socket)
        {
            connection_list.removeAt(i);
            break;
        }
    }
    socket->deleteLater();
}


// READ data sent from the Desktop GUI
void MainWindow::readSocket()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

    QByteArray ReceiveData = socket->readAll();
    QString data_str = QString::fromStdString(ReceiveData.toStdString());

    if(data_str == "a" || data_str == "b")
    {
        MainWindow::updateOnOff(QString(ReceiveData) + motor_speed);
    }
    else
    {
        motor_speed = data_str;
    }

    ui->textEdit->setTextColor(QColor("red"));
    this->ui->textEdit->append(data_str);
}

// SEND data to Desktop GUI
void MainWindow::sendMessage(QTcpSocket *socket, QByteArray vexData)
{
    if(socket) //if socket is available
    {
        if(socket->isOpen())
        {
            socket->write(vexData);
        }
        else
            QMessageBox::critical(this,"QTCPServer","Socket doesn't seem to be opened");
    }
    else
        QMessageBox::critical(this,"QTCPServer","Not connected");
}

//When data is received from the GUI, update ui
void MainWindow::displayMessage(const QString& str)
{
    ui->textEdit->setTextColor(QColor("red"));
    this->ui->textEdit->append(str);
}




// RECEIVE data from the VEX V5 Brain
void MainWindow::readSerial()
{
    //qDebug() << "Serialport Works";
    QByteArray serialData = vex->readAll();
    QString temp = QString::fromStdString(serialData.toStdString());
    //call the send message function to send serial data from VEX to the GUI
    foreach (QTcpSocket* socket,connection_list)
    {
        sendMessage(socket, serialData);
    }
    ui->textEdit->setTextColor(QColor("green"));
    this->ui->textEdit->append(temp);
}


// SEND data to the VEX V5 Brain
void MainWindow::updateOnOff(QString command)
{
    if(vex->isWritable())
    {
        vex->write(command.toStdString().c_str());
        qDebug() << "data sent to the V5 Brain";
    }
    else
    {
        qDebug() << "Couldn't write to serial!";
    }
}
