#include "mainwindow.h"

#include <QApplication>
#include "serial_class.h"

#include <string>
#include "serial_class.h"

//serialib serial;
//std::string port_name;
serial_class serial;



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    if(serial.port_opened() == true)
            serial.close();
    return a.exec();
}
