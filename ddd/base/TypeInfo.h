// $Id$ -*- C++ -*-
// Run-time type information

// Copyright (C) 1995-1997 Technische Universitaet Braunschweig, Germany.
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

#ifndef _DDD_TypeInfo_h
#define _DDD_TypeInfo_h

#include "config.h"

#include <typeinfo>

// Public interface.  Defined according to: Stroustrup, the C++
// Programming Language, 2nd Ed., Section 13.5

// Use ISO C++ run-time type information
#define static_type_info(T)  typeid(T)
#define ptr_type_info(p)     typeid(p)
#define ref_type_info(r)     typeid(&(r))
#define ptr_cast(T, p)       dynamic_cast<T *>(p)
#define ref_cast(T, r)       dynamic_cast<T &>(r)
#define const_ptr_cast(T, p) dynamic_cast<const T *>(p)
#define const_ref_cast(T, r) dynamic_cast<const T &>(r)


// Old-style definition macros are no more required
#define DECLARE_TYPE_INFO
#define DEFINE_TYPE_INFO_0(T)
#define DEFINE_TYPE_INFO_1(T, B1)
#define DEFINE_TYPE_INFO_2(T, B1, B2)
#define DEFINE_TYPE_INFO_3(T, B1, B2, B3)
#define DEFINE_TYPE_INFO_4(T, B1, B2, B3, B4)
#define DEFINE_TYPE_INFO_5(T, B1, B2, B3, B4, B5)
#define DEFINE_TYPE_INFO_6(T, B1, B2, B3, B4, B5, B6)
#define DEFINE_TYPE_INFO_7(T, B1, B2, B3, B4, B5, B6, B7)
#define DEFINE_TYPE_INFO_8(T, B1, B2, B3, B4, B5, B6, B7, B8)
#define DEFINE_TYPE_INFO_9(T, B1, B2, B3, B4, B5, B6, B7, B8, B9)

#endif // _DDD_TypeInfo_h
// DON'T ADD ANYTHING BEHIND THIS #endif
