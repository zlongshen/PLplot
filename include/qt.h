/*

This software is provided under the LGPL in March 2009 by the
Cluster Science Centre
QSAS team,
Imperial College, London

Copyright (C) 2009  Imperial College, London
Copyright (C) 2009  Alan W. Irwin

This is free software; you can redistribute it and/or modify
it under the terms of the GNU General Lesser Public License as published
by the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

To received a copy of the GNU Library General Public License
write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

    History:


March 2009:  v1.00
Initial release.

*/

/*
Interpretation of the -geometry XSIZExYSIZE option (or the third and fourth 
parameters of plspage if those are specified by the user instead) for
the various devices:
- the raster devices (bmpqt, jpgqt, pngqt, ppmqt, tiffqt):
XSIZE and YSIZE define the x and y size in pixels
- qtwidget:
XSIZE and YSIZE define the default x and y size of the widget in
pixels, as well as its aspect ratio, which is kept when the widget is
resized.
- svgqt, epsqt, pdfqt:
XSIZE and YSIZE define the x and y size in points (1/72 of inches).
EPS and PDF files will be drawn in A4 pages for Qt versions before 4.4

Interpretation of the -dpi DPI option (or the first parameter of
plspage if that is specified by the user instead).
DPI is ignored for all but the raster devices.  For those devices
DPI should be set to the DPI value of the monitor being used to view
the results if exact character sizes are desired.  Otherwise, DEFAULT_DPI
(set to an average value for modern monitors) is used instead.
*/

#ifndef QT_H
#define QT_H

#include <iostream>
#include <QImage>
#include <QPainter>
#include <QLinkedList>
#include <QPrinter>
#include <QApplication>
#include <QWidget>
#include <QMouseEvent>
#include <QTabWidget>
#include <QMainWindow>
#include <QPicture>
#include <QMutex>

#include "plplotP.h"
#include "drivers.h"

// Used for devices (epsqt, pdfqt, svgqt) with known physical size in points.
#define POINTS_PER_INCH 72

// Average value of dots per inch assumed for modern monitors if the user
// does not specify a value appropriate to their monitor with plspage or
// the -dpi option.  Used only for devices with size specified in pixels 
// but not qtwidget since it has independent access to information (e.g.,
// delivered by X) about the DPI of the monitor.  So this value is only
// used for the raster devices (bmpqt, jpgqt, pngqt, ppmqt, tiffqt).
#define DEFAULT_DPI 72

// These values are in units of pixels (the raster devices and qtwidget)
// or points (epsqt, pdfqt, svgqt).  In the points case, this corresponds
// to the A4 paper size.
#define QT_DEFAULT_X 842
#define QT_DEFAULT_Y 595

class PLDLLIMPEXP_QT QtPLDriver;

// Master Device Handler for multiple streams
// Only handles multiple Qt devices
class PLDLLIMPEXP_QT MasterHandler:public QObject
{
    Q_OBJECT
    
    public:
        MasterHandler();

        bool isMasterDevice(QtPLDriver* d);
        void setMasterDevice(QtPLDriver* d);
        void DeviceChangedPage(QtPLDriver* d);
        void DeviceClosed(QtPLDriver* d);

    signals:
        void MasterChangedPage();
        void MasterClosed();

    protected:
        QtPLDriver* masterDevice;
};

// Basic class, offering the common interface to all Qt plplot devices
class PLDLLIMPEXP_QT QtPLDriver
{
    public:
        // Constructor, taking the device size as arguments
        QtPLDriver(PLINT i_iWidth=QT_DEFAULT_X, PLINT i_iHeight=QT_DEFAULT_Y);

        virtual ~QtPLDriver(); // does not delete emitter!
                
