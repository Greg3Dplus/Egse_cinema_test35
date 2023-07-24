#ifndef SPW_CAMERA_H
#define SPW_CAMERA_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QFileInfo>

#include "spw_interface.h"

#define TMTC_SIZE_MAX 32

const quint8 tmtc_channelNumber = 1;
const quint8 vid_channelNumber = 2;

const quint8 TC_LOGICAL_AD  = 32;
const quint8 TM_LOGICAL_AD  = 33;
const quint8 VID_LOGICAL_AD = 34;

const quint8 PROTOCOL_ID    = 0x55;

class SPW_camera: public QObject
{
    Q_OBJECT

public:
    SPW_camera();
    ~SPW_camera();

    void set_SPW_params(quint8 spw_port_num, quint16 spw_mult, quint16 spw_div);
    quint8 getPortNum(void);
    quint16 get_SPW_mult(void);
    quint16 get_SPW_div(void);

    QString get_tmtc_channel (void);
    QString get_vid_channel (void);

    void setSPW_TimeoutVal(QString channel, qint32 timeout);
    bool config_interfaces(quint16 mult, quint16 div);

    bool close_all_channels();

    quint8 config_CU();
    void save_script_file(QString file_name);

    bool SPW_send_pkt(QByteArray data, QString channelName, quint8 numLink);

    QByteArray SPW_rcv_pkt(QString channel, QByteArray LogicalAddress);
    QByteArray SPW_rcv_pkt(bool *status, QString channel, QByteArray LogicalAddress);
    bool config_SPW_Brick(quint8 port_num, quint16 mult, quint16 div);
    void DisplayConfigPortInformation(Port *pPort, bool clearErrors);
    bool get_SPW_Running();

    bool send_TC(quint32 addr, quint8 data);
    bool send_TC(quint8 nb_data, quint32 addr, quint32 data);
    bool send_TC(quint8 nb_data, quint32 addr, const quint8* data);
    bool send_TC(quint32 addr, QByteArray data);

    QByteArray send_TM(quint8 nb_data, quint32 addr);
    quint8 send_TM(quint32 addr);

    QByteArray read_data(quint32 nb_data, quint32 addr);
    void write_data(quint32 addr, QByteArray data);

    QByteArray SPW_rcv_vid_pkt(QByteArray LogicalAddress);
    void setSPW_vid_TimeoutVal(qint32 timeout);

private:
    // SpaceWire
    SPW_interface spacewire;
    quint8 spw_port_num;
    quint16 spw_mult;
    quint16 spw_div;
    QString tmtc_channel;
    QString vid_channel;
    bool IsSPWRunning;

    QStringList GUI_commands;  // Will record all TM/TC commands sent

signals:
    void CHANNEL_NOT_OPEN();
    void PACKET_SIZE_ERROR();

};

#endif // SPW_CAMERA_H
