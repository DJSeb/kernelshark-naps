/** Copyright (C) 2025, David Jaromír Šebánek <djsebofficial@gmail.com> **/

/**
 * @file    naps.c
 * @brief   Contains definitions of functions used by the plugin
 *          upon plugin loading and deloading, as well as handler's
 *          (un)registriations and a few event-processing functions.
*/


// C
#include <stdbool.h>

// KernelShark
#include "libkshark.h"
#include "libkshark-plot.h"
#include "libkshark-plugin.h"
#include "libkshark-tepdata.h"
#ifndef _UNMODIFIED_KSHARK
#include "libkshark-couplebreak.h"
#endif

// Plugin header
#include "naps.h"

// Static variables

///
/// @brief Font used by KernelShark when plotting.
static struct ksplot_font font;

/// @brief Font used by the plugin when highlighting
/// harder to see text.
static struct ksplot_font bold_font;

///
/// @brief Path to font file.
static char* font_file = NULL;

///
/// @brief Path to the bold font file.
static char* bold_font_path = NULL;

// Header file definitions

/**
 * @brief Checks if the bold font is loaded. If it isn't loaded yet, it initializes it.
 * 
 * @note Font to be loaded is *FreeSansBold*. This shouldn't produce issues,
 * as KernelShark uses said font in regular form and bold should be included.
 * If it does produce an issue, change `bold_font_path` above to the font file you
 * wish to use.
 * 
 * @returns True if font is loaded, false otherwise.
 */
struct ksplot_font* get_bold_font_ptr() {
    if (!ksplot_font_is_loaded(&bold_font)) {
        ksplot_init_font(&bold_font, FONT_SIZE + 2, bold_font_path);
    }
    
    return &bold_font;
}

/**
 * @brief Get pointer to the font. Initializes the font
 * if this hasn't happened yet.
 * 
 * @returns Pointer to the font
*/
struct ksplot_font* get_font_ptr() {
    if (!ksplot_font_is_loaded(&font)) {
        ksplot_init_font(&font, FONT_SIZE, font_file);
    }

    return &font;
}

// Context & plugin loading

/**
 * @brief Frees structures of the context and invalidates other number fields.
 * 
 * @param nr_ctx: Pointer to plugin's context to be freed
*/ 
static void _nr_free_ctx(struct plugin_naps_context* nr_ctx)
{
	if (!nr_ctx) {
		return;
    }

	kshark_free_data_container(nr_ctx->collected_events);

    nr_ctx->sswitch_event_id = nr_ctx->waking_event_id = -1;
}

/// @cond Doxygen_Suppress
// KernelShark-provided magic that will define the most basic
// plugin context functionality - init, freeing and getting context.
KS_DEFINE_PLUGIN_CONTEXT(struct plugin_naps_context , _nr_free_ctx);
/// @endcond

/**
 * @brief Process sched_waking events as tep records during plugin loads,
 * adds PID of who is being awoken into the sched_waking entry's auxiliary
 * field - this pid will also become the owner of the event.
 * Else, it adds in invalid -1 (which isn't a valid PID).
 * 
 * @param ctx: Pointer to plugin context
 * @param stream: Pointer to the KernelShark stream with data
 * @param rec: Pointer to the tep record of the entry
 * @param entry: Pointer KernelShark event entry
 * 
 * @note This function should be called only if couplebreak is disabled
 * in a KernelShark stream, as it attempts to basically use 
 * (in couplebreak terms) an origin entry as a target entry
 * for visualization purposes.
 * @warning Incompatiblity - this function may make some plugins
 * expecting certain values in entries to be incompatible with
 * this plugin, e.g. sched_events plugin.
*/
static void waking_evt_tep_processing(struct plugin_naps_context* ctx, 
   [[maybe_unused]] struct kshark_data_stream* stream,
   void* rec, struct kshark_entry* entry)
{
   struct tep_record* record = (struct tep_record*)rec;
   unsigned long long val; int ret;
   struct kshark_data_container* collected_events = ctx->collected_events;

   ret = tep_read_number_field(ctx->sched_waking_pid_field, record->data, &val);

   if (ret == 0) {
       // This is a source of possible incompatibility with other plugins.
       // Changing the PID also moves the event into another task's task plot,
       // which is crucial for interval plots.
       entry->pid = (int32_t)val;
       entry->visible &= ~KS_PLUGIN_UNTOUCHED_MASK;
       // If some events change the entry's PID further, this is a
       // storage of the PID naps captured dring its load - it helps
       // consistency of data for the plugin ever so slightly.
       kshark_data_container_append(collected_events, entry, val);
   } else {
       // Couldn't read number field, move on. Minus one will also always
       // produce a negative result in check functions.
       kshark_data_container_append(collected_events, entry, (int64_t)-1);
   }
}

/**
 * @brief Selects supported events from unsorted trace file data
 * during plugin and data loading.
 * 
 * @note Effective during KShark's get_records function.
 * 
 * @param stream: KernelShark's data stream
 * @param rec: Tep record structure holding data collected by trace-cmd
 * @param entry: KernelShark entry to be processed
 * 
 * @note Supported events are: `sched/sched_switch`,
 *                             `sched/sched_waking`.
 * However, if couplebreak feature is present, waking events detected change to
 * `couplebreak/sched_waking[target]`.
*/
static void _select_events(struct kshark_data_stream* stream,
    [[maybe_unused]] void* rec, struct kshark_entry* entry) {

    struct plugin_naps_context *nr_ctx = __get_context(stream->stream_id);
    if (!nr_ctx) return;
    struct kshark_data_container* nr_ctx_collected_events = nr_ctx->collected_events;
    if (!nr_ctx_collected_events) return;
   
    /** -1 is a nonsensical value, but necessary so that the container
     * isn't considered empty.
    */
    if (entry->event_id == nr_ctx->sswitch_event_id) {
        kshark_data_container_append(nr_ctx_collected_events, entry, (int64_t)-1);
    } else if (entry->event_id == nr_ctx->waking_event_id) {
#ifndef _UNMODIFIED_KSHARK
        if (stream->couplebreak_on) {
            // Couplebreak target events do not need to be additionally processed.
            // While not necessary to store the pid, as the entry will be accessible
            // as well, it is stored for consistency with the non-couplebreak case.
            // Moreover, if a plugin decided to change the PID of couplebreak
            // entries, this helps the naps plugin be consistent with the data it saw
            // during load and which it will use during operation.
            kshark_data_container_append(nr_ctx_collected_events, entry, entry->pid);
        } else {
            waking_evt_tep_processing(nr_ctx, stream, rec, entry);
        }
#else
        waking_evt_tep_processing(nr_ctx, stream, rec, entry);
#endif
    }
}

/** 
 * @brief Initializes the plugin's context and registers handlers of the
 * plugin. This is also the function where the plugin decides, based on
 * current couplebreak status, if it will use `sched/sched_waking` events
 * or `couplebreak/sched_waking[target]` events.
 * 
 * @param stream: KernelShark's data stream for which to initialize the
 * plugin
 * 
 * @returns `0` if any error happened. `1` if initialization was successful.
*/
int KSHARK_PLOT_PLUGIN_INITIALIZER(struct kshark_data_stream* stream) {
    if (!font_file || !bold_font_path) {
        font_file = ksplot_find_font_file("FreeSans", "FreeSans");
        bold_font_path = ksplot_find_font_file("FreeSans", "FreeSansBold");
    }
    if (!font_file || !bold_font_path) return 0;
    
    struct plugin_naps_context* nr_ctx = __init(stream->stream_id);

    if (!nr_ctx) {
		__close(stream->stream_id);
		return 0;
	}

    if (!kshark_is_tep(stream)) {
        __close(stream->stream_id);
        return 0;
    }

    nr_ctx->tep = kshark_get_tep(stream);
    bool waking_found = define_wakeup_event(nr_ctx->tep, &nr_ctx->tep_waking);

    if (waking_found) {
        nr_ctx->sched_waking_pid_field = tep_find_any_field(nr_ctx->tep_waking, "pid");
    }

    nr_ctx->collected_events = kshark_init_data_container();

    nr_ctx->sswitch_event_id = kshark_find_event_id(stream, "sched/sched_switch");

#ifndef _UNMODIFIED_KSHARK
    int swaking_id = kshark_find_event_id(stream, "sched/sched_waking");

    nr_ctx->waking_event_id = (stream->couplebreak_on) ?
        COUPLEBREAK_SWT_ID : swaking_id;
#else
    nr_ctx->waking_event_id = kshark_find_event_id(stream, "sched/sched_waking");
#endif

    kshark_register_event_handler(stream, nr_ctx->sswitch_event_id, _select_events);
    kshark_register_event_handler(stream, nr_ctx->waking_event_id, _select_events);
    kshark_register_draw_handler(stream, draw_nap_rectangles);

    return 1;
}

/**
 * @brief Deinitializes the plugin's context and unregisters handlers of the
 * plugin.
 * 
 * @param stream: KernelShark's data stream in which to deinitialize the
 * plugin.
 * 
 * @returns `0` if any error happened. `1` if deinitialization was successful.
*/
int KSHARK_PLOT_PLUGIN_DEINITIALIZER(struct kshark_data_stream* stream) {
    struct plugin_naps_context* nr_ctx = __get_context(stream->stream_id);

    int retval = 0;

    if (nr_ctx) {
        // Don't have dangling pointers
        nr_ctx->tep = NULL;
        nr_ctx->sched_waking_pid_field = NULL;
        nr_ctx->sched_waking_pid_field = NULL;

        kshark_unregister_event_handler(stream, nr_ctx->sswitch_event_id, _select_events);
        kshark_unregister_event_handler(stream, nr_ctx->waking_event_id, _select_events);
        kshark_unregister_draw_handler(stream, draw_nap_rectangles);
        retval = 1;
    }

    if (stream->stream_id >= 0)
        __close(stream->stream_id);

    return retval;
}

/**
 * @brief Initializes menu for the plugin and gives the plugin pointer
 * to KernelShark's main window.
 * 
 * @param gui_ptr: Pointer to KernelShark's GUI, its main window
 * 
 * @returns Pointer to KernelShark's main window.
*/
void* KSHARK_MENU_PLUGIN_INITIALIZER(void* gui_ptr) {
	return plugin_set_gui_ptr(gui_ptr);
}