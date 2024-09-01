// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This is wrapper around cafe to get and write scalar
// values to EPICS. Cafe is an internal PSI library developed
// by Jan Chrin.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#pragma once
#include <string>
#include "cafe.h"

class DataFetch {
public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    DataFetch();
    
    // Deconstructor
    ~DataFetch();

    // Get a double from EPICS
    // @param the PV
    // @param pointer where to write the output
    // @return 0 if everythin went well
    int get_double(std::string pv, double* output);
    
    // Write a double to EPICS
    // @param the PV
    // @param value to write
    // @return 0 if everythin went well
    int put_double(std::string pv, double input);

private:
    /************************************************************
    *                       members
    ************************************************************/

    CAFE* m_cafe;       // Internal instance of CAFE
};
