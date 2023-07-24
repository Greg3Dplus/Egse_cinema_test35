#include "spw_camera.h"

SPW_camera::SPW_camera()
{
    this->tmtc_channel  = "tmtc_channel";
    this->vid_channel   = "vid_channel";

    this->IsSPWRunning = false;

    this->spacewire.SetEnableError(true);
    this->spacewire.SetEnableMessage(false);
}

SPW_camera::~SPW_camera() {
    this->close_all_channels();
    this->spacewire.SetLink(this->spw_port_num, false);
}

bool SPW_camera::SPW_send_pkt(QByteArray data, QString channelName, quint8 numLink) {
    return this->spacewire.SPW_send_pkt(data, channelName, numLink);
}

QByteArray SPW_camera::SPW_rcv_vid_pkt(QByteArray LogicalAddress) {

    return this->SPW_rcv_pkt(this->vid_channel, LogicalAddress);
}

QByteArray SPW_camera::SPW_rcv_pkt(QString channel, QByteArray LogicalAddress) {
    bool status;
    return this->SPW_rcv_pkt(&status, channel, LogicalAddress);
}

QByteArray SPW_camera::SPW_rcv_pkt(bool *status, QString channel, QByteArray LogicalAddress) {
    return this->spacewire.SPW_rcv_pkt(status, channel, LogicalAddress, 2, 0);
}

quint8 SPW_camera::send_TM(quint32 addr) {
    QByteArray ret = send_TM(1, addr);

    return static_cast<quint8>(ret.at(0));
}

QByteArray SPW_camera::send_TM(quint8 nb_data, quint32 addr) {

    QByteArray data;
    data.fill(0x00, nb_data);

    if (nb_data > 128) {
        qDebug () << "ERROR: Too many data in TM";
    }

    Packet transmitPacket;
    TransmitOperation transmitOp;

    quint8 addr_byte_3 = (0xFF000000 & addr) >> 24;
    quint8 addr_byte_2 = (0x00FF0000 & addr) >> 16;
    quint8 addr_byte_1 = (0x0000FF00 & addr) >>  8;
    quint8 addr_byte_0 = (0x000000FF & addr) >>  0;
    quint8 RW_nb_data  =  0x80 + nb_data;

    if (nb_data == 128) {
        RW_nb_data     =  0x80;
    }

    QByteArray transmitBuffer;
    transmitBuffer.append(TC_LOGICAL_AD);
    transmitBuffer.append(PROTOCOL_ID);
    transmitBuffer.append(static_cast<qint8>(RW_nb_data));
    transmitBuffer.append(static_cast<qint8>(addr_byte_3));
    transmitBuffer.append(static_cast<qint8>(addr_byte_2));
    transmitBuffer.append(static_cast<qint8>(addr_byte_1));
    transmitBuffer.append(static_cast<qint8>(addr_byte_0));


    this->spacewire.SPW_send_pkt(transmitBuffer, this->tmtc_channel, this->spw_port_num);

    QByteArray logicalAdd;
    logicalAdd.append(TM_LOGICAL_AD);
    data = this->SPW_rcv_pkt(this->tmtc_channel, logicalAdd);

    if (data.size() != nb_data)
    {
        data.clear();
        data.fill(0x00, nb_data);
        emit PACKET_SIZE_ERROR();
        qDebug().noquote()  << "Size error";
    }
    return data;
}

bool SPW_camera::send_TC(quint32 addr, quint8 data) {

    return send_TC(1, addr, &data);
}

bool SPW_camera::send_TC(quint8 nb_data, quint32 addr, quint32 data) {
    quint8 data_ar [TMTC_SIZE_MAX];

    for (quint8 i = 0; i < nb_data; i++) {
        data_ar[i] = (data >> (8*i)) & 0xff;
    }

    return send_TC(nb_data, addr, data_ar);
}

