// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class is a widget and implements the ui from 
// forms/realtimeplot.ui. It only displays passiv parameters
// and doesn't change any configuration.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#include <cmath>
#include <cstdlib>
#include <limits>
#include <qlocale.h>
#include <qnamespace.h>
#include <qpainter.h>
#include <qpen.h>
#include <qwidget.h>
#include <QTimer>
#include <QDateTime>
#include <qwt_scale_widget.h>
#include <string>

#include "real_time_plot.h"
#include "config.h"
#include "pid_control.h"
#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_scale_draw.h"


// Helper function just useful in this context
namespace  {

    // Convert double to string for Qt to display
    // @param double to convert
    // @return char* pointer to the static text
    const char* double_to_string(double input) {
        std::ostringstream strs;
        strs << input;
        static std::string standart_string = "";
        standart_string = strs.str();
        return standart_string.c_str();
    }

    // Class to customize the color of a QwtScale
    class CustomScaleDraw : public QwtScaleDraw {
    public:
        /************************************************************
        *                       functions
        ************************************************************/

        // Constructor
        CustomScaleDraw() {}

        // Set the color of the lines
        // @param pointer to the painter
        virtual void drawBackbone(QPainter* painter) const override {
            QPen pen = painter->pen();
            pen.setColor(Qt::magenta);
            painter->setPen(pen);
            QwtScaleDraw::drawBackbone(painter);
        }

        // Set the color of the ticks
        // @param pointer to the painter
        virtual void drawTick(QPainter* painter, double value, double len) const override {
            QPen pen = painter->pen();
            pen.setColor(Qt::magenta);
            painter->setPen(pen);
            QwtScaleDraw::drawTick(painter, value, len);
        }

        // Set the color of the label
        // @param value to display
        // @return QwtText object to return
        virtual QwtText label(double value) const override
        {
            QwtText lbl = QwtScaleDraw::label(value);
            lbl.setColor(Qt::magenta); 
            return lbl;
        }
    };
}

/************************************************************
*                       public
************************************************************/

// Constructor
RealTimePlot::RealTimePlot(PIDControl* pid_control, QWidget* parent) {
    m_ui.setupUi(this);
    m_plot = new  QwtPlot();
    m_ui.main_layout->insertWidget(1, m_plot);

    m_pid_control = pid_control;

    m_plot->setCanvasBackground(Qt::white);
    m_plot->enableAxis(QwtPlot::yRight);

    this->setMinimumHeight(0);

    QwtScaleWidget* y_right = m_plot->axisWidget(QwtPlot::yRight);
    y_right->setColorBarEnabled(true);
    y_right->setScaleDraw(new CustomScaleDraw());

    m_plot->setAxisScale(QwtPlot::yLeft, 0, 10);
    m_plot->setAxisScale(QwtPlot::yRight, 0, 10);

    m_curve_activ = new QwtPlotCurve();
    m_curve_activ->setPen(QPen(Qt::black, 2));
    m_curve_activ->setYAxis(QwtPlot::yLeft);
    m_curve_activ->attach(m_plot);

    m_curve_passiv = new QwtPlotCurve();
    m_curve_passiv->setPen(QPen(Qt::magenta, 2));
    m_curve_passiv->setYAxis(QwtPlot::yRight);
    m_curve_passiv->attach(m_plot);
}

// Deconstructor
RealTimePlot::~RealTimePlot() {}

// Start drawing also resets the state
void RealTimePlot::start(Config* config) {
    m_config = config;
    m_state = m_pid_control->get_state();

    for (int i = - m_state->activ_data.size(); i < 1; i++) {
        m_x_data.push_back(i);
    }

    m_curve_activ->setSamples(m_x_data.data(), m_state->activ_data.data(), m_state->activ_data.size());
    m_curve_passiv->setSamples(m_x_data.data(), m_state->passiv_data.data(), m_state->passiv_data.size());

    m_plot->replot();

    delete m_timer;
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RealTimePlot::update_plot);

    // Time for one interation in ms
    m_timer->start(1000 / m_config->rate);
}

// Stop drawing
void RealTimePlot::stop() {
    // Timer is not stoped directly to make the error message disapper with time
    m_stop_updating = true;
}

// Resume drawing
void RealTimePlot::resume() {
    m_stop_updating = false;
    m_timer->start(1000 / m_config->rate);
}

/************************************************************
*                       slots
************************************************************/

// Called when m_timer is triggered
void RealTimePlot::update_plot() {
    std::string error_message = m_pid_control->get_latest_error();
    if (error_message != m_ui.error_label->text().toStdString()) {
        m_epochs_since_last_error = 0;
        m_ui.error_label->setText(error_message.c_str());
    }
    else {
        if ((++m_epochs_since_last_error * (1.0 / m_config->rate)) > 5) {
            m_ui.error_label->setText("");
            m_pid_control->set_error("");
            if (m_stop_updating) m_timer->stop();
        }
    }

    if (m_stop_updating) return;

    m_x_data.erase(m_x_data.begin());
    m_x_data.push_back(m_x_data[m_x_data.size() - 2] + 1);

    m_curve_activ->setSamples(m_x_data.data(), m_state->activ_data.data(), m_state->activ_data.size());
    m_curve_passiv->setSamples(m_x_data.data(), m_state->passiv_data.data(), m_state->passiv_data.size());

    set_axis_scale();
    m_plot->replot();
    
    m_ui.activ_parameter->setText(double_to_string(m_state->activ_data.back()));
    m_ui.passiv_parameter->setText(double_to_string(m_state->passiv_data.back()));
    m_ui.rate->setText(double_to_string(m_state->actual_rate));
    
    // Restart the timer in case the user has changed the timer
    m_timer->stop();
    m_timer->start(1000 / m_config->rate);
}

/************************************************************
*                       private
************************************************************/

// Sets the axis scale for the current data
void RealTimePlot::set_axis_scale() {
    m_plot->setAxisScale(QwtPlot::xBottom, m_x_data.front(), m_x_data.back());
    double activ_min, activ_max, passiv_min, passiv_max;
    find_min_max(m_state->activ_data,  &activ_min,  &activ_max);
    find_min_max(m_state->passiv_data, &passiv_min, &passiv_max);

    // Apply this so that the curves probably overlapp
    activ_min *= 0.9;
    passiv_max *= 1.1;

    if (activ_min < m_config->activ.min) activ_min = m_config->activ.min;
    if (activ_max > m_config->activ.max) activ_max = m_config->activ.max;
    if (passiv_min < m_config->passiv.min) passiv_min = m_config->passiv.min;
    if (passiv_max > m_config->passiv.max) passiv_max = m_config->passiv.max;

    m_plot->setAxisScale(QwtPlot::yLeft, activ_min, activ_max);
    m_plot->setAxisScale(QwtPlot::yRight, passiv_min, passiv_max);
}

// Find the max and min valud of an array for the axis scale
void RealTimePlot::find_min_max(std::vector<double>& data, double* min, double* max) {
    *min =  std::numeric_limits<double>::infinity();
    *max = -std::numeric_limits<double>::infinity();

    for (double value : data) {
        if (!std::isnan(value)) {
            if (value < *min) *min = value;
            if (value > *max) *max = value;
        }
    }

    // Give more space than the maximum, this has to change depending 
    // if the value is below or above 0
    if      (*min < 0 ) *min *= 0.88;
    else if (*min > 0 ) *min *= 0.92;
    else if (*min == 0) *min  = -0.2;
    
    if      (*max > 0 ) *max *= 1.12;
    else if (*max < 0 ) *max *= 1.08;
    else if (*max == 0) *max  = 0.20;
}
