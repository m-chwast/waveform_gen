#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "serialib.h"
#include "serial_class.h"
#include "waveeditform.h"
#include <string>
#include <thread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void write_to_textbox(std::string, const QString);
    void write_all_to_textbox();


private slots:
    void on_pushButton_clicked();
    void timer_callback();


    void on_pushButton_2_clicked();

    void on_comboBox_textActivated(const QString &arg1);

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

private:
    Ui::MainWindow *ui;
    QTimer *timer_1;

    WaveEditForm waveform;
    std::unique_ptr<std::thread> ReadThread;
    //std::thread* ReadThread;
};


class keyEnterReceiver : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject * obj, QEvent * event);
};


extern serial_class serial;


#endif // MAINWINDOW_H
