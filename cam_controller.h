#ifndef CAM_CONTROLLER_H
#define CAM_CONTROLLER_H

#include <QString>

#include "spw_camera.h"
#include "prospect_regs.h"
#include "cmv_registers.h"
#include "cmv4000.h"
#include "imx253.h"

class CAM_controller
{
public:
    CAM_controller(IMX253 *imx253, CMV4000 *cmv4000, SPW_camera *spw_camera);

    quint8 config_SENS_CTL(quint8 cam_fw_type, quint8 test_mode, quint8 nb_bits, quint8 footer);
    double get_cmv_in_freq(quint8 cam_fw_type, quint8 cmv_freq_div);
    void   clk_config(quint8 cam_fw_type, quint8 cmv_freq_div, quint8 bypass);
    void   Set_Exp_cmv(double exposure_time, double cmv_in_freq);
    void   Set_Exp_imx(double exposure_time_ms, quint8 imx_in_freq,  quint8 nb_bits_sensor, quint8 nb_chan, quint32 height, quint8 DDR_mode_disable);
    void   Set_nb_chan_reg(quint8 nb_chan);
    double Get_IMX_Freq(quint8 IMXFreqCurrentIndex);
    void reset_sensor();
private:
    SPW_camera *spw_camera;
    CMV4000 *cmv4000;
    IMX253  *imx253;
};

#endif // CAM_CONTROLLER_H
