import QtQuick 2.8
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.2
import "components"
import Nymea 1.0

Page {
    id: root
    header: GuhHeader {
        text: qsTr("Magic")
        onBackPressed: pageStack.pop()

        HeaderButton {
            imageSource: Qt.resolvedUrl("images/add.svg")
            onClicked: {
                addRule()
            }
        }
    }

    function addRule() {
        var newRule = Engine.ruleManager.createNewRule();
        d.editRulePage = pageStack.push(Qt.resolvedUrl("magic/EditRulePage.qml"), {rule: newRule });
        d.editRulePage.StackView.onRemoved.connect(function() {
            newRule.destroy();
        })
        d.editRulePage.onAccept.connect(function() {
            d.editRulePage.busy = true;
            Engine.ruleManager.addRule(d.editRulePage.rule);
        })
        d.editRulePage.onCancel.connect(function() {
            pageStack.pop();
        })
    }

    QtObject {
        id: d
        property var editRulePage: null
    }

    Connections {
        target: Engine.ruleManager
        onAddRuleReply: {
            d.editRulePage.busy = false;
            if (ruleError == "RuleErrorNoError") {
                print("should tag rule now:", d.editRulePage.rule.id, d.editRulePage.ruleIcon, d.editRulePage.ruleColor)
                Engine.tagsManager.tagRule(ruleId, "color", d.editRulePage.ruleColor)
                Engine.tagsManager.tagRule(ruleId, "icon", d.editRulePage.ruleIcon)
                pageStack.pop();
            } else {
                var popup = errorDialog.createObject(app, {errorCode: ruleError })
                popup.open();
            }
        }

        onEditRuleReply: {
            d.editRulePage.busy = false;
            if (ruleError == "RuleErrorNoError") {
                print("should tag rule now:", d.editRulePage.ruleIcon, d.editRulePage.ruleColor)
                Engine.tagsManager.tagRule(d.editRulePage.rule.id, "color", d.editRulePage.ruleColor)
                Engine.tagsManager.tagRule(d.editRulePage.rule.id, "icon", d.editRulePage.ruleIcon)
                pageStack.pop();
            } else {
                var popup = errorDialog.createObject(app, {errorCode: ruleError })
                popup.open();
            }
        }
    }

    ListView {
        anchors.fill: parent

        model: RulesFilterModel {
            id: rulesProxy
            rules: Engine.ruleManager.rules
        }
        delegate: MeaListItemDelegate {
            id: ruleDelegate
            width: parent.width
            iconName: "../images/" + (model.executable ? (iconTag ? iconTag.value : "slideshow") : "magic") + ".svg"
            iconColor: model.executable ? (colorTag ? colorTag.value : app.guhAccent) : !model.enabled ? "red" : (model.active ? app.guhAccent : "grey")
            text: model.name
            canDelete: true

            property var colorTag: model.executable ? Engine.tagsManager.tags.findRuleTag(model.id, "color") : null
            property var iconTag: model.executable ? Engine.tagsManager.tags.findRuleTag(model.id, "icon") : null
            Connections {
                target: Engine.tagsManager.tags
                onCountChanged: {
                    colorTag = Engine.tagsManager.tags.findRuleTag(model.id, "color")
                    iconTag = Engine.tagsManager.tags.findRuleTag(model.id, "icon")
                }
            }

            onDeleteClicked: Engine.ruleManager.removeRule(model.id)

            onClicked: {
                var newRule = rulesProxy.get(index).clone();
                d.editRulePage = pageStack.push(Qt.resolvedUrl("magic/EditRulePage.qml"), {rule: newRule})
                d.editRulePage.StackView.onRemoved.connect(function() {
                    newRule.destroy();
                })
                d.editRulePage.onAccept.connect(function() {
                    d.editRulePage.busy = true;
                    Engine.ruleManager.editRule(d.editRulePage.rule);
                })
                d.editRulePage.onCancel.connect(function() {
                    pageStack.pop();
                })
            }
        }
    }

    EmptyViewPlaceholder {
        anchors { left: parent.left; right: parent.right; margins: app.margins }
        anchors.verticalCenter: parent.verticalCenter
        visible: Engine.ruleManager.rules.count === 0
        title: qsTr("There is no magic set up yet.")
        text: qsTr("Use magic to make your things smart! In a few easy steps you'll have your things wired up and work for you.")
        imageSource: "images/magic.svg"
        buttonText: qsTr("Add some magic")
        onImageClicked: addRule()
        onButtonClicked: addRule()
    }

    Component {
        id: errorDialog
        ErrorDialog {}
    }
}