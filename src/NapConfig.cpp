/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    NapConfig.cpp
 * @brief   This file has definitions of the config window class
 *          for the plugin as well as the config singleton object.
*/

// C
#include <stdint.h>

// C++
#include <limits>

// KernelShark
#include "libkshark.h"
#include "KsPlotTools.hpp"

// Plugin
#include "NapConfig.hpp"

// Configuration object functions

/**
 * @brief Get the configuration object as a read-only reference.
 * Utilizes Meyers singleton creation (static local variable), which
 * also ensures the same address when taking a pointer.
 * 
 * @note Meyers singleton is not quite destruction-safe, as it's
 * destroyed during atexit() call (destructor calling it after it
 * was destroyed would be problematic). But this plugin doesn't
 * have this issue, so no need to worry about that.
 * 
 * @returns Const reference to the configuration object.
 */
NapConfig& NapConfig::get_instance() {
    static NapConfig instance;
    return instance;
}

/**
 * @brief Gets the current limit of entries in the histogram.
 * 
 * @returns Limit of entries in the histogram.
 */
int32_t NapConfig::get_histo_limit() const
{ return _histo_entries_limit; }

/**
 * @brief Gets whether to use task-like outline coloring for nap rectangles.
 * 
 * @return Whether to use task-like outline coloring for nap rectangles. 
 */
bool NapConfig::get_use_task_coloring() const
{ return _use_task_coloring; }

// Window

// Member functons

/**
 * @brief Constructor for the configuration window.
 * 
 * @note Due to its nature and setup functions, this function
 * is also dependent on the configuration 'NapConfig' singleton.
*/
NapConfigWindow::NapConfigWindow()
    : QWidget(NapConfig::main_w_ptr),
    _histo_label("Entries on histogram until nap rectangles appear: "),
    _histo_limit(this),
    _task_col_label("Use task coloring: "),
    _task_col_btn(this),
    _close_button("Close", this),
    _apply_button("Apply", this)
{
    setWindowTitle("Naps Plugin Configuration");
    // Set window flags to make header buttons
    setWindowFlags(Qt::Dialog | Qt::WindowMinimizeButtonHint
                   | Qt::WindowCloseButtonHint);
    setMaximumHeight(300);

    setup_histo_section();
    setup_tasklike_coloring();
    
    // Connect endstage buttons to actions
    setup_endstage();

    // Create the layout
    setup_layout();
}

/**
 * @brief Loads current configuration values into the configuration
 * window's control elements and other Qt objects.
 * 
* @note Function is obviously dependent on the configuration
 * 'NapConfig' singleton.
*/
void NapConfigWindow::load_cfg_values() {
    // Easier coding
    // Configuration access here
    NapConfig& cfg = NapConfig::get_instance();

    _histo_limit.setValue(cfg._histo_entries_limit);
    _task_col_btn.setChecked(cfg._use_task_coloring);
}

/**
 * @brief Update the configuration object's values with the values
 * from the configuration window.
 * 
 * @note Function is obviously dependent on the configuration
 * 'NapConfig' singleton.
*/
void NapConfigWindow::update_cfg() {
    // Configuration access here
    NapConfig& cfg = NapConfig::get_instance();

    cfg._histo_entries_limit = _histo_limit.value();
    cfg._use_task_coloring = _task_col_btn.isChecked();

    // Display a successful change dialog
    // We'll see if unique ptr is of any use here
    auto succ_dialog = new QMessageBox{QMessageBox::Information,
                "Configuration change success",
                "All configuration changes have been applied.",
                QMessageBox::StandardButton::Ok, this};
    succ_dialog->show();
}

/**
 * @brief Sets up spinbox and explanation label.
 * Spinbox's limit values are also set.
 * 
 * @note Function is also dependent on the configuration
 * 'NapConfig' singleton.
 */
void NapConfigWindow::setup_histo_section() {
    // Configuration access here
    NapConfig& cfg = NapConfig::get_instance();

    _histo_limit.setMinimum(0);
    _histo_limit.setMaximum(std::numeric_limits<int>::max());
    _histo_limit.setValue(cfg._histo_entries_limit);

    _histo_label.setFixedHeight(32);
    _histo_layout.addWidget(&_histo_label);
    _histo_layout.addStretch();
    _histo_layout.addWidget(&_histo_limit);
}

/**
 * @brief Sets up the "use task coloring" checkbox, label and layout.
 * 
 * @note Function is also dependent on the configuration
 * 'NapConfig' singleton.
 */
void NapConfigWindow::setup_tasklike_coloring() {
    // Configuration access here
    NapConfig& cfg = NapConfig::get_instance();

    _task_col_btn.setChecked(cfg._use_task_coloring);

    _task_col_layout.addWidget(&_task_col_label);
    _task_col_layout.addStretch();
    _task_col_layout.addWidget(&_task_col_btn);
}

/**
 * @brief Sets up the Apply and Close buttons by putting
 * them into a layout and assigning actions on pressing them.
 */
void NapConfigWindow::setup_endstage() {
    _endstage_btns_layout.addWidget(&_apply_button);
    _endstage_btns_layout.addWidget(&_close_button);

    connect(&_close_button,	&QPushButton::pressed,
            this, &QWidget::close);
    connect(&_apply_button, &QPushButton::pressed,
            this, [this]() { this->update_cfg(); this->close(); });
}

/**
 * @brief Sets up the main layout of the configuration dialog.
*/
void NapConfigWindow::setup_layout() {
    // Don't allow resizing
    _layout.setSizeConstraint(QLayout::SetFixedSize);

    // Add all control elements
    _layout.addLayout(&_histo_layout);
    _layout.addStretch();
    _layout.addLayout(&_task_col_layout);
    _layout.addStretch();
    _layout.addLayout(&_endstage_btns_layout);

    // Set the layout of the dialog
	setLayout(&_layout);
}
