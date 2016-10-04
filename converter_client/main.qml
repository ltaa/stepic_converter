
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2

ApplicationWindow {

    id: root
    visible: true
    width: recId.width
    height: recId.height

    color: "skyblue"
    Rectangle {
        id: recId
        width: 640
        height: 480
        color: "skyblue"

        Rectangle {
            id: openFileId
            anchors.verticalCenter: recId.verticalCenter
            anchors.left: recId.left
            anchors.leftMargin: 10
            width: 200
            height: 50
            color: "lightcyan"

            Text {
                id: fopenId

                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                text:  qsTr("open file")
            }

            MouseArea {
                anchors.fill: parent

                onClicked: {
                    fileDialog.visible = true
                }

            }
        }

        ComboBox {
            id: formatBoxId
            anchors.left: openFileId.right
            anchors.leftMargin: 20
            anchors.top: openFileId.top
            width: openFileId.width
            height: openFileId.height

            textRole: "key"
            model: ListModel {
                ListElement { key: "ogg"; value: "ogg" }
                ListElement { key: "flac"; value: "flac" }
                ListElement { key: "mp3"; value: "mp3" }
            }
        }

    }

    FileDialog {
        id: fileDialog
        title: "Please choose a file"
        onAccepted: {
            iodata.sendData(fileDialog.fileUrls, formatBoxId.currentText)
        }
        onRejected: {
            console.log("Canceled")
        }
    }


    title: "AppWindow title"



}
