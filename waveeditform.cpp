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

extern serial_class serial;


WaveEditForm::WaveEditForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WaveEditForm)
{
    ui->setupUi(this);

    QObject::connect(this, &WaveEditForm::transmit_widgets_change_sig, this, &WaveEditForm::transmit_widgets_change);
    QObject::connect(this, &WaveEditForm::transmit_set_progress_bar_val_sig, this, &WaveEditForm::transmit_set_progress_bar_val);
    QObject::connect(this, &WaveEditForm::transmit_error_sig, this, &WaveEditForm::transmit_error);


    v_per_div = 0.5;
    ms_per_div = 1;
    dac_divider = 1;
    name = "new wave";
    resultant_wave = new wave;

    transmit_thread = nullptr;

    draw_background();

}


WaveEditForm::~WaveEditForm()
{
    delete resultant_wave;

    if(transmit_thread != nullptr)
        transmit_thread->join();

    delete ui;
}

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




bool WaveEditForm::update_resultant()
{
    resultant_wave->period = 1000;
    resultant_wave->dac_divider = 1;
    double a;
    for(wave &v : wave_list)
    {
        int b = (int)resultant_wave->get_period(time_unit::us);
        int c = (int)v.get_period(time_unit::us);
        //approximation:
        if(b > c)
        {
            int tmp = b;
            b = c;
            c = tmp;
        }
        int tmp = c / b;
        double relative_error = abs(1.0 - (double)b * tmp / c);
        if(relative_error > 0.01)   //1% is good
        {
            a = 1000 * std::lcm(b, c);
            if(a < 1)  ///exact solution unavailable, lets try next approximation
            {
                if(b > c)
                {
                    int tmp = b;
                    b = c;
                    c = tmp;
                }
                int tmp = c / b;
                double relative_error = abs(1.0 - (double)b * tmp / c);
                if(relative_error < 0.02)   //2% is pretty good
                {
                    a = 1000 * c;
                }
                else
                    return false;
            }
        }
        else
            a = 1000 * c;
        resultant_wave->period = a;
    }
    if(wave_list.empty() == true)
    {
        resultant_wave->period = 1000000;   ///1 kHz by default
        resultant_wave->dac_divider = 1;
    }
    float frequency = 1/resultant_wave->get_period(time_unit::s);
    int sample_no = DAC_FREQUENCY / resultant_wave->dac_divider / frequency;
    while(sample_no > max_samples)
    {
        resultant_wave->dac_divider *= 2;
        sample_no = DAC_FREQUENCY / resultant_wave->dac_divider / frequency;
    }
    resultant_wave->samples.clear();
    for(int i = 0; i < sample_no; i++)
        resultant_wave->samples.push_back(0);
    for(wave &v : wave_list)
    {
        for(int i = 0; i < sample_no; i++)
            resultant_wave->samples[i] += v.samples[(int)(i / ((double)v.dac_divider / resultant_wave->dac_divider)) % v.samples.size()];
    }
    return true;

}


void WaveEditForm::on_pushButton_clicked()
{

        bool ok1, ok2;
        double freq = ui->lineEdit->text().toDouble(&ok1);
        float ampl = ui->lineEdit_2->text().toDouble(&ok2);

        if(freq < 1 || freq > (DAC_FREQUENCY / dac_divider) / 10 || false)   //assume min 10 samples for correct wave generation
        {
            ui->lineEdit->setText("incorrect");
            return;
        }
        if(ampl <= 0 || ampl > 10.0)
        {
            ui->lineEdit_2->setText("incorrect");
            return;
        }
        if(ok1 && ok2)
        {
            wave *tmp_wave;
            if(ui->comboBox->currentText() == "sine")
            {
                tmp_wave = new wave(1000000000.0 / freq, ampl / volts_per_step, freq, wave_type::sine);
            }
            else if(ui->comboBox->currentText() == "square")
            {
                tmp_wave = new wave(1000000000.0/freq, ampl/  volts_per_step, freq, wave_type::square);
            }
            else if(ui->comboBox->currentText() == "triangle")
            {
                tmp_wave = new wave(1000000000.0/freq, ampl/  volts_per_step, freq, wave_type::triangle);
            }
            else if(ui->comboBox->currentText() == "sawtooth")
            {
                tmp_wave = new wave(1000000000.0/freq, ampl/  volts_per_step, freq, wave_type::sawtooth);
            }
            else if(ui->comboBox->currentText() == "const")
            {
                tmp_wave = new wave(1000000000.0/freq, ampl/  volts_per_step, freq, wave_type::offset);
            }
            else
                tmp_wave = new wave(1000000000.0 / 100, 1.0 / volts_per_step, 100, wave_type::sine);
            wave_list.push_back(*tmp_wave);

            if(update_resultant() == false)
            {
                wave_list.pop_back();
                ui->lineEdit->setText("couldn't merge");
                update_resultant();
            }
            else
            {
                ui->comboBox_2->addItem(tmp_wave->name);
                if(ui->checkBox_2->isChecked() == true || ui->lineEdit_4->text() == "" || (ui->lineEdit_4->text().toDouble()) <= 0)
                    ms_per_div = resultant_wave->get_period() * 1000 / 10;
                draw_background();
                draw_wave();
            }
            delete(tmp_wave);
        }
}


