// $Id$ -*- C++ -*-
// DDD logos and logo functions

// Copyright (C) 1996-1998 Technische Universitaet Braunschweig, Germany.
// Copyright (C) 2000 Universitaet Passau, Germany.
// Copyright (C) 2003 Free Software Foundation, Inc.
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

char logo_rcsid[] = 
    "$Id$";

//#include <vector>
#include <stdint.h>

#include "logo.h"
#include "config.h"

#include <X11/xpm.h>

#include "assert.h"
#include "string-fun.h"
#include "AppData.h"
#include "base/cook.h"
#include "x11/InitImage.h"

// X pixmaps
#define char const char
#include "icons/ddd.xpm"
#include "icons/dddsplash.xpm"
#undef char

#include <iostream>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>

// ANSI C++ doesn't like the XtIsRealized() macro
#ifdef XtIsRealized
#undef XtIsRealized
#endif

//-----------------------------------------------------------------------------
// DDD logo
//-----------------------------------------------------------------------------

static int xpm(const _XtString name, int ret)
{
    if (ret != XpmSuccess)
    {
	std::cerr << "XPM: " << name << ": ";
	switch (ret)
	{
	case XpmColorError:
	    std::cerr << "warning: failed to allocate some color\n";
	    ret = XpmSuccess;	// ignore
	    break;

	case XpmOpenFailed:
	    std::cerr << "could not open file\n";
	    break;

	case XpmFileInvalid:
	    std::cerr << "could not parse file\n";
	    break;
	    
	case XpmNoMemory:
	    std::cerr << "insufficient working storage\n";
	    break;

	case XpmColorFailed:
	    std::cerr << "no color found\n";
	    break;

	default:
	    std::cerr << "error " << ret << "\n";
	    break;
	}
    }

    return ret;
}

// Add a color key specification
static void add_color_key(XpmAttributes& attr, const string& color_key)
{
    attr.valuemask |= XpmColorKey;
    if (color_key == "c")
	attr.color_key = XPM_COLOR;
    else if (color_key == "g4")
	attr.color_key = XPM_GRAY4;
    else if (color_key == "g")
	attr.color_key = XPM_GRAY;
    else if (color_key == "m")
	attr.color_key = XPM_MONO;
    else
    {
	if (color_key != "best")
	    std::cerr << "XPM: invalid color key " << quote(color_key) << "\n";

	attr.valuemask &= ~XpmColorKey;
    }
}

// Add a `close colors' specification.  The default value 40000 is
// taken from the XPM documentation -- `seems to be right about many
// situations'.
static void add_closeness(XpmAttributes& attr, int closeness = 40000)
{
    attr.valuemask |= XpmCloseness;
    attr.closeness = closeness;
}


static void create_logo_pixmaps(Widget w, Pixmap &icon, Pixmap &mask)
{
    Display *dpy = XtDisplay(w);
    Window root = RootWindowOfScreen(XtScreen(w));

    XWindowAttributes root_attr;
    XGetWindowAttributes(dpy, root, &root_attr);

    XpmAttributes attr{};
    attr.valuemask = XpmVisual | XpmColormap | XpmDepth;
    attr.visual    = root_attr.visual;
    attr.colormap  = root_attr.colormap;
    attr.depth     = root_attr.depth;
    add_closeness(attr);

    int ret = xpm("ddd3_5.xpm",
                  XpmCreatePixmapFromData(dpy, root,
                                          (char**)ddd_xpm,
                                          &icon, &mask, &attr));
    XpmFreeAttributes(&attr);

    if (ret != XpmSuccess) {
        if (icon) XFreePixmap(dpy, icon);
        if (mask) XFreePixmap(dpy, mask);
        icon = mask = 0;
    }
}

static Pixmap iconlogo_pixmap = 0;
static Pixmap iconlogo_mask   = 0;

Pixmap iconlogo(Widget w)
{
    if (!iconlogo_pixmap || !iconlogo_mask)
        create_logo_pixmaps(w, iconlogo_pixmap, iconlogo_mask);

    return iconlogo_pixmap;
}

Pixmap iconmask(Widget w)
{
    if (!iconlogo_pixmap || !iconlogo_mask)
        create_logo_pixmaps(w, iconlogo_pixmap, iconlogo_mask);

    return iconlogo_mask;
}

