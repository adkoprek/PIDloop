// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This structure holds data needed for a device config
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <string>


typedef struct Device {
    // The EPICS PV of the device
    std::string name;

    // Min and Max are initialized to (-)13+10 because in real application nothing is
    // outside this borders these numbers are not shown in the table, the table is in 
    // such a case just blank
    double min = -1e+10;
    double max =  1e+10;

    // Only valid when the device is an active device
    double setpoint;
    double hold_value = 1e+10;
} Device;
