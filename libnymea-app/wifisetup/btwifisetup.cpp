#include "btwifisetup.h"
#include "bluetoothdeviceinfo.h"
#include "types/wirelessaccesspoints.h"
#include "types/wirelessaccesspoint.h"

#include <QJsonDocument>

static QBluetoothUuid wifiServiceUuid =                 QBluetoothUuid(QUuid("e081fec0-f757-4449-b9c9-bfa83133f7fc"));
static QBluetoothUuid wifiCommanderCharacteristicUuid = QBluetoothUuid(QUuid("e081fec1-f757-4449-b9c9-bfa83133f7fc"));
static QBluetoothUuid wifiResponseCharacteristicUuid =  QBluetoothUuid(QUuid("e081fec2-f757-4449-b9c9-bfa83133f7fc"));
static QBluetoothUuid wifiStatusCharacteristicUuid =    QBluetoothUuid(QUuid("e081fec3-f757-4449-b9c9-bfa83133f7fc"));

static QBluetoothUuid networkServiceUuid =                  QBluetoothUuid(QUuid("ef6d6610-b8af-49e0-9eca-ab343513641c"));
static QBluetoothUuid networkStatusCharacteristicUuid =     QBluetoothUuid(QUuid("ef6d6611-b8af-49e0-9eca-ab343513641c"));
static QBluetoothUuid networkCommanderCharacteristicUuid =  QBluetoothUuid(QUuid("ef6d6612-b8af-49e0-9eca-ab343513641c"));
static QBluetoothUuid networkResponseCharacteristicUuid =   QBluetoothUuid(QUuid("ef6d6613-b8af-49e0-9eca-ab343513641c"));
static QBluetoothUuid networkingEnabledCharacteristicUuid = QBluetoothUuid(QUuid("ef6d6614-b8af-49e0-9eca-ab343513641c"));
static QBluetoothUuid wirelessEnabledCharacteristicUuid =   QBluetoothUuid(QUuid("ef6d6615-b8af-49e0-9eca-ab343513641c"));

static QBluetoothUuid systemServiceUuid =                 QBluetoothUuid(QUuid("e081fed0-f757-4449-b9c9-bfa83133f7fc"));
static QBluetoothUuid systemCommanderCharacteristicUuid = QBluetoothUuid(QUuid("e081fed1-f757-4449-b9c9-bfa83133f7fc"));
static QBluetoothUuid systemResponseCharacteristicUuid =  QBluetoothUuid(QUuid("e081fed2-f757-4449-b9c9-bfa83133f7fc"));

BtWiFiSetup::BtWiFiSetup(QObject *parent) : QObject(parent)
{
    m_accessPoints = new WirelessAccessPoints(this);
    qRegisterMetaType<BluetoothDeviceInfo*>("const BluetoothDeviceInfo*");
}

void BtWiFiSetup::connectToDevice(const BluetoothDeviceInfo *device)
{
    qDebug() << "device" << device;
    if (m_btController) {
        delete m_btController;
        m_status = StatusDisconnected;
        emit statusChanged(m_status);

    }

    m_btController = QLowEnergyController::createCentral(device->bluetoothDeviceInfo(), this);
    connect(m_btController, &QLowEnergyController::connected, this, [this](){
        qDebug() << "Bluetooth connected";
        m_btController->discoverServices();
    });

    connect(m_btController, &QLowEnergyController::disconnected, this, [this](){
        qDebug() << "Bluetooth disconnected";
        m_status = StatusDisconnected;
        emit statusChanged(m_status);
    });

    typedef void (QLowEnergyController::*errorsSignal)(QLowEnergyController::Error);
    connect(m_btController, static_cast<errorsSignal>(&QLowEnergyController::error), this, [this](QLowEnergyController::Error error){
        qDebug() << "Bluetooth error:" << error;
        emit this->error();
    });

    connect(m_btController, &QLowEnergyController::discoveryFinished, this, [this](){
        qDebug() << "Bluetooth service discovery finished";
        setupServices();
    });

    m_status = StatusConnectingToBluetooth;
    emit statusChanged(m_status);
    m_btController->connectToDevice();
}

void BtWiFiSetup::connectDeviceToWiFi(const QString &ssid)
{
    if (m_status != StatusConnectedToBluetooth) {
        qWarning() << "Cannot connect to wifi in state" << m_status;
    }
}

BtWiFiSetup::Status BtWiFiSetup::status() const
{
    return m_status;
}

