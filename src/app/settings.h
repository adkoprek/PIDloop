// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class is a widget and implements the ui from
// forms/settings.ui. It can load data from the Config
// struct and modify it.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <QWidget>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qslider.h>
#include <qspinbox.h>

#include "../logic/config.h"
#include "../../forms/ui_settings.h"
#include "data_fetch.h"
#include "pid_control.h"


class Settings : public QWidget {
    Q_OBJECT

public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    // @param intial Config* instance pointer
    // @param potiner to PIDContorl instance
    // @param parent Widget
    Settings(Config* config, PIDControl* pid_control, QWidget* parent = nullptr);

    // Deconstructor
    ~Settings();

    // Configure UI with new Config
    // @param pointer to the Config struct
    void configure(Config* config);

    // Set new step size when clicking on boxes or sliders
    // @param new step size if < 1 then it will be rounded to 1 for sliders
    void set_step_size(double step);

    // Updated with current data and write errors
    void update_running_data();
    
    // Rest every condition device to a white background
    void reset_condition_devices_color();

public slots:
    /************************************************************
    *                       slots
    ************************************************************/

    // Called when a cell in a table changes (with return pressed or clicked away)
    void on_table_changed(int row, int column);
    
    // Disable or enable boundary
    // @param true to enable
    void change_boundary_state(bool new_state);

private:
    /************************************************************
    *                       functions
    ************************************************************/
    
    // Set up the ui with the data
    void setup_custom_ui();
    
    // Connect everything in the ui
    void create_connections();
    
    // Connect all the boxes and sliders to be linked
    void connect_sliders_with_boxes();

    // Updated table data when new config arives
    void update_table();
    
    // Set foreground color for an entire table row
    // @param row index
    // @param the Qt color
    void set_table_row_color(int row, Qt::GlobalColor color);
    
    // Reset the backgroudn of a given row (to white)
    // @param row index
    void reset_row_background(int row);

    // Fetch data from passed field index and assign it correctly to the config
    // @param the row of the cell
    // @param the column of the cell
    void fetch_table_data(int row, int column);

    
    /************************************************************
    *                       members
    ************************************************************/

    Ui::Settings m_ui;                  // Holds the ui

    bool m_parameter_update = false;    // Disable on_table_changed slot
    double m_old_boundary;              // Save the boundary value when it gets disabled 
                                        // it gets set to 10e-9 so that only the gain above
                                        // boundry is used in physical applications
                                        
    DataFetch* m_data_fetch;            // Internaly managed pointer to DataFetch for getting data from EPICS
    Config* m_config;                   // Pointer to the current Config struct
    PIDControl* m_pid_control;          // Passed pointer to the current PIDControl
};
