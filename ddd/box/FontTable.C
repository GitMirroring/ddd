// $Id$
// Font tables

// Copyright (C) 1995 Technische Universitaet Braunschweig, Germany.
// Written by Andreas Zeller <zeller@gnu.org>.
// 
// This file is part of DDD.
// 
// DDD is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// 
// DDD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with DDD -- see the file COPYING.
// If not, see <http://www.gnu.org/licenses/>.
// 
// DDD is the data display debugger.
// For details, see the DDD World-Wide-Web page, 
// `http://www.gnu.org/software/ddd/',
// or send a mail to the DDD developers <ddd@gnu.org>.

char FontTable_rcsid[] = 
    "$Id$";

#include "base/strclass.h"

#include <iostream>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "FontTable.h"

DEFINE_TYPE_INFO_0(FontTable)

// FontTable

// Return XFontStruct for given font name NAME
BoxFont *FontTable::operator[](const string& name)
{
    auto it = table.find(name);
    if (it != table.end())
        return it->second;

    // Insert new font
    BoxFont* font = XftFontOpenName(m_display, DefaultScreen(m_display), (name + ":antialias=true").chars());
    if (!font)
    {
        std::cerr << "Warning: Could not load font \"" << name << "\"";

        // Try default font
        font = XftFontOpen(m_display, DefaultScreen(m_display), XFT_FAMILY, XftTypeString, "", NULL);
        std::cerr << ", using default font instead\n";
    }

    table[name] = font;
    return font;
}

