/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 1.1

/*!
    \qmlclass ValueSelector
    \inqmlmodule Ubuntu.Components.ListItems 0.1
    \brief List item displaying single selected value when not expanded,
    where expanding it opens a listing of all the possible values for selection.

    \b{This component is under heavy development.}

    Examples:
    \qml
        import Ubuntu.Components.ListItems 0.1 as ListItem
        Column {
            width: 250
            ListItem.ValueSelector {
                text: "Standard"
                values: ["Value 1", "Value 2", "Value 3", "Value 4"]
            }
            ListItem.ValueSelector {
                text: "Disabled"
                values: ["Value 1", "Value 2", "Value 3", "Value 4"]
                enabled: false
            }
            ListItem.ValueSelector {
                text: "Expanded"
                values: ["Value 1", "Value 2", "Value 3", "Value 4"]
                expanded: true
            }
            ListItem.ValueSelector {
                text: "Icon"
                iconSource: "icon.png"
                values: ["Value 1", "Value 2", "Value 3", "Value 4"]
                selectedIndex: 2
            }
        }
    \endqml
*/
Base {
    id: selector
    height: column.height

    /*!
      \preliminary
      The text that is shown in the list item as a label.
      \qmlproperty string text
     */
    property alias text: label.text

    /*!
      \preliminary
      The location of the icon to show in the list item (optional).
      \qmlproperty url iconSource
     */
    property alias iconSource: iconHelper.source

    /*!
      \preliminary
      The list of values that will be shown under the label text
     */
    property variant values

    /*!
      \preliminary
      The index of the currently selected element from the \l values array.
     */
    property int selectedIndex: 0

    /*!
      \preliminary
      Specifies whether the selector is 'open' or 'closed'.
     */
    property bool expanded: false

    Column {
        id: column
        anchors {
            left: parent.left
            right: parent.right
            topMargin: 2
            bottomMargin: 2
        }

        Base {
            id: selectorMain
            height: 54
            __showTopSeparator: false
            __showBottomSeparator: false
            onClicked: selector.expanded = !selector.expanded

            IconVisual {
                id: iconHelper
                anchors {
                    left: parent.left
                    top: parent.top
                }
            }
            LabelVisual {
                id: label
                anchors {
                    verticalCenter: parent.verticalCenter
                    leftMargin: 5
                    left: iconHelper.right
                }
                width: Math.min(invisibleLabel.implicitWidth, parent.width - 10)
            }
            LabelVisual {
                id: invisibleLabel
                visible: false
                text: label.text
                elide: Text.ElideNone
            }
            LabelVisual {
                id: valueLabel
                anchors {
                    verticalCenter: parent.verticalCenter
                    right: accordion.left
                    rightMargin: 5
                    leftMargin: 5
                    left: label.right
                }
                fontSize: "medium"
                text: selector.values[selector.selectedIndex]
                font.bold: selector.expanded
                horizontalAlignment: Text.AlignRight
            }
            Item {
                id: accordion
                width: 30
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }
                Image {
                    id: accordionIcon
                    anchors.centerIn: parent
                    // TODO: Replace this placeholder graphic with the official one.
                    source: "artwork/ListItemProgressionArrow.png"
                    opacity: enabled ? 1.0 : 0.5
                    rotation: expanded ? 270 : 90
                    width: implicitWidth / 1.5
                    height: implicitHeight / 1.5

                    states: [
                        State {
                            name: "expanded"
                            when: selector.expanded
                            PropertyChanges { target: accordionIcon; rotation: 270 }
                        }, State {
                            name: "closed"
                            when: !selector.expanded
                            PropertyChanges { target: accordionIcon; rotation: 90 }
                        }
                    ]

                    transitions: Transition {
                        PropertyAnimation {
                            target: accordionIcon
                            properties: "rotation"
                            duration: 100
                        }
                    }
                }
            }
        }

        Repeater {
            model: selector.values
            Selectable {
                id: valueBase
                height: selector.expanded ? 40 : 0
                visible: valueBase.height > 0
                onClicked: {
                    selector.selectedIndex = index
                    selector.expanded = false
                }
                selected: index === selector.selectedIndex

                __showBottomSeparator: index === selector.values.length - 1
                __showTopSeparator: true // TODO: show different (less wide) separator?

                Rectangle {
                    color: "#e0e0e0"
                    anchors.fill: parent
                    z: -1
                }

                LabelVisual {
                    text: modelData
                    anchors {
                        left: parent.left
                        leftMargin: 24
                        verticalCenter: parent.verticalCenter
                    }
                    font.italic: true
                    font.bold: valueBase.selected
                    property real heightMargin: valueBase.height - implicitHeight
                    visible: heightMargin > 0
                    // fade in/out the values when expanding/contracting the selector.
                    opacity: heightMargin < 10 ? heightMargin/10 : 1
                }

                states: [ State {
                        name: "expanded"
                        when: selector.expanded
                        PropertyChanges {
                            target: valueBase
                            height: 40
                        }
                    }, State {
                        name: "closed"
                        when: !selector.expanded
                        PropertyChanges {
                            target: valueBase
                            height: 0
                        }
                    }
                ]
                transitions: Transition {
                    PropertyAnimation {
                        target: valueBase
                        properties: "height"
                        duration: 100
                    }
                }
            }
        }
    }
}