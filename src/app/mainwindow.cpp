// _____ _____ _____  _                       
// |  __ \_   _|  __ \| |                      
// | |__) || | | |  | | |     ___   ___  _ __  
// |  ___/ | | | |  | | |    / _ \ / _ \|  _ \
// | |    _| |_| |__| | |___| (_) | (_) | |_) |
// |_|   |_____|_____/|______\___/ \___/| .__/ 
// https://git.psi.ch/hipa_apps/pidloop |_|    
//                                      
// This class implements QMainWindow and the ui
// defined in forms/mainwindow.ui. It is responsible
// for reading and writing configuration. Lock file 
// managment is inspired by @Jochem Snuverink.
//
// @Author: Adam Koprek
// @Maintainer: Jochem Snuverink

#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <cmath>
#include <cstdlib>
#include <qchar.h>
#include <qcoreevent.h>
#include <qcursor.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <QScreen>
#include <qwidget.h>
#include <string>
#include <thread>
#include <QCloseEvent>
#include <QTimer>

#include "mainwindow.h"
#include "config.h"
#include "config_parser.h"
#include "pid_control.h"
#include "real_time_plot.h"
#include "settings.h"


/************************************************************
*                       public
************************************************************/

// Constructor
MainWindow::MainWindow(QWidget* parent) {
    m_config_parser = new ConfigParser();
    m_config = new Config();
    m_pid_control = new PIDControl();
    m_real_time_plot = new RealTimePlot(m_pid_control);
    setup_custom_ui();
}

// Destructor
MainWindow::~MainWindow() {
    release_lock();
    delete m_config_parser;
    delete m_config;
}

/************************************************************
*                       slots
************************************************************/

// Called when regulate button is clicked
void MainWindow::on_regulate_clicked() {
    if (m_running) return;
    if (m_config->activ.name == "") return show_dialog("Give an an active parameter");
    if (m_config->passiv.name == "") return show_dialog("Give an an passiv parameter");

    m_running = true;
    m_ui.regulate_button->setStyleSheet("background-color: green;");
    m_ui.hold_button->setStyleSheet("");
    m_timer->start(1000 / m_config->rate);

    // If new file gets loaded reset the plot
    if (m_new_file) {
        m_pid_control->setup(m_config);
        m_work_thread = new std::thread(&PIDControl::start, m_pid_control);
        m_real_time_plot->start(m_config);
    }
    else {
        m_work_thread = new std::thread(&PIDControl::start, m_pid_control);
        m_real_time_plot->resume();
    }
}

// Called when hold button is clicked
void MainWindow::on_hold_clicked() {
    if (!m_running) return;
    m_timer->stop();
    m_running = false;
    m_ui.regulate_button->setStyleSheet("");
    m_ui.hold_button->setStyleSheet("background-color: red;");
    m_new_file = false;
    m_pid_control->stop();
    m_work_thread->join();
    delete m_work_thread;
    m_work_thread = nullptr;
    m_real_time_plot->stop();
    m_settings->reset_condition_devices_color();
}

// Called when clear button is clicked
void MainWindow::on_clear_clicked() {
    QMessageBox* message_box = new QMessageBox();
    message_box->setWindowTitle("Are you sure?");
    message_box->setText("This action will clear all the fields and stop the simulation");
    message_box->setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    message_box->setDefaultButton(QMessageBox::No);
    int result = message_box->exec();
    if (result == QMessageBox::No) return;
    on_hold_clicked();
    release_lock();
    m_ui.main_layout->removeWidget(m_settings);
    m_ui.main_layout->removeWidget(m_real_time_plot);
    delete m_settings;
    delete m_real_time_plot;
    delete m_config;
    m_config = new Config();
    m_config->dynamic_gain = m_ui.dynamic_gain_action->isChecked();
    m_settings = new Settings(m_config, m_pid_control);
    m_settings->change_boundary_state(m_ui.dynamic_gain_action->isChecked());
    m_real_time_plot = new RealTimePlot(m_pid_control);
    if (m_ui.minimize_button->text() == "Maximize") m_settings->hide();
    m_ui.main_layout->insertWidget(2, m_settings);
    m_ui.main_layout->insertWidget(4, m_real_time_plot);
    m_ui.dynamic_gain_action->setChecked(true);
    m_settings->change_boundary_state(true);
}

