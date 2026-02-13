// $Id$ -*- C++ -*-
// DDD logos and logo functions

// Copyright (C) 1996-1998 Technische Universitaet Braunschweig, Germany.
// Copyright (C) 2000 Universitaet Passau, Germany.
// Copyright (C) 2003 Free Software Foundation, Inc.
// Written by Andreas Zeller <zeller@gnu.org> 
//        and Stefan Eickeler <eickeler@gnu.org>.
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
#include "ddd.h"

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

#include <vector>

// ANSI C++ doesn't like the XtIsRealized() macro
#ifdef XtIsRealized
#undef XtIsRealized
#endif

//-----------------------------------------------------------------------------
// DDD logo
//-----------------------------------------------------------------------------

void install_modern_icons(Widget shell, const string& color_key);


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

    int ret = xpm("ddd.xpm",
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

//-----------------------------------------------------------------------------
// DDD Splash Screen
//-----------------------------------------------------------------------------

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

    int ret = xpm("splash.xpm",
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
// Retro Toolbar Icons
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

// single modern toolbar sprite sheet (1400 x 800, 7 x 4 grid of 200x200 cells)
#include "icons/toolbar/modern_iconset.xpm"

// single modern glyphs sprite sheet (1000 x 400, 5 x 2 grid of 200x200 cells)
#include "icons/glyphs/modern_glyphset.xpm"
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
void install_retro_icons(Widget shell,
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

void install_icons(Widget shell,
		   const string& color_key,
		   const string& active_color_key)
{
    if (app_data.retro_style)
        install_retro_icons(shell, color_key, active_color_key);
    else
        install_modern_icons(shell, color_key);
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

//-----------------------------------------------------------------------
// Modern Toolbar Icons
//-----------------------------------------------------------------------

template <class PIXTYPE>
class Image
{
public:
    int xdim = 0;  // Width  (pixels)
    int ydim = 0;  // Height (pixels)
    int cdim = 0;  // Number of colour planes (RGB ⇒ 3, Gray ⇒ 1, ...)
    std::vector<PIXTYPE> pixmap; // Contiguous planar image buffer (size = cdim·xdim·ydim)

    Image() = default;

    Image(int width, int height, int channels)
        : xdim(width), ydim(height), cdim(channels),
          pixmap(width * height * channels)
    {  }

    // Rule-of-zero (vector handles copy/move/destruct)
    Image(const Image&) = default;
    Image(Image&&) noexcept = default;
    Image& operator=(const Image&) = default;
    Image& operator=(Image&&) noexcept = default;
    ~Image() = default;

    void clear()
    {
        std::fill(pixmap.begin(), pixmap.end(), PIXTYPE{});
    }

    PIXTYPE& at(int x, int y, int c = 0)
    {
        return pixmap[(c * ydim + y) * xdim + x];
    }

    const PIXTYPE& at(int x, int y, int c = 0) const
    {
        return pixmap[(c * ydim + y) * xdim + x];
    }

    PIXTYPE* data() noexcept { return pixmap.data(); }
    const PIXTYPE* data() const noexcept { return pixmap.data(); }
};

using IconPix   = uint8_t;
using IconImage = Image<IconPix>;


// Fixed‑point box sampling scaler on one planar channel of Image<uint8_t>.
// inimg/outimg must have the same cdim.
void ScaleImage(const Image<uint8_t> *inimg, Image<uint8_t> *outimg)
{
    assert(inimg  != nullptr);
    assert(outimg != nullptr);

    const uint32_t oneq10 = 1024; // 1.0 as Q.10

    int inxdim  = inimg->xdim;
    int inydim  = inimg->ydim;
    int outxdim = outimg->xdim;
    int outydim = outimg->ydim;

    if (inxdim <= 0 || inydim <= 0 || outxdim <= 0 || outydim <= 0)
        return;

    int64_t *intermediateline = new int64_t[inxdim]; // as  Q.10
    int64_t *acculine         = new int64_t[inxdim]; // accumulators as Q.10

    int64_t sxscale = (int64_t)outxdim * oneq10 / inxdim;
    int64_t syscale = (int64_t)outydim * oneq10 / inydim;

    for (int color=0; color<inimg->cdim; color++)
    {
        int64_t rowsourceweight = syscale;
        for (int x = 0; x < inxdim; ++x)
            acculine[x] = oneq10 / 2;

        int iny = 0;
        for (int y = 0; y < outydim; ++y)
        {
            // 1) scale Y from inimg into intermediateline
            if (outydim == inydim)
            {
                // No vertical scaling: copy row
                const uint8_t *inputline = &inimg->at(0, y, color);
                for (int x = 0; x < inxdim; ++x)
                    intermediateline[x] = inputline[x] * oneq10;
            }
            else
            {
                int64_t rowdestweight = oneq10;
                while (rowsourceweight <= rowdestweight)
                {
                    const uint8_t *inputline = &inimg->at(0, iny, color);
                    for (int x = 0; x < inxdim; ++x)
                        acculine[x] += rowsourceweight * (int64_t)inputline[x];

                    rowdestweight -= rowsourceweight;
                    rowsourceweight = syscale;
                    if (iny < inydim - 1)
                        ++iny;
                }

                const uint8_t *inputline = &inimg->at(0, iny, color);
                for (int x = 0; x < inxdim; ++x)
                {
                    int64_t g = acculine[x] + rowdestweight * (int64_t)inputline[x];
                    intermediateline[x] = g;
                    acculine[x] = oneq10 / 2;
                }

                rowsourceweight -= rowdestweight;
            }

            // 2) scale X from intermediateline into outimg
            if (outxdim == inxdim)
            {
                // No horizontal scaling: copy row
                uint8_t *outline = &outimg->at(0, y, color);
                for (int x = 0; x < outxdim; ++x)
                    outline[x] = uint8_t(intermediateline[x] / oneq10);
            }
            else
            {
                int64_t g = oneq10 / 2;
                int64_t colsourceweight = sxscale;
                int incol = 0;

                uint8_t *outline = &outimg->at(0, y, color);

                for (int x = 0; x < outxdim; ++x)
                {
                    int64_t coldestweight = oneq10;
                    while (colsourceweight <= coldestweight)
                    {
                        g += colsourceweight * (int64_t)intermediateline[incol];

                        coldestweight -= colsourceweight;
                        colsourceweight = sxscale;
                        if (incol < inxdim - 1)
                            ++incol;
                    }

                    g += coldestweight * (int64_t)intermediateline[incol];
                    outline[x] = uint8_t(g / oneq10 / oneq10);
                    g = oneq10 / 2;

                    colsourceweight -= coldestweight;
                }
            }
        }
    }

    delete [] acculine;
    delete [] intermediateline;
}


// Convert an XImage (24‑bit RGB) to a 1‑channel grayscale IconImage.
// The modern toolbar sheet is already grayscale, so we just take red.
static IconImage ximage_to_gray_image(const XImage *src)
{
    IconImage img(src->width, src->height, 1);

    for (int y = 0; y < src->height; ++y)
    {
        for (int x = 0; x < src->width; ++x)
        {
            unsigned long p = XGetPixel(const_cast<XImage*>(src), x, y);
            unsigned char r = (p >> 16) & 0xFF;
            img.at(x, y) = (IconPix)r;
        }
    }

    return img;
}

// Convert a 1‑channel grayscale IconImage to an XImage, blending onto
// BACKGROUND by treating white as fully transparent and black as opaque ink.
//
// It takes both a FOREGROUND and BACKGROUND pixel and blends the
// grayscale icon as:
//
//   gray g in [0,255]  -> intensity I = g / 255
//   alpha = 1 - I      (coverage of foreground)
//   C = (1 - alpha)*BG + alpha*FG = BG * I + FG * (1 - I)
//
// So:
//   g = 255 (white) -> pixel = BACKGROUND
//   g =   0 (black) -> pixel = FOREGROUND
//
static XImage *blend_to_ximage(Widget w, Visual *visual, const IconImage &img,
                               Pixel foreground, Pixel background)
{
    Display *dpy = XtDisplay(w);

    XImage *dst = XCreateImage(dpy, visual,
                               24, ZPixmap, 0, 0,
                               img.xdim, img.ydim,
                               32, 0);
    if (!dst)
        return nullptr;

    dst->data = (char *)malloc(dst->bytes_per_line * dst->height);
    if (!dst->data)
    {
        XDestroyImage(dst);
        return nullptr;
    }

    uint8_t fg_r = (foreground >> 16) & 0xFF;
    uint8_t fg_g = (foreground >> 8)  & 0xFF;
    uint8_t fg_b =  foreground        & 0xFF;

    uint8_t bg_r = (background >> 16) & 0xFF;
    uint8_t bg_g = (background >> 8)  & 0xFF;
    uint8_t bg_b =  background        & 0xFF;

    for (int y = 0; y < img.ydim; ++y)
    {
        for (int x = 0; x < img.xdim; ++x)
        {
            uint8_t g = img.at(x, y); // 0=black .. 255=white

            unsigned int inv = 255u - g;

            unsigned long out_r = (bg_r * g + fg_r * inv) / 255u;
            unsigned long out_g = (bg_g * g + fg_g * inv) / 255u;
            unsigned long out_b = (bg_b * g + fg_b * inv) / 255u;

            unsigned long out_p = (out_r << 16) | (out_g << 8) | out_b;

            XPutPixel(dst, x, y, out_p);
        }
    }

    return dst;
}

static XImage *image_to_mask(Widget w, Visual *visual, const IconImage &img)
{
    Display *dpy = XtDisplay(w);

    XImage *mask = XCreateImage(dpy, visual,
                               1, XYBitmap, 0, 0,
                               img.xdim, img.ydim,
                               8, 0);
    if (!mask)
        return nullptr;


    mask->data = (char *)malloc(mask->height * mask->bytes_per_line);

    if (!mask->data)
    {
        XDestroyImage(mask);
        return nullptr;
    }

    memset(mask->data, 0, mask->height * mask->bytes_per_line);

    for (int y = 0; y < img.ydim; ++y)
    {
        // determine the first and last foreground pixel and use it as mask
        int firstpixel = img.xdim;
        int lastpixel = -1;

        for (int x = 0; x < img.xdim; ++x)
        {
            uint8_t g = img.at(x, y); // 0=black .. 255=white
            if (g<128)
            {
                firstpixel = std::min(firstpixel, x);
                lastpixel = std::max(lastpixel, x);
            }
        }

        if (lastpixel>0)
        {
            for (int x = firstpixel; x <=lastpixel; x++)
                XPutPixel(mask, x, y, 1);
        }
    }

    return mask;
}


// Sprite‑sheet geometry: 1400 x 800, 200 x 200 grid => 7 x 4
static const int MODERN_ICON_CELL = 200;
static const int MODERN_ICON_COLS = 7;
static const int MODERN_ICON_ROWS = 4;
static const int MODERN_GLYPH_CELL = 200;
static const int MODERN_GLYPH_COLS = 5;
static const int MODERN_GLYPH_ROWS = 2;

static IconImage modern_toolbar_sheet;
static bool modern_toolbar_sheet_loaded = false;

static IconImage modern_glyph_sheet;
static bool modern_glyph_sheet_loaded = false;

static int width_of_plain_arrow  = 0;
static int width_of_plain_stop   = 0;

int get_stop_width() { return width_of_plain_stop;}
int get_arrow_width() { return width_of_plain_arrow;}

struct SheetEntry {
    int         gridx;
    int         gridy;
    const char *name;
};

static const SheetEntry icon_sheet[] = {
    { 2, 0, BREAK_AT_ICON },
    { 2, 1, CLEAR_AT_ICON },
    { 6, 2, CLUSTER_ICON },
    { 5, 3, DELETE_ICON },
    { 5, 0, DISPREF_ICON },
    { 3, 3, DISABLE_ICON },
    { 5, 0, DISPLAY_ICON },
    { 2, 3, ENABLE_ICON },
    { 5, 1, FIND_BACKWARD_ICON },
    { 1, 0, FIND_FORWARD_ICON },
    { 0, 3, HIDE_ICON },
    { 0, 0, LOOKUP_ICON },
    { 4, 3, MAKETEMP_ICON },
    { 2, 0, NEW_BREAK_ICON },
    { 4, 0, NEW_DISPLAY_ICON },
    { 3, 0, NEW_WATCH_ICON },
    { 6, 0, PLOT_ICON },
    { 4, 0, PRINT_ICON },
    { 1, 3, PROPERTIES_ICON },
    { 1, 2, ROTATE_ICON },
    { 2, 2, SET_ICON },
    { 0, 2, SHOW_ICON },
    { 6, 1, UNCLUSTER_ICON },
    { 3, 2, UNDISPLAY_ICON },
    { 3, 1, UNWATCH_ICON },
    { 3, 0, WATCH_ICON },
    { 0, 1, "questionmark", },
    { 1, 1, "exclamationmark", }
};

static const SheetEntry glyph_sheet[] = {
    { 3, 0, "plain_arrow" },
    { 3, 0, "grey_arrow" },
    { 4, 1, "past_arrow" },
    { 4, 0, "signal_arrow" },
    { 3, 1, "drag_arrow" },
    { 0, 0, "plain_stop" },
    { 1, 0, "plain_cond" },
    { 2, 0, "plain_temp" },
    { 0, 0, "multi_stop" },
    { 1, 0, "multi_cond" },
    { 2, 0, "multi_temp" },
    { 0, 0, "grey_stop" },
    { 1, 0, "grey_cond" },
    { 2, 0, "grey_temp" },
    { 0, 1, "drag_stop" },
    { 1, 1, "drag_cond" },
    { 2, 1, "drag_temp" }
};


static const size_t icon_sheet_count =
    sizeof(icon_sheet) / sizeof(icon_sheet[0]);

static const size_t glyph_sheet_count =
    sizeof(glyph_sheet) / sizeof(glyph_sheet[0]);

// Load the modern toolbar sheet (modern_iconset.xpm) into IconImage once.
static bool load_modern_toolbar_sheet_image(Widget w,
                                            const string &color_key,
                                            const XWindowAttributes &win_attr)
{
    if (modern_toolbar_sheet_loaded)
        return modern_toolbar_sheet.xdim > 0;

    XpmAttributes attr{};
    attr.valuemask = XpmVisual | XpmColormap | XpmDepth;
    attr.visual    = win_attr.visual;
    attr.colormap  = win_attr.colormap;
    attr.depth     = win_attr.depth;
    add_color_key(attr, color_key);
    add_closeness(attr);

    XImage *sheet_ximage = nullptr;
    XImage *shape        = nullptr;

    int ret = xpm("modern_iconset.xpm",
                  XpmCreateImageFromData(XtDisplay(w),
                                         (char **)modern_iconset_xpm,
                                         &sheet_ximage,
                                         &shape,
                                         &attr));
    XpmFreeAttributes(&attr);
    if (shape)
        XDestroyImage(shape);

    modern_toolbar_sheet_loaded = true;

    if (ret != XpmSuccess || !sheet_ximage)
    {
        if (sheet_ximage)
            XDestroyImage(sheet_ximage);
        modern_toolbar_sheet = IconImage(); // empty
        return false;
    }

    modern_toolbar_sheet = ximage_to_gray_image(sheet_ximage);
    XDestroyImage(sheet_ximage);

    return modern_toolbar_sheet.xdim > 0;
}

// Load the modern glyph sheet (modern_iconset.xpm) into IconImage once.
static bool load_modern_glyph_sheet_image(Widget w,
                                            const string &color_key,
                                            const XWindowAttributes &win_attr)
{
    if (modern_glyph_sheet_loaded)
        return modern_glyph_sheet.xdim > 0;

    XpmAttributes attr{};
    attr.valuemask = XpmVisual | XpmColormap | XpmDepth;
    attr.visual    = win_attr.visual;
    attr.colormap  = win_attr.colormap;
    attr.depth     = win_attr.depth;
    add_color_key(attr, color_key);
    add_closeness(attr);

    XImage *sheet_ximage = nullptr;
    XImage *shape        = nullptr;

    int ret = xpm("modern_glyphset.xpm",
                  XpmCreateImageFromData(XtDisplay(w),
                                         (char **)modern_glyphset_xpm,
                                         &sheet_ximage,
                                         &shape,
                                         &attr));
    XpmFreeAttributes(&attr);
    if (shape)
        XDestroyImage(shape);

    modern_glyph_sheet_loaded = true;

    if (ret != XpmSuccess || !sheet_ximage)
    {
        if (sheet_ximage)
            XDestroyImage(sheet_ximage);
        modern_glyph_sheet = IconImage(); // empty
        return false;
    }

    modern_glyph_sheet = ximage_to_gray_image(sheet_ximage);
    XDestroyImage(sheet_ximage);

    return modern_glyph_sheet.xdim > 0;
}

static IconImage extract_patch(const IconImage &sheet,
                                    int grid_x, int grid_y,
                                    int cell_size)
{
    IconImage patch(cell_size, cell_size, 1);

    int src_x0 = grid_x * cell_size;
    int src_y0 = grid_y * cell_size;

    for (int y = 0; y < cell_size; ++y)
    {
        int sy = src_y0 + y;
        for (int x = 0; x < cell_size; ++x)
        {
            int sx = src_x0 + x;
            IconPix v = 255; // default white
            if (sx >= 0 && sy >= 0 &&
                sx < sheet.xdim && sy < sheet.ydim)
            {
                v = sheet.at(sx, sy, 0);
            }
            patch.at(x, y, 0) = v;
        }
    }

    return patch;
}

static void install_modern_button_icon(Widget shell, const _XtString name,
                                       int gridx, int gridy,
                                       const string& color_key,
                                       Pixel foreground,
                                       Pixel insensitive_foreground,
                                       Pixel background,
                                       Pixel arm_background,
                                       const XWindowAttributes& win_attr)
{
    int dst_size = 4 * app_data.variable_width_font_size;

    if (!load_modern_toolbar_sheet_image(shell, color_key, win_attr))
        return;

   IconImage src = extract_patch(modern_toolbar_sheet,
                                       gridx, gridy,
                                       MODERN_ICON_CELL);

    IconImage dst(dst_size, dst_size, src.cdim);
    ScaleImage(&src, &dst);

    // Normal icon
    XImage *img = blend_to_ximage(shell, win_attr.visual, dst, foreground, background);
    if (img)
        XmInstallImage(img, XMST(name));

    // Insensitive icon: same grayscale, but dark-gray foreground
    string insensitive_name = string(name) + "-xx";
    XImage *imgxx = blend_to_ximage(shell, win_attr.visual, dst, insensitive_foreground, background);
    if (imgxx)
        XmInstallImage(imgxx, XMST(insensitive_name.chars()));

    // Armed icon: active color key, arm background
    string armed_name = string(name) + "-arm";
    XImage *imgarm = blend_to_ximage(shell, win_attr.visual, dst, foreground, arm_background);
    if (imgarm)
        XmInstallImage(imgarm, XMST(armed_name.chars()));

    // Highlight icon: active color key, normal background
    string hi_name = string(name) + "-hi";
    XImage *imghi = blend_to_ximage(shell, win_attr.visual, dst, foreground, background);
    if (imghi)
        XmInstallImage(imghi, XMST(hi_name.chars()));
}

void install_modern_icons(Widget shell, const string& color_key)
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

    // Choose a darker foreground for insensitive icons
    Pixel insensitive_foreground = bottom_shadow;

    // DDD icon (always in color, keep old implementation)
    install_icon(shell, DDD_ICON,
                 ddd_xpm,
                 "best", background, win_attr, false);

    // Toolbar icons from modern sprite sheet
    for (size_t i = 0; i < icon_sheet_count; ++i)
    {
        const SheetEntry &entry = icon_sheet[i];
        install_modern_button_icon(shell, entry.name, entry.gridx, entry.gridy, color_key,
                                   foreground, insensitive_foreground,
                                   background, arm_background,
                                   win_attr);
    }
}

void install_glyphs(Widget shell)
{
    // Determine attributes
    XWindowAttributes win_attr;
    XGetWindowAttributes(XtDisplay(shell),
                         RootWindowOfScreen(XtScreen(shell)),
                         &win_attr);

    if (!load_modern_glyph_sheet_image(shell, string("c"), win_attr))
        return;

    Display *display = XtDisplay(toplevel);
    XrmDatabase db = XtDatabase(display);

    // Glyph icons from modern glyph sprite sheet
    for (size_t i = 0; i < glyph_sheet_count; ++i)
    {
        const SheetEntry &entry = glyph_sheet[i];

        int dst_size = 1.8 * app_data.fixed_width_font_size;
        if (i<5)
              dst_size = 2.4 * app_data.fixed_width_font_size; // increase size of arrows

        if (i==0)
            width_of_plain_arrow = dst_size;
        else if (i==5)
            width_of_plain_stop = dst_size;

        IconImage src = extract_patch(modern_glyph_sheet,
                                       entry.gridx, entry.gridy,
                                       MODERN_GLYPH_CELL);

        IconImage dst(dst_size, dst_size, src.cdim);
        ScaleImage(&src, &dst);

        string resource = string(entry.name) + ".foreground";

        string str_name  = "ddd*" + resource;
        string str_class = "Ddd*" + resource;

        char *type;
        XrmValue xrmvalue;
        Bool ok = XrmGetResource(db, str_name.chars(), str_class.chars(), &type, &xrmvalue);
        string fg;
        if (ok)
            fg = string(xrmvalue.addr, xrmvalue.size - 1);

        ok = XrmGetResource(db, "ddd*XmText.background", "Ddd*XmText.background", &type, &xrmvalue);
        string bg;
        if (ok)
            bg = string(xrmvalue.addr, xrmvalue.size - 1);

        Colormap colormap = DefaultColormap(display, DefaultScreen(display));

        XColor colorfg, colorfgx;
        XAllocNamedColor(display, colormap, fg.chars(), &colorfg, &colorfgx);

        XColor colorbg, colorbgx;
        XAllocNamedColor(display, colormap, bg.chars(), &colorbg, &colorbgx);

        if (app_data.dark_mode)
        {
            // brighten color of glyphs in dark mode
            colorfgx.pixel += 0x00202020;

            // invert background
            colorbgx.pixel ^= 0x00ffffff;
        }

        XImage *img = blend_to_ximage(shell, win_attr.visual, dst, colorfgx.pixel, colorbgx.pixel);
        if (img)
            XmInstallImage(img, XMST(entry.name));

        XImage *mask = image_to_mask(shell, win_attr.visual, dst);
        if (mask)
            XmInstallImage(mask, XMST((string(entry.name) + "-mask").chars()));
    }
}