void WaveEditForm::on_pushButton_2_clicked()
{
    if(wave_list.empty() == true)
        return;
    std::list<wave>::iterator to_delete;
    for(std::list<wave>::iterator i = wave_list.begin(); i != wave_list.end(); i++)
    {
        if(ui->comboBox_2->currentText() == i->name)
        {
            to_delete = i;
            break;
        }
    }

    wave_list.erase(to_delete);
    ui->comboBox_2->removeItem(ui->comboBox_2->currentIndex());
    update_resultant();
    update_wave();
}

void WaveEditForm::update_wave()
{
    if(ui->checkBox_2->isChecked() == true)
        ms_per_div = resultant_wave->get_period() * 1000 / 10;
    draw_background();
    draw_wave();
}

void WaveEditForm::transmit_wave(wave wave_to_transmit)
{
    std::vector<uint8_t> data;
    uint16_t samples = wave_to_transmit.samples.size();
    if(samples == 0)
    {
        emit transmit_widgets_change_sig(true);
        return;
    }
    emit transmit_widgets_change_sig(false);
    emit transmit_set_progress_bar_val_sig(0);
    ////first two bytes is dac_divider
    ////second two is number of samples
    data.push_back(((uint16_t)wave_to_transmit.dac_divider >> 8));
    data.push_back((uint16_t)wave_to_transmit.dac_divider & 0xFF);
    data.push_back(samples >> 8);
    data.push_back(samples & 0xFF);


    if(serial.port_opened() == true)
    {
        serial.send_data("wave\n");
        std::unique_lock<std::mutex> lck {serial.ask_mutex};
        serial.ask_for("LD\n");
        lck.unlock();
        uint timeout_cnt = 0;
        while(serial.ask_cplt() == false)
        {
            if(timeout_cnt >= TIMEOUT_MS)
            {
                emit transmit_widgets_change_sig(true);
                emit transmit_error_sig(error_code_t::TIMEOUT_NO_LD);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timeout_cnt++;
        }
        serial.send_data(&data[0], data.size());
        lck.lock();
        serial.ask_for_16b(samples);
        lck.unlock();
        timeout_cnt = 0;
        while(serial.ask_cplt() == false)
        {
            if(timeout_cnt >= TIMEOUT_MS)
            {
                emit transmit_widgets_change_sig(true);
                emit transmit_error_sig(error_code_t::TIMEOUT_NO_SAMPLES);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timeout_cnt++;
        }
        data.clear();   ///tested to this point4
        ///next bytes are data bytes
        for(int i = 0; i < ceil((double)samples / 100); i++)
        {
            for(int j = 0; j < 100; j++)
            {
                if(i * 100 + j < samples)
                {
                    data.push_back((wave_to_transmit.samples[i * 100 + j] >> 8) & 0xFF);
                    data.push_back((wave_to_transmit.samples[i * 100 + j]) & 0xFF);
                }
                else
                {
                    data.push_back(0);
                    data.push_back(0);
                }
                // sum += data[j];
            }
            serial.send_data(&data[0], 200);

            lck.lock();
            if(i < ceil((double)samples / 100) - 1)
                serial.ask_for("OK\n");
            else
                serial.ask_for("END\n");
            lck.unlock();
            timeout_cnt = 0;
            while(serial.ask_cplt() == false)
            {
                if(timeout_cnt >= TIMEOUT_MS)
                {
                    if(i < ceil((double)samples / 100) - 1)
                        emit transmit_error_sig(error_code_t::TIMEOUT_NO_OK);
                    else
                        emit transmit_error_sig(error_code_t::TIMEOUT_NO_END);
                    emit transmit_widgets_change_sig(true);
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                timeout_cnt++;
            }
            data.clear();
            emit transmit_set_progress_bar_val_sig((i * 100) / (samples / 100) + 1);
        }
        emit transmit_error_sig(error_code_t::NONE);
    }
    emit transmit_set_progress_bar_val_sig(100);
    emit transmit_widgets_change_sig(true);
}



void WaveEditForm::on_checkBox_3_stateChanged(int /*arg1*/)
{
    update_wave();
}


void WaveEditForm::on_checkBox_stateChanged(int /*arg1*/)
{
    if(ui->checkBox->isChecked() == false)
    {
        double tmpv = 0;
        ui->lineEdit_3->setEnabled(true);
        if(ui->lineEdit_3->text() != "" && (tmpv = (ui->lineEdit_3->text().toDouble())) > 0)
        {
            v_per_div = tmpv;
            update_wave();
        }

    }
    else
    {
        ui->lineEdit_3->setEnabled(false);
        v_per_div = 0.5;
        update_wave();
    }
}


void WaveEditForm::on_lineEdit_3_editingFinished()
{
    double tmpv = 0;
    if(ui->lineEdit_3->text() != "" && (tmpv = (ui->lineEdit_3->text().toDouble())) > 0)
    {
        v_per_div = tmpv;
        update_wave();
    }
}


void WaveEditForm::on_lineEdit_3_textEdited(const QString & /*arg1*/)
{
    double tmpv = 0;
    if(ui->lineEdit_3->text() != "" && (tmpv = (ui->lineEdit_3->text().toDouble())) > 0)
    {
        v_per_div = tmpv;
        update_wave();
    }
}


void WaveEditForm::on_checkBox_4_stateChanged(int /*arg1*/)
{
    update_wave();
}


void WaveEditForm::on_comboBox_2_currentIndexChanged(int /*index*/)
{
    update_wave();
}


void WaveEditForm::on_checkBox_2_stateChanged(int /*arg1*/)
{
    if(ui->checkBox_2->isChecked() == true)
    {
        ms_per_div = resultant_wave->get_period() * 1000 / 10;
        ui->lineEdit_4->setEnabled(false);
    }
    else
    {
        ui->lineEdit_4->setEnabled(true);
        double tmpt = 0;
        if(ui->lineEdit_4->text() != "" && (tmpt = (ui->lineEdit_4->text().toDouble())) > 0)
        {
            ms_per_div = tmpt;
            update_wave();
        }
        else
           ms_per_div = resultant_wave->get_period() * 1000 / 10;
    }
    update_wave();
}


void WaveEditForm::on_lineEdit_4_textEdited(const QString &/*arg1*/)
{
    double tmpt = 0;
    if(ui->lineEdit_4->text() != "" && (tmpt = (ui->lineEdit_4->text().toDouble())) > 0)
    {
        ms_per_div = tmpt;
        update_wave();
    }
}


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
    ui->comboBox_2->clear();
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
    *resultant_wave = new_wave.resultant_wave;
    wave_list.clear();
    wave_list = new_wave.wave_list;
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

void WaveEditForm::on_transmitButton_clicked()
{
    ui->transmitButton->setEnabled(false);
    std::unique_ptr<std::thread>::pointer tmp = transmit_thread.release();
    if(tmp)
        tmp->join();
    transmit_thread.reset(std::unique_ptr<std::thread>::pointer(new std::thread{&WaveEditForm::transmit_wave, this, *resultant_wave}));
}

void WaveEditForm::transmit_widgets_change(bool tr_enabled)
{
    ui->transmitButton->setEnabled(tr_enabled);
    ui->progressBar->setEnabled(!tr_enabled);
}

void WaveEditForm::transmit_set_progress_bar_val(int value)
{
    if(value < 0)
        value = -value;
    value %= 101;
    ui->progressBar->setValue(value);
}

void WaveEditForm::transmit_error(error_code_t error_code)
{
    if(error_code == error_code_t::NONE)
    {
        return;
    }
    QString message_str = "Timeout error, transmission aborted.\nError detalis: ";
    switch(error_code)
    {
    case error_code_t::TIMEOUT_NO_LD:
    {
        message_str += "\"LD\\n\" not received.";
        break;
    }
    case error_code_t::TIMEOUT_NO_END:
    {
        message_str += "\"END\\n\" not received.";
        break;
    }
    case error_code_t::TIMEOUT_NO_OK:
    {
        message_str += "\"OK\\n\" not received.";
        break;
    }
    case error_code_t::TIMEOUT_NO_SAMPLES:
    {
        message_str += "number of samples not received.";
        break;
    }
    default:
    {
        message_str += "other error.";
        break;
    }
    }
    QMessageBox::information(this, "Transmission error", message_str);
}



