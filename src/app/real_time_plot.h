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

#pragma once
#include <ctime>
#include <linux/limits.h>
#include <qobjectdefs.h>
#include <qvector.h>
#include <qwidget.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <vector>

#include "config.h"
#include "pid_control.h"
#include "state.h"
#include "../../forms/ui_realtimeplot.h"


class RealTimePlot : public QWidget {
    Q_OBJECT

public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    // @param pointer to instance of PIDControl to get data
    // @param parent Widget
    RealTimePlot(PIDControl* pid_control, QWidget* parent = nullptr);

    // Deconstructor
    ~RealTimePlot();

    // Start drawing also resets the state
    // @param pointer to current drawing
    void start(Config* config);

    // Stop drawing
    void stop();

    // Resume drawing
    void resume();

public slots:
    /************************************************************
    *                       slots
    ************************************************************/

    // Called when m_timer is triggered
    void update_plot();

private:
    /************************************************************
    *                       functions
    ************************************************************/

    // Sets the axis scale for the current data
    void set_axis_scale();

    // Find the max and min valud of an array for the axis scale
    // @param pointer to vector with data
    // @param pointer where minimum value will be written
    // @param pointer where maximum value will be written
    void find_min_max(std::vector<double>& data, double* min, double* max);

    /************************************************************
    *                       members
    ************************************************************/

    Ui::RealTimePlot m_ui;              // Holds the ui
                                        //
    QwtPlot* m_plot;                    // Instance of the plot to be shown
    QwtPlotCurve* m_curve_activ;        // QwtCureve for active device (left scale)
    QwtPlotCurve* m_curve_passiv;       // QwtCureve for passiv device (right scale)

    QTimer* m_timer = nullptr;          // Timer to update diagram
    double m_epochs_since_last_error;   // Countes how many errors since the last unique error
    bool m_stop_updating = false;       // Set to stop the timer

    PIDControl* m_pid_control;          // Pointer from outside to PIDControl instance
    State* m_state;                     // Current state with data from PIDControl
    Config* m_config;                   // Current pointer to Config struct
    std::vector<double> m_x_data;       // Just filled with the counter values
};