        // Draws a line from (x1, y1) to (x2, y2) in internal plplot coordinates
        virtual void drawLine(short x1, short y1, short x2, short y2);
        virtual void drawPolyline(short * x, short * y, PLINT npts);
        virtual void drawPolygon(short * x, short * y, PLINT npts);
        virtual void drawText(PLStream* pls, EscText* txt);
        virtual void setColor(int r, int g, int b, double alpha);
        virtual void setBackgroundColor(int r, int g, int b, double alpha){}
        virtual void setWidth(PLINT w);
        // Set pen to draw solid strokes (called after drawing dashed strokes)
        virtual void setSolid();
        // Conversion factor from internal plplot coordinates to device coordinates
        double downscale;
        double m_dWidth, m_dHeight;
        static QMutex mutex; // All-purpose mutex

    protected:
        // Returns font with the good size for a QPicture's resolution
        QFont getFont(PLUNICODE code);
        // Draws text in a QPicture using a sub-QPicture (!), updates the current xOffset
        void drawTextInPicture(QPainter*, const QString&);
        // Gets the QPicture displaying text, with the base chrht char height
        QPicture getTextPicture(PLUNICODE fci, PLUNICODE* text, int len, PLFLT chrht);

        // Text-related variables
        bool underlined;
        bool overlined;
        double currentFontScale;
        double currentFontSize;
        double yOffset;
        double xOffset;
                
        QPainter* m_painterP;
};

#if defined (PLD_bmpqt) || defined(PLD_jpgqt) || defined (PLD_pngqt) || defined(PLD_ppmqt) || defined(PLD_tiffqt)
// Driver painting whatever raster format Qt can save
class PLDLLIMPEXP_QT QtRasterDevice: public QtPLDriver, public QImage
{
    public:
        QtRasterDevice(int i_iWidth=QT_DEFAULT_X,
                        int i_iHeight=QT_DEFAULT_Y);
        virtual ~QtRasterDevice();

        virtual void setBackgroundColor(int r, int g, int b, double alpha);
        void definePlotName(const char* fileName, const char* format);
        void savePlot();
        virtual void setResolution(double dotsPerInch)
        {
            setDotsPerMeterX((int)((dotsPerInch/25.4)*1000.));
            setDotsPerMeterY((int)((dotsPerInch/25.4)*1000.));
        }
    
    protected:
        char format[5];
        QString fileName;
};
#endif

#if defined(PLD_svgqt) && QT_VERSION >= 0x040300
#include <QSvgGenerator>
// Driver painting on an SVG device
class PLDLLIMPEXP_QT QtSVGDevice: public QtPLDriver, public QSvgGenerator
{
    public:
        QtSVGDevice(int i_iWidth=QT_DEFAULT_X,
                    int i_iHeight=QT_DEFAULT_Y);

        virtual ~QtSVGDevice();

        virtual void setBackgroundColor(int r, int g, int b, double alpha);
        void definePlotName(const char* fileName);
        void savePlot();
        
    protected:
};
#endif

#if defined (PLD_epsqt) || defined (PLD_pdfqt)
// Driver painting on an EPS or PDF device, uses QPrinter
// A (possibly dummy) QApplication must be declared before use
class PLDLLIMPEXP_QT QtEPSDevice: public QtPLDriver, public QPrinter
{
    public:
#if QT_VERSION < 0x040400
        QtEPSDevice(int i_iWidth=-1, int i_iHeight=-1);
#else
        QtEPSDevice(int i_iWidth=QT_DEFAULT_X, int i_iHeight=QT_DEFAULT_Y);
#endif
                
        virtual ~QtEPSDevice();

        virtual void setBackgroundColor(int r, int g, int b, double alpha);
        void definePlotName(const char* fileName, int ifeps);
        void savePlot();

    protected:
};
#endif

#if defined (PLD_qtwidget) || defined (PLD_extqt)

typedef enum ElementType_
{
    LINE,
    POLYLINE,
    POLYGON,
    RECTANGLE,
    SET_WIDTH,
    SET_COLOUR,
    SET_SMOOTH,
    TEXT,
    SET_BG_COLOUR
} ElementType; // Identifiers for elements of the buffer

struct ColourStruct_
{
    PLINT R, G, B, A;
};

struct TextStruct_
{
    PLFLT x;
    PLFLT y;
    PLFLT clipxmin;
    PLFLT clipymin;
    PLFLT clipxmax;
    PLFLT clipymax;
    PLFLT rotation;
    PLFLT shear;
    PLFLT stride;
    PLFLT just;
    PLUNICODE* text;
    PLUNICODE fci;
    PLINT len;
    PLFLT chrht;
};


