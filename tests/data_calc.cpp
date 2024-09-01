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

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "data_calc.h"


/************************************************************
*                       public
************************************************************/

// Load the coefficient from a file
int DataCalc::load(std::string filename) {
    std::ifstream file;
    file.open(filename,std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        file.close();
        return 1;
    }

    file >> m_a;
    file >> m_b;
    file >> m_c;
    file >> m_d;

    std::cout << "Thre read coefficient were:" << std::endl; 
    std::cout << "a: " << m_a << std::endl;
    std::cout << "b: " << m_b << std::endl;
    std::cout << "c: " << m_c << std::endl;
    std::cout << "d: " << m_d << std::endl;

    file.close();
    return 0;
}

// Set some noise in percent
void DataCalc::set_noise(double new_noise) { 
    m_noise = new_noise;
}

// Put the new active value
void DataCalc::put(double new_setting) {
    m_setting = new_setting;
}

// Get the caluclated prediction based on the prvious put
double DataCalc::get() {
    double value = m_a * pow(m_setting, 3) + m_b * pow(m_setting, 2) + m_c * m_setting + m_d;

    if (m_noise) {
        int p_half = value * 0.01;
        if (p_half == 0) p_half = 1;
        char rand_char = rand() % (2 * p_half);
        int noise = rand_char - p_half;
        value += noise;
    }

    return value;
}
