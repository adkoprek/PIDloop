// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This structure holds the current data of the
// PIDControl loop
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <vector>


typedef struct State {
    // Counter to display for diagram and reducegain (< 30)
    int counter = 0;

    // Holds the current passiv value
    double current_value = 0;

    // Holds the last 3 errors where at index 0 the oldest resides
    std::vector<double> error = {0, 0, 0};

    // The actual rate that is applied
    int actual_rate = 0;

    // Holds the last 500 data points where at index 0 the oldest resides
    std::vector<double> activ_data;
    std::vector<double> passiv_data;

    // Holds the current value for every condition device
    // in the same order as in the configuration
    std::vector<double> condition_data;
} State;