QString BtWiFiSetup::modelNumber() const
{
    return m_modelNumber;
}

QString BtWiFiSetup::manufacturer() const
{
    return m_manufacturer;
}

QString BtWiFiSetup::softwareRevision() const
{
    return m_softwareRevision;
}

QString BtWiFiSetup::firmwareRevision() const
{
    return m_firmwareRevision;
}

QString BtWiFiSetup::hardwareRevision() const
{
    return m_hardwareRevision;
}

BtWiFiSetup::NetworkStatus BtWiFiSetup::networkStatus() const
{
    return m_networkStatus;
}

BtWiFiSetup::WirelessStatus BtWiFiSetup::wirelessStatus() const
{
    return m_wirelessStatus;
}

bool BtWiFiSetup::networkingEnabled() const
{
    return m_networkingEnabled;
}

bool BtWiFiSetup::wirelessEnabled() const
{
    return m_wirelessEnabled;
}

WirelessAccessPoints *BtWiFiSetup::accessPoints() const
{
    return m_accessPoints;
}

WirelessAccessPoint *BtWiFiSetup::currentConnection() const
{
    return m_currentConnection;
}

void BtWiFiSetup::setupServices()
{
    m_deviceInformationService = m_btController->createServiceObject(QBluetoothUuid::DeviceInformation, m_btController);
    m_networkService = m_btController->createServiceObject(networkServiceUuid, m_btController);
    m_wifiService = m_btController->createServiceObject(wifiServiceUuid, m_btController);
    m_systemService = m_btController->createServiceObject(systemServiceUuid, m_btController);

    if (!m_wifiService || !m_deviceInformationService || !m_networkService) {
        qWarning() << "Required services not found on remote device.";
        m_btController->disconnectFromDevice();
        return;
    }

    // Device information
    connect(m_deviceInformationService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state) {
        if (state != QLowEnergyService::ServiceDiscovered)
            return;
        qDebug() << "Device info service discovered";
        m_manufacturer = QString::fromUtf8(m_deviceInformationService->characteristic(QBluetoothUuid::ManufacturerNameString).value());
        emit manufacturerChanged();
        m_modelNumber = QString::fromUtf8(m_deviceInformationService->characteristic(QBluetoothUuid::ModelNumberString).value());
        emit modelNumberChanged();
        m_softwareRevision = QString::fromUtf8(m_deviceInformationService->characteristic(QBluetoothUuid::SoftwareRevisionString).value());
        emit softwareRevisionChanged();
        m_firmwareRevision = QString::fromUtf8(m_deviceInformationService->characteristic(QBluetoothUuid::FirmwareRevisionString).value());
        emit firmwareRevisionChanged();
        m_hardwareRevision = QString::fromUtf8(m_deviceInformationService->characteristic(QBluetoothUuid::HardwareRevisionString).value());
        emit hardwareRevisionChanged();
    });
    m_deviceInformationService->discoverDetails();


    // network service
    connect(m_networkService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state){
        if (state != QLowEnergyService::ServiceDiscovered)
            return;
        qDebug() << "Network service discovered";
        QLowEnergyCharacteristic networkCharacteristic = m_networkService->characteristic(networkStatusCharacteristicUuid);
        QLowEnergyCharacteristic networkingEnabledCharacteristic = m_networkService->characteristic(networkingEnabledCharacteristicUuid);
        QLowEnergyCharacteristic wirelessEnabledCharacteristic = m_networkService->characteristic(wirelessEnabledCharacteristicUuid);
        if (!networkCharacteristic.isValid() || !networkingEnabledCharacteristic.isValid() || !wirelessEnabledCharacteristic.isValid()) {
            qWarning() << "Required characteristics not found on remote device (NetworkService)";
            m_btController->disconnectFromDevice();
            return;
        }
        // Enable notifications
        m_networkService->writeDescriptor(networkCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));
        m_networkService->writeDescriptor(networkingEnabledCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));
        m_networkService->writeDescriptor(wirelessEnabledCharacteristic.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

        m_networkStatus = static_cast<NetworkStatus>(networkCharacteristic.value().toHex().toUInt(nullptr, 16));
        emit networkStatusChanged();
        m_networkingEnabled = networkingEnabledCharacteristic.value().toHex().toUInt(nullptr, 16);
        emit networkingEnabledChanged();
        m_wirelessEnabled = wirelessEnabledCharacteristic.value().toHex().toUInt(nullptr, 16);
        emit wirelessEnabledChanged();

    });
    connect(m_networkService, &QLowEnergyService::characteristicChanged, this, &BtWiFiSetup::characteristicChanged);
    m_networkService->discoverDetails();

    // Wifi service
    connect(m_wifiService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state){
        if (state != QLowEnergyService::ServiceDiscovered)
            return;

        qDebug() << "Wifi service discovered";
        m_status = StatusConnectedToBluetooth;
        emit statusChanged(m_status);

        // Enable notifations
        m_wifiService->writeDescriptor(m_wifiService->characteristic(wifiResponseCharacteristicUuid).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));
        m_wifiService->writeDescriptor(m_wifiService->characteristic(wifiStatusCharacteristicUuid).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

        QVariantMap request;
        request.insert("c", (int)WirelessServiceCommandGetNetworks);
        streamData(request);
    });
    connect(m_wifiService, &QLowEnergyService::characteristicChanged, this, &BtWiFiSetup::characteristicChanged);
    m_wifiService->discoverDetails();


    // System service (optional)
    if (m_systemService) {
        connect(m_systemService, &QLowEnergyService::stateChanged, this, [this](QLowEnergyService::ServiceState state){
            if (state != QLowEnergyService::ServiceDiscovered)
                return;
            qDebug() << "System service discovered";
            m_systemService->writeDescriptor(m_systemService->characteristic(systemResponseCharacteristicUuid).descriptor(QBluetoothUuid::ClientCharacteristicConfiguration), QByteArray::fromHex("0100"));

        });
        m_systemService->discoverDetails();
    }
}

