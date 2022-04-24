#include "serial_class.h"
#include "mainwindow.h"
#include <mutex>


serial_class::serial_class()
{

}

/*
bool serial_class::read_string()
{
    if(port.isDeviceOpen() == true && data_available() != 0)
    {
        received_message new_message;
        new_message.reception_ms = QTime::currentTime().msec();
        new_message.reception_time = QTime::currentTime();
        char tmp[200];
        for(char & v : tmp) v = '\0';
        port.readString(tmp, '\n', 200);
        std::string tmpstring(tmp);
        new_message.message = QString::fromStdString(tmpstring);
        std::unique_lock<std::mutex> lck {serial.receive_mutex};
        lck.lock();
        serial.received_data.push_back(new_message);
        lck.unlock();
        return true;
    }
    return false;
}*/

void serial_class::send_data(QString text)
{
    if(port.isDeviceOpen() == true)
        port.writeString(text.toStdString().c_str());
}

void serial_class::send_data(uint8_t *data, uint size)
{
    if(port.isDeviceOpen() == true)
        port.writeBytes(data, size);
}


void serial_class::read_data_continuously()
{
    while(1)
    {
        if(serial.port_opened() == true && serial.data_available() != 0)
        {
            received_message new_message;
            new_message.reception_ms = QTime::currentTime().msec();
            new_message.reception_time = QTime::currentTime();
            char tmp[200];
            for(char & v : tmp) v = '\0';
            serial.port.readString(tmp, '\n', 200);
            std::string tmpstring(tmp);
            new_message.message = QString::fromStdString(tmpstring);
            std::unique_lock<std::mutex> lck {serial.receive_mutex};
            //lck.lock();
            serial.received_data.push_back(new_message);
            if(new_message.message == serial.ask)   ///NOT TESTED
                serial.ask_received = true;
            lck.unlock();
        }
    }
}

void serial_class::ask_for(int number)
{
    char tmp[5];
    for(int i = 0; i < 4; i++)
        tmp[i] = (number >> ((3-i) * 8)) & 0xFF;
    tmp[4] = '\n';
    ask = tmp;
}
