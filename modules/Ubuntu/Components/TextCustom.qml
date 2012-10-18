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
import "fontUtils.js" as FontUtils
import "." 0.1 as Theming

/*!
    \qmltype TextCustom
    \inqmlmodule Ubuntu.Components 0.1
    \ingroup ubuntu
    \brief The TextCustom class is DOCME

    \b{This component is under heavy development.}
*/
Text {
    color: (Theming.ItemStyle.style && Theming.ItemStyle.style.color) ?
             Theming.ItemStyle.style.color : "black"
    style: (Theming.ItemStyle.style && Theming.ItemStyle.style.style) ?
             Theming.ItemStyle.style.style : Text.Normal
    styleColor: (Theming.ItemStyle.style && Theming.ItemStyle.style.styleColor) ?
                    Theming.ItemStyle.style.styleColor : color
//    font.family: "UbuntuBeta"

    /*!
       \preliminary
       DOCME
    */
    property string fontSize: (Theming.ItemStyle.style && Theming.ItemStyle.style.fontSize) ?
                                Theming.ItemStyle.style.fontSize : "medium"
    font.pixelSize: FontUtils.sizeToPixels(fontSize)
}
