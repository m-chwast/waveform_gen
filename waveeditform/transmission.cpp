#include "waveeditform.h"
#include "ui_waveeditform.h"
#include <cmath>
#include <vector>
#include <chrono>
#include <numeric>
#include <utility>
#include <mutex>
#include <thread>
#include "wave.h"
#include "wave_data.h"
#include "mainwindow.h"
#include <QMessageBox>


extern serial_class serial;


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

