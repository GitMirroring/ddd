// $Id$ -*- C++ -*-
// Kill unmanaged sashes

// Copyright (C) 1996 Technische Universitaet Braunschweig, Germany.
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

char sashes_rcsid[] = 
    "$Id$";

#include "sashes.h"

#include <Xm/Xm.h>
#include <Xm/PanedW.h>
#include <X11/StringDefs.h>
#include <Xm/SashP.h>   // for XmIsSash
#include <stdint.h>
#include <X11/cursorfont.h>
#include "x11/Sash.h"


//-----------------------------------------------------------------------------
// Sashes
//-----------------------------------------------------------------------------

// Unmanage all sashes of PANED
void unmanage_sashes(Widget paned)
{
    if (paned == 0 || !XtIsSubclass(paned, xmPanedWindowWidgetClass))
        return;

    WidgetList children   = 0;
    Cardinal num_children = 0;

    XtVaGetValues(paned,
                  XtNchildren, &children,
                  XtNnumChildren, &num_children,
                  XtPointer(0));

    for (Cardinal i = 0; i < num_children; i++)
        if (XmIsSash(children[i]))
        {
            XtUnmanageChild(children[i]);
            XtUnmapWidget(children[i]);
        }
}

// Disable traversal for all sashes of PANED
void untraverse_sashes(Widget paned)
{
    if (paned == 0 || !XtIsSubclass(paned, xmPanedWindowWidgetClass))
        return;

    WidgetList children   = 0;
    Cardinal num_children = 0;

    XtVaGetValues(paned,
                  XtNchildren, &children,
                  XtNnumChildren, &num_children,
                  XtPointer(0));

    for (Cardinal i = 0; i < num_children; i++)
        if (XmIsSash(children[i]))
            XtVaSetValues(children[i], XmNtraversalOn, False, XtPointer(0));
}

static void draw_sash_line_internal(Widget w, unsigned char orientation)
{
    Display *dpy = XtDisplay(w);
    Window   win = XtWindow(w);
    if (!win)
        return;

    Dimension width, height;
    Pixel     fg, bg;
    XGCValues gcv;
    GC        gc;

    XtVaGetValues(w,
                  XmNwidth,      &width,
                  XmNheight,     &height,
                  XmNforeground, &fg,
                  XmNbackground, &bg,
                  NULL);

    gcv.foreground = bg;
    GC gc_bg = XCreateGC(dpy, win, GCForeground, &gcv);

    /* Clear the entire sash area to background */
    XFillRectangle(dpy, win, gc_bg, 0, 0, width, height);
    XFreeGC(dpy, gc_bg);

    /* Draw the 1‑pixel line */
    gcv.foreground = fg;
    gc = XCreateGC(dpy, win, GCForeground, &gcv);

    if (orientation == XmVERTICAL)
    {
        // Panes stacked vertically -> horizontal sash -> horizontal line
        int y = (int)height / 2;
        XFillRectangle(dpy, win, gc, 0, y, width, 1);
    }
    else
    {
        // XmHORIZONTAL: panes side‑by‑side -> vertical sash -> vertical line
        int x = (int)width / 2;
        XFillRectangle(dpy, win, gc, x, 0, 1, height);
    }

    XFreeGC(dpy, gc);
}

void draw_sash_line(Widget w, XtPointer client, XEvent *event, Boolean*)
{
    if (event->type != Expose)
        return;

    unsigned char orientation = (unsigned char)(uintptr_t)client;
    draw_sash_line_internal(w, orientation);
}

static void repaint_all_sashes_cb(XtPointer client, XtIntervalId*)
{
    Widget paned = (Widget)client;
    if (!XtIsRealized(paned))
        return;

    WidgetList    children;
    Cardinal      nchildren;
    unsigned char orientation;

    XtVaGetValues(paned,
                  XmNorientation, &orientation,
                  XmNchildren,    &children,
                  XmNnumChildren, &nchildren,
                  NULL);

    for (Cardinal i = 0; i < nchildren; ++i)
    {
        Widget w = children[i];
        if (XmIsSash(w) && XtIsRealized(w))
            draw_sash_line_internal(w, orientation);
    }
}

static void sash_extra_handler(Widget w, XtPointer,
                               XEvent *event, Boolean*)
{
    if (event->type != ButtonRelease)
        return;

    Widget      paned = XtParent(w);
    XtAppContext app  = XtWidgetToApplicationContext(w);

    // Run repaint_all_sashes_cb after Motif finishes its own ButtonRelease actions
    XtAppAddTimeOut(app, 0, repaint_all_sashes_cb, (XtPointer)paned);
}

static void sash_cursor_handler(Widget w, XtPointer client,
                                XEvent *event, Boolean*)
{
    Cursor   cursor = (Cursor)(uintptr_t)client;
    Display *dpy    = XtDisplay(w);
    Window   win    = XtWindow(w);

    if (!win)
        return;

    if (event->type == EnterNotify)
        XDefineCursor(dpy, win, cursor);
    else if (event->type == LeaveNotify)
        XUndefineCursor(dpy, win); //  Restore default (parent’s) cursor

}

