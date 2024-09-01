// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class can simulate a control loop sceneario
// where calculates for every activ value a prediction
// using a fitted function that had to be calculated 
// previously with the script test_data/scripts/fit.py. 
// This function can be at most a 3th degree polynom. 
// Noise can also be added.
//
// Compatible files:
//   - test_data/extx-myc2-param.txt (ip2 simulation)
//   - test_data/kip2-mxc1-param.txt (ucn simulation)
//
// @Author: Jochem Snuverink
// @Maintainer: Jochem Snuverink

#pragma once
#include <string>


class DataCalc {
public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    DataCalc() = default;
    
    // Deconstructor
    ~DataCalc() = default;

    // Load the coefficient from a file
    // @param path to the file
    int load(std::string filename);

    // Set some noise in percent
    // @param percent of noise
    void set_noise(double new_noise);

    // Put the new active value
    // @param new value
    void put(double);
    
    // Get the caluclated prediction based on the prvious put
    // @return the prediction value
    double get();

private:
    /************************************************************
    *                       members
    ************************************************************/

    double m_noise = 0;     // Noise level by default 0

    double m_setting;       // The new activ value

    // The cofficient of a 3th degree polynomial
    double m_a;
    double m_b;
    double m_c;
    double m_d;
};