class BufferElement
{
    public:
        ElementType Element;

        union DataType
        {
            QLineF* Line;
            QPolygonF* Polyline;
            QRectF * Rect;
            struct ColourStruct_* ColourStruct;
            struct TextStruct_* TextStruct;
            PLINT intParam;
            PLFLT fltParam;
        } Data;
};

// This widget allows to use plplot as a plotting engine in a Qt Application
// The aspect ratio of the plotted data is constant, so gray strips are used
// to delimit the page when the widget aspect ratio is not the one of the plotted page
class PLDLLIMPEXP_QT QtPLWidget: public QWidget, public QtPLDriver
{
    Q_OBJECT

    public:
        // Parameters are the actual size of the page, NOT the size of the widget
        // Call QWidget::resize for that
        QtPLWidget(int i_iWidth=QT_DEFAULT_X, int i_iHeight=QT_DEFAULT_Y, QWidget* parent=0);
                
        virtual ~QtPLWidget();
                
        void clearWidget();
        void clearBuffer();
                
        int pageNumber;
                
        void drawLine(short x1, short y1, short x2, short y2);
        void drawPolyline(short * x, short * y, PLINT npts);
        void drawPolygon(short * x, short * y, PLINT npts);
        void setColor(int r, int g, int b, double alpha);
        void setBackgroundColor(int r, int g, int b, double alpha);
        void setWidth(PLINT r);
        void drawText(PLStream* pls, EscText* txt);

    protected:
                
        void resizeEvent(QResizeEvent*);
        void paintEvent(QPaintEvent*);
                
        void getPlotParameters(double & io_dXFact, double & io_dYFact, double & io_dXOffset, double & io_dYOffset); // gives the parameters to scale and center the plot on the page
        void doPlot(QPainter* p, double x_fact, double y_fact, double x_offset, double y_offset); // Actually draws the plot. Deported in a function for readability
        void renderText(QPainter* p, struct TextStruct_* s, double x_fact, double x_offset, double y_fact, double y_offset);
                
        double m_dAspectRatio; // Is kept constant during resizes
        QPixmap * m_pixPixmap; // stores the drawn image as long as it does not have to be regenerated

        QLinkedList<BufferElement> m_listBuffer; // Buffer holding the draw instructions
        bool m_bAwaitingRedraw;
        int m_iOldSize; // Holds the size of the buffer. Modified => image has to be redrawn

        struct
        {
            int r;
            int g;
            int b;
            double alpha;
        } lastColour;

                
    protected slots:
        void mouseReleaseEvent ( QMouseEvent * event );
        void keyPressEvent(QKeyEvent* event);
        void closeEvent(QCloseEvent* event);
        void nextPage();
};

#endif

#if defined (PLD_extqt)
class PLDLLIMPEXP_QT QtExtWidget: public QtPLWidget
{
    Q_OBJECT

    public:
        QtExtWidget(int i_iWidth=QT_DEFAULT_X, int i_iHeight=QT_DEFAULT_Y, QWidget* parent=0);
        
        virtual ~QtExtWidget();

        void captureMousePlotCoords(PLFLT* x, PLFLT* y);

    public slots:

        void mouseMoveEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);
        void mousePressEvent(QMouseEvent* event);

    protected:
        void paintEvent(QPaintEvent*);

        struct
        {
            bool isTracking;
            double cursor_x, cursor_y;
        } cursorParameters;

        bool killed;
};

PLDLLIMPEXP_QT void plsetqtdev(QtExtWidget* widget); // Registers the widget as plot device, as the widget has to be created in the Qt application GUI, prior to any plplot call. Must be called before plinit().

PLDLLIMPEXP_QT void plfreeqtdev(); // Deletes and unregisters the device. 

#endif

#if defined(plplot_pyqt4_EXPORTS)
#define initplplot_pyqt4 PLDLLIMPEXP_PYQT4 initplplot_pyqt4
#endif

#endif