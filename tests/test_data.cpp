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

#include <algorithm>
#include <fstream>
#include <iostream>

#include "test_data.h"


/************************************************************
*                       public
************************************************************/

// Load data from a file
int TestData::load(std::string filename) {
    std::ifstream file;
    file.open(filename,std::ios_base::in);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }
    int nrLines = std::count(std::istreambuf_iterator<char>(file),
                             std::istreambuf_iterator<char>(), '\n');
    file.close();

    m_activ .reserve(nrLines);
    m_passiv.reserve(nrLines);

    file.open(filename,std::ios_base::in);
    std::string dummy, activName, passivName;
    file >> dummy >> activName >> passivName;

    std::cout << "reading " << activName << " " << passivName << std::endl;

    double activValue, passivValue;
    int linenumber = 0;
    while (file.good()) {
        file >> activValue >> passivValue;
        m_activ .push_back( activValue);
        m_passiv.push_back(passivValue);
        linenumber++;
        if (linenumber > nrLines + 10) break;
    }

    file.close();
    return 0;
}

// Set some noise in percent
void TestData::set_noise(double noise) {
    m_noise = noise;
}

// Put the new active value
void TestData::put(double new_setting) {
    m_setting = new_setting;
}

// Get the caluclated prediction based on the prvious put
double TestData::get() {
    // find closest setting and interpolate
    // assume that activ are strictly ordered reversely(!)
    auto iter = std::lower_bound(m_activ.rbegin(),
                                 m_activ.rend(),
                                 m_setting);

    if (iter == m_activ.rend()) {
        return (*(m_passiv.rend() - 1));
    } else if (iter == m_activ.rbegin()) {
        return (*(m_passiv.rbegin()));
    } else {
        int index = (iter - m_activ.rbegin() - 2);
        double upper_bound = *iter--;
        double lower_bound = *iter;
        // std::cout << "lower value is " << lower_bound << " with index " <<   index   << std::endl;
        // std::cout << "upper value is " << upper_bound << " with index " << ++index << std::endl;

        double lower_passiv = *(m_passiv.rbegin() + index);
        double upper_passiv = *(m_passiv.rbegin() + index + 1);
        // std::cout << "lower passiv value is " << lower_passiv << std::endl;
        // std::cout << "upper passiv value is " << upper_passiv << std::endl;

        // interpolate
        double fraction = (m_setting - lower_bound) / (upper_bound - lower_bound);
        // std::cout << "fraction is " << fraction << std::endl;
        return (1 - fraction) * (lower_passiv) + fraction * upper_passiv;
    }
}