// Return the DDD splash screen
Pixmap dddsplash(Widget w, const string& color_key,
		 Dimension& width, Dimension& height)
{
    Pixmap logo = 0;

    Window window = None;
    if (XtIsRealized(w))
	window = XtWindow(w);
    else
	window = RootWindowOfScreen(XtScreen(w));

    assert(window != None);

    XWindowAttributes win_attr;
    XGetWindowAttributes(XtDisplay(w), window, &win_attr);

    XpmAttributes attr = {};
    attr.valuemask    = XpmVisual | XpmColormap | XpmDepth;
    attr.visual       = win_attr.visual;
    attr.colormap     = win_attr.colormap;
    attr.depth        = win_attr.depth;
    add_color_key(attr, color_key);
    add_closeness(attr);

    int ret = xpm("splash3_5.xpm",
                    XpmCreatePixmapFromData(XtDisplay(w), window,
                                            (char **)dddsplash_xpm, &logo,
                                            (Pixmap *)0, &attr));
    XpmFreeAttributes(&attr);

    if (ret == XpmSuccess)
    {
        // Xpm has filled these
        width  = attr.width;
        height = attr.height;
        XpmFreeAttributes(&attr);
        return logo;
    }

    XpmFreeAttributes(&attr);

    if (logo != 0)
        XFreePixmap(XtDisplay(w), logo);
    logo = 0;

    width  = 0;
    height = 0;
    return 0;
}

//-----------------------------------------------------------------------
// Toolbar icons
//-----------------------------------------------------------------------


// To avoid compilation warnings, make all char *'s constant
// We cannot do this in the XPM file since this violates XPM format specs
typedef char oRiGiNaL_char;
#define char const oRiGiNaL_char

// X Pixmaps
#include "icons/toolbar/breakat.xpm"
#include "icons/toolbar/clearat.xpm"
#include "icons/toolbar/cluster.xpm"
#include "icons/toolbar/delete.xpm"
#include "icons/toolbar/deref.xpm"
#include "icons/toolbar/disable.xpm"
#include "icons/toolbar/display.xpm"
#include "icons/toolbar/enable.xpm"
#include "icons/toolbar/findbwd.xpm"
#include "icons/toolbar/findfwd.xpm"
#include "icons/toolbar/hide.xpm"
#include "icons/toolbar/lookup.xpm"
#include "icons/toolbar/maketemp.xpm"
#include "icons/toolbar/newbreak.xpm"
#include "icons/toolbar/newdisplay.xpm"
#include "icons/toolbar/newwatch.xpm"
#include "icons/toolbar/plot.xpm"
#include "icons/toolbar/print.xpm"
#include "icons/toolbar/properties.xpm"
#include "icons/toolbar/rotate.xpm"
#include "icons/toolbar/set.xpm"
#include "icons/toolbar/show.xpm"
#include "icons/toolbar/uncluster.xpm"
#include "icons/toolbar/undisplay.xpm"
#include "icons/toolbar/unwatch.xpm"
#include "icons/toolbar/watch.xpm"

// Same, but insensitive
#include "icons/toolbar/breakat.xpmxx"
#include "icons/toolbar/clearat.xpmxx"
#include "icons/toolbar/cluster.xpmxx"
#include "icons/toolbar/delete.xpmxx"
#include "icons/toolbar/deref.xpmxx"
#include "icons/toolbar/disable.xpmxx"
#include "icons/toolbar/display.xpmxx"
#include "icons/toolbar/enable.xpmxx"
#include "icons/toolbar/findbwd.xpmxx"
#include "icons/toolbar/findfwd.xpmxx"
#include "icons/toolbar/hide.xpmxx"
#include "icons/toolbar/lookup.xpmxx"
#include "icons/toolbar/maketemp.xpmxx"
#include "icons/toolbar/newbreak.xpmxx"
#include "icons/toolbar/newdisplay.xpmxx"
#include "icons/toolbar/newwatch.xpmxx"
#include "icons/toolbar/plot.xpmxx"
#include "icons/toolbar/print.xpmxx"
#include "icons/toolbar/properties.xpmxx"
#include "icons/toolbar/rotate.xpmxx"
#include "icons/toolbar/set.xpmxx"
#include "icons/toolbar/show.xpmxx"
#include "icons/toolbar/uncluster.xpmxx"
#include "icons/toolbar/undisplay.xpmxx"
#include "icons/toolbar/unwatch.xpmxx"
#include "icons/toolbar/watch.xpmxx"

#undef char

void invert_colors(XImage *image, Pixel background)
{
    if (!image)
        return;

    for (int y = 0; y < image->height; y++)
    {
        for (int x = 0; x < image->width; x++)
        {
            // Get the pixel color
            unsigned long pixel = XGetPixel(image, x, y);
            if (pixel == background)
                continue;

            // Extract RGB components (assuming 24-bit depth)
            unsigned char red = (pixel >> 16) & 0xFF;
            unsigned char green = (pixel >> 8) & 0xFF;
            unsigned char blue = pixel & 0xFF;

            // Invert the colors
            red = 255 - red;
            green = 255 - green;
            blue = 255 - blue;

            // Combine back into a single pixel value
            unsigned long inverted_pixel = (red << 16) | (green << 8) | blue;

            // Set the new pixel color
            XPutPixel(image, x, y, inverted_pixel);
        }
    }
}

