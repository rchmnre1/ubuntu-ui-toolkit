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

import QtQuick 2.0
import Ubuntu.Components 0.1
import Ubuntu.Components.Pickers 0.1

Row {
    ListModel {
        id: pickerModel
        Component.onCompleted: {
            for (var i = 0; i < 100; i++) {
                append({"label": "Item #"+i});
            }
        }
    }

    Picker {
        height: units.gu(40)
        model: pickerModel
        delegate: PickerDelegate {
            Label {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData
            }
        }
    }

    Picker {
        height: units.gu(40)
        model: pickerModel
        circular: false
        delegate: PickerDelegate {
            Label {
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: modelData
            }
        }
    }
}