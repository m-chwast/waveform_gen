#ifndef SERIAL_CLASS_H
#define SERIAL_CLASS_H

#include "serialib.h"
#include <list>
#include <mutex>
#include <QString>
#include <QTime>


//using namespace std;

struct received_message
{
    QString message;
    QTime reception_time;
    int reception_ms;
};

class serial_class
{
private:
    serialib port;
    std::list<QString> data_to_send;
    std::string port_name;
    uint baud;

    QString ask;
    bool ask_received;
public:
    std::list<received_message> received_data;
    std::mutex receive_mutex;
    std::mutex ask_mutex;

    serial_class();
    ~serial_class() {}

    bool connect()      {port.openDevice(port_name.c_str(), baud); return port.isDeviceOpen();}
    bool close()    {port.closeDevice(); return port.isDeviceOpen();}
    bool port_opened()  {return port.isDeviceOpen();}
    void set_baud(uint val)     {baud = val;}
    void set_port_name(QString name)    {port_name = name.toStdString();}
    int data_available()    {return port.available();}
   // bool read_string();
    void send_data(QString text);
    void send_data(uint8_t * data, uint size);
    static void read_data_continuously();
    void ask_for(QString str) {ask = str;}
    void ask_for_16b(int16_t number) {char num[] = {char(number >> 8), char(number & 0xFF), '\n'}; ask = num;}
    void ask_for(int number);
    QString current_ask() {return ask;}
    bool ask_cplt() {if(ask_received == true) {ask_received = false; return true; } else return false;}
};











#endif // SERIAL_CLASS_H
