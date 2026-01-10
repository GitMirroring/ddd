// $Id$ -*- C++ -*-
// Kill unmanaged sashes

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

char scrollbar_rcsid[] =
    "$Id$";

#include "scrollbar.h"

#include <Xm/Xm.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <X11/StringDefs.h>
#include <stdint.h>


typedef struct {
    Pixel   base_bg;        // hidden scroll area: background
    Pixel   hover_bg;       // visible scroll area: trough color
    Boolean hovered;        // current visual state
    Boolean pressed_inside; // mouse button pressed inside scrollbar
    Boolean initialized;    // initial visual state applied?
} ScrollbarHoverState;

static void scrollbar_hover_destroy_cb(Widget, XtPointer client, XtPointer)
{
    ScrollbarHoverState *st = (ScrollbarHoverState *)client;
    if (st)
        XtFree((char *)st);
}

static void scrollbar_apply_bg(Widget w, ScrollbarHoverState *st, Boolean hover)
{
    if (!XtIsRealized(w))
        return;

    if (st->hovered == hover)
        return;  //

    st->hovered = hover;
    Pixel new_bg = hover ? st->hover_bg : st->base_bg;

    Display *dpy = XtDisplay(w);
    Window   win = XtWindow(w);
    if (!win)
        return;

    XtVaSetValues(w, XmNbackground, new_bg, NULL);
    XSetWindowBackground(dpy, win, new_bg);

    XClearArea(dpy, win, 0, 0, 0, 0, True);  // force redraw
}

// Force background regardless of current st->hovered value
static void scrollbar_force_bg(Widget w, ScrollbarHoverState *st, Boolean hover)
{
    st->hovered = !hover;       // ensure scrollbar_apply_bg does work
    scrollbar_apply_bg(w, st, hover);
}

static void scrollbar_hover_handler(Widget w, XtPointer client,
                        XEvent *event, Boolean*)
{
    ScrollbarHoverState *st = (ScrollbarHoverState *)client;

    if (!XtIsRealized(w))
        return;

    XButtonEvent *be = &event->xbutton;
    switch (event->type)
    {

        case Expose:
            // First expose after realize: enforce hidden state (base_bg)
            if (!st->initialized)
            {
                st->initialized = True;
                scrollbar_force_bg(w, st, False);  // hidden at startup
            }
            break;

        case EnterNotify:
            /* pointer over scrollbar -> visible scroll area */
            scrollbar_apply_bg(w, st, True);
            break;

        case LeaveNotify:
            /* hide ONLY if not dragging from a press that started inside */
            if (!st->pressed_inside)
                scrollbar_apply_bg(w, st, False);
        break;

        case ButtonPress:
            if (be->button == Button1)
            {
                st->pressed_inside = True;
                /* ensure visible when press starts inside */
                scrollbar_apply_bg(w, st, True);
            }
            break;

        case ButtonRelease:
            if (be->button == Button1)
            {
                st->pressed_inside = False;

                /* After release:
                 *              - if pointer is outside -> hide
                 *              - if pointer is inside -> stay visible (hover) */

                Dimension width, height;
                XtVaGetValues(w,
                              XmNwidth,  &width,
                              XmNheight, &height,
                              NULL);

                Boolean inside =
                (be->x >= 0 && be->x < (int)width &&
                be->y >= 0 && be->y < (int)height);

                if (!inside)
                    scrollbar_apply_bg(w, st, False);
            }
            break;

        default:
            break;
    }
}

