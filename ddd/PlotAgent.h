// $Id$ -*- C++ -*-
// Gnuplot interface

// Copyright (C) 1998 Technische Universitaet Braunschweig, Germany.
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

#ifndef _DDD_PlotAgent_h
#define _DDD_PlotAgent_h

#include "agent/LiterateA.h"
#include "assert.h"
#include "base/PrintGC.h"
#include "agent/ChunkQueue.h"

#include <fstream>
#include <vector>
#include <algorithm>

#include <stdint.h>

// Event types
const unsigned Plot = LiterateAgent_NTypes;   // Plot data received

const unsigned PlotAgent_NTypes = Plot + 1;   // number of events

struct PixelCache
{
    enum DataType {
        DT_UINT8,
        DT_INT8,
        DT_UINT16,
        DT_INT16,
        DT_UINT32,
        DT_INT32,
        DT_FLOAT32,
        DT_FLOAT64
    };

    enum Layout {
        L_INTERLEAVED,   // pixel: [c0, c1, c2, ...]
        L_PLANAR         // plane0 all, plane1 all, ...
    };

    DataType data_type = DT_UINT8;
    Layout   layout = L_INTERLEAVED;

    int      width = 0;
    int      height = 0;
    int      channels = 0;      // e.g. 1 or 3
    size_t   pixel_size = 0;    // bytes per channel sample

    std::vector<uint8_t> pixmap;  // raw bytes, size = width*height*channels*pixel_size

    PixelCache() {}

    bool valid() const
    {
        return width > 0 && height > 0 && channels > 0 && !pixmap.empty();
    }

    bool read_image(string file, int xdim, int ydim, int cdim, string gdbtype, Layout layout);
    bool write_image_interleaved(const string& filename);

    void *pixelat(int x, int y, int c = 0)
    {
        if (layout==L_PLANAR)
            return &pixmap[((c * height + y) * width + x) * pixel_size];
        else
            return &pixmap[((y * width + x) * channels + c) * pixel_size];

    }

    string print_pixel_value(int x, int y)
    {
        if (x<0 || x>width || y<0 || y>height)
            return "";

        string result = "";
        char buf[64];
        for (int c=0; c<channels; c++)
        {
            switch (data_type)
            {
                case DT_UINT8:
                    snprintf(buf, sizeof(buf), "%3d", *(uint8_t*)pixelat(x, y, c));
                    break;
                case DT_INT8:
                    snprintf(buf, sizeof(buf), "%3d", *(int8_t*)pixelat(x, y, c));
                    break;
                case DT_UINT16:
                    snprintf(buf, sizeof(buf), "%d", *(uint16_t*)pixelat(x, y, c));
                    break;
                case DT_INT16:
                    snprintf(buf, sizeof(buf), "%d", *(int16_t*)pixelat(x, y, c));
                    break;
                case DT_UINT32:
                    snprintf(buf, sizeof(buf), "%d", *(uint32_t*)pixelat(x, y, c));
                    break;
                case DT_INT32:
                    snprintf(buf, sizeof(buf), "%d", *(int32_t*)pixelat(x, y, c));
                    break;
                case DT_FLOAT32:
                    snprintf(buf, sizeof(buf), "%f", *(float*)pixelat(x, y, c));
                    break;
                case DT_FLOAT64:
                    snprintf(buf, sizeof(buf), "%lf", *(double*)pixelat(x, y, c));
                    break;
            }

            result += buf;
            if (c<channels-1)
                result += ", ";
        }
        return result;
    }
};

struct PlotElement
{
    enum PlotType {DATA_1D, DATA_2D, DATA_3D, IMAGE, RGBIMAGE, BGRIMAGE} plottype = DATA_2D;
    string file;		// allocated temporary file
    string title;		// Title currently plotted
    string value;		// Scalar
    bool binary = false;        // true for binary files
    string gdbtype;             // type of the variable as reported by GDB
    string xdim;                // x dimension of array
    string ydim;                // y dimension of array
    PixelCache imagedata;
};

class PlotAgent: public LiterateAgent {

public:
    DECLARE_TYPE_INFO

private:

    std::vector<PlotElement> elements;  // Data for the elements of the plot command
    std::ofstream plot_os;		// Stream used for adding data

    string init_commands;	// Initialization commands
    bool need_reset;		// Reset with next plot

protected:
    void reset();

    string getGnuplotType(string gdbtype);

public:
    static string plot_2d_settings;
    static string plot_3d_settings;

    // Constructor for Agent users
    PlotAgent(XtAppContext app_context, const string& pth,
	      unsigned nTypes = PlotAgent_NTypes)
	: LiterateAgent(app_context, pth, nTypes),
	  plot_os(), init_commands(""),
	  need_reset(false)
    {
	reset();
    }

    PixelCache *getPixelCache()
    {
        if (elements.empty())
            return nullptr;

        return &elements[0].imagedata;
    }

    // Start and initialize
    void start_with(const string& init);

    // Kill
    void abort();

    // Start plotting new data with TITLE in NDIM dimensions
    PlotElement &start_plot(const string& title);
    void open_stream(const PlotElement &emdata);

    // Add plot point
    void add_point(int x, const string& v);
    void add_point(double x, const string& v);
    void add_point(int x, int y, const string& v);
    void add_point(double x, double y, const string& v);

    // Add a break
    void add_break();

    // End plot
    void close_stream();

    // Flush accumulated data
    int flush();

    // Return number of dimensions
    int dimensions() const
    {
        if (std::any_of(elements.begin(), elements.end(), [&](const PlotElement &elem) { return elem.plottype == PlotElement::DATA_3D; }))
            return 3;
        return 2;
    }

    bool is_any_of_elements(PlotElement::PlotType type) const
    {
        return std::any_of(elements.begin(), elements.end(), [&](const PlotElement &elem) { return elem.plottype == type; });
    }

    bool isImage() const
    {
        return (is_any_of_elements(PlotElement::IMAGE) || is_any_of_elements(PlotElement::RGBIMAGE) || is_any_of_elements(PlotElement::BGRIMAGE));
    }

    // Print plot to FILENAME
    void print(const string& filename, 
	       const PrintGC& gc = PostScriptPrintGC());

    // Show plot state
    void set_state(const string& state);
};

#endif // _DDD_PlotAgent_h
// DON'T ADD ANYTHING BEHIND THIS #endif
