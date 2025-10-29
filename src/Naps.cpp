/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    Naps.cpp
 * @brief   This file contains functions which serving as a bridge between
 *          the plugin's C++ part and C part. Most of them are static, but
 *          a few functions defined in `naps.h` are also defined here, all
 *          to access C++ part's code.
*/

// C++
#include <map>

// KernelShark
#include "libkshark.h"
#include "libkshark-plugin.h"
#include "KsMainWindow.hpp"
#include "KsGLWidget.hpp"

// Plugin headers
#include "naps.h"
#include "NapConfig.hpp"
#include "NapRectangle.hpp"

// Usings
/**
 * @brief Type to be used by the PREV_STATE_TO_COLOR constant.
*/
using prev_state_colors_t = std::map<const char, KsPlot::Color>;

// Constants
/**
 * @brief Constant map of assigned colors to prev_state abbreviations.
*/
static const prev_state_colors_t PREV_STATE_TO_COLOR {
    {'D', {255, 0, 0}}, // Red
    {'I', {255, 255, 0}}, // Yellow
    {'P', {255, 165, 0}}, // Orange
    {'R', {0, 255, 0}}, // Green
    {'S', {0, 0, 255}}, // Blue
    {'T', {0, 255, 255}}, // Cyan
    {'t', {139, 69, 19}}, // Brown
    {'X', {255, 0, 255}}, // Magenta
    {'Z', {128, 0, 128}} // Purple
};


// Static variables
/**
 * @brief Static pointer to the configuration window.
 */
static NapConfigWindow* cfg_window;

// Static functions

/**
 * @brief Loads values into the configuration window from
 * the configuration object and shows the window afterwards.
 * 
 * @note Function depends on the file-global variable `cfg_window`.
*/
static void config_show([[maybe_unused]] KsMainWindow*) {
    cfg_window->load_cfg_values();
    cfg_window->show();
}

/**
 * @brief Gets the color of the task used by KernelShark for that task.
 * Used during nap rectangle creation to figure out the outline of the rectangle.alignas
 * Such outlines are the only color signal to the user to which task the nap belongs.
 * Slightly useful if the user accidentally scrolls to another task, the color can
 * help recognize this situation.
 * 
 * @param pid Process ID of the task, whose color we wish to get
 * @return KernelShark's Color object holding a copy of the task's color
 * 
 * @note  Function also depends on the configuration `NapConfig` singleton.
 */
static const KsPlot::Color _get_task_color(int pid) {
    // Fail-safe color, all white (makes the rectangle seem
    // thinner, which is a small signal that the task color wasn't found).
    static const KsPlot::Color DEFAULT_COLOR
        { (uint8_t)256, (uint8_t)256, (uint8_t)256};
    
    KsPlot::Color result_color = DEFAULT_COLOR;
    // Configuration access here.
    KsTraceGraph* graph = NapConfig::main_w_ptr->graphPtr();
    // A journey to get the color of the task.
    const KsPlot::ColorTable& pid_color_table = graph->glPtr()->getPidColors();
    bool pid_color_exists = static_cast<bool>(pid_color_table.count(pid));

    if (pid_color_exists) {
        result_color = pid_color_table.at(pid);
    }

    return result_color;
}

/**
 * @brief Returns either black if the background color's intensity is too great,
 * otherwise returns white. Limit to intensity is `128.0`.
 * 
 * @param bg_color_intensity: Computed intensity from an RGB color
 * 
 * @returns Black on high intensity, white otheriwse.
*/
static KsPlot::Color _black_or_white_text(float bg_color_intensity) {
    const static KsPlot::Color WHITE {0xFF, 0xFF, 0xFF};
    const static KsPlot::Color BLACK {0, 0, 0};
    constexpr float INTENSITY_LIMIT = 128.f;

    return (bg_color_intensity > INTENSITY_LIMIT) ? BLACK : WHITE;
}

/**
 * @brief Gets the color intensity using the formula
 * `(red * 0.299) + (green * 0.587) + (blue * 0.114)`.
 * 
 * @param c: RGB color value whose components will be checked
 * 
 * @returns Color intensity as floating-point value.
*/
static float _get_color_intensity(const KsPlot::Color& c) {
    // Color multipliers are chosen the way they are based on human
    // eye's receptiveness to each color (green being the color human
    // eyes focus on the most).
    return (c.b() * 0.114f) + (c.g() * 0.587f) + (c.r() * 0.299f);
}

