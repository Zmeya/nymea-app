/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*
* Copyright 2013 - 2020, nymea GmbH
* Contact: contact@nymea.io
*
* This file is part of nymea.
* This project including source code and documentation is protected by
* copyright law, and remains the property of nymea GmbH. All rights, including
* reproduction, publication, editing and translation, are reserved. The use of
* this project is subject to the terms of a license agreement to be concluded
* with nymea GmbH in accordance with the terms of use of nymea GmbH, available
* under https://nymea.io/license
*
* GNU General Public License Usage
* Alternatively, this project may be redistributed and/or modified under the
* terms of the GNU General Public License as published by the Free Software
* Foundation, GNU version 3. This project is distributed in the hope that it
* will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details.
*
* You should have received a copy of the GNU General Public License along with
* this project. If not, see <https://www.gnu.org/licenses/>.
*
* For any further details and any questions please contact us under
* contact@nymea.io or see our FAQ/Licensing Information on
* https://nymea.io/license/faq
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

import QtQuick 2.6
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.1
import "../components"
import "../delegates"
import Nymea 1.0

Page {
    id: root

    property bool selectInterface: false
    property alias showEvents: interfacesProxy.showEvents
    property alias showActions: interfacesProxy.showActions
    property alias showStates: interfacesProxy.showStates
    property alias shownInterfaces: devicesProxy.shownInterfaces
    property bool allowSelectAny: false
    property bool multipleSelection: false
    property alias requiredEventName: devicesProxy.requiredEventName
    property alias requiredStateName: devicesProxy.requiredStateName
    property alias requiredActionName: devicesProxy.requiredActionName

    signal backPressed();
    signal thingSelected(var device);
    signal thingsSelected(var devices);
    signal interfaceSelected(string interfaceName);
    signal anySelected();

    header: NymeaHeader {
        text: root.selectInterface ?
                  qsTr("Select kind of things") :
                  root.shownInterfaces.length > 0 ? qsTr("Select %1").arg(app.interfaceToDisplayName(root.shownInterfaces[0])) : qsTr("Select thing")
        onBackPressed: root.backPressed()

        HeaderButton {
            imageSource: "../images/find.svg"
            color: filterInput.shown ? Style.accentColor : Style.iconColor
            onClicked: filterInput.shown = !filterInput.shown
        }
    }

    InterfacesProxy {
        id: interfacesProxy
        devicesFilter: engine.deviceManager.devices
    }

    DevicesProxy {
        id: devicesProxy
        engine: _engine
        groupByInterface: true
        nameFilter: filterInput.shown ? filterInput.text : ""
        Component.onCompleted: {
            print("showing devices for interfaces", devicesProxy.shownInterfaces)
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListFilterInput {
            id: filterInput
            Layout.fillWidth: true
        }

        NymeaSwipeDelegate {
            Layout.fillWidth: true
            text: qsTr("Any %1").arg(app.interfaceToDisplayName(root.shownInterfaces[0]))
            visible: root.allowSelectAny
            onClicked: {
                root.anySelected();
            }
        }
        ThinDivider { visible: root.allowSelectAny }

        GroupedListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.selectInterface ? interfacesProxy : devicesProxy
            clip: true
            property var checkBoxCache: ({})
            function toggleCheckBoxCache(deviceId) {
                var newCache = listView.checkBoxCache;
                if (!newCache.hasOwnProperty(deviceId) || !newCache[deviceId]) {
                    newCache[deviceId] = true
                } else {
                    newCache[deviceId] = false
                }
                listView.checkBoxCache = newCache;
                print("new checked state;", newCache[deviceId])
            }

            delegate: NymeaSwipeDelegate {
                width: parent.width
                text: root.selectInterface ? model.displayName : model.name
                iconName: root.selectInterface ? app.interfaceToIcon(model.name) : app.interfacesToIcon(model.interfaces)
                onClicked: {
                    if (root.selectInterface) {
                        root.interfaceSelected(interfacesProxy.get(index).name)
                    } else if (!root.multipleSelection) {
                        root.thingSelected(devicesProxy.get(index))
                    } else {
                        listView.toggleCheckBoxCache(model.id)
                    }
                }
                progressive: !root.multipleSelection

                additionalItem: root.multipleSelection ? entryCheckBox : null
                CheckBox {
                    id: entryCheckBox
                    height: parent.height
                    visible: root.multipleSelection
                    checked: listView.checkBoxCache.hasOwnProperty(model.id) && listView.checkBoxCache[model.id]
                    onClicked: listView.toggleCheckBoxCache(model.id)
                }
            }
        }

        Button {
            Layout.fillWidth: true
            Layout.margins: app.margins
            text: qsTr("OK")
            visible: root.multipleSelection
            onClicked: {
                var devices = []
                for (var i = 0; i < devicesProxy.count; i++) {
                    var device = devicesProxy.get(i);
                    if (listView.checkBoxCache.hasOwnProperty(device.id) && listView.checkBoxCache[device.id]) {
                        devices.push(device)
                    }
                }
                root.thingsSelected(devices)
            }
        }
    }
}