bool SPW_camera::send_TC(quint32 addr, QByteArray data) {
    quint8 data_ar [TMTC_SIZE_MAX];

    if (data.size() > TMTC_SIZE_MAX) {
        qDebug() << "ERROR in send_TC function: Too many data requested";
        return false;
    }

    for (quint8 i = 0; i < data.size(); i++) {
        data_ar[i] = static_cast<quint8>(data.at(i));
    }

    return send_TC(static_cast<quint8>(data.size()), addr, data_ar);
}

bool SPW_camera::send_TC(quint8 nb_data, quint32 addr, const quint8* data)
{
    QString cmd_str = "WRITE " + QString::number(addr, 16);

    if (nb_data > 128) {
        qDebug () << "ERROR: Too many data in TC";
    }

    Packet transmitPacket;
    TransmitOperation transmitOp;

    quint8 addr_byte_3 = (0xFF000000 & addr) >> 24;
    quint8 addr_byte_2 = (0x00FF0000 & addr) >> 16;
    quint8 addr_byte_1 = (0x0000FF00 & addr) >>  8;
    quint8 addr_byte_0 = (0x000000FF & addr) >>  0;
    quint8 RW_nb_data  = nb_data;
    if (nb_data == 128) {
        RW_nb_data = 0;
    }

    QByteArray transmitBuffer;
    transmitBuffer.append(TC_LOGICAL_AD);
    transmitBuffer.append(PROTOCOL_ID);
    transmitBuffer.append(static_cast<qint8>(RW_nb_data));
    transmitBuffer.append(static_cast<qint8>(addr_byte_3));
    transmitBuffer.append(static_cast<qint8>(addr_byte_2));
    transmitBuffer.append(static_cast<qint8>(addr_byte_1));
    transmitBuffer.append(static_cast<qint8>(addr_byte_0));
    for (quint8 i = 0; i < nb_data; ++i) {
        transmitBuffer.append(static_cast<qint8>(data[i]));
        cmd_str.append(" " + QString::number(data[i], 16));
    }

    this->GUI_commands.append(cmd_str);
    return this->SPW_send_pkt(transmitBuffer, this->tmtc_channel, this->spw_port_num);

}

void SPW_camera::write_data(quint32 addr, QByteArray data) {
    quint32 nb_burst;
    quint32 nb_data = static_cast<quint32>(data.size());

    if (nb_data % TMTC_SIZE_MAX == 0) {
        nb_burst = (nb_data / TMTC_SIZE_MAX);
    }
    else {
        nb_burst = (nb_data / TMTC_SIZE_MAX) + 1;
    }

    // qDebug () << "TC nbBurst = " + QString::number(nb_burst, 10);

    for (quint32 num_burst = 0; num_burst < nb_burst; num_burst++) {
        quint8 nb_sub_data = TMTC_SIZE_MAX;
        if (num_burst == nb_burst-1) {
            nb_sub_data = static_cast<quint8>(nb_data - TMTC_SIZE_MAX*(nb_burst-1));
        }

        QByteArray dataToSend = data.mid(static_cast<qint32>(num_burst * TMTC_SIZE_MAX), nb_sub_data);

        send_TC(addr + num_burst * TMTC_SIZE_MAX, dataToSend);
    }
}

QByteArray SPW_camera::read_data(quint32 nb_data, quint32 addr) {
    quint32 nb_burst;
    QByteArray dataOut;
    dataOut.clear();

    if (nb_data % TMTC_SIZE_MAX == 0) {
        nb_burst = (nb_data / TMTC_SIZE_MAX);
    }
    else {
        nb_burst = (nb_data / TMTC_SIZE_MAX) + 1;
    }
    //qDebug () << "TM nbBurst = " + QString::number(nb_burst, 10);

    for (quint32 num_burst = 0; num_burst < nb_burst; num_burst++) {
        quint8 nb_sub_data = TMTC_SIZE_MAX;
        if (num_burst == nb_burst-1) {
            nb_sub_data = static_cast<quint8>(nb_data - TMTC_SIZE_MAX*(nb_burst-1));
        }
        dataOut.append(send_TM(nb_sub_data, addr + num_burst * TMTC_SIZE_MAX));
        //qDebug () << "TM address = 0x" + QString::number(addr + num_burst * TMTC_SIZE_MAX, 16);
    }

    return dataOut;
}

