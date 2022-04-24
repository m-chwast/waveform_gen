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
    dont_draw = false;
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
    if(dont_draw == true)
        return;
    if(ui->checkBox_2->isChecked() == true)
        ms_per_div = resultant_wave->get_period() * 1000 / 10;
    draw_background();
    draw_wave();
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
    if(ui->comboBox_2->currentIndex() != -1)
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