XImage *scale_image(Widget w, Visual *visual, XImage *inputimage, int scalefactor)
{
    XImage *outputimage = XCreateImage(XtDisplay(w), visual, inputimage->depth, inputimage->format, 0, 0,
                                       scalefactor*inputimage->width, scalefactor*inputimage->height, inputimage->bitmap_pad, 0);
    outputimage->data =	(char *) malloc(outputimage->bytes_per_line * outputimage->height);
    for (int y = 0; y < outputimage->height; y++)
    {
        for (int x = 0; x < outputimage->width; x++)
        {
            unsigned long pixel = XGetPixel(inputimage, x/scalefactor, y/scalefactor);
            XPutPixel(outputimage, x, y, pixel);
        }
    }

    return outputimage;
}

static void install_icon(Widget w, const _XtString name,
			 const char **xpm_data, 
			 const string& color_key,
			 Pixel background,
			 const XWindowAttributes& win_attr,
			 bool is_button = false)
{
    XpmColorSymbol cs;
    cs.name  = CONST_CAST(char *,"Background");
    cs.value = 0;
    cs.pixel = background;

    XpmAttributes attr;
    attr.valuemask    =
        XpmVisual | XpmColormap | XpmDepth | XpmColorSymbols;
    attr.visual       = win_attr.visual;
    attr.colormap     = win_attr.colormap;
    attr.depth        = win_attr.depth;
    attr.colorsymbols = &cs;
    attr.numsymbols   = 1;
    add_color_key(attr, color_key);
    add_closeness(attr);

    XImage *image = nullptr;
    XImage *shape = nullptr;

    int ret =
        xpm(name, XpmCreateImageFromData(XtDisplay(w), (char **)xpm_data,
                                            &image, &shape, &attr));

    XpmFreeAttributes(&attr);
    if (shape != nullptr)
        XDestroyImage(shape);

    if (ret == XpmSuccess && image != nullptr)
    {
        if (is_button && app_data.dark_mode)
                invert_colors(image, background);

        if (app_data.scale_toolbar)
        {
            XImage *scaledimage = scale_image(w, win_attr.visual, image, 2);
            XDestroyImage(image);
            image = scaledimage;
        }

        Boolean ok = XmInstallImage(image, XMST(name));
        if (ok)
            return;
    }

    std::cerr << "Could not install " << quote(name) << " pixmap\n";
    if (image != nullptr)
        XDestroyImage(image);
}

static void install_button_icon(Widget w, const _XtString name,
				const char **xpm_data, 
				const char **xpm_xx_data,
				const string& color_key,
				const string& active_color_key,
				Pixel background,
				Pixel arm_background,
				const XWindowAttributes& win_attr)
{
    install_icon(w, name,
		 xpm_data,
		 color_key, background, win_attr, true);

    string insensitive_name = string(name) + "-xx";
    install_icon(w, insensitive_name.chars(),
		 xpm_xx_data,
		 color_key, background, win_attr, true);

    string armed_name = string(name) + "-arm";
    install_icon(w, armed_name.chars(),
		 xpm_data,
		 active_color_key, arm_background, win_attr, true);

    string highlight_name = string(name) + "-hi";
    install_icon(w, highlight_name.chars(),
		 xpm_data,
		 active_color_key, background, win_attr, true);
}

