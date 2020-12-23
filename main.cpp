#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.setFixedSize(600,650);
    w.setWindowTitle("Chembot Demo");
    w.setStyleSheet("background-color: rgb(86, 86, 86)");
    w.show();
    return a.exec();
}
