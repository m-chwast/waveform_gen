#ifndef WAVE_DATA_H
#define WAVE_DATA_H

#include <QString>
#include "wave.h"
#include <list>



class wave_data
{
private:


public:

    QString file_name;

    std::list<wave> wave_list;
    wave resultant_wave;

    double ms_per_div, v_per_div;
    int dac_divider;




    wave_data();
    wave_data(QString name);

    bool save();

};

#endif // WAVE_DATA_H