/**
 * @brief General function for checking whether to show a nap rectangle
 * in the plot, based on if an entry exists, is visible in the graph
 * and if graph itself is visible. 
 * 
 * @param entry: Pointer to an event entry which could be a part of the
 * nap rectangle
 * 
 * @returns True if entry is visible in a visible graph
 * (and entry's pointer wasn't null).
 */
static bool _nap_rect_check_function_general(const kshark_entry* entry) {
    if (!entry) return false;
    bool is_visible_event = entry->visible
        & kshark_filter_masks::KS_EVENT_VIEW_FILTER_MASK;
    bool is_visible_graph = entry->visible
        & kshark_filter_masks::KS_GRAPH_VIEW_FILTER_MASK;
    return is_visible_event && is_visible_graph;
}

/**
 * @brief Creates a nap rectangle to be displayed on the plot via
 * KernelShark's plot objects API.
 * 
 * @param graph KernelShark graph
 * @param bin KernelShark bins
 * @param data: Container of two entries between which to draw the nap
 * rectangle
 * 
 * @returns Pointer to the heap-created nap rectangle.
 * 
 * @note  Function also depends on the file-global variable
 * `PREV_STATE_TO_COLOR` and the configuration `NapConfig` singleton.
 */
static NapRectangle* _make_nap_rect(std::vector<const KsPlot::Graph*> graph,
    std::vector<int> bin,
    std::vector<kshark_data_field_int64*> data,
    KsPlot::Color, float)
{
    // Positioning constants, relevant only here, hence defined here
    constexpr int HEIGHT = 8;
    constexpr int HEIGHT_OFFSET = -10;
    // Looking into KernelShark's source doesn't immediately reveal
    // why the graph is passed in a vector, but this is how it is
    // done in makeLatencyBox function in `KsPlugins.hpp`, which
    // works seemingly well, hence same approach was chosen here.
    KsPlot::Point start_base_point = graph[0]->bin(bin[0])._val;
    KsPlot::Point end_base_point = graph[0]->bin(bin[1])._val;

    /* Rectangle:
        0----------3
        |          |
        |          |
        1----------2
    */

    auto point_0 = KsPlot::Point{start_base_point.x() + 1,
        start_base_point.y() - HEIGHT_OFFSET - HEIGHT};
    auto point_1 = KsPlot::Point{start_base_point.x() + 1,
        start_base_point.y() - HEIGHT_OFFSET};
    auto point_3 = KsPlot::Point{end_base_point.x() - 1,
        end_base_point.y() - HEIGHT_OFFSET - HEIGHT};
    auto point_2 = KsPlot::Point{end_base_point.x() - 1,
        end_base_point.y() - HEIGHT_OFFSET};

    // Put relevant entries into variables
    kshark_entry* switch_entry = data[0]->entry;
    kshark_entry* wakeup_entry = data[1]->entry;

    // Create the rectangle and color it
    KsPlot::Rectangle rect;
    rect.setFill(true);
    // Access to global variable here.
    rect._color = PREV_STATE_TO_COLOR.at(get_switch_prev_state(switch_entry));

    rect.setPoint(0, point_0);
    rect.setPoint(1, point_1);
    rect.setPoint(2, point_2);
    rect.setPoint(3, point_3);

    // Prepare outline color
    KsPlot::Color outline_col = rect._color;
    
    // Configuration access here.
    if(NapConfig::get_instance().get_use_task_coloring()) {
        outline_col = _get_task_color(switch_entry->pid);
    }

    // Prepare text color
    float bg_intensity = _get_color_intensity(rect._color);
    const KsPlot::Color text_color = _black_or_white_text(bg_intensity);

    // Create the final nap rectangle and return it
    NapRectangle* nap_rect = new NapRectangle{switch_entry, wakeup_entry, rect,
        outline_col, text_color};
    return nap_rect;
}

/**
 * @brief The actual drawing function of the plugin. It initializes check
 * functions for selecting naps-relevant entries and draws naps via
 * KernelShark's interval plotting API.
 * 
 * @note Naps-relevant entries are: `sched/sched_switch`
 * `sched/sched_waking` OR `couplebreak/sched_waking[target]`. These are
 * chosen between in the C part based on a stream's setting of couplebreak.
 * 
 * @param argVCpp: The C++ arguments of the drawing function of the plugin
 * @param plugin_data: Input location for the container of the event's data
 * @param ctx: Pointer to the plugin's context
 * @param val: Process or CPU ID value
 */
