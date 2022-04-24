#include "wave.h"
#include <algorithm>
#include <numeric>
#include <math.h>
#include <QString>


wave::wave(double per, uint16_t amplitude_steps, float frequency, wave_type type_of_wave)
{
    period = per;
    type = type_of_wave;
    dac_divider = 1;

    if(type_of_wave == wave_type::offset)
    {
        period = 1000;
    }

    std::vector<int16_t> tmpvect(max_samples);

    switch(type)
    {
    case wave_type::sine :
    {
        for(uint i = 0; i < tmpvect.size(); i++)
        {
            tmpvect[i] = amplitude_steps * ( sin(2 * 3.14 *(float)i / tmpvect.size()));
        }
        break;
    }
    case wave_type::square :
    {
        for(uint i = 0; i < tmpvect.size(); i++)
        {
            tmpvect[i] = (i < tmpvect.size() / 2) ? amplitude_steps : -amplitude_steps;
        }
        break;
    }
    case wave_type::triangle :
    {
        double change =  amplitude_steps / ((double)tmpvect.size() / 4);
        for(uint i = 1; i < tmpvect.size() / 4; i++)
            tmpvect[i] = i * change;
        for(uint i = tmpvect.size() / 4; i < tmpvect.size() / 2; i++)
            tmpvect[i] = amplitude_steps - (i - tmpvect.size() / 4) * change;
        for(uint i = tmpvect.size() / 2; i < 3 * (tmpvect.size() / 4); i++)
            tmpvect[i] = -((i - tmpvect.size() / 2) * change);
        for(uint i = 3 * (tmpvect.size() / 4); i < tmpvect.size(); i++)
            tmpvect[i] = -amplitude_steps + (i - 3 * (tmpvect.size() / 4)) * change;
        break;
    }
    case wave_type::sawtooth :
    {
        double change =  (double)amplitude_steps / tmpvect.size();
        for(uint i = 1; i < tmpvect.size(); i++)
            tmpvect[i] = i * change;
        break;
    }
    case wave_type::offset :
    {
        for(uint i = 0; i < tmpvect.size(); i++)
        {
            tmpvect[i] = amplitude_steps;
        }
        break;
    }
    default:
    {
        for(uint i = 0; i < tmpvect.size(); i++)
            tmpvect[i] = 0;
        break;
    }
    }


    float sample_no = (DAC_FREQUENCY / dac_divider) / frequency;
    while(sample_no > tmpvect.size())
    {
        dac_divider *= 2;
        sample_no = (DAC_FREQUENCY / dac_divider) / frequency;
    }
    for(int i = 0; i < sample_no; i++)
        samples.push_back(tmpvect[(int)(i *  tmpvect.size() / sample_no)] );

    set_name();

}

double wave::get_period(time_unit time)
{
    double divider = 1;
    switch(time)
    {
    case time_unit::us:
        divider = 1000;
        break;
    case time_unit::ms:
        divider = 1000000;
        break;
    case time_unit::s:
        divider = 1000000000;
        break;
    default:
        break;
    }
    return period / divider;
}

float wave::get_frequency(frequency_unit freq)
{
    uint32_t multiplier = 1;
    switch(freq)
    {
    case frequency_unit::Hz:
        multiplier = 1000000;
        break;
    case frequency_unit::kHz:
        multiplier = 1000;
        break;
    case frequency_unit::MHz:
        break;
    default:
        break;
    }
    return multiplier * 1000.0 / period;
}

uint16_t wave::amplitude_steps()
{
    auto res = std::minmax_element(samples.begin(), samples.end());
    return *(res.second) - *(res.first);
}

inline float wave::amplitude_volts()	{return amplitude_steps() * volts_per_step;}

float wave::mean()	{return std::accumulate(samples.begin(), samples.end(), (uint32_t)0) / (float)samples.size();}

float wave::rms()
{
    float res = 0;
    for(int16_t elem : samples)
        res += elem * elem;
    res /= samples.size();
    return sqrt(res);
}

bool wave::resize_by(float scale)
{
    if(scale < 0)
        return false;
    bool not_distorted = true;
    for(int16_t& val : samples)
    {
        if(val * scale > 1023)
        {
            val = 1023;
            not_distorted = false;
        }
        else
            val *= scale;
    }
    return not_distorted;
}

inline bool wave::resize_to(uint16_t desired_amplitude_steps)	{return resize_by(float(desired_amplitude_steps) / amplitude_steps());}

inline bool wave::resize_to(float desired_amplitude_volts)	{return resize_by(desired_amplitude_volts / amplitude_volts()); }

bool wave::offset_steps(int16_t offset)
{
    bool not_distorted = true;
    for(int16_t& val : samples)
    {
        if((int32_t)val + offset > 1023)
        {
            val = 1023;
            not_distorted = false;
        }
        else if((int32_t)val + offset < 0)
        {
            val = 0;
            not_distorted = false;
        }
        else
            val += offset;
    }
    return not_distorted;
}

void wave::set_name()
{
    QString tmp;
    if(type == wave_type::sine)
        tmp += "sine ";
    else if(type == wave_type::square)
        tmp += "square ";
    else if(type == wave_type::triangle)
        tmp += "triangle ";
    else if(type == wave_type::sawtooth)
        tmp += "saw ";
    else if(type == wave_type::general)
        tmp += "general ";

    tmp += QString::number(get_frequency(frequency_unit::Hz) / 1000) + " kHz ";
    tmp+= QString::number(amplitude() / 2, 'g', 3) + " V";   //amplitude refers to vpp
    name = tmp;

    if(type == wave_type::offset)
    {
        name = "offset " + QString::number(samples[0] * volts_per_step, 'g', 3) + " V";
    }
}


