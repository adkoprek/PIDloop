// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class is a widget and implements the ui from
// forms/settings.ui. It can load data from the Config
// struct and modify it.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#include <QPushButton>
#include <iostream>
#include <iterator>
#include <qabstractitemmodel.h>
#include <qchar.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qpainter.h>
#include <qpen.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qstyleoption.h>
#include <qtablewidget.h>
#include <qstyleditemdelegate.h>
#include <qtimer.h>
#include <qvalidator.h>
#include <sstream>
#include <string>
#include <strings.h>

#include "settings.h"
#include "config.h"
#include "data_fetch.h"
#include "device.h"
#include "pid_control.h"


// Internal helper functions only in this context
namespace  {

    // Convert double to string for Qt to display
    // @param double to convert
    // @return char* pointer to the static text
    const char* double_to_string(double input) {
        std::ostringstream strs;
        strs << input;
        static std::string standart_string = "";
        standart_string = strs.str();
        return standart_string.c_str();
    }

    // Class for a custom double validator
    class DoubleOrEmptyValidator : public QValidator {
    public:
        /************************************************************
        *                       functions
        ************************************************************/

        // Constructor
        // @param the Qt parent
        DoubleOrEmptyValidator(QObject *parent = nullptr) : QValidator(parent) {}

        // Return of new input is valid
        // @param the new input
        // @param the new position
        // @return Acceptable if input is valid otherwise Invalid
        State validate(QString &input, int &pos) const override {
            if (input.isEmpty()) return Acceptable;

            bool ok;
            input.toDouble(&ok);
            if (ok) return Acceptable;
            else return Invalid;
        }
    };

    // Class for a custom delegate to apply the DoubleOrEmptyValidator to a table column
    class DoubleDelegate : public QStyledItemDelegate {
    public:
        /************************************************************
        *                       functions
        ************************************************************/

        // Constructor
        // @param the Qt parent
        DoubleDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

