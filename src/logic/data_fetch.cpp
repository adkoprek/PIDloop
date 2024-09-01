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

#include <string>

#include "data_fetch.h"


/************************************************************
*                       public
************************************************************/

// Constructor
DataFetch::DataFetch() {
    m_cafe = new CAFE();
    m_cafe->channelOpenPolicy.setTimeout(0.1);
}

// Deconstructor
DataFetch::~DataFetch() {
    m_cafe->closeHandles();
    delete m_cafe;
}

// Get a double from EPICS
int DataFetch::get_double(std::string pv, double* output) {
    int status = m_cafe->get(pv.c_str(), *output);
    if (status != ICAFE_NORMAL) return - 1;
    return 0;
}

// Write a double to EPICS
int DataFetch::put_double(std::string pv, double input) {
    int status = m_cafe->set(pv.c_str(), input);
    if (status != ICAFE_NORMAL) return -1;
    return 0;
}
