/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    NapRectangle.cpp
 * @brief   Definitions of plugin's drawable nap rectangle class and global
 *          functions.
 * 
 * @note    Nap := space in the histogram between a sched_switch and
 *          the closest next sched_waking or couplebreak/sched_waking[target]
 *          event in the task plot.
*/

// C++
#include <map>
#include <string>

// KernelShark
#include "libkshark.h"

// Plugin headers
#include "NapRectangle.hpp"
#include "NapConfig.hpp"

// Static functions

/**
 * @brief Map of abbreviations of prev_states to their full names.
 */
static const std::map<const char, const char*> LETTER_TO_NAME {{
    {'D', "uninterruptible (disk) sleep"},
    {'I', "idle"},
    {'P', "parked"},
    {'R', "running"},
    {'S', "sleeping"},
    {'T', "stopped"},
    {'t', "tracing stop"},
    {'X', "dead"},
    {'Z', "zombie"}
}};

// Member functions

/**
 * @brief Draws all the basic plot objects the nap rectangle is composed of.
 * In order of drawing: the rectangle, the outlines & if wide enough to display
 * text, the text box.
 * 
 * @note Despite the signature, the other parameters are ignored and are included
 * only to satisfy the inherited function.
*/
void NapRectangle::_draw(const KsPlot::Color&, float) const {
    // Don't draw into other plots, i.e. check that the rectangle
    // won't be angled up or down. This check might be redundant
    // as the entries collected should be on the same task plot,
    // therefore such situation won't occur, but better safe than
    // sorry.
    if (_rect.pointY(0) == _rect.pointY(3)) {
        _rect.draw();
        _outline_up.draw();
        _outline_down.draw();
        
        // Make sure the text fits in the rectangle and draw it if so.
        int nap_rect_width = (_rect.pointX(3) - _rect.pointX(0));
        int minimal_width = _raw_text.size() * FONT_SIZE;
        if (nap_rect_width > minimal_width) {
            _text.draw();
        }
    }
}

/**
 * @brief Constructor of the nap rectangle.
 * 
 * @param start: Pointer to the event entry from which to start the
 * nap
 * @param end: Pointer to the event entry at which to end the nap
 * @param rect: KernelShark rectangle to display as basis for the
 * nap rectangle
 * @param outline_col: Color of the outlines of the nap rectangle
 * @param text_col: Color of the text to be displayed on the nap rectangle
*/
NapRectangle::NapRectangle(const kshark_entry* start,
    const kshark_entry* end,
    const KsPlot::Rectangle& rect,
    const KsPlot::Color& outline_col,
    const KsPlot::Color& text_col)
    : _start_entry(start), _end_entry(end), _rect(rect)
{
    // Outline
    const ksplot_point upper_point_a = *rect.point(0);
    const ksplot_point upper_point_b = *rect.point(3);
    _outline_up._color = outline_col;
    _outline_up.setA(upper_point_a.x, upper_point_a.y);
    _outline_up.setB(upper_point_b.x, upper_point_b.y);

    const ksplot_point lower_point_a = *rect.point(1);
    const ksplot_point lower_point_b = *rect.point(2);
    _outline_down._color = outline_col;
    _outline_down.setA(lower_point_a.x, lower_point_a.y);
    _outline_down.setB(lower_point_b.x, lower_point_b.y);

    // Text
    const char start_ps = get_switch_prev_state(start);
    std::string raw_text{LETTER_TO_NAME.at(start_ps)};
    // Capitalize to be more readable (and slightly cooler)
    for(auto& character : raw_text) {
        character = std::toupper(character);
    }
    _raw_text = raw_text;

    const int base_x_1 = _rect.pointX(0);
    const int base_x_2 = _rect.pointX(3);
    const int base_y = _rect.pointY(1);

    const int rectangle_end = ((base_x_2 - base_x_1) / 2);
    // This is a rough estimate for centering, but it works.
    const int text_centering = (_raw_text.size() * FONT_SIZE / 3);
    
    const int actual_x = base_x_1 + rectangle_end - text_centering;
    // Move the text down a little bit
    KsPlot::Point final_point{actual_x, base_y + 1};


    _text = KsPlot::TextBox(get_bold_font_ptr(), _raw_text,
                            text_col, final_point);
}

/**
 * @brief Destructor of the nap rectangle. It behaves just like the default
 * destructor, but it also explicitly nulls its observer pointers.
 */
NapRectangle::~NapRectangle() {
    // KernelShark deletes the entries itself, we just have observers.
    _start_entry = nullptr;
    _end_entry = nullptr;
    // Other objects are deleted by C++ like default behaviour.
}

// Global functions

/**
 * @brief Gets the abbreviated name of a prev_state from the info field of a
 * KernelShark entry, leveraging the specific format of information strings
 * of entries in KernelShark.
 * 
 * @param entry: `sched/sched_switch` event entry whose prev_state we wish to get
 *  
 * @returns Char representing the abbreviated previous state of the task.
 */
char get_switch_prev_state(const kshark_entry* entry) {
    auto info_as_str = std::string(kshark_get_info(entry));
    std::size_t start = info_as_str.find(" ==>");
    char prev_state = info_as_str.substr(start - 1, 1)[0];
    return prev_state;
}