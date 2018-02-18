/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2017 Simon Stuerz <simon.stuerz@guh.io>                  *
 *                                                                         *
 *  This file is part of guh-control.                                      *
 *                                                                         *
 *  guh-control is free software: you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 3 of the License.                *
 *                                                                         *
 *  guh-control is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh-control. If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "devicemanager.h"
#include "engine.h"
#include "jsonrpc/jsontypes.h"

DeviceManager::DeviceManager(JsonRpcClient* jsonclient, QObject *parent) :
    JsonHandler(parent),
    m_vendors(new Vendors(this)),
    m_plugins(new Plugins(this)),
    m_devices(new Devices(this)),
    m_deviceClasses(new DeviceClasses(this)),
    m_jsonClient(jsonclient)
{
    m_jsonClient->registerNotificationHandler(this, "notificationReceived");
}

void DeviceManager::clear()
{
    m_devices->clearModel();
    m_deviceClasses->clearModel();
    m_vendors->clearModel();
    m_plugins->clearModel();
}

void DeviceManager::init()
{
    m_jsonClient->sendCommand("Devices.GetPlugins", this, "getPluginsResponse");
}

QString DeviceManager::nameSpace() const
{
    return "Devices";
}

Vendors *DeviceManager::vendors() const
{
    return m_vendors;
}

Plugins *DeviceManager::plugins() const
{
    return m_plugins;
}

Devices *DeviceManager::devices() const
{
    return m_devices;
}

DeviceClasses *DeviceManager::deviceClasses() const
{
    return m_deviceClasses;
}

void DeviceManager::addDevice(const QUuid &deviceClassId, const QString &name, const QVariantList &deviceParams)
{
    qDebug() << "add device " << deviceClassId.toString();
    QVariantMap params;
    params.insert("deviceClassId", deviceClassId.toString());
    params.insert("name", name);
    params.insert("deviceParams", deviceParams);
    m_jsonClient->sendCommand("Devices.AddConfiguredDevice", params, this, "addDeviceResponse");
}

void DeviceManager::notificationReceived(const QVariantMap &data)
{
    QString notification = data.value("notification").toString();
    if (notification == "Devices.StateChanged") {
//        qDebug() << "Device state changed" << data.value("params");
        Device *dev = m_devices->getDevice(data.value("params").toMap().value("deviceId").toUuid());
        if (!dev) {
            qWarning() << "Device state change notification received for an unknown device";
            return;
        }
        dev->setStateValue(data.value("params").toMap().value("stateTypeId").toUuid(), data.value("params").toMap().value("value"));
    } else if (notification == "Devices.DeviceAdded") {
        Device *dev = JsonTypes::unpackDevice(data.value("params").toMap().value("device").toMap(), m_devices);
        if (!dev) {
            qWarning() << "Cannot parse json device:" << data;
            return;
        }
        m_devices->addDevice(dev);
    } else if (notification == "Devices.DeviceRemoved") {
        QUuid deviceId = data.value("params").toMap().value("deviceId").toUuid();
        qDebug() << "JsonRpc: Notification: Device removed" << deviceId.toString();
        Device *device = m_devices->getDevice(deviceId);
        m_devices->removeDevice(device);
        device->deleteLater();
    } else {
        qWarning() << "DeviceManager unhandled device notification received" << notification;
    }
}

void DeviceManager::getVendorsResponse(const QVariantMap &params)
{
//    qDebug() << "Got GetSupportedVendors response" << params;
    if (params.value("params").toMap().keys().contains("vendors")) {
        QVariantList vendorList = params.value("params").toMap().value("vendors").toList();
        foreach (QVariant vendorVariant, vendorList) {
            Vendor *vendor = JsonTypes::unpackVendor(vendorVariant.toMap(), Engine::instance()->deviceManager()->vendors());
            m_vendors->addVendor(vendor);
//            qDebug() << "Added Vendor:" << vendor->name();
        }
    }

    qDebug() << "start getting deviceClass at" << QDateTime::currentDateTime();

    m_jsonClient->sendCommand("Devices.GetSupportedDevices", this, "getSupportedDevicesResponse");
}

void DeviceManager::getSupportedDevicesResponse(const QVariantMap &params)
{
    if (params.value("params").toMap().keys().contains("deviceClasses")) {
        QVariantList deviceClassList = params.value("params").toMap().value("deviceClasses").toList();
        foreach (QVariant deviceClassVariant, deviceClassList) {
            DeviceClass *deviceClass = JsonTypes::unpackDeviceClass(deviceClassVariant.toMap(), Engine::instance()->deviceManager()->deviceClasses());
            qDebug() << "Server has device class:" << deviceClass->name() << deviceClass->id();
            m_deviceClasses->addDeviceClass(deviceClass);
        }
    }
    m_jsonClient->sendCommand("Devices.GetConfiguredDevices", this, "getConfiguredDevicesResponse");
}

void DeviceManager::getPluginsResponse(const QVariantMap &params)
{
    qDebug() << "received plugins";
    if (params.value("params").toMap().keys().contains("plugins")) {
        QVariantList pluginList = params.value("params").toMap().value("plugins").toList();
        foreach (QVariant pluginVariant, pluginList) {
            Plugin *plugin = JsonTypes::unpackPlugin(pluginVariant.toMap(), Engine::instance()->deviceManager()->plugins());
            m_plugins->addPlugin(plugin);
        }
    }
    m_jsonClient->sendCommand("Devices.GetSupportedVendors", this, "getVendorsResponse");
}