// Called when minimize button is clicked
void MainWindow::on_minimize_clikced() {
    if (m_ui.minimize_button->text() == "Minimize") {
        m_ui.minimize_button->setText("Maximize");
        m_settings->hide();
        m_old_geometry = this->geometry();

        // Set height to 1/3 of monitor height
        QScreen* screen = QGuiApplication::screenAt(this->geometry().center());
        int new_height = screen->geometry().height() * 0.3;
        this->setFixedHeight(new_height);
    }

    else {
        this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        this->setMinimumSize(0, 0);
        m_ui.minimize_button->setText("Minimize");
        m_settings->show();
        this->setGeometry(m_old_geometry);
    }
}

// Called when the read config action is clicked
void MainWindow::on_read_config_clicked() {
    std::string directory = getenv("APPDATA");
    QString file_path = QFileDialog::getOpenFileName(
            this, "Open Configuration File", directory.c_str(), "*.reg");

    if (file_path.isEmpty()) return;

    std::string lock_path = file_path.toStdString() + ".lock"; 
    if (lock_path == m_last_lock) release_lock();
    std::string lock_message = check_for_lock(lock_path);
    int create_lock_error = 0;
    if (lock_message != "") {
        show_dialog("There is an lock file preset, that prevents the opening of the configuration\nThe Path is: " + 
                    lock_path + "\nOn the host: " + lock_message); 
    }
    else {
        create_lock_error = create_lock(lock_path);
        if (create_lock_error != 0 && create_lock_error != 1) 
            show_dialog("Couldn't create lock file because the current user doesn't have permission to write the lock file. You're on your own.\nThe Path is: " +
                         lock_path);
    }

    on_hold_clicked();
    delete m_real_time_plot;
    m_real_time_plot = new RealTimePlot(m_pid_control);
    m_ui.main_layout->insertWidget(3, m_real_time_plot);
    m_new_file = true;
    delete m_config;
    m_config = new Config;
    int return_code = m_config_parser->load_config(file_path.toStdString());
    if (return_code != 0) {
        show_dialog("The file couldn't be opened");
        return;
    }
    return_code = m_config_parser->parse_config(m_config);
    if (return_code == 0) m_settings->configure(m_config);
    else if (return_code == -1) show_dialog("No <pidControl> nor <PIDLoop> root tag found");
    else if (return_code == -2) show_dialog("The active device couldn't be parsed");
    else if (return_code == -3) show_dialog("The passiv device couldn't be parsed");
    else if (return_code == -4) show_dialog("The PID-Parameters couldn't be parsed");
    else if (return_code == -5) show_dialog("No <Matrix> nor <Params> found");
    else if (return_code == -6) show_dialog("The params couldn't be parsed");
    else if (return_code == -7) show_dialog("The Matrix couldn't be parsed");
    else if (return_code == -8) show_dialog("One of the condition devices couldn't be parsed");
    release_lock();
    m_ui.dynamic_gain_action->setChecked(m_config->dynamic_gain);
    if (!create_lock_error) m_last_lock = lock_path;
    std::string file_name = file_path.replace((directory + "/").c_str(), "").toStdString();
    this->setWindowTitle((std::string("PIDLoop - ") + file_name).c_str());
}

// Called when the save config action is clicked
void MainWindow::on_save_config_clikced() {
    std::string directory = getenv("APPDATA");
    QString file_path = QFileDialog::getSaveFileName(
            this, "Open Configuration File", 
            (directory + std::string("/untitled.reg")).c_str(), 
            "*.reg");

    if (file_path.isEmpty()) return;
    m_config_parser->dump(m_config);
    int return_code = m_config_parser->save_config(file_path.toStdString());
    if (return_code != 0) {
        show_dialog("The file couldn't be opened");
        return;
    }
    std::string file_name = file_path.replace(directory.c_str(), "").toStdString();
    this->setWindowTitle((std::string("PIDLoop - ") + file_name).c_str());
}

// Called when the stepsize changes
void MainWindow::on_step_chosen(double step) {
    if (step != 100) m_ui.step_100->setChecked(false);
    if (step != 50)  m_ui.step_50->setChecked(false);
    if (step != 10)  m_ui.step_10->setChecked(false);
    if (step != 1)   m_ui.step_1->setChecked(false);
    if (step != 0.5) m_ui.step_05->setChecked(false);
    if (step != 0.1) m_ui.step_01->setChecked(false);
    m_settings->set_step_size(step);
}

/************************************************************
*                       private
************************************************************/

