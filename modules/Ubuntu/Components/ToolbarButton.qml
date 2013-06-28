/*
 * Copyright (C) 2013 Canonical Ltd.
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

import QtQuick 2.0
// FIXME: When a module contains QML, C++ and JavaScript elements exported,
// we need to use named imports otherwise namespace collision is reported
// by the QML engine. As workaround, we use Theming named import.
// Bug to watch: https://bugreports.qt-project.org/browse/QTBUG-27645
import Ubuntu.Components 0.1 as Theming

/*!
    \qmltype ToolbarButton
    \inqmlmodule Ubuntu.Components 0.1
    \ingroup ubuntu
    \brief An \l ActionItem that represents a button in the toolbar.
        ToolbarButtons should be included in \l ToolbarItems to define the tools of a \l Page.
        The behavior and look of the toolbar button can be specified by setting an \l Action for
        the button, or by setting the other properties inherited by the \l ActionItem.

        Example of different ways to define the toolbar button:
        \qml
        import QtQuick 2.0
        import Ubuntu.Components 0.1

        MainView {
            width: units.gu(50)
            height: units.gu(80)

            Action {
                id: action1
                text: "action 1"
                iconSource: Qt.resolvedUrl("icon1.png")
                onTriggered: print("one!")
            }

            Page {
                title: "test page"

                Label {
                    anchors.centerIn: parent
                    text: "Hello, world"
                }

                tools: ToolbarItems {
                    // reference to an action:
                    ToolbarButton {
                        action: action1
                    }

                    // define the action:
                    ToolbarButton {
                        action: Action {
                            text: "Second action"
                            iconSource: Qt.resolvedUrl("icon2.png")
                            onTriggered: print("two!")
                        }
                        // override the text of the action:
                        text: "action 2"
                    }

                    // no associated action:
                    ToolbarButton {
                        iconSource: Qt.resolvedUrl("icon3.png")
                        text: "button"
                        onTriggered: print("three!")
                    }
                }
            }
        }
        \endqml
        See \l ToolbarItems for more information on how to use ToolbarButton.
*/
ActionItem {
    id: toolbarButton
    Theming.ItemStyle.class: "toolbar-button"
    height: parent ? parent.height : undefined
    width: units.gu(5)
}