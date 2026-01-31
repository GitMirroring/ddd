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

#ifndef _DDD_FontTable_h
#define _DDD_FontTable_h
#include "config.h"


#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <map>

#include "base/strclass.h"
#include "base/TypeInfo.h"


typedef XftFont BoxFont;

class FontTable {
public:
    DECLARE_TYPE_INFO

private:
    Display* m_display;
    std::map<string, BoxFont*> table;  // todo: switch to std::unordered_map

    FontTable(const FontTable&);
    FontTable& operator=(const FontTable&);

public:
    FontTable(Display* display) : m_display(display) {}

    ~FontTable()
    {
        for (auto& kv : table)
            if (kv.second)
                XftFontClose(m_display, kv.second);
    }

    BoxFont* operator[](const string& name);

    Display* getDisplay() { return m_display; }
};

#endif