static void _draw_nap_rectangles(KsCppArgV* argVCpp,
    kshark_data_container* plugin_data,
    const plugin_naps_context* ctx,
    int val)
{
    IsApplicableFunc nap_rect_check_func_switch;
    IsApplicableFunc nap_rect_check_func_waking;

    nap_rect_check_func_switch = [=] (kshark_data_container* data_c, ssize_t t) {
        kshark_entry* entry = data_c->data[t]->entry;

        bool is_switch = (entry->event_id == ctx->sswitch_event_id);
        bool correct_pid = (entry->pid == val);
        return _nap_rect_check_function_general(entry) && is_switch && correct_pid;
    };

    nap_rect_check_func_waking = [=] (kshark_data_container* data_c, ssize_t i) {
        kshark_entry* entry = data_c->data[i]->entry;
        int64_t d_field = data_c->data[i]->field;

        bool is_waking = (entry->event_id == ctx->waking_event_id);
        bool correct_waking_pid = (d_field == val);
        return _nap_rect_check_function_general(entry) && is_waking && correct_waking_pid;
    };

    // Noteworthy thing here is that KernelShark will automatically
    // select a pair and NOT use members of it again - there won't be multiple
    // nap rectangles from one entry.
    // Hopefully this will never change.
    eventFieldIntervalPlot(argVCpp, plugin_data, nap_rect_check_func_switch,
                            plugin_data, nap_rect_check_func_waking,
                            _make_nap_rect, {0, 0, 0}, -1);
}

// Functions defined in C header

/**
 * @brief Callback function called by KernelShark to draw naps as rectangles,
 * if conditions for drawing are met. This function is actually just a wrapper
 * for its C++ implementation `_draw_nap_rectangles`, this one mostly just
 * checks pre-conditions and then calls the C++ function. 
 * 
 * @param argv_c Arguments for the plugin's drawing function (e.g. visible
 * bins in the histogram)
 * @param sd Stream identifier number
 * @param val Process ID or CPU ID value (depends on `draw_action` argument)
 * @param draw_action Action to be performed (e.g. drawing task plots or CPU
 * plots)
 * 
 * @note Function also depends on the configuration `NapConfig` singleton.
 */
void draw_nap_rectangles(struct kshark_cpp_argv* argv_c, int sd,
    int val, int draw_action)
{
    KsCppArgV* argVCpp KS_ARGV_TO_CPP(argv_c);
    plugin_naps_context* ctx = __get_context(sd);
    kshark_data_container* plugin_data;

    // Get config data
    const NapConfig& config = NapConfig::get_instance();
    const int32_t HISTO_ENTRIES_LIMIT = config.get_histo_limit();

    // Don't draw if not drawing tasks or
    // if there are too many bins to draw.
    bool is_task_draw = (draw_action == KSHARK_TASK_DRAW);
    bool is_too_many_bins = (argVCpp->_histo->tot_count > HISTO_ENTRIES_LIMIT);
    
    if (!is_task_draw || is_too_many_bins) {
        return;
    }

    plugin_data = ctx->collected_events;
    if (!plugin_data) {
        // Couldn't get the context container (any reason)
        return;
    }

    _draw_nap_rectangles(argVCpp, plugin_data, ctx, val);
}

/**
 * @brief Give the plugin a pointer to KernelShark's main window to allow
 * GUI manipulation and menu creation.
 * 
 * This is where plugin menu is made and initialized first. It's lifetime
 * is managed by KernelShark afterward.
 * 
 * @param gui_ptr: Pointer to the main KernelShark window.
 * 
 * @returns Pointer to the configuration menu window instance. Returned
 * window pointer is a void pointer (to interoperate with C, who doesn't
 * know these types).
 * 
 * @note Function also depends on the configuration `NapConfig` singleton
 * & on the file-global variable `cfg_window`.
*/
__hidden void* plugin_set_gui_ptr(void* gui_ptr) {
    KsMainWindow* main_w = static_cast<KsMainWindow*>(gui_ptr);
    // Configuration access here.
    NapConfig::main_w_ptr = main_w;

    // File-global variable access here.
    if (cfg_window == nullptr) {
        cfg_window = new NapConfigWindow();
    }

    QString menu("Tools/Naps Configuration");
    main_w->addPluginMenu(menu, config_show);

    return cfg_window;
}
