// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class implements QMainWindow and the ui
// defined in forms/mainwindow.ui. It is responsible
// for reading and writing configuration. Lock file 
// managment is inspired by @Jochem Snuverink.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <QMainWindow>
#include <QWidget>
#include <qobjectdefs.h>
#include <qtimer.h>
#include <qvariant.h>
#include <string>
#include <thread>

#include "../../forms/ui_mainwindow.h"
#include "config_parser.h"
#include "pid_control.h"
#include "real_time_plot.h"
#include "settings.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    // @param Widget parent
    MainWindow(QWidget* parent = nullptr);

    // Destructor
    ~MainWindow();

public slots:
    /************************************************************
    *                       slots
    ************************************************************/
    
    // Called when regulate button is clicked
    void on_regulate_clicked();

    // Called when hold button is clicked
    void on_hold_clicked();

    // Called when clear button is clicked
    void on_clear_clicked();

    // Called when minimize button is clicked
    void on_minimize_clikced();

    // Called when the read config action is clicked
    void on_read_config_clicked();

    // Called when the save config action is clicked
    void on_save_config_clikced();

    // Called when the stepsize changes
    // @param the new step size
    void on_step_chosen(double step);

private:
    /************************************************************
    *                       functions
    ************************************************************/

    // Function that setups the ui
    void setup_custom_ui();

    // Show a generic error message just with an ok button
    // @param message to show
    void show_dialog(std::string message);

    // Update ui when new data arrives from the logic
    void update_ui();

    // Check if the given lockfile exists
    // @param path to file to check
    // @return "" if the fiel doesn't exist otherwise the hostname of the mshine that created it
    std::string check_for_lock(std::string path);

    // Create lock file
    // @param the path to the file
    // @return 0 if operation successfull
    int create_lock(std::string path);

    // Release last lock file
    // @return 0 if successfull
    int release_lock();

    /************************************************************
    *                       members
    ************************************************************/

    Ui::MainWindow m_ui;                // Holds the ui
    Settings* m_settings;               // Internal Instance of the Settings widget
    RealTimePlot* m_real_time_plot;     // Internal Instance of the RealTimePlot widget
    QRect m_old_geometry;               // Holds the last full screen geometry

    PIDControl* m_pid_control;          // Internal Instance of the PIDControl class
    bool m_running = false;             // Flag to chek if programm running
    Config* m_config = nullptr;         // Internal Instance of the current Config struct
    bool m_new_file = true;             // Check latly a new file was loaded to reset the plot
    ConfigParser* m_config_parser;      // Internal Instance of the ConfigParser class
    QTimer* m_timer;                    // Timer to update ui
    std::thread* m_work_thread;         // Work thread for PIDControl

    std::string m_last_lock = "";       // Path to the last lock file
};
