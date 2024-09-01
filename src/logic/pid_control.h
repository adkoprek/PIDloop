// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This is the main class that does the complete PID
// calculation. And manages error messages.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <string>

#include "config.h"
#include "data_calc.h"
#include "data_fetch.h"
#include "state.h"


class PIDControl {
public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor 
    PIDControl();

    // Deconstructor
    ~PIDControl();

    // Setout the control with a new configuration
    // @param pointer to the new Config* struct
    void setup(Config* config);

    // Start the calculations
    void start();

    // Stop the clculations
    void stop();

    // Get the latest error
    // @retunr the error message
    std::string get_latest_error();

    // Set an error message externally
    // @param new message
    void set_error(std::string error);

    // Get the current pointer to the State struct
    // @return pointer to the State* struct
    State* get_state();

    // Check if a condition device is out of bounds
    // @return true if one is out of bounds
    bool is_out_of_bounds();

private:
    /************************************************************
    *                       functions
    ************************************************************/

    // Caclulcate the actual new activ value
    void calc_new_activ();

    // Calculate the actuall PID
    // @return the new offset value
    double calc_pid();

    // Fetch the passiv parameter from EPICS
    void get_passiv_parameter();

    // Check every condition device
    // @return 0 if every device is in bounds
    int check_condition_devices();

    // Handles any kind of holding
    void handle_hold();

    /************************************************************
    *                       members
    ************************************************************/

    bool m_out_of_bounds = false;           // Flag that remembers if previous loop was out of bounds
    bool m_stop_flag = false;               // Flag to stop loop
                                                
    std::string m_error_message = "";       // The current error message
                                            
    // DataFetch modfies EPICS data and is to be used in production
    // DataCalc* is a simulation to use in testing
    DataFetch* m_data_fetch;
    DataCalc* m_data_calc;

    Config* m_config;                       // External pointer to current config
    State* m_state = nullptr;               // Internaly managed State of the controllery managed State of the
};
