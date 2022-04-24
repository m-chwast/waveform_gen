#include "waveeditform.h"
#include "ui_waveeditform.h"
#include <cmath>
#include <vector>
#include <QFileDialog>
#include <QIODevice>
#include <QFile>
#include <QMessageBox>
#include "wave.h"
#include "wave_data.h"
#include "mainwindow.h"




void WaveEditForm::on_loadButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select a saved waveform", "./saves/", "*.wsv");
    if(filename == "")
        return;
    wave_data new_wave;
    new_wave.file_name = filename;
    QFile file(filename);
    file.open(QIODevice::ReadOnly);
    char data_buf[4096];
    QString data = "\n";
    int waves_number = 0, sample_number = 0;
    memset(data_buf, 0, 256);
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("number of waves: ", Qt::CaseInsensitive))
    {
        data.remove("number of waves: ", Qt::CaseInsensitive);
        waves_number = data.toInt();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("ms_per_div ", Qt::CaseInsensitive))
    {
        data.remove("ms_per_div ", Qt::CaseInsensitive);
        new_wave.ms_per_div = data.toDouble();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("v_per_div ", Qt::CaseInsensitive))
    {
        data.remove("v_per_div ", Qt::CaseInsensitive);
        new_wave.v_per_div = data.toDouble();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("dac_divider ", Qt::CaseInsensitive))
    {
        data.remove("dac_divider ", Qt::CaseInsensitive);
        new_wave.dac_divider = data.toInt();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    ///resultant wave
    if(data.contains("resultant_wave"))
        data.remove("resultant_wave");
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("samples ", Qt::CaseInsensitive))
    {
        data.remove("samples ", Qt::CaseInsensitive);
        sample_number = data.toInt();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("period ", Qt::CaseInsensitive))
    {
        data.remove("period ", Qt::CaseInsensitive);
        new_wave.resultant_wave.period = data.toDouble();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("dac_divider ", Qt::CaseInsensitive))
    {
        data.remove("dac_divider ", Qt::CaseInsensitive);
        new_wave.resultant_wave.dac_divider = data.toInt();
    }
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("data", Qt::CaseInsensitive))
        data.remove("data", Qt::CaseInsensitive);
    new_wave.resultant_wave.samples.clear();
    for(int i = 0; i < sample_number; i++)
    {
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        new_wave.resultant_wave.samples.push_back(data.toInt());
    }

    ///other waves
    ui->comboBox_2->setCurrentIndex(-1);
    ui->comboBox_2->clear();
    dont_draw = true;
    do
    {
        file.readLine(data_buf, 256);
        data = data_buf;
        memset(data_buf, 0, 256);
    }while(data == "\n");
    if(data.contains("wave_list"))
        data.remove("wave_list");
    for(int current_wave = 0; current_wave < waves_number; current_wave++)
    {
        wave tmpwave;
        tmpwave.samples.clear();
        sample_number = 0;
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("wave "))
            data.remove("wave ");
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("name "))
        {
            data.remove("name ");
            if(data.contains("\n"))
                data.remove("\n");
            tmpwave.name = data;
        }
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("samples ", Qt::CaseInsensitive))
        {
            data.remove("samples ", Qt::CaseInsensitive);
            sample_number = data.toInt();
        }
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("period ", Qt::CaseInsensitive))
        {
            data.remove("period ", Qt::CaseInsensitive);
            tmpwave.period = data.toDouble();
        }
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("dac_divider ", Qt::CaseInsensitive))
        {
            data.remove("dac_divider ", Qt::CaseInsensitive);
            tmpwave.dac_divider = data.toInt();
        }
        do
        {
            file.readLine(data_buf, 256);
            data = data_buf;
            memset(data_buf, 0, 256);
        }while(data == "\n");
        if(data.contains("data", Qt::CaseInsensitive))
            data.remove("data", Qt::CaseInsensitive);
        for(int i = 0; i < sample_number; i++)
        {
            do
            {
                file.readLine(data_buf, 256);
                data = data_buf;
                memset(data_buf, 0, 256);
            }while(data == "\n");
            tmpwave.samples.push_back(data.toInt());
        }
        if(tmpwave.name.contains("sine", Qt::CaseInsensitive))
            tmpwave.type = wave_type::sine;
        else if(tmpwave.name.contains("triangle", Qt::CaseInsensitive))
            tmpwave.type = wave_type::triangle;
        else if(tmpwave.name.contains("saw", Qt::CaseInsensitive))
            tmpwave.type = wave_type::sawtooth;
        else if(tmpwave.name.contains("square", Qt::CaseInsensitive))
            tmpwave.type = wave_type::square;
        else if(tmpwave.name.contains("triangle", Qt::CaseInsensitive))
            tmpwave.type = wave_type::triangle;
        else if(tmpwave.name.contains("offset", Qt::CaseInsensitive))
            tmpwave.type = wave_type::offset;
        else if(tmpwave.name.contains("general", Qt::CaseInsensitive))
            tmpwave.type = wave_type::general;

        new_wave.wave_list.push_back(tmpwave);
        ui->comboBox_2->addItem(tmpwave.name);

    }
    file.close();
    this->ms_per_div = new_wave.ms_per_div;
    this->v_per_div = new_wave.v_per_div;
    this->dac_divider = new_wave.dac_divider;
    this->name = new_wave.file_name;
    resultant_wave->samples.clear();
    *resultant_wave = new_wave.resultant_wave;
    resultant_wave->samples = new_wave.resultant_wave.samples;
    wave_list.clear();
    wave_list = new_wave.wave_list;
    ui->comboBox_2->setCurrentIndex(0);
    dont_draw = false;
    update_wave();
}


