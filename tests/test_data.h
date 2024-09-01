// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class can simulate a control loop sceneario
// where it loads data points from a file and does
// an interpolation on each request to prdict the 
// exact value
//
// Compatible files:
//   - test_data/KIP2-MXC1_2024-07-25.txt (ucn simulation)
//
// @Author: Jochem Snuverink
// @Maintainer: Jochem Snuverink

#pragma once
#include <string>
#include <vector>


class TestData {
public:
    /************************************************************
    *                       functions
    ************************************************************/

    // Constructor
    TestData() = default;
    
    // Deconstructor
    ~TestData() = default;

    // Load data from a file
    // @param path to the file
    // @return 0 if operation successfull
    int load(std::string filename);

    // Set some noise in percent
    // @param percent of noise
    void set_noise(double);

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

    double m_setting;               // The new active value

    // arrays of data to estimate the value
    std::vector<double> m_activ;
    std::vector<double> m_passiv;

    double m_noise = 0.0;           // % of noise that is added
};
