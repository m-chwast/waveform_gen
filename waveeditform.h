#ifndef WAVEEDITFORM_H
#define WAVEEDITFORM_H

#include <QWidget>
#include <QCloseEvent>
#include <QApplication>
#include <QString>
#include <QFile>
#include <wave.h>
#include <thread>
#include <utility>



enum class error_code_t {TIMEOUT_NO_LD, TIMEOUT_NO_OK, TIMEOUT_NO_END, TIMEOUT_OTHER, TIMEOUT_NO_SAMPLES, NONE};


namespace Ui {
class WaveEditForm;
}

class WaveEditForm : public QWidget
{
    Q_OBJECT

public:
    explicit WaveEditForm(QWidget *parent = nullptr);
    ~WaveEditForm();


protected:
    void closeEvent(QCloseEvent */*event*/) override {QApplication::quit();};

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_checkBox_3_stateChanged(int arg1);

    void on_checkBox_stateChanged(int arg1);

    void on_lineEdit_3_editingFinished();

    void on_lineEdit_3_textEdited(const QString &arg1);

    void on_checkBox_4_stateChanged(int arg1);

    void on_comboBox_2_currentIndexChanged(int index);

    void on_checkBox_2_stateChanged(int arg1);

    void on_lineEdit_4_textEdited(const QString &arg1);

    void on_loadButton_clicked();

    void on_saveButton_clicked();

    void on_saveButton_2_clicked();

    void on_transmitButton_clicked();


    void transmit_widgets_change(bool tr_enabled);
    void transmit_set_progress_bar_val(int value);
    void transmit_error(error_code_t error_code);




signals:
    void transmit_widgets_change_sig(bool tr_enabled);
    void transmit_set_progress_bar_val_sig(int value);
    void transmit_error_sig(error_code_t error_code);

private:
    Ui::WaveEditForm *ui;
    constexpr static int wave_start_hor = 50;
    constexpr static uint TIMEOUT_MS = 100;
    float v_per_div;
    float ms_per_div;

    std::list<wave> wave_list;

    wave * resultant_wave;

    int dac_divider;

    QString name;

    std::unique_ptr<std::thread> transmit_thread;

    void draw_background();
    void draw_wave();
    bool update_resultant();
    void update_wave();

    void transmit_wave(wave wave_to_transmit);

    QString save_wave();

};


#endif // WAVEEDITFORM_H
