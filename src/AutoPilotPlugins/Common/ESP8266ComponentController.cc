/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @brief  ESP8266 WiFi Config Qml Controller
///     @author Gus Grubba <mavlink@grubba.com>

#include "ESP8266ComponentController.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"
#include "UAS.h"

QGC_LOGGING_CATEGORY(ESP8266ComponentControllerLog, "ESP8266ComponentControllerLog")

#define MAX_RETRIES 5

//-----------------------------------------------------------------------------
ESP8266ComponentController::ESP8266ComponentController()
    : _waitType(WAIT_FOR_NOTHING)
    , _retries(0)
{
    for(int i = 1; i < 12; i++) {
        _channels.append(QString::number(i));
    }
    _baudRates.append("57600");
    _baudRates.append("115200");
    _baudRates.append("230400");
    _baudRates.append("460800");
    _baudRates.append("921600");
    connect(&_timer, &QTimer::timeout, this, &ESP8266ComponentController::_processTimeout);
    UASInterface* uas = dynamic_cast<UASInterface*>(_vehicle->uas());
    connect(uas, &UASInterface::commandAck, this, &ESP8266ComponentController::_commandAck);
    Fact* ssid = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID4");
    connect(ssid, &Fact::valueChanged, this, &ESP8266ComponentController::_ssidChanged);
    Fact* paswd = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD4");
    connect(paswd, &Fact::valueChanged, this, &ESP8266ComponentController::_passwordChanged);
    Fact* baud = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "UART_BAUDRATE");
    connect(baud, &Fact::valueChanged, this, &ESP8266ComponentController::_baudChanged);
    Fact* ver = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "SW_VER");
    connect(ver, &Fact::valueChanged, this, &ESP8266ComponentController::_versionChanged);
}

//-----------------------------------------------------------------------------
ESP8266ComponentController::~ESP8266ComponentController()
{

}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::version()
{
    uint32_t uv = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "SW_VER")->rawValue().toUInt();
    QString versionString = QString("%1.%2.%3").arg(uv >> 24).arg((uv >> 16) & 0xFF).arg(uv & 0xFFFF);
    return versionString;
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiSSID()
{
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID1")->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID2")->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID3")->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID4")->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiSSID(QString ssid)
{
    char tmp[20];
    memset(tmp, 0, sizeof(tmp));
    std::string	sid = ssid.toStdString();
    strncpy(tmp, sid.c_str(), sizeof(tmp));
    Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID1");
    Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID2");
    Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID3");
    Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_SSID4");
    uint32_t u;
    memcpy(&u, &tmp[0], sizeof(uint32_t));
    f1->setRawValue(QVariant(u));
    memcpy(&u, &tmp[4], sizeof(uint32_t));
    f2->setRawValue(QVariant(u));
    memcpy(&u, &tmp[8], sizeof(uint32_t));
    f3->setRawValue(QVariant(u));
    memcpy(&u, &tmp[12], sizeof(uint32_t));
    f4->setRawValue(QVariant(u));
}

//-----------------------------------------------------------------------------
QString
ESP8266ComponentController::wifiPassword()
{
    uint32_t s1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD1")->rawValue().toUInt();
    uint32_t s2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD2")->rawValue().toUInt();
    uint32_t s3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD3")->rawValue().toUInt();
    uint32_t s4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD4")->rawValue().toUInt();
    char tmp[20];
    memcpy(&tmp[0],  &s1, sizeof(uint32_t));
    memcpy(&tmp[4],  &s2, sizeof(uint32_t));
    memcpy(&tmp[8],  &s3, sizeof(uint32_t));
    memcpy(&tmp[12], &s4, sizeof(uint32_t));
    return QString(tmp);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setWifiPassword(QString password)
{
    char tmp[20];
    memset(tmp, 0, sizeof(tmp));
    std::string	pwd = password.toStdString();
    strncpy(tmp, pwd.c_str(), sizeof(tmp));
    Fact* f1 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD1");
    Fact* f2 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD2");
    Fact* f3 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD3");
    Fact* f4 = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "WIFI_PASSWORD4");
    uint32_t u;
    memcpy(&u, &tmp[0], sizeof(uint32_t));
    f1->setRawValue(QVariant(u));
    memcpy(&u, &tmp[4], sizeof(uint32_t));
    f2->setRawValue(QVariant(u));
    memcpy(&u, &tmp[8], sizeof(uint32_t));
    f3->setRawValue(QVariant(u));
    memcpy(&u, &tmp[12], sizeof(uint32_t));
    f4->setRawValue(QVariant(u));
}