// Public function: install on any XmScrollBar
void install_scrollbar_hover_style(Widget scrollbar)
{
    if (scrollbar==nullptr || !XmIsScrollBar(scrollbar))
        return;

    Pixel bg, trough/*, bottom, dummy*/;
    unsigned char orientation;

    XtVaGetValues(scrollbar,
                  XmNtroughColor, &trough,
                  // XmNforeground,  &dummy,
                  XmNorientation,       &orientation,
                  NULL);

    // workaround a bug in Motif
    // Switch to TROUGH mode with a dummy trough color -> rebuild GC once
    XtVaSetValues(scrollbar,
                  XmNsliderVisual, XmTROUGH_COLOR,
                  // XmNtroughColor,  dummy,
                  NULL);

    // Now set the real trough color -> rebuild GC again with correct Pixel
    XtVaSetValues(scrollbar,
                  XmNtroughColor,       trough,
                  XmNshowArrows,        XmNONE,
                  XmNshadowThickness,   0,
                  XmNhighlightThickness, 0,
                  NULL);    // Pixel trough_before, bg;

    // Use the scrollbar's own colors
    XtVaGetValues(scrollbar,
                  XmNbackground,        &bg,      // hidden scroll area
                  XmNtroughColor,       &trough,  // visible scroll area
                  // XmNbottomShadowColor, &bottom,  // slider color
                  NULL);

    // Allocate #808080 in the scrollbar's colormap
    Display  *dpy  = XtDisplay(scrollbar);
    Colormap  cmap;
    XtVaGetValues(scrollbar, XmNcolormap, &cmap, NULL);

    XColor screen_def, exact_def;
    Pixel slider_pixel = BlackPixelOfScreen(XtScreen(scrollbar)); // fallback

    if (XAllocNamedColor(dpy, cmap, "#808080",
        &screen_def, &exact_def)) {
        slider_pixel = screen_def.pixel;
        }

    if (orientation == XmHORIZONTAL)
        XtVaSetValues(scrollbar, XmNheight, 16, NULL);
    else
        XtVaSetValues(scrollbar, XmNwidth,  16, NULL);

    ScrollbarHoverState *st = (ScrollbarHoverState *)XtMalloc(sizeof(*st));
    st->base_bg        = bg;
    st->hover_bg       = trough;
    st->hovered        = False;
    st->pressed_inside = False;
    st->initialized    = XtIsRealized(scrollbar) ? True : False;

    /* Base look:
     *      - flat slider
     *      - no arrows
     *      - slider = bottomShadowColor (foreground)
     *      - scroll area initially hidden (background);
     *
     *        Realize may have set the window background to trough,
     *        but we fix that in Expose / or immediately if realized. */
    XtVaSetValues(scrollbar,
                  XmNshowArrows,        XmNONE,
                  XmNshadowThickness,   0,
                  XmNhighlightThickness,1,
                  XmNsliderVisual,      XmFOREGROUND_COLOR,
                  // XmNforeground,        bottom,      // slider color
                  XmNforeground,        slider_pixel, // use #808080
                  XmNbackground,        st->base_bg, // hidden scroll
                  NULL);

    if (XtIsRealized(scrollbar))
    {
        // If already realized, enforce hidden state immediately
        Display *dpy = XtDisplay(scrollbar);
        Window   win = XtWindow(scrollbar);
        if (win)
        {
            XSetWindowBackground(dpy, win, st->base_bg);
            XClearArea(dpy, win, 0, 0, 0, 0, True);
        }
    }

    // Hover + press/drag behavior + startup init (Expose)
    XtAddEventHandler(scrollbar,
                      ExposureMask |
                      EnterWindowMask | LeaveWindowMask |
                      ButtonPressMask | ButtonReleaseMask,
                      False,
                      scrollbar_hover_handler,
                      (XtPointer)st);

    XtAddCallback(scrollbar, XmNdestroyCallback, scrollbar_hover_destroy_cb, (XtPointer)st);
}

void modernize_scrollbar(Widget w)
{
    if (!XmIsScrolledWindow(w))
        w = XtParent(w);

    Widget hbar = nullptr;
    Widget vbar = nullptr;
    XtVaGetValues(w,
                  XmNhorizontalScrollBar, &hbar,
                  XmNverticalScrollBar,   &vbar,
                  nullptr);
    install_scrollbar_hover_style(vbar);
    install_scrollbar_hover_style(hbar);
}