void install_sash_handlers(Widget paned)
{
    WidgetList    children;
    Cardinal      nchildren;
    unsigned char orientation;

    XtVaGetValues(paned,
                  XmNorientation, &orientation,
                  XmNchildren,    &children,
                  XmNnumChildren, &nchildren,
                  NULL);



    if (orientation == XmHORIZONTAL)
        XtVaSetValues(paned,
                    XmNsashHeight,  10000,   // large enough
                    XmNsashWidth,   7, // thickness of the splitter
                    NULL);
    else
        XtVaSetValues(paned,
                    XmNsashHeight,  7,   // thickness of the splitter
                    XmNsashWidth,   10000,  // large enough
                    NULL);


    Display *dpy = XtDisplay(paned);
    Cursor   cursor;

    if (orientation == XmHORIZONTAL)
        cursor = XCreateFontCursor(dpy, XC_sb_h_double_arrow);
    else
        cursor = XCreateFontCursor(dpy, XC_sb_v_double_arrow);

    Pixel   sash_fg_pixel  = 0;
    XtVaGetValues(paned, XmNbottomShadowColor, &sash_fg_pixel, NULL);

    for (Cardinal i = 0; i < nchildren; ++i)
    {
        Widget w = children[i];
        if (!XmIsSash(w))
            continue;

        Dimension width, height;
        XtVaGetValues(w,
                      XmNwidth,  &width,
                      XmNheight, &height,
                      NULL);

        // Skip sashes that are effectively invisible
        if (width  == 0 || height == 0)
            continue;


        XtVaSetValues(w,
                      XmNforeground, sash_fg_pixel,
                      XmNshadowThickness,    0,
                      XmNhighlightThickness, 0,
                      XmNborderWidth,        0,
                      XmNcursor,             cursor,
                      NULL);

        // Draw on expose
        XtAddEventHandler(w,
                          ExposureMask,
                          False,
                          draw_sash_line,
                          (XtPointer)(uintptr_t)orientation);

        // Fix-up after drag
        XtAddEventHandler(w,
                          ButtonReleaseMask,
                          False,
                          sash_extra_handler,
                          NULL);

        // Cursor on hover
        XtAddEventHandler(w,
                          EnterWindowMask | LeaveWindowMask,
                          False,
                          sash_cursor_handler,
                          (XtPointer)(uintptr_t)cursor);
    }
}

#include <stdio.h>

// void hide_bottom_sash(Widget paned)
// {
//     if (paned == 0 || !XtIsSubclass(paned, xmPanedWindowWidgetClass))
//         return;
//
//     WidgetList children   = 0;
//     Cardinal   num_children = 0;
//     unsigned char orientation;
//
//     XtVaGetValues(paned,
//                   XmNchildren,    &children,
//                   XmNnumChildren, &num_children,
//                   XmNorientation, &orientation,
//                   NULL);
//
//     Widget bottom_sash = 0;
//     Position best_coord = -32768;  // “smallest possible”
//
//     for (Cardinal i = 0; i < num_children; ++i)
//     {
//         Widget w = children[i];
//         WidgetClass wc = XtClass(w);
//
//         printf("%2d: %p  class: %-20s  managed=%d  realized=%d\n", i, w, wc->core_class.class_name, XtIsManaged(w), XtIsRealized(w));
//         if (!XmIsSash(w))
//             continue;
//
//         Position x, y;
//         XtVaGetValues(w, XmNx, &x, XmNy, &y, NULL);
//         printf("x %d   y %d\n", x, y);
//
//         Position coord = (orientation == XmVERTICAL) ? y : x;
//         if (coord >= best_coord)
//         {
//             best_coord = coord;
//             bottom_sash = w;
//         }
//     }
//
//     printf("bottom sasf %p\n", bottom_sash);
//     if (bottom_sash != 0)
//     {
//         XtUnmanageChild(bottom_sash);
//         XtUnmapWidget(bottom_sash);   // mostly redundant, but harmless
//     }
// }

static bool find_paned_and_direct_child(Widget w,
                                        Widget& paned_out,
                                        Widget& pane_out)
{
    if (!w)
        return false;

    Widget pane   = w;
    Widget parent = XtParent(pane);

    // Walk up until we find an XmPanedWindow; remember the child
    while (parent && !XtIsSubclass(parent, xmPanedWindowWidgetClass))
    {
        pane   = parent;
        parent = XtParent(parent);
    }

    if (!parent)
        return false;  // no paned window in ancestor chain

    paned_out = parent;
    pane_out  = pane;
    return true;
}

void hide_sash_for_child(Widget any_child_in_pane)
{
    Widget paned = 0;
    Widget pane  = 0;

    if (!find_paned_and_direct_child(any_child_in_pane, paned, pane))
        return;

    WidgetList    children   = 0;
    Cardinal      nchildren  = 0;
    unsigned char orientation;

    XtVaGetValues(paned,
                  XmNorientation, &orientation,
                  XmNchildren,    &children,
                  XmNnumChildren, &nchildren,
                  NULL);

    // Geometry of the pane (direct child of paned)
    Position  px, py;
    Dimension pw, ph;
    XtVaGetValues(pane,
                  XmNx,      &px,
                  XmNy,      &py,
                  XmNwidth,  &pw,
                  XmNheight, &ph,
                  NULL);

    Position pane_coord = (orientation == XmVERTICAL) ? py : px;

    Widget  target     = 0;
    Position best_delta = 32767; // minimal positive distance

    for (Cardinal i = 0; i < nchildren; ++i)
    {
        Widget w = children[i];
        if (!XmIsSash(w))
            continue;

        Position x, y;
        XtVaGetValues(w, XmNx, &x, XmNy, &y, NULL);

        Position coord = (orientation == XmVERTICAL) ? y : x;

        // We want the sash immediately above the pane: coord < pane_coord,
        // and pane_coord - coord as small as possible.
        if (coord < pane_coord)
        {
            Position delta = pane_coord - coord;
            if (delta < best_delta)
            {
                best_delta = delta;
                target     = w;
            }
        }
    }

    if (target)
    {
        XtUnmanageChild(target);
        XtUnmapWidget(target);
    }
}