void WaveEditForm::on_saveButton_clicked()
{
    QString filename;
    if(QFile::exists(name) == true)
        filename = name;
    else if(QFile::exists("./saves/" + name) == true)
        filename = "./saves/" + name;
    else
    {
        filename = QFileDialog::getSaveFileName(this, "Save Waveform", "./saves/");
        if(filename == "")
            return;
        name = filename;
    }
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    file.write(save_wave().toStdString().c_str());
    file.close();
}


void WaveEditForm::on_saveButton_2_clicked()
{
    QString filename = QFileDialog::getSaveFileName(this, "Save Waveform", "./saves/");
    if(filename == "")
        return;
    if(filename.length() > 3)
    {
        int tmp = filename.length();
        if(!(filename[tmp - 1] == 'v' && filename[tmp - 2] == 's' && filename[tmp - 3] == 'w' && filename[tmp - 4] == '.'))
            filename += ".wsv";
    }
    name = filename;
    QFile file(filename);
    file.open(QIODevice::NewOnly);
    file.close();
    on_saveButton_clicked();
}

QString WaveEditForm::save_wave()
{
    QString data = "";
    wave tmpwave = *resultant_wave;
    data.append("Number of waves: " + QString::number(wave_list.size()) + "\n");
    data.append("ms_per_div " + QString::number(ms_per_div) + '\n');
    data.append("v_per_div " + QString::number(v_per_div) + '\n');
    data.append("dac_divider " + QString::number(dac_divider) + '\n');

    data.append("resultant wave\n");

    //data.append("name " + tmpwave.name + '\n');
    data.append("samples " + QString::number(tmpwave.samples.size()) + "\n");
    data.append("period " + QString::number(tmpwave.period) + '\n');
    data.append("dac_divider " + QString::number(tmpwave.dac_divider) + '\n');
    data.append("data\n");
    for(uint i = 0; i < tmpwave.samples.size(); i++)
        data.append(QString::number(tmpwave.samples[i]) + '\n');
    /*{
        char tmp = (tmpwave.samples[i] >> 8) & 0xFF;
        data.append(tmp);
        tmp = tmpwave.samples[i] & 0xFF;
        data.append(tmp);
    }*/

    data.append("wave_list\n");
    int i = 0;
    for(const wave & tmpwave : wave_list)
    {
        data.append("wave " + QString::number(i) + '\n');
        data.append("name " + tmpwave.name + '\n');
        data.append("samples " + QString::number(tmpwave.samples.size()) + "\n");
        data.append("period " + QString::number(tmpwave.period) + '\n');
        data.append("dac_divider " + QString::number(tmpwave.dac_divider) + '\n');
        data.append("data\n");
        for(uint i = 0; i < tmpwave.samples.size(); i++)
            data.append(QString::number(tmpwave.samples[i]) + '\n');
        /*{,
            char tmp = (tmpwave.samples[i] >> 8) & 0xFF;
            data.append(tmp);
            tmp = tmpwave.samples[i] & 0xFF;
            data.append(tmp);
        }*/
        i++;
    }

    return data;
}
