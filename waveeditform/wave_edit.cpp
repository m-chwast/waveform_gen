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
