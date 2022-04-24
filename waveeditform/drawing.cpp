#include "waveeditform.h"
#include "ui_waveeditform.h"
#include <cmath>
#include <vector>
#include <chrono>
#include <numeric>
#include <utility>
#include <mutex>
#include <thread>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QFileDialog>
#include <QIODevice>
#include <QFile>
#include <QMessageBox>
#include "wave.h"
#include "wave_data.h"
#include "mainwindow.h"






void WaveEditForm::draw_background()
{
    QPainter painter;
   // QPixmap pixmap(":images/sinx.png");
    QPixmap pixmap(ui->label->width(), ui->label->height());
    pixmap.fill(QColor {35, 35, 35});

    painter.begin(&pixmap);

    ///grid
    for(int i = 49; i < ui->label->height(); i += 50)
    {
        if(i != 0) {painter.setPen(QColor {200,200,200}); painter.drawText(5, i+5, QString::number(v_per_div * (9 - i/25) / 2, 10, 2)); painter.setPen(QColor {0,0,0});}
        for(int j = 49; j < ui->label->width(); j+= 20)
            painter.drawLine(j, i, j+10, i);
    }
    for(int j = 99 + wave_start_hor - 100; j < ui->label->width(); j += 100)
    {
        for(int i = 5; i < ui->label->height() - 20; i += 20)
            painter.drawLine(j, i, j, i+10);
       if((j+1)/100 < 10){ painter.setPen(QColor {200,200,200}); painter.drawText(j - 10, 514, QString::number(ms_per_div * (int)((j + 1)/100), 10, 2)); painter.setPen(QColor {0,0,0});}
    }
    painter.setPen(QColor {200,200,200});
    painter.drawText(10, 20, "[V]");
    painter.drawText(ui->label->width() - 50, 514, "[ms]");


    painter.end();

    ui->label->setPixmap(pixmap);
}

void WaveEditForm::draw_wave()
{
    if(wave_list.empty() == true)
        return;
    wave &waveform = *resultant_wave;
    dac_divider = waveform.dac_divider;
    ////editing DAC sps label:
    {
        float sps = DAC_FREQUENCY / dac_divider / 1000000;
        ui->label_8->setText("DAC: " + QString::number(sps) + " MSps");
    }
    double period_ms = waveform.get_period(time_unit::ms);
    uint size = waveform.samples.size();
    double time_step = (size / period_ms) * ms_per_div / 100;   //how many samples goes for one point
    double displayed_points = 0, displayed_index = 0;
    QPixmap pixmap = ui->label->pixmap();
    QPainter painter;
    painter.begin(&pixmap);
    painter.setPen(QColor {255,0,0});
    int16_t old_point = 0;
    while(displayed_points < 1000)
    {
        float sample = waveform.samples[(int)floor(displayed_index) % waveform.samples.size()] * volts_per_step;
        int16_t sample_y_pos = (sample / v_per_div) * 50;
        if(ui->checkBox_3->isChecked() == false || displayed_points == 0)
            painter.drawPoint(wave_start_hor + displayed_points, 499 - 250 - sample_y_pos);
        else if(ui->checkBox_3->isChecked() == true)
            painter.drawLine(wave_start_hor + displayed_points, 499 - 250 - sample_y_pos, wave_start_hor + displayed_points - 1, old_point);
        displayed_index += time_step;
        displayed_points++;
        old_point = 499 - 250 - sample_y_pos;
    }

    if(ui->checkBox_4->isChecked() == true)
    {
        std::list<wave>::iterator tmpit;
        for(std::list<wave>::iterator i = wave_list.begin(); i != wave_list.end(); i++)
        {
            if(ui->comboBox_2->currentText() == i->name)
            {
                tmpit = i;
                break;
            }
        }
        wave &waveform = *tmpit;
        double period_ms = waveform.get_period(time_unit::ms);
        uint size = waveform.samples.size();
        double time_step = (size / period_ms) * ms_per_div / 100;
        old_point = 0;
        displayed_points = 0;
        displayed_index = 0;
        painter.setPen(QColor {0,0,255});
        while(displayed_points < 1000)
        {
            float sample = waveform.samples[(int)floor(displayed_index) % waveform.samples.size()] * volts_per_step;
            int16_t sample_y_pos = (sample / v_per_div) * 50;
            if(ui->checkBox_3->isChecked() == false || displayed_points == 0)
                painter.drawPoint(wave_start_hor + displayed_points, 499 - 250 - sample_y_pos);
            else if(ui->checkBox_3->isChecked() == true)
                painter.drawLine(wave_start_hor + displayed_points, 499 - 250 - sample_y_pos, wave_start_hor + displayed_points - 1, old_point);
            displayed_index += time_step;
            displayed_points++;
            old_point = 499 - 250 - sample_y_pos;
        }
    }
    painter.end();
    ui->label->setPixmap(pixmap);
}