// Function that setups the ui
void MainWindow::setup_custom_ui() {
    m_ui.setupUi(this);
    m_timer = new QTimer(this);
    m_settings = new Settings(m_config, m_pid_control);
    m_ui.main_layout->insertWidget(2, m_settings);
    m_ui.main_layout->insertWidget(4, m_real_time_plot);

    m_ui.boundary_action->setChecked(true);

    connect(m_ui.regulate_button,     &QPushButton::clicked, this,       &MainWindow::on_regulate_clicked   );
    connect(m_ui.hold_button,         &QPushButton::clicked, this,       &MainWindow::on_hold_clicked       );
    connect(m_ui.clear_button,        &QPushButton::clicked, this,       &MainWindow::on_clear_clicked      );
    connect(m_ui.minimize_button,     &QPushButton::clicked, this,       &MainWindow::on_minimize_clikced   );

    connect(m_ui.open_config_action,  &QAction::triggered,   this,       &MainWindow::on_read_config_clicked);
    connect(m_ui.save_config_action,  &QAction::triggered,   this,       &MainWindow::on_save_config_clikced);
    connect(m_ui.boundary_action,     &QAction::triggered,   [this]()    {
        m_settings->change_boundary_state(m_ui.boundary_action->isChecked());
    });

    connect(m_ui.step_100,            &QAction::triggered,   [this]()    { on_step_chosen(100); });
    connect(m_ui.step_50,             &QAction::triggered,   [this]()    { on_step_chosen(50.); });
    connect(m_ui.step_10,             &QAction::triggered,   [this]()    { on_step_chosen(10.); });
    connect(m_ui.step_5,              &QAction::triggered,   [this]()    { on_step_chosen(5.0); });
    connect(m_ui.step_1,              &QAction::triggered,   [this]()    { on_step_chosen(1.0); });
    connect(m_ui.step_05,             &QAction::triggered,   [this]()    { on_step_chosen(0.5); });
    connect(m_ui.step_01,             &QAction::triggered,   [this]()    { on_step_chosen(0.1); });

    connect(m_timer,                  &QTimer::timeout,      this,       &MainWindow::update_ui);

    connect(m_ui.dynamic_gain_action, &QAction::triggered,   [this]()    { m_config->dynamic_gain = !m_config->dynamic_gain; });
}

// Show a generic error message just with an ok button
void MainWindow::show_dialog(std::string message) {
    QMessageBox* message_box = new QMessageBox();
    message_box->setWindowTitle("Error");
    message_box->setText(QString(message.c_str()));
    message_box->setStandardButtons(QMessageBox::Ok);
    message_box->exec();
}

// Update ui when new data arrives from the logic
void MainWindow::update_ui() {
    m_settings->update_running_data();
    if (m_pid_control->is_out_of_bounds()) 
        m_ui.regulate_button->setStyleSheet("background-color: orange;");
    else
        m_ui.regulate_button->setStyleSheet("background-color: green;");
    m_timer->stop();
    m_timer->start(1000 / m_config->rate);
}

// Check if the given lockfile exists
std::string MainWindow::check_for_lock(std::string path) {
    char hostname_char[HOST_NAME_MAX];
    gethostname(hostname_char, HOST_NAME_MAX);
    QString hostname(hostname_char); 
    if (hostname.startsWith("hipaw") && !hostname.startsWith("prow")) return "";

    if (QFile::exists(path.c_str())) {
        QFile file(path.c_str());
        file.open(QIODevice::ReadOnly);
        QString read_hostname = QString(file.readAll()).trimmed();
        return read_hostname.toStdString();
    }

    return "";
}

// Create lock file
int MainWindow::create_lock(std::string path) {
    char hostname_char[HOST_NAME_MAX];
    gethostname(hostname_char, HOST_NAME_MAX);
    QString hostname(hostname_char); 
    if (!hostname.startsWith("hipaw") && !hostname.startsWith("prow")) return 1;

    QFile file(path.c_str());
    if (!file.open(QIODevice::WriteOnly)) return -1;
    file.write(hostname.toLatin1());

    return 0;
}
 
// Release last lock file
int MainWindow::release_lock() {
    if (m_last_lock == "") return 0;
    QFile file(m_last_lock.c_str());
    if (!file.remove()) {
        file.open(QIODevice::ReadOnly);
        QString read_hostname = QString(file.readAll()).trimmed();
        show_dialog("The old lock file couldn't be deleted please delete it manually.\nFile Path" +
                    m_last_lock + "\nOn the host: " + read_hostname.toStdString());
        m_last_lock = "";
        return -1;
    }
    m_last_lock = "";
    return 0;
}
