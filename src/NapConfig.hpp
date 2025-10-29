/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    NapConfig.hpp
 * @brief   This file has declaration of the config window class
 *          for the plugin as well as the config object.
 * 
 * @note    Definitions in `NapConfig.cpp`.
*/

#ifndef _NR_NAP_CONFIG_HPP
#define _NR_NAP_CONFIG_HPP

//C++
#include <stdint.h>
#include <map>

// Qt
#include <QtWidgets>

// KernelShark
#include "libkshark.h"
#include "KsPlotTools.hpp"
#include "KsMainWindow.hpp"

// For friending purposes :)
class NapConfigWindow;

/**
 * @brief Singleton class for the config object of the plugin.
 * Holds the histogram limit until nap rectangles activate and
 * a pointer to the main window of KernelShark for GUI manipulation.
 * 
 * It's preinitialised to some sane defaults and is NOT persistent,
 * i.e. settings won't be preserved across different KernelShark sessions.
*/
class NapConfig {
// Necessary for the config window to manipulate values in the config object.
friend class NapConfigWindow;
public: // Class data members
    ///
    /// @brief Pointer to the main window used for window hierarchies.
    inline static KsMainWindow* main_w_ptr = nullptr;
private: // Data members
    /// @brief Limit value of how many entries may be visible in a
    /// histogram for the plugin to take effect.
    int32_t _histo_entries_limit{10000};
public: // Functions
    static NapConfig& get_instance();
    int32_t get_histo_limit() const;
private: // Constructor
    /// @brief Default constructor, hidden to enforce singleton pattern.
    NapConfig() = default;
};

/**
 * @brief QtWidget's child class for modifying configuration via GUI.
 * It is a fixed max height dialog window that allows modification
 * of all that is in the config object by applying changes via
 * the Apply button. Changes won't be saved unless this is done and
 * selected changes can be discarded by closing the window.
 * 
 * @note This class is unsurprisingly heavily dependent on the
 * configuration 'NapConfig' singleton.
 */
class NapConfigWindow : public QWidget {
// Non-Qt portion
public: // Functions
    NapConfigWindow();
    void load_cfg_values();
private:
    void update_cfg();
// Qt portion
private: // Qt data members
    ///
    /// @brief Layout for the widget's control elements.
    QVBoxLayout     _layout;
    
    /// @brief Layout for the Apply and Close buttons.
    QHBoxLayout     _endstage_btns_layout;

    // Histo limit

    /// @brief Layout used for the spinbox and explanation
    /// of what it does in the label.
    QHBoxLayout     _histo_layout;

    ///
    /// @brief Explanation of what the spinbox next to it does.
    QLabel          _histo_label;

    /// @brief Spinbox used to change the limit of entries visible
    /// before nap rectangles show up.
    QSpinBox        _histo_limit;

public: // Qt data members
    ///
    /// @brief Close button for the widget.
    QPushButton     _close_button;

    /// @brief Button applies changes to the
    /// configuration object and shows an info dialog.
    QPushButton     _apply_button;
private: // "Only Qt"-relevant functions
    void setup_histo_section();
    void setup_endstage();
    void setup_layout();
};

#endif // _NR_NAP_CONFIG_HPP
