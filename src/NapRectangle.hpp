/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    NapRectangle.hpp
 * @brief   Declarations & definitions of plugin's drawable nap rectangle class.
 * 
 * @note    Nap := space in the histogram between a sched_switch and
 *          the closest next sched_waking event in the task plot.
 * @note    Definitions in `NapRectangle.cpp`.
*/

#ifndef _NR_NAP_RECTANGLE_HPP
#define _NR_NAP_RECTANGLE_HPP

// C++
#include <cstring>
#include <string>

// KernelShark
#include "libkshark.h"
#include "KsPlotTools.hpp"

// Plugin
#include "naps.h"
 
/**
 * @brief Represents a rectangle that visualises the "nap"
 * of a task - the space between a sched_switch and its
 * closest next sched/sched_waking.
 * 
 * It cannot be interacted with, only drawn and looked at.
 * 
 * It will show:
 * 
 * - a rectangle with a background (this background is assigned a
 * color based on the prev_state of the sched_switch event the nap
 * rectangle uses)
 * 
 * - upper and lower higlights of the outline (their color is the
 * color of the task assigned to it by KernelShark)
 * 
 * - text with the full name of the prev_state of the sched_switch
 * event of the nap rectangle, but only if if there is enough space
 * to write that text so that it doesn't overflow its boundaries.
 */
class NapRectangle: public KsPlot::PlotObject {
private:
    ///
    /// @brief Observer of the entry where the nap rectangle starts.
    const kshark_entry* _start_entry = nullptr;
    ///
    /// @brief Observer of the entry where the nap rectangle ends.
    const kshark_entry* _end_entry = nullptr;
    ///
    /// @brief Main shape of the nap rectangle
    KsPlot::Rectangle _rect;
    ///
    /// @brief Upper line for the outline highlight.
    KsPlot::Line _outline_up;
    ///
    /// @brief Lower line for the outline highlight.
    KsPlot::Line _outline_down;
    /// @brief KernelShark plot objects for displaying prev_state
    /// information of the sched_switch on the nap rectangle.
    KsPlot::TextBox _text;
    /// @brief Full name of a prev_state the sched_switch of the nap
    /// rectangle was in.
    std::string _raw_text;
private:
    void _draw(const KsPlot::Color&, float) const override;
public:
    explicit NapRectangle(const kshark_entry* start,
        const kshark_entry* end,
        const KsPlot::Rectangle& rect,
        const KsPlot::Color& outline_col,
        const KsPlot::Color& text_col);

    // No use for an empty constructor
    NapRectangle() = delete;
    ~NapRectangle();
};

char get_switch_prev_state(const kshark_entry* entry);

#endif // _NAP_RECTANGLE_HPP
