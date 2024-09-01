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

#include <chrono>
#include <limits>
#include <thread>

#include "pid_control.h"
#include "data_fetch.h"
#include "state.h"

// Define this macro if you want to use this application with a simulation
// #define TEST


/************************************************************
*                       public
************************************************************/

// Constructor 
PIDControl::PIDControl() {
    m_data_fetch = new DataFetch();
#ifdef TEST
    m_data_calc = new DataCalc();
    m_data_calc->load("../../../test_data/kip2-mxc1-param.txt");
    m_data_calc->set_noise(0.005);
#endif // DEBUG
}

// Deconstructor
PIDControl::~PIDControl() {
    delete m_data_fetch;
    delete m_state;
}

// Setout the control with a new configuration
void PIDControl::setup(Config* config) {
    m_config = config; 

    delete m_state;
    m_state = new State();
    for (int i = 0; i < 499; i++) {
        m_state->activ_data.push_back(std::numeric_limits<double>::quiet_NaN());
        m_state->passiv_data.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    m_state->activ_data.push_back(0);
    m_state->passiv_data.push_back(0);

    for (int i = 0; i < config->condition_devices.size(); i++)
        m_state->condition_data.push_back(0);
}

// Start the calculations
void PIDControl::start() {
    m_stop_flag = false;
    m_state->error = {0, 0, 0};

#ifndef TEST
    int error = m_data_fetch->get_double(m_config->activ.name, &m_state->current_value);
    if (error != 0) m_error_message = "Failed to get pv from EPICS: " + m_config->activ.name;
#endif // !TEST // !TEST
#ifdef TEST
    m_state->current_value = 414.172;
#endif // !TEST // !TEST

    while (!m_stop_flag) {
        int time_milliseconds = 1.0 / m_config->rate * 1000;
        auto start = std::chrono::high_resolution_clock::now();
        auto target = start + std::chrono::milliseconds(time_milliseconds);
        
        calc_new_activ();
        get_passiv_parameter();
        m_out_of_bounds = check_condition_devices();

        m_state->counter++;
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (duration > time_milliseconds) {
            m_state->actual_rate = 1000 / duration;
            continue;
        } 
        else {
            m_state->actual_rate = m_config->rate;
            std::this_thread::sleep_for(std::chrono::milliseconds(time_milliseconds - duration));
        }
    }

    handle_hold();
}

// Stop the clculations
void PIDControl::stop() { m_stop_flag = true; }

// Get the latest error
std::string PIDControl::get_latest_error() { return m_error_message; }

// Set an error message externally
void PIDControl::set_error(std::string error) { m_error_message = error; }

// Get the current pointer to the State struct
State* PIDControl::get_state() { return m_state; }

// Check if a condition device is out of bounds
bool PIDControl::is_out_of_bounds() { return m_out_of_bounds; }

/************************************************************
*                       private
************************************************************/

// Caclulcate the actual new activ value
void PIDControl::calc_new_activ() {
    if (!m_out_of_bounds) {
        double new_value = calc_pid();
        double clip = (m_config->activ.max - m_config->activ.min) * 0.03;
        if      (new_value > clip) new_value =  clip;
        else if (new_value < -clip) new_value = -clip;
        m_state->current_value += new_value;


        if      (m_state->current_value > m_config->activ.max) 
            m_state->current_value = m_config->activ.max;
        else if (m_state->current_value < m_config->activ.min)
            m_state->current_value = m_config->activ.min;

#ifndef TEST
        int error = m_data_fetch->put_double(m_config->activ.name, m_state->current_value);
        if (error != 0) {
            m_error_message = "Failed to write pv on EPICS: " + m_config->activ.name;
            error = m_data_fetch->get_double(m_config->activ.name, &m_state->current_value);
            if (error != 0) m_error_message = "Failed to get pv from EPICS: " + m_config->activ.name;
        }
#endif // !TEST
#ifdef TEST
        m_data_calc->put(m_state->current_value); 
#endif // TEST
    }
    else {
#ifndef TEST
        int error = m_data_fetch->get_double(m_config->activ.name, &m_state->current_value);
        if (error != 0) m_error_message = "Failed to get pv from EPICS: " + m_config->activ.name;
#endif // !TEST
    }

    m_state->activ_data.erase(m_state->activ_data.begin());
    m_state->activ_data.push_back(m_state->current_value);
}

// Calculate the actuall PID
double PIDControl::calc_pid() {
    double gain;
    double current_passiv = m_state->passiv_data.back();
    if (current_passiv <= m_config->gain_boundary) gain = m_config->gain_below_boundary;
    else                                           gain = m_config->gain_above_boundary;

    // This lowers the gain in the beginning
    double k_p;
    if (m_state->counter <= 20)
        k_p = (gain * (m_state->counter + 5) / 25) / 100;
    else
        k_p = gain / 100;

    // This is the dynamic gain option that multiplies the k_p parameter with an 
    // linear function when the current passive value is between 70 - 97% to the 
    // setpoint. This supposed to optimize the increasing of the beam intensity
    // afer a UCN kick
    if (m_config->dynamic_gain) {
        double setpoint = m_config->activ.setpoint;
        if (current_passiv > (0.7 * setpoint) && current_passiv < (0.97 * setpoint)) {
            double percentage = current_passiv / setpoint;
            k_p *= 16 * percentage - 10.4;
        }
    }

    // Calculate the new error
    double latest_error = m_config->activ.setpoint - m_state->passiv_data.back();
    m_state->error.erase(m_state->error.begin());
    m_state->error.push_back(latest_error);
    auto e = m_state->error;

    // For this calculation refer to the article in docs/pid_calc.md
    double k_i = k_p / m_config->i_param;
    double setpoint = m_config->activ.setpoint;
    double k_d = k_p * m_config->d_param;
    double d_t = 1.0 / m_config->rate;

    double offset_e2 = (k_p + k_d / (10 * d_t)) * e[2];
    double offset_e1 = (-k_p + k_i * d_t - (2 * k_d) / (10 * d_t)) * e[1];
    double offset_e0 = k_d / (10 * d_t) * e[0];
    double offset = m_config->coefficient * (offset_e2 + offset_e1 + offset_e0);

    return offset;
}

// Fetch the passiv parameter from EPICS
void PIDControl::get_passiv_parameter() {
    double value_passiv;
#ifndef TEST
    int error = m_data_fetch->get_double(m_config->passiv.name, &value_passiv);
    if (error != 0) m_error_message = "Failed to get pv from EPICS: " + m_config->passiv.name;
#endif // !TEST
#ifdef TEST
    value_passiv = m_data_calc->get();
#endif // !TEST

    m_state->passiv_data.erase(m_state->passiv_data.begin());
    m_state->passiv_data.push_back(value_passiv);
}

// Check every condition device if it is out of bounds
int PIDControl::check_condition_devices() {
    m_state->condition_data.clear();
    for (int i = 0; i < m_config->condition_devices.size(); i++) {
        double value_condition;
        int error = m_data_fetch->get_double(m_config->condition_devices[i].name, &value_condition);
        if (error != 0) {
            m_error_message = "Failed to get pv from EPICS: " + m_config->condition_devices[i].name;
            continue;
        }

        m_state->condition_data.push_back(value_condition);

        // Check if in bounds
        if (value_condition > m_config->condition_devices[i].max ||
            value_condition < m_config->condition_devices[i].min) {
            return -1;
        }
    }

    return 0;
}

// Handles any kind of holding
void PIDControl::handle_hold() {
    if (m_config->activ.hold_value > m_config->activ.max || m_config->activ.hold_value < m_config->activ.min) return;

    int error = m_data_fetch->put_double(m_config->activ.name, m_config->activ.hold_value);
    if (error != 0)
        m_error_message = "Failed to write hold value to pv on EPICS: " + m_config->activ.name;
}
