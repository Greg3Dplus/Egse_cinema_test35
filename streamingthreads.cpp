#include "streamingthreads.h"


StreamingThreads::StreamingThreads(GUI_params* gui_params, CU_Frame **rcv_data_Byte, QMutex *mutex)
{
    this->gui_params    = gui_params;

    this->rcv_data_Byte = rcv_data_Byte;
    this->mutex = mutex;

}

StreamingThreads::~StreamingThreads()
{

}

void StreamingThreads::run()
{
    // qDebug() << "Inside RUN";
    this->mutex->lock();
    if (this->gui_params->spw_camera.get_SPW_Running() == 0)
    {
        this->mutex->unlock();
        return;
    }
    this->mutex->unlock();

    quint16 num_frame = 0;

    bool EOS = false;
    while (! EOS) {

        QByteArray logicalAdd;
        logicalAdd.append(VID_LOGICAL_AD);
        // qDebug() << "Before video pkt";
        QByteArray video_pkt = this->gui_params->spw_camera.SPW_rcv_vid_pkt(logicalAdd);
        num_frame++;

        qDebug().noquote()  << "Video packet received " << QString::number(video_pkt.size());
        this->mutex->lock();
        if ((*this->rcv_data_Byte)->init_pkt(video_pkt)) {
            emit FrameReceived();

            if ((*this->rcv_data_Byte)->isEOS()) {
                EOS = true;
                //qDebug().noquote() << "is EOS!";
            }
        }
        else {
            EOS = true;
        }
        this->mutex->unlock();
    }

    qDebug().noquote() << "Received "+ QString::number(num_frame,10) + " frames";

    quint8 CMV_STS = this->gui_params->spw_camera.send_TM(SENS_CTL_ba | CMVCTL_STS_AD);
    if ((CMV_STS & CMVCTL_STS_AHBERR_gm) != 0) {
        qDebug().noquote()  << "ERROR: CMV_STS AHBERR bit is asserted";
    }

    emit END_SEQUENCE();


    emit END_THREAD();

}

bool StreamingThreads::compare_ar(QByteArray ar1, QByteArray ar2) {
    quint32* idx_ignore = {};
    return compare_ar(ar1, ar2, idx_ignore, 0);
}

bool StreamingThreads::compare_ar(QByteArray ar1, QByteArray ar2, quint32* idx_ignore, quint32 size_ignore) {
    bool ret = true;

    if (ar1.size() != ar2.size()) {
        qDebug () << "ERROR: Size differs: Size 1 = " << ar1.size() << ", size2 = " << ar2.size();
        return false;
    }

    for (qint32 i = 0; i < ar1.size(); i++) {
        bool to_ignore = false;
        for (quint32 j = 0; j < size_ignore; j++) {
            if (idx_ignore[j] == static_cast<quint32>(i)) {
                to_ignore = true;
                //qDebug () << "Index " + QString::number(i, 10) + " ignored";
            }
        }

        if (ar1.at(i) != ar2.at(i) && to_ignore == false) {
            qDebug () << "Data " + QString::number(i) + " differs: Data1 = 0x" + QString::number(static_cast<quint8>(ar1.at(i)), 16) +
                         ", Data2 = 0x" + QString::number(static_cast<quint8>(ar2.at(i)), 16);
            ret = false;
        }
    }

    return ret;
}

quint32 StreamingThreads::to_int(quint8 size, QByteArray data) {
    return to_int(size, data, 0);
}

quint32 StreamingThreads::to_int(quint8 size, QByteArray data, quint32 offset) {
    quint32 value = 0;

    for (quint32 i = offset; i < offset + size; i++) {
        value |= static_cast<quint32>(data.at(static_cast<qint32>(i)) << (8*i));
    }

    return value;
}

void StreamingThreads::print_log(QString string,  QTextStream &out) {
    qDebug().noquote()  << string;
    out << string + "\n";
}

// Return bandwidth in MPixels/s
double StreamingThreads::get_DDR_bandwidth(quint8 coAdding_val, quint8 cam_fw_type) {
    //return 4 * SysClk_freq * 0.7;     // We use 30% margin
    if (cam_fw_type == CAM_FW_CO2M) {
        if (coAdding_val == 1) {
            return 320;
        }
        else {
            return 160;
        }
    }else if (cam_fw_type == CAM_FW_CINEMA) {
        if (coAdding_val == 1) {
            return 332; // 65% of 512
        }
        else {
            return 166;// 65% of 256
        }
    }
    else {
        return 26;
    }
}