        // Override function for the creation of the editor
        QWidget *createEditor(QWidget* parent, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override {
            // The editor is underneath a QLineEdit on which an Validator can be applied
            QLineEdit* editor = new QLineEdit(parent);
            auto validator = new DoubleOrEmptyValidator(editor);
            editor->setValidator(validator);
            return editor;
        }
    };
}

/************************************************************
*                       public
************************************************************/

// Constructor
Settings::Settings(Config* config, PIDControl* pid_control, QWidget* parent) {
    m_pid_control = pid_control;
    m_data_fetch = new DataFetch();
    m_config = config;
    setup_custom_ui();
}

// Deconstructor
Settings::~Settings() {
    delete m_data_fetch;
}

// Configure UI with new Config
void Settings::configure(Config* config) {
    m_config = config;

    m_ui.extern_setpoint->setText(m_config->extern_setpoint.c_str());
    if (m_config->extern_setpoint != "") m_ui.extern_setpoint_on->setCheckState(Qt::Checked);


    m_old_boundary = m_config->gain_boundary;

    //  Setup parameters with sliders
    m_ui.setpoint_slider->setRange(m_config->passiv.min, m_config->passiv.max);
    m_ui.boundary_slider->setRange(m_config->passiv.min, m_config->passiv.max);

    m_ui.setpoint->setRange(m_config->passiv.min, m_config->passiv.max);
    m_ui.boundary->setRange(m_config->passiv.min, m_config->passiv.max);

    m_ui.setpoint_slider->setValue(m_config->passiv.setpoint);
    m_ui.boundary_slider->setValue(m_old_boundary);

    m_ui.setpoint->setValue(m_config->passiv.setpoint);
    m_ui.boundary->setValue(m_old_boundary);

    m_ui.gain_above->setValue(m_config->gain_above_boundary);
    m_ui.gain_below->setValue(m_config->gain_below_boundary);
    m_ui.gain_above_slider->setValue(m_config->gain_above_boundary);
    m_ui.gain_below_slider->setValue(m_config->gain_below_boundary);

    // Setup just QSpinBoxes
    m_ui.i_param->setValue(m_config->i_param * 100);  // In .01s => * 100
    m_ui.d_param->setValue(m_config->d_param * 100);
    m_ui.rate->setValue(m_config->rate);

    // Create Formula
    m_ui.activ_param->setText(m_config->activ.name.c_str());
    m_ui.coefficient->setText(double_to_string(m_config->coefficient));
    m_ui.passiv_param->setText(m_config->passiv.name.c_str());

    m_parameter_update = true;
    update_table();
    m_parameter_update = false;
}

// Set new step size when clicking on boxes or sliders
void Settings::set_step_size(double step) {
    m_ui.setpoint->setSingleStep(step);
    m_ui.setpoint_slider->setPageStep(((int)step) == 0 ? 1 : step);
    m_ui.boundary->setSingleStep(step);
    m_ui.boundary_slider->setPageStep(((int)step) == 0 ? 1 : step);
    m_ui.gain_above->setSingleStep(((int)step) == 0 ? 1 : step);
    m_ui.gain_above_slider->setPageStep(((int)step) == 0 ? 1 : step);
    m_ui.gain_below->setSingleStep(((int)step) == 0 ? 1 : step);
    m_ui.gain_below_slider->setPageStep(((int)step) == 0 ? 1 : step);
}

// Updated with current data and write errors
void Settings::update_running_data() {
    if (m_config->use_extern_setpoint) {
        double extern_setpoint_value;
        int error = m_data_fetch->get_double(m_config->extern_setpoint, &extern_setpoint_value);
        if (error != 0) {
            m_pid_control->set_error("Failed to get pv from EPICS: " + m_config->extern_setpoint);
        }
        else {
            m_ui.setpoint->setValue(extern_setpoint_value);
            m_ui.setpoint_slider->setValue(extern_setpoint_value);
        }
    }

    State* state = m_pid_control->get_state();
    for (int i = 0; i < m_config->condition_devices.size(); i++) {
        double value_condition = state->condition_data[i];
        if (value_condition > m_config->condition_devices[i].max) {
            auto item = m_ui.params_table->item(2 + i, 2);
            if (m_ui.params_table->isPersistentEditorOpen(item)) return;
            item->setBackground(Qt::red);
        }
        else if (value_condition < m_config->condition_devices[i].min) {
            auto item = m_ui.params_table->item(2 + i, 1);
            if (m_ui.params_table->isPersistentEditorOpen(item)) return;
            item->setBackground(Qt::red);
        }
        else {
            auto item_1 = m_ui.params_table->item(2 + i, 1);
            auto item_2 = m_ui.params_table->item(2 + i, 2);
            if (m_ui.params_table->isPersistentEditorOpen(item_1)) return;
            if (m_ui.params_table->isPersistentEditorOpen(item_2)) return;
            reset_row_background(2 + i);
        }
    }
}

// Rest every condition device to a white background
void Settings::reset_condition_devices_color() {
    for (int i = 0; i < m_config->condition_devices.size(); i++)
        set_table_row_color(2 + i, Qt::black);
}

/************************************************************
*                       slots
************************************************************/

// Called when a cell in a table changes (with return pressed or clicked away)
void Settings::on_table_changed(int row, int column) {
    if (m_parameter_update) return;
    if (row < 2 && column == 0) return;

    auto table = m_ui.params_table;

    if (row == table->rowCount() - 1) {
        if (table->item(row, column)->text().isEmpty()) return;

        m_parameter_update = true;
        table->insertRow(m_ui.params_table->rowCount());
        if (column != 0) m_ui.params_table->setItem(row, 0, new QTableWidgetItem(""));
        if (column != 1) m_ui.params_table->setItem(row, 1, new QTableWidgetItem(""));
        if (column != 2) m_ui.params_table->setItem(row, 2, new QTableWidgetItem(""));
        auto item = new QTableWidgetItem("------");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_ui.params_table->setItem(row + 1, 3, item);
        m_parameter_update = false;

        m_config->condition_devices.push_back(Device());
    }

    if (table->item(row, 0)->text().isEmpty() && table->item(row, 1)->text().isEmpty() &&
        table->item(row, 2)->text().isEmpty() && row > 1) {
        table->removeRow(row);
        m_config->condition_devices.erase(std::next(m_config->condition_devices.begin(), row));
        return;
    }

    fetch_table_data(row, column);
}

// Disable or enable boundary
void Settings::change_boundary_state(bool new_state) {
    if (new_state) {
        m_ui.boundry_label->show();
        m_ui.boundary->show();
        m_ui.boundary_slider->show(); 
        m_config->gain_boundary = m_old_boundary;

        m_ui.gain_below_label->show();
        m_ui.gain_below->show();
        m_ui.gain_below_slider->show();

        m_ui.gain_above_label->setText("Gain above boundary");
    }
    else {
        m_ui.boundry_label->hide();
        m_ui.boundary->hide();
        m_ui.boundary_slider->hide(); 
        m_old_boundary = m_config->gain_boundary;
        m_config->gain_boundary = -1e10;

        m_ui.gain_below_label->hide();
        m_ui.gain_below->hide();
        m_ui.gain_below_slider->hide();

        m_ui.gain_above_label->setText("Gain");
    }
}

/************************************************************
*                       private
************************************************************/

// Set up the ui with the data
void Settings::setup_custom_ui() {
    m_ui.setupUi(this);

    update_table();
    create_connections();


    DoubleDelegate* double_delegate = new DoubleDelegate(m_ui.params_table);
    m_ui.params_table->setItemDelegateForColumn(1, double_delegate);
    m_ui.params_table->setItemDelegateForColumn(2, double_delegate);
    m_ui.params_table->setItemDelegateForColumn(3, double_delegate);

    auto validator = new DoubleOrEmptyValidator();
    m_ui.coefficient->setValidator(validator);

    m_ui.gain_below->setRange(0, 200);
    m_ui.gain_below_slider->setRange(0, 200);
    m_ui.gain_above->setRange(0, 200);
    m_ui.gain_above_slider->setRange(0, 200);
    m_ui.i_param->setRange(0, 300);
    m_ui.d_param->setRange(0, 300);
    m_ui.rate->setRange(1, 10);
}

// Connect everything in the ui
void Settings::create_connections() {
    connect_sliders_with_boxes();
    connect(m_ui.activ_param,                       &QLineEdit::editingFinished,    [this]() { 
        m_config->activ.name = m_ui.activ_param->text().toStdString(); 
        m_parameter_update = true;
        update_table();
        m_parameter_update = false;
    });
    connect(m_ui.passiv_param,                      &QLineEdit::editingFinished,    [this]() { 
        m_config->passiv.name = m_ui.passiv_param->text().toStdString();
        m_parameter_update = true;
        update_table();
        m_parameter_update = false;
    });
    connect(m_ui.i_param->findChild<QLineEdit*>(),  &QLineEdit::editingFinished,    [this]() {
        m_config->i_param = m_ui.i_param->value() / 100.0;
    });
    connect(m_ui.i_param->findChild<QLineEdit*>(),  &QLineEdit::returnPressed,      [this]() {
        m_config->i_param = m_ui.i_param->value() / 100.0;
    });
    connect(m_ui.d_param->findChild<QLineEdit*>(),  &QLineEdit::editingFinished,    [this]() {
        m_config->d_param = m_ui.d_param->value() / 100.0;
    });
    connect(m_ui.d_param->findChild<QLineEdit*>(),  &QLineEdit::returnPressed,      [this]() {
        m_config->d_param = m_ui.d_param->value() / 100.0;
    });
    connect(m_ui.rate->findChild<QLineEdit*>(),     &QLineEdit::editingFinished,    [this]() {
        m_config->rate = m_ui.rate->value();
    });
    connect(m_ui.rate->findChild<QLineEdit*>(),     &QLineEdit::returnPressed,      [this]() {
        m_config->rate = m_ui.rate->value();
    });
    connect(m_ui.coefficient,                       &QLineEdit::editingFinished,    [this]() {
        m_config->coefficient = m_ui.coefficient->text().toDouble();
    });
    connect(m_ui.extern_setpoint_on,                &QCheckBox::stateChanged,       [this](){
        m_config->use_extern_setpoint = m_ui.extern_setpoint_on->checkState();
    });
    connect(m_ui.extern_setpoint,                   &QLineEdit::editingFinished,    [this](){
        m_config->extern_setpoint = m_ui.extern_setpoint->text().toStdString();
    });

    connect(m_ui.params_table, &QTableWidget::cellChanged, this, &Settings::on_table_changed);
}

// Connect all the boxes and sliders to be linked
void Settings::connect_sliders_with_boxes() {
    connect(m_ui.setpoint->findChild<QLineEdit*>(), &QLineEdit::editingFinished, [this]() {
        m_ui.setpoint_slider->setValue(m_ui.setpoint->value());
        m_config->activ.setpoint = m_ui.setpoint->value();
    });
    connect(m_ui.setpoint->findChild<QLineEdit*>(), &QLineEdit::returnPressed,   [this]() {
        m_ui.setpoint_slider->setValue(m_ui.setpoint->value());
        m_config->activ.setpoint = m_ui.setpoint->value();
    });
    connect(m_ui.setpoint_slider,                   &QSlider::valueChanged,      [this](int value) {
        m_ui.setpoint->setValue(value);
        m_config->activ.setpoint = value;
    });

    connect(m_ui.boundary->findChild<QLineEdit*>(), &QLineEdit::editingFinished, [this]() {
        m_ui.boundary_slider->setValue(m_ui.boundary->value());
        m_config->gain_boundary = m_ui.boundary->value();
    });
    connect(m_ui.boundary->findChild<QLineEdit*>(), &QLineEdit::returnPressed,   [this]() {
        m_ui.boundary_slider->setValue(m_ui.boundary->value());
        m_config->gain_boundary = m_ui.boundary->value();
    });
    connect(m_ui.boundary_slider,                   &QSlider::valueChanged,      [this](int value) {
        m_ui.boundary->setValue(value);
        m_config->gain_boundary = value;
    });

    connect(m_ui.gain_below->findChild<QLineEdit*>(), &QLineEdit::editingFinished, [this]() {
        m_ui.gain_below_slider->setValue(m_ui.gain_below->value());
        m_config->gain_below_boundary = m_ui.gain_below->value();
    });
    connect(m_ui.gain_below->findChild<QLineEdit*>(), &QLineEdit::returnPressed,   [this]() {
        m_ui.gain_below_slider->setValue(m_ui.gain_below->value());
        m_config->gain_below_boundary = m_ui.gain_below->value();
    });
    connect(m_ui.gain_below_slider,                   &QSlider::valueChanged,      [this](int value) {
        m_ui.gain_below_slider->setValue(value);
        m_config->gain_below_boundary = value;
    });

    connect(m_ui.gain_above->findChild<QLineEdit*>(), &QLineEdit::editingFinished, [this]() {
        m_ui.gain_above_slider->setValue(m_ui.gain_above->value());
        m_config->gain_above_boundary = m_ui.gain_above->value();
    });
    connect(m_ui.gain_above->findChild<QLineEdit*>(), &QLineEdit::returnPressed,   [this]() {
        m_ui.gain_above_slider->setValue(m_ui.gain_above->value());
        m_config->gain_above_boundary = m_ui.gain_above->value();
    });
    connect(m_ui.gain_above_slider,                   &QSlider::valueChanged,      [this](int value) {
        m_ui.gain_above->setValue(value);
        m_config->gain_above_boundary = value;
    });
}

// Updated table data when new config arives
void Settings::update_table() {
    m_ui.params_table->setRowCount(3 + m_config->condition_devices.size());

    // Set the activ device
    Device* device = &m_config->activ; 
    auto item = new QTableWidgetItem(device->name.c_str());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_ui.params_table->setItem(0, 0, item);
    if (device->min != -1e+10)
         m_ui.params_table->setItem(0, 1, new QTableWidgetItem(double_to_string(device->min)));
    else m_ui.params_table->setItem(0, 1, new QTableWidgetItem(""));
    if (device->max != 1e+10)
         m_ui.params_table->setItem(0, 2, new QTableWidgetItem(double_to_string(device->max)));
    else m_ui.params_table->setItem(0, 2, new QTableWidgetItem(""));
    if (device->max != 1e+10)
         m_ui.params_table->setItem(0, 3, new QTableWidgetItem(double_to_string(device->hold_value)));
    else m_ui.params_table->setItem(0, 3, new QTableWidgetItem(""));

    // Set the passiv device
    device = &m_config->passiv;
    item = new QTableWidgetItem(device->name.c_str());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_ui.params_table->setItem(1, 0, item);
    if (device->min != -1e+10)
         m_ui.params_table->setItem(1, 1, new QTableWidgetItem(double_to_string(device->min)));
    else m_ui.params_table->setItem(1, 1, new QTableWidgetItem(""));
    if (device->max != 1e+10)
         m_ui.params_table->setItem(1, 2, new QTableWidgetItem(double_to_string(device->max)));
    else m_ui.params_table->setItem(1, 2, new QTableWidgetItem(""));
    item = new QTableWidgetItem("------");
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_ui.params_table->setItem(1, 3, item);
    set_table_row_color(1, Qt::magenta);

    // Set the condition devices
    for (int i = 0; i < m_config->condition_devices.size(); i++) {
        device = &m_config->condition_devices[i];
        m_ui.params_table->setItem(2 + i, 0, new QTableWidgetItem(device->name.c_str()));
        if (device->min != -1e+10)
             m_ui.params_table->setItem(2 + i, 1, new QTableWidgetItem(double_to_string(device->min)));
        else m_ui.params_table->setItem(2 + i, 1, new QTableWidgetItem(""));
        if (device->max != 1e+10)
             m_ui.params_table->setItem(2 + i, 2, new QTableWidgetItem(double_to_string(device->max)));
        else m_ui.params_table->setItem(2 + i, 2, new QTableWidgetItem(""));
        item = new QTableWidgetItem("------");
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        m_ui.params_table->setItem(2 + i, 3, item);
    }

    // Set the hold value for new row
    item = new QTableWidgetItem("------");
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_ui.params_table->setItem(m_ui.params_table->rowCount() - 1, 3, item);
}

// Set foreground color for an entire table row
void Settings::set_table_row_color(int row, Qt::GlobalColor color) {
    m_ui.params_table->item(row, 0)->setForeground(color);
    m_ui.params_table->item(row, 1)->setForeground(color);
    m_ui.params_table->item(row, 2)->setForeground(color);
}

// Reset the backgroudn of a given row (to white)
void Settings::reset_row_background(int row) {
    m_ui.params_table->item(row, 0)->setBackground(Qt::white);
    m_ui.params_table->item(row, 1)->setBackground(Qt::white);
    m_ui.params_table->item(row, 2)->setBackground(Qt::white);
}

// Fetch data from passed field index and assign it correctly to the config
void Settings::fetch_table_data(int row, int column) {
    auto table = m_ui.params_table;

    if (row == 0 && column == 0) 
        m_config->activ.name = table->item(row, column)->text().toStdString();
    else if (row == 1 && column == 0) 
        m_config->passiv.name = table->item(row, column)->text().toStdString();
    else if (row == 0 && column == 1) 
        m_config->activ.min = table->item(row, column)->text().toDouble();
    else if (row == 1 && column == 1) {
        double new_min = table->item(row, column)->text().toDouble();
        m_config->passiv.min = new_min;
        m_ui.setpoint->setRange(new_min, m_ui.setpoint->maximum());
        m_ui.boundary->setRange(new_min, m_ui.boundary->maximum());
        m_ui.setpoint_slider->setRange(new_min, m_ui.setpoint_slider->maximum());
        m_ui.boundary_slider->setRange(new_min, m_ui.boundary_slider->maximum());
    }
    else if (row == 0 && column == 2) 
        m_config->activ.max = table->item(row, column)->text().toDouble();
    else if (row == 1 && column == 2) {
        double new_max = table->item(row, column)->text().toDouble(); 
        m_config->passiv.max = new_max;
        m_ui.setpoint->setRange(m_ui.setpoint->minimum(), new_max);
        m_ui.boundary->setRange(m_ui.boundary->minimum(), new_max);
        m_ui.setpoint_slider->setRange(m_ui.setpoint_slider->minimum(), new_max);
        m_ui.boundary_slider->setRange(m_ui.boundary_slider->minimum(), new_max);
    }
    else if (row == 0 && column == 3) 
        m_config->activ.hold_value = table->item(row, column)->text().toDouble();
    else if (row > 1) {
        if (column == 0) 
            m_config->condition_devices[row - 2].name = table->item(row, column)->text().toStdString();
        else if (column == 1)
            m_config->condition_devices[row - 2].min = table->item(row, column)->text().toDouble();
        else if (column == 2)
            m_config->condition_devices[row - 2].max = table->item(row, column)->text().toDouble();
    }
}