void BtWiFiSetup::streamData(const QVariantMap &request)
{
    QLowEnergyCharacteristic characteristic = m_wifiService->characteristic(wifiCommanderCharacteristicUuid);
    if (!characteristic.isValid()) {
        qWarning() << "WifiSetupManager: WirelessService: Wireless commander characteristic not valid";
        return;
    }

    QByteArray data = QJsonDocument::fromVariant(request).toJson(QJsonDocument::Compact) + '\n';
    qDebug() << "WifiSetupManager: WirelessService: Start streaming response data:" << data.count() << "bytes";

    int sentDataLength = 0;
    QByteArray remainingData = data;
    while (!remainingData.isEmpty()) {
        QByteArray package = remainingData.left(20);
        sentDataLength += package.count();
        m_wifiService->writeCharacteristic(characteristic, package);
        remainingData = remainingData.remove(0, package.count());
    }

    qDebug() << "WifiSetupManager: WirelessService: Finished streaming request data";
}

void BtWiFiSetup::processWiFiPacket(const QVariantMap &data)
{
    WirelessServiceCommand command = static_cast<WirelessServiceCommand>(data.value("c").toInt());
    WirelessServiceResponse responseCode = (WirelessServiceResponse)data.value("r").toInt();
    if (responseCode != WirelessServiceResponseSuccess) {
        qWarning() << "Error in wifi command:" << responseCode;
        return;
    }

    qDebug() << "command reply:" << command;
    switch (command) {
    case WirelessServiceCommandGetNetworks:

        m_accessPoints->clearModel();

        foreach (const QVariant &data, data.value("p").toList()) {
            WirelessAccessPoint *accessPoint = new WirelessAccessPoint(this);

            accessPoint->setSsid(data.toMap().value("e").toString());
            accessPoint->setMacAddress(data.toMap().value("m").toString());
            accessPoint->setSignalStrength(data.toMap().value("s").toInt());
            accessPoint->setProtected(data.toMap().value("p").toBool());
            accessPoint->setHostAddress("");
            m_accessPoints->addWirelessAccessPoint(accessPoint);
        }
    }
}

void BtWiFiSetup::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &data)
{
    m_inputBuffers[characteristic.uuid()].append(data);
    if (m_inputBuffers[characteristic.uuid()].endsWith("\n")) {
        QByteArray data = m_inputBuffers.take(characteristic.uuid());

        QJsonParseError error;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "Invalid json data received:" << error.errorString() << data;
            m_btController->disconnectFromDevice();
            return;
        }

        if (characteristic.uuid() == wifiResponseCharacteristicUuid) {
            processWiFiPacket(jsonDoc.toVariant().toMap());
        } else {
            qWarning() << "Unhandled packet from characteristic" << characteristic.uuid();
        }

    }
}
