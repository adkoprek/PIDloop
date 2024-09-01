// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This structure holds data read from .reg files
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <cstdint>
#include <vector>

#include "device.h"


typedef struct Config {
    // Data of the main devices
    Device activ;
    Device passiv;

    // When enabled the setpoint is controled by the pv from extern_setpoint
    bool use_extern_setpoint = false;
    std::string extern_setpoint;

    // General PID settings
    int64_t gain_below_boundary = 0; 
    int64_t gain_above_boundary = 0;
    double gain_boundary = 0;
    double i_param = 0;
    double d_param = 0;
    int64_t rate = 1;

    double coefficient = 0;

    // Danymic gain is a special frunction only really usefull for the ucn current regulation
    bool dynamic_gain = false;

    // Holds all the condition devices that are checked and can hold the execution
    std::vector<Device> condition_devices;
} Config;