void DeviceManager::getConfiguredDevicesResponse(const QVariantMap &params)
{
    if (params.value("params").toMap().keys().contains("devices")) {
        QVariantList deviceList = params.value("params").toMap().value("devices").toList();
        foreach (QVariant deviceVariant, deviceList) {
            Device *device = JsonTypes::unpackDevice(deviceVariant.toMap(), Engine::instance()->deviceManager()->devices());
            if (!device) continue;

//            qDebug() << QJsonDocument::fromVariant(deviceVariant).toJson();
            DeviceClass *dc = m_deviceClasses->getDeviceClass(device->deviceClassId());
            if (!dc) {
                qWarning() << "Can't find a deviceClass for this device" << device->name();
                continue;
            }

            // set initial state values
            QVariantList stateVariantList = deviceVariant.toMap().value("states").toList();
            foreach (const QVariant &stateMap, stateVariantList) {
                QUuid stateTypeId = stateMap.toMap().value("stateTypeId").toUuid();
                StateType *st = dc->stateTypes()->getStateType(stateTypeId);
                if (!st) {
                    qWarning() << "Can't find a statetype for this state";
                    continue;
                }
                QVariant value = stateMap.toMap().value("value");
                if (st->type() == "Bool") {
                    value.convert(QVariant::Bool);
                } else if (st->type() == "Double") {
                    value.convert(QVariant::Double);
                } else if (st->type() == "Int") {
                    value.convert(QVariant::Int);
                }
                device->setStateValue(stateTypeId, value);
            }
            Engine::instance()->deviceManager()->devices()->addDevice(device);
        }
    }
}

void DeviceManager::addDeviceResponse(const QVariantMap &params)
{
    if (params.value("params").toMap().value("deviceError").toString() != "DeviceErrorNoError") {
        qWarning() << "Failed to add the device:" << params.value("params").toMap().value("deviceError").toString();
    } else if (params.value("params").toMap().keys().contains("device")) {
        QVariantMap deviceVariant = params.value("params").toMap().value("device").toMap();
        Device *device = JsonTypes::unpackDevice(deviceVariant, m_devices);
        qDebug() << "Device added" << device->id().toString();
        m_devices->addDevice(device);
    }
    emit addDeviceReply(params.value("params").toMap());
}

void DeviceManager::removeDeviceResponse(const QVariantMap &params)
{
    qDebug() << "Device removed response" << params;
    emit removeDeviceReply(params.value("params").toMap());
}

void DeviceManager::pairDeviceResponse(const QVariantMap &params)
{
    emit pairDeviceReply(params.value("params").toMap());
}

void DeviceManager::confirmPairintResponse(const QVariantMap &params)
{
    emit confirmPairingReply(params.value("params").toMap());
}

void DeviceManager::addDiscoveredDevice(const QUuid &deviceClassId, const QUuid &deviceDescriptorId, const QString &name)
{
    qDebug() << "JsonRpc: add discovered device " << deviceClassId.toString();
    QVariantMap params;
    params.insert("deviceClassId", deviceClassId.toString());
    params.insert("name", name);
    params.insert("deviceDescriptorId", deviceDescriptorId.toString());
    m_jsonClient->sendCommand("Devices.AddConfiguredDevice", params);
}

void DeviceManager::pairDevice(const QUuid &deviceClassId, const QUuid &deviceDescriptorId)
{
    qDebug() << "JsonRpc: pair device " << deviceClassId.toString();
    QVariantMap params;
    params.insert("name", "name");
    params.insert("deviceClassId", deviceClassId.toString());
    params.insert("deviceDescriptorId", deviceDescriptorId.toString());
    m_jsonClient->sendCommand("Devices.PairDevice", params, this, "pairDeviceResponse");
}

void DeviceManager::confirmPairing(const QUuid &pairingTransactionId, const QString &secret)
{
    qDebug() << "JsonRpc: confirm pairing" << pairingTransactionId.toString();
    QVariantMap params;
    params.insert("pairingTransactionId", pairingTransactionId.toString());
    params.insert("secret", secret);
    m_jsonClient->sendCommand("Devices.ConfirmPairing", params, this, "confirmPairingResponse");
}

void DeviceManager::removeDevice(const QUuid &deviceId)
{
    qDebug() << "JsonRpc: delete device" << deviceId.toString();
    QVariantMap params;
    params.insert("deviceId", deviceId.toString());
    m_jsonClient->sendCommand("Devices.RemoveConfiguredDevice", params, this, "removeDeviceResponse");
}

void DeviceManager::executeAction(const QUuid &deviceId, const QUuid &actionTypeId, const QVariantList &params)
{
    qDebug() << "JsonRpc: execute action " << deviceId.toString() << actionTypeId.toString() << params;
    QVariantMap p;
    p.insert("deviceId", deviceId.toString());
    p.insert("actionTypeId", actionTypeId.toString());
    if (!params.isEmpty()) {
        p.insert("params", params);
    }

    qDebug() << "Params:" << p;
    m_jsonClient->sendCommand("Actions.ExecuteAction", p);
}