// Install toolbar icons in Motif cache.  COLOR_KEY indicates the XPM
// visual type for inactive buttons.  ACTIVE_COLOR_KEY is the XPM visual
// type for active buttons (entered or armed).
void install_icons(Widget shell, 
		   const string& color_key,
		   const string& active_color_key)
{
    static bool installed = false;
    if (installed)
	return;
    installed = true;

    // Determine attributes
    XWindowAttributes win_attr;
    XGetWindowAttributes(XtDisplay(shell), 
			 RootWindowOfScreen(XtScreen(shell)),
			 &win_attr);

    Pixel background;
    XtVaGetValues(shell, XmNbackground, &background, XtPointer(0));

    if (app_data.dark_mode)
        background ^= 0x00ffffff; // invert color in dark mode

    // Determine default arm background
    Pixel foreground, top_shadow, bottom_shadow, select;
    XmGetColors(XtScreen(shell), win_attr.colormap, background,
		&foreground, &top_shadow, &bottom_shadow, &select);

    // LessTif 0.87 and earlier does not return a suitable select color
    Pixel arm_background = select;

    // DDD icon (always in color)
    install_icon(shell, DDD_ICON,
		 ddd_xpm,
		 "best", background, win_attr, false);
    // Toolbar icons
    install_button_icon(shell, BREAK_AT_ICON, 
      		        breakat_xpm, breakat_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, CLEAR_AT_ICON, 
      		        clearat_xpm, clearat_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, CLUSTER_ICON, 
      		        cluster_xpm, cluster_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, DELETE_ICON, 
      		        delete_xpm, delete_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, DISPREF_ICON, 
      		        deref_xpm, deref_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, DISABLE_ICON, 
      		        disable_xpm, disable_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, DISPLAY_ICON, 
      		        display_xpm, display_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, ENABLE_ICON, 
      		        enable_xpm, enable_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, FIND_BACKWARD_ICON, 
      		        findbwd_xpm, findbwd_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, FIND_FORWARD_ICON, 
      		        findfwd_xpm, findfwd_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, HIDE_ICON, 
      		        hide_xpm, hide_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, LOOKUP_ICON, 
      		        lookup_xpm, lookup_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, MAKETEMP_ICON, 
      		        maketemp_xpm, maketemp_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, NEW_BREAK_ICON, 
      		        newbreak_xpm, newbreak_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, NEW_DISPLAY_ICON, 
      		        newdisplay_xpm, newdisplay_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, NEW_WATCH_ICON, 
      		        newwatch_xpm, newwatch_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, PLOT_ICON, 
      		        plot_xpm, plot_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, PRINT_ICON, 
      		        print_xpm, print_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, PROPERTIES_ICON, 
      		        properties_xpm, properties_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, ROTATE_ICON, 
      		        rotate_xpm, rotate_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, SET_ICON, 
      		        set_xpm, set_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, SHOW_ICON, 
      		        show_xpm, show_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, UNCLUSTER_ICON, 
      		        uncluster_xpm, uncluster_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, UNDISPLAY_ICON, 
      		        undisplay_xpm, undisplay_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, UNWATCH_ICON, 
      		        unwatch_xpm, unwatch_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);

    install_button_icon(shell, WATCH_ICON, 
      		        watch_xpm, watch_xx_xpm,
			color_key, active_color_key,
			background, arm_background, win_attr);
}



//-----------------------------------------------------------------------
// Set pixmap and label
//-----------------------------------------------------------------------

void set_label(Widget w, const MString& new_label, const char *image)
{
    if (w == 0)
	return;

    assert(XtIsSubclass(w, xmLabelWidgetClass));

    XmString old_label = 0;
    XtVaGetValues(w, XmNlabelString, &old_label, XtPointer(0));
    if (old_label == 0 ||
	XmStringCompare(new_label.xmstring(), old_label) == 0)
    {
	Arg args[10];
	Cardinal arg = 0;
	XtSetArg(args[arg], XmNlabelString, new_label.xmstring()); arg++;

	if (image != 0)
	{
	    Pixel foreground = 0;
	    Pixel background = 0;
	    Dimension highlight_thickness = 0;
	    Pixmap bottom_shadow_pixmap = XmUNSPECIFIED_PIXMAP;

	    XtVaGetValues(w,
			  XmNforeground, &foreground,
			  XmNbackground, &background,
			  XmNbottomShadowPixmap, &bottom_shadow_pixmap,
			  XmNhighlightThickness, &highlight_thickness,
			  XtPointer(0));

	    string s1 = image;
	    string s2 = s1 + "-xx";
	    string s3 = s1 + "-arm";
	    string s4 = s1 + "-hi";

	    Pixmap p1 = XmGetPixmap(XtScreen(w), XMST(s1.chars()), foreground, background);
	    Pixmap p2 = XmGetPixmap(XtScreen(w), XMST(s2.chars()), foreground, background);
	    Pixmap p3 = XmGetPixmap(XtScreen(w), XMST(s3.chars()), foreground, background);
	    Pixmap p4 = XmGetPixmap(XtScreen(w), XMST(s4.chars()), foreground, background);

	    if (bottom_shadow_pixmap == XmUNSPECIFIED_PIXMAP)
	    {
		// The button is active (non-flattened) - pixmaps are swapped
		Pixmap swap = p4;
		p4 = p1;
		p1 = swap;
	    }

	    if (p1 != XmUNSPECIFIED_PIXMAP)
	    {
		XtSetArg(args[arg], XmNlabelPixmap, p1); arg++;
	    }
	    if (p2 != XmUNSPECIFIED_PIXMAP)
	    {
		XtSetArg(args[arg], XmNlabelInsensitivePixmap, p2); arg++;
	    }
	    if (p3 != XmUNSPECIFIED_PIXMAP)
	    {
		XtSetArg(args[arg], XmNarmPixmap, p3); arg++;
	    }
	    if (p4 != XmUNSPECIFIED_PIXMAP && highlight_thickness == 0)
	    {
		XtSetArg(args[arg], XmNhighlightPixmap, p4); arg++;
	    }
	}
	XtSetValues(w, args, arg);
    }
    XmStringFree(old_label);
}
