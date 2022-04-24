#ifndef WAVE_H
#define WAVE_H

#include <vector>
#include <stdint.h>
#include <QString>


////constants:

constexpr float reference_voltage = 3.3;
constexpr float volts_per_step = reference_voltage / 1023;
constexpr uint16_t max_samples = 20000;
constexpr double DAC_FREQUENCY = 1000000;
constexpr double V_MAX = 2.5, V_MIN = -2.5;


/////enums:

enum class frequency_unit {Hz, kHz, MHz};
enum class time_unit {s, ms, us};
enum class wave_type {sine, square, triangle, sawtooth, general, offset};

//////classes:

class wave
{
private:
    void set_name();




protected:





public:
    wave() : period{1000}, type{wave_type::general}, dac_divider{1}  {}
    wave(double per) :period {per}, type{wave_type::sine} {}
    //wave(double per, uint16_t amplitude_steps, float frequency) {wave(per, amplitude_steps, frequency, wave_type::sine);}
    wave(double per, uint16_t amplitude_steps, float frequency, wave_type type_of_wave);
    ~wave() {}


    volatile double period;	//period is stored in nanoseconds
    std::vector<int16_t> samples;
    wave_type type;
    int dac_divider;
    QString name;


    double get_period(time_unit time);
    double get_period()	{return get_period(time_unit::s);}
    float get_frequency(frequency_unit freq);
    float get_frequency() {return get_frequency(frequency_unit::Hz);}
    uint16_t amplitude_steps();
    float amplitude_volts();
    float amplitude()	{return amplitude_volts();}
    float rms();
    float mean();
    int16_t get_sample(uint16_t sample_no)	{if(sample_no < samples.size())	return samples[sample_no]; else return 0;}
    float get_sample_volts(uint16_t sample_no)	{return volts_per_step * get_sample(sample_no);}
    bool resize_by(float scale);
    bool resize_to(float desired_amplitude_volts);
    bool resize_to(uint16_t desired_amplitude_steps);
    bool maximize() {return resize_to(reference_voltage);}
    bool offset_steps(int16_t offset);
    bool offset_volts(float offset)	{return offset_steps(offset / volts_per_step);}
    //bool clip(float voltage);



};

inline bool operator==(wave v, wave u) {return u.get_period() == v.get_period() && u.amplitude_steps() == v.amplitude_steps();}

class sine_wave : public wave
{
private:

public:
    sine_wave() : wave(1000000000.0 / 50)	{sine_wave((float)1, (float)50);}
    sine_wave(float amplitude, float frequency)	: wave(1000000000.0 / frequency, (uint16_t)(amplitude / volts_per_step), frequency, wave_type::sine) {}//{sine_wave((uint16_t)(amplitude / volts_per_step), frequency);}
   // sine_wave(uint16_t amplitude_steps, float frequency) : wave(1000000000.0 / frequency, amplitude_steps, frequency) {};

    ~sine_wave() {}

};






#endif // WAVE_H