void SPW_camera::save_script_file(QString file_name) {
    QFile outputFile(file_name);
    QFileInfo fileinfo(file_name);

    qDebug().noquote()  << "Save script file to " << fileinfo.absoluteFilePath();

    // Create new file
    if (outputFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        QTextStream out(&outputFile);

        for(int i = 0; i < this->GUI_commands.size(); ++i) {
            out << this->GUI_commands.at(i) << "\n";
        }

        outputFile.close();
    }
    else {
        qDebug () << "ERROR: File not opened";
    }
}

quint8 SPW_camera::config_CU()
{
    quint8 configurate;

    this->spacewire.ClearChannel();
    this->spacewire.ResetDevice();
    //if (this->spacewire.getLinkStatus(this->spw_port_num))
        //this->spacewire.SetLink(this->spw_port_num, false);

    configurate = this->config_interfaces(this->spw_mult, this->spw_div);
    this->spacewire.SetLink(this->spw_port_num, true);
    this->IsSPWRunning = this->spacewire.getLinkStatus(this->spw_port_num);
    this->spacewire.setLinkPortNumber(this->spw_port_num);

    return configurate;
}

bool SPW_camera::get_SPW_Running() {
    return this->IsSPWRunning;
}

void SPW_camera::set_SPW_params(quint8 spw_port_num, quint16 spw_mult, quint16 spw_div) {
    this->spw_port_num = spw_port_num;
    this->spw_mult = spw_mult;
    this->spw_div = spw_div;
}

quint8 SPW_camera::getPortNum(void) {
    return this->spw_port_num;
}

quint16 SPW_camera::get_SPW_mult(void) {
    return this->spw_mult;
}

quint16 SPW_camera::get_SPW_div(void) {
    return this->spw_div;
}

void SPW_camera::DisplayConfigPortInformation(Port *pPort, bool clearErrors)
{
    Port *pConfigPort;
    ConfigPortErrors *pErrors = nullptr;

    /* cast given port to more specific form (it is known to be a configuration
     * port)
     */
    pConfigPort = reinterpret_cast<Port*>(pPort);

    /* display header for port information */
    qDebug () << "##\n##\nPort " + QString::number(pConfigPort->GetPortNumber(), 10) + " is a configuration port";

    /* get error information for the port */
    pErrors = reinterpret_cast<ConfigPortErrors*>(pConfigPort->GetErrors());

    /* if failed to obtain error information for port */
    if (!pErrors)
    {
        qDebug () << "Error: failed to obtain error information for config port";
    }
    else
    {
        /* display error information */
        /*qDebug () << "Error information:";
        qDebug () << "Number of errors found on port: " + pErrors->GetErrorCount();
        qDebug () << "Cargo too large error: " + (pErrors->GetCargoTooLarge() ? "set" : "not set");
        qDebug () << "Command not implemented error: " + pErrors->GetCommandNotImplemented() ? "set" : "not set";
        qDebug () << "Early EEP error: " + pErrors->GetEarlyEEP() ? "set" : "not set";
        qDebug () << "Early EOP error: " + pErrors->GetEarlyEOP() ? "set" : "not set";
        qDebug () << "Invalid Data CRC error: " + pErrors->GetInvalidDataCRC() ? "set" : "not set";
        qDebug () << "Invalid data length error: " + pErrors->GetInvalidDataLength() ? "set" : "not set";
        qDebug () << "Invalid destination key error: " + pErrors->GetInvalidDestinationKey() ? "set" : "not set";
        qDebug () << "Invalid destination logical address error: " + pErrors->GetInvalidDestinationLogicalAddress() ? "set" : "not set";
        qDebug () << "Invalid header CRC error: %s\n" + pErrors->GetInvalidHeaderCRC() ? "set" : "not set";
        qDebug () << "Invalid register address error: %s\n" + pErrors->GetInvalidRegisterAddress() ? "set" : "not set";
        qDebug () << "Invalid read-modify-write data length error: %s\n" + pErrors->GetInvalidRMWDataLength() ? "set" : "not set";
        qDebug () << "Late EEP error: " + pErrors->GetLateEEP() ? "set" : "not set";
        qDebug () << "Late EOP error: " + pErrors->GetLateEOP() ? "set" : "not set";
        qDebug () << "Port timeout error: " + pErrors->GetPortTimeoutError() ? "set" : "not set";
        qDebug () << "Source logical address error: " + pErrors->GetSourceLogicalAddressError() ? "set" : "not set";
        qDebug () << "Source path address error: " + pErrors->GetSourcePathAddressError() ? "set" : "not set";
        qDebug () << "Unsupported protocol error: " + pErrors->GetUnsupportedProtocol() ? "set" : "not set";
        qDebug () << "Unused RMAP command or packet type error: " + pErrors->GetUnusedRMAPCommandOrPacketType() ? "set" : "not set";
        qDebug () << "Verify buffer overrun error: " + pErrors->GetVerifyBufferOverrun() ? "set" : "not set";*/

        /* if errors are to be cleared */
        if (clearErrors)
        {
            if (!pConfigPort->ClearErrors())
            {
                qDebug () << "Error: failed to clear errors on port";
            }
        }

        /* free error information */
        delete pErrors;
    }
}

void SPW_camera::setSPW_vid_TimeoutVal(qint32 timeout) {
    this->setSPW_TimeoutVal(this->vid_channel, timeout);
}

void SPW_camera::setSPW_TimeoutVal(QString channel, qint32 timeout) {
    this->spacewire.setTimeOutVal(channel, timeout);
}

bool SPW_camera::close_all_channels() {

    if (this->spacewire.closeChannel(this->tmtc_channel) &&
            this->spacewire.closeChannel(this->vid_channel))
        return true;
    else return false;
}

bool SPW_camera::config_interfaces(quint16 mult, quint16 div) {

    if (!this->spacewire.NewChannel(this->tmtc_channel, tmtc_channelNumber, 1000)) return false;
    if (!this->spacewire.NewChannel(this->vid_channel, vid_channelNumber, 5000))   return false;
    if (!this->spacewire.find_SPW_Device())                                  return false;
    if (!this->spacewire.config_SPW_Brick(this->spw_port_num, mult, div))                        return false;
    this->spacewire.SetRoutingMode();

    if (!this->spacewire.RoutingAddress(TC_LOGICAL_AD, 0, 0, 1, (1 << this->spw_port_num)))
        qDebug().noquote()  << "Erreur";

    if (!this->spacewire.RoutingAddress(TM_LOGICAL_AD, 0, 0, 1, 1 <<(tmtc_channelNumber + 3)))
        qDebug().noquote()  << "Erreur";

    if (!this->spacewire.RoutingAddress(VID_LOGICAL_AD, 0, 0, 1, 1 <<(vid_channelNumber  + 3)))
        qDebug().noquote()  << "Erreur";


    if (!this->spacewire.config_router())                                    return false;

    qDebug().noquote()  << "Opening SPW channel...";
    if (!this->spacewire.open_spw_channel(this->tmtc_channel, 2))            return false;
    if (!this->spacewire.open_spw_channel(this->vid_channel, 2))             return false;

    return true;

}

bool SPW_camera::config_SPW_Brick(quint8 port_num, quint16 mult, quint16 div) {
    return this->spacewire.config_SPW_Brick(port_num, mult, div);
}

QString SPW_camera::get_tmtc_channel (void)
{
    return this->tmtc_channel;
}

QString SPW_camera::get_vid_channel (void)
{
    return this->vid_channel;
}
