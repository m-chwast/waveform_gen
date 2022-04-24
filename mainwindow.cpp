#include <filesystem>
#include <string>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "waveeditform.h"
#include "serialib.h"
#include "serial_class.h"
#include <thread>
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QScrollBar>
#include <QString>


#include <QKeyEvent>

bool send_message_key_pressed = false;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    timer_1 = new QTimer(this);
    connect(timer_1, SIGNAL(timeout()), this, SLOT(timer_callback()));
    timer_1->start(200);
    on_pushButton_2_clicked();  //refreshes com port list

    keyEnterReceiver* key = new keyEnterReceiver();
    ui->centralwidget->installEventFilter(key);

    ////serial read thread:
    ReadThread = std::unique_ptr<std::thread>(new std::thread {serial_class::read_data_continuously});
    waveform.show();
}

MainWindow::~MainWindow()
{
    ReadThread->detach();
    delete ui;
    ////user code
    delete timer_1;

}


void MainWindow::on_pushButton_clicked()
{
    if(serial.port_opened() == false)
    {
        serial.set_baud(115200);
        for(int i = 0; i < 5; i++)
        {
            serial.connect();
            if(serial.port_opened() == true)
                break;
        }
        if(serial.port_opened() == true)
            ui->pushButton->setText("Disconnect");
    }
    else
    {
        serial.close();
        if(serial.port_opened() == false)
            ui->pushButton->setText("Connect");
    }
}

void MainWindow::timer_callback()
{
    if(serial.port_opened() == true)
    {
        write_all_to_textbox();
    }

}




void MainWindow::on_pushButton_2_clicked()
{
    const std::string path = "/dev/serial/by-id/";
    ui->comboBox->clear();
    try
    {
        for(const auto & entry : std::filesystem::directory_iterator(path))
        {
            std::string tmpstring = entry.path();
            ui->comboBox->addItem(QString::fromStdString(tmpstring));
        }
    }
    catch(...) {}
    if(ui->comboBox->count() == 1)
    {
        ui->comboBox->setCurrentText(ui->comboBox->itemText(0));
        ui->comboBox->setCurrentIndex(0);
        on_comboBox_textActivated(ui->comboBox->itemText(0));
    }
}


void MainWindow::on_comboBox_textActivated(const QString &arg1)
{
    serial.set_port_name(arg1);
}


void MainWindow::on_pushButton_3_clicked()
{
    if(serial.port_opened() == true)
    {
        std::string tmp;
        tmp = ui->lineEdit->text().toStdString();
        tmp += "\n";
        serial.send_data(ui->lineEdit->text() + "\n");
        write_to_textbox(tmp, "Send: ");
        ui->lineEdit->clear();
    }
}


void MainWindow::write_to_textbox(std::string text, const QString destination)
{
    QString prefix;
    if(ui->checkBox->isChecked() == true)
        prefix = QTime::currentTime().toString() + "." + QString::number(QTime::currentTime().msec()) + ": ";
    if(ui->checkBox_3->isChecked() == true)
        prefix += destination;
    int barpos = ui->textBrowser->verticalScrollBar()->value();
    ui->textBrowser->moveCursor(QTextCursor::End);
    ui->textBrowser->insertPlainText(prefix + QString::fromStdString(text));
    ui->textBrowser->verticalScrollBar()->setValue(barpos);
}

void MainWindow::write_all_to_textbox()
{
    std::unique_lock<std::mutex> lck {serial.receive_mutex};
    while(serial.received_data.begin() != serial.received_data.end())
    {
        QString prefix;
        if(ui->checkBox->isChecked() == true)
            prefix = serial.received_data.begin()->reception_time.toString() + "." + QString::number(serial.received_data.begin()->reception_ms) + ": ";
        if(ui->checkBox_3->isChecked() == true)
            prefix += "Received: ";
        int barpos = ui->textBrowser->verticalScrollBar()->value();
        ui->textBrowser->moveCursor(QTextCursor::End);
        ui->textBrowser->insertPlainText(prefix + serial.received_data.begin()->message);
        ui->textBrowser->verticalScrollBar()->setValue(barpos);
        serial.received_data.pop_front();
    }
    lck.unlock();
    if(ui->checkBox_2->isChecked() == true)
    {
        QScrollBar *tmpbar = ui->textBrowser->verticalScrollBar();
        tmpbar->setValue(tmpbar->maximum());
    }
    if(send_message_key_pressed == true)
    {
        send_message_key_pressed = false;
        if(focusWidget() == ui->lineEdit)
            on_pushButton_3_clicked();
    }
}








bool keyEnterReceiver::eventFilter(QObject* obj, QEvent* event)
{
    if(event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if(key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return)
        {
            send_message_key_pressed = true;
        }
        else
            return QObject::eventFilter(obj, event);
        return true;
    }
    else
        return QObject::eventFilter(obj, event);
    return false;
}


void MainWindow::on_pushButton_4_clicked()
{
    ui->textBrowser->clear();
}


void MainWindow::on_pushButton_5_clicked()
{
    waveform.show();
}