//-----------------------------------------------------------------------------
int
ESP8266ComponentController::baudIndex()
{
    int b = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "UART_BAUDRATE")->rawValue().toInt();
    switch (b) {
        case 57600:
            return 0;
        case 115200:
            return 1;
        case 230400:
            return 2;
        case 460800:
            return 3;
        case 921600:
        default:
            return 4;
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::setBaudIndex(int idx)
{
    if(idx >= 0 && idx != baudIndex()) {
        int baud = 921600;
        switch(idx) {
            case 0:
                baud = 57600;
                break;
            case 1:
                baud = 115200;
                break;
            case 2:
                baud = 230400;
                break;
            case 3:
                baud = 460800;
                break;
            default:
                baud = 921600;
        }
        Fact* b = getParameterFact(MAV_COMP_ID_UDP_BRIDGE, "UART_BAUDRATE");
        b->setRawValue(baud);
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::reboot()
{
    _waitType = WAIT_FOR_REBOOT;
    emit busyChanged();
    _retries  = MAX_RETRIES;
    _reboot();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::restoreDefaults()
{
    _waitType = WAIT_FOR_RESTORE;
    emit busyChanged();
    _retries  = MAX_RETRIES;
    _restoreDefaults();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_reboot()
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
        &msg,
        _vehicle->id(),
        MAV_COMP_ID_UDP_BRIDGE,
        MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN,
        1.0f, // Confirmation
        0.0f, // Param1
        1.0f, // Param2
        0.0f,0.0f,0.0f,0.0f,0.0f);
    qCDebug(ESP8266ComponentControllerLog) << "_reboot()";
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    _timer.start(1000);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_restoreDefaults()
{
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(
        qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
        qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
        &msg,
        _vehicle->id(),
        MAV_COMP_ID_UDP_BRIDGE,
        MAV_CMD_PREFLIGHT_STORAGE,
        1.0f, // Confirmation
        2.0f, // Param1
        0.0f,0.0f,0.0f,0.0f,0.0f,0.0f);
    qCDebug(ESP8266ComponentControllerLog) << "_restoreDefaults()";
    _vehicle->sendMessageOnLink(_vehicle->priorityLink(), msg);
    _timer.start(1000);
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_processTimeout()
{
    if(!--_retries) {
        qCDebug(ESP8266ComponentControllerLog) << "_processTimeout Giving Up";
        _timer.stop();
        _waitType = WAIT_FOR_NOTHING;
        emit busyChanged();
    } else {
        switch(_waitType) {
            case WAIT_FOR_REBOOT:
                qCDebug(ESP8266ComponentControllerLog) << "_processTimeout for Reboot";
                _reboot();
                break;
            case WAIT_FOR_RESTORE:
                qCDebug(ESP8266ComponentControllerLog) << "_processTimeout for Restore Defaults";
                _restoreDefaults();
                break;
        }
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_commandAck(UASInterface*, uint8_t compID, uint16_t command, uint8_t result)
{
    if(compID == MAV_COMP_ID_UDP_BRIDGE) {
        if(result != MAV_RESULT_ACCEPTED) {
            qWarning() << "ESP8266ComponentController command" << command << "rejected.";
            return;
        }
        if((_waitType == WAIT_FOR_REBOOT  && command == MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN) ||
           (_waitType == WAIT_FOR_RESTORE && command == MAV_CMD_PREFLIGHT_STORAGE))
        {
            _timer.stop();
            _waitType = WAIT_FOR_NOTHING;
            emit busyChanged();
            qCDebug(ESP8266ComponentControllerLog) << "_commandAck for" << command;
            if(command == MAV_CMD_PREFLIGHT_STORAGE) {
                _autopilot->refreshAllParameters(MAV_COMP_ID_UDP_BRIDGE);
            }
        }
    }
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_ssidChanged(QVariant)
{
    emit wifiSSIDChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_passwordChanged(QVariant)
{
    emit wifiPasswordChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_baudChanged(QVariant)
{
    emit baudIndexChanged();
}

//-----------------------------------------------------------------------------
void
ESP8266ComponentController::_versionChanged(QVariant)
{
    emit versionChanged();
}
