/*
 * Copyright 2013 Canonical Ltd.
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

//![0]
import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Layouts 0.1

Item {
    id: root
    width: units.gu(30)
    height: units.gu(30)

    Layouts {
        objectName: "layouts"
        id: layouts
        anchors.fill: parent

        layouts: [
            //![row layout]
            ConditionalLayout {
                name: "row"
                when: layouts.width > units.gu(50)
                Item {
                    id: column
                    ItemLayout {
                        id: first
                        item: "blue"
                        anchors {
                            left: parent.left
                            top: parent.top
                            right: parent.right
                        }
                        height: parent.height / 2

                    }
                    ItemLayout {
                        id: second
                        item: "red"
                        anchors {
                            left: parent.left
                            top: first.bottom
                            bottom: parent.bottom
                        }
                        width: parent.width / 2
                    }
                    ItemLayout {
                        id: third
                        item: "green"
                        anchors {
                            right: parent.right
                            top: first.bottom
                            bottom: parent.bottom
                        }
                        width: parent.width / 2
                    }
                }
            }
            //![row layout]
        ]
        //![default layout]
        Button {
            id: button1
            text: "Item #1"
            color: "red"
            Layouts.item: "red"
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: units.gu(15)
        }
        Button {
            id: button2
            text: "Item #2"
            color: "green"
            Layouts.item: "green"
            anchors {
                top: parent.top
                left: button1.right
                right: parent.right
            }
            height: units.gu(10)
        }
        Button {
            id: button3
            text: "Item #3"
            color: "blue"
            Layouts.item: "blue"
            anchors{
                left: button1.right
                right: parent.right
                bottom: parent.bottom
            }
            height: units.gu(10)
        }
        //![default layout]
    }
}
//![0]
