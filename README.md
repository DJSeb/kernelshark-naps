# Project Overview

**Naps** (also referred to as naps or just Plugin) is a plugin for the trace-cmd data visualiser KernelShark. Nap, in
Plugin's definition, is the space in a single task's plot between a switch event and a waking event which wakes this
task again. Plugin also informs the user about the previous state of the task just before the switch.

## Purpose

It's main purpose is to show duration and previous state of a task that has just switched and will wake up during the
trace again. Such feature is not included in KernelShark or its default plugins. Closest one may get is the
sched_events plugin, which only visualises switch-to-switch or waking-to-switch portions of the trace.

## Features

- Plugin will visualise naps as rectangles between events _sched/sched_switch_ & either _sched/sched_waking_.
- Plugin will color the nap rectangles using information from previous state of the task and its PID. PID-color is
  determined internally by KernelShark, Plugin only accesses it and uses it for the rectangle's outlines, if the task
  coloring option was checked in Plugin's configuration (only for custom KernelShark). Color from the previous state
  is used to fill the shape's insides. It is chosen via to a state-to-color mapping as follows:
  - Uninterruptible disk sleep -> red
  - Idle -> Yellow
  - Parked -> Orange
  - Running -> Green
  - Sleeping -> Blue
  - Stopped -> Cyan
  - Tracing stop -> Brown
  - Dead -> Magenta
  - Zombie -> Purple
- Plugin will also write into the rectangle, if it is wide enough, the full name and abbreviation of the previous
  state of the task. The text will be either black or white, chosen via color intensity of the background color.
- Plugin is exclusive with sched_events plugin and if both are enabled, none of them will work correctly.
- Plugin offers an task-like coloring version, which will change the top and bottom outline of nap rectangles,
  to be of the same color as the task owning said rectangle. This option has to be turned on in the configuration
  upon each plugin load, as it is not persistent. It couldn't be hardwired into the code due to issues with session
  loads.

# Project layout

- Naps (root directory, this directory)
  - build
    - if present, houses _build files_
    - if present, might contain bin directory
      - houses _built binaries_
  - doc
    - technical
    - user
      - _user-manual.md_
      - _user-manual.odt_
      - _user-manual.pdf_
    - images
      - images used in both user and technical documentations
    - doxygen-awesome-css (houses files which prettify doxygen output)
    - _mainpage.doxygen_
    - _design.doxygen_
    - _Doxyfile_
  - src
    - _CMakeLists.txt_ (Further CMake instructions for building the binary)
    - **source files of the plugin**
  - _CMakeLists.txt_ (Main build file)
  - _FindTraceEvent.cmake_ (finds traceevent during plugin's build)
  - _README.md_ (what you're reading currently)
  - _LICENSE_

# Usage & documentation

For user documentation refer to the [user manual](./doc/user/user-manual.md) (slightly out of date in comparison
to its Czech version).

Code is heavily commented, up to every private function. For detailed understanding of how Plugin works,
refer to the source files in `src` directory.

Technical documentation has to be generated via Doxygen and the included
Doxyfile.

For a design document, please refer to the `doc/technical` directory, as this is included in the technical documentation,
or read it unprocessed in file `doc/design.doxygen`.

For API documentation, please refer to the `doc/technical` directory.

# Acknowledgments

- The idea of this plugin was suggested by **Jan Kára RNDr.**, who also serves as project's **supervisor**.
- **Yordan Karadzhov** - maintains KernelShark, inspiration in already written plugins was of immense help
- ChatGPT deserves the "greatest evolution of the rubber ducky" award - not very helpful, but great to just unload bad ideas onto
- **Markdown PDF** - VS Code extension, allowed me to export the manual with pictures easily
- **Doxygen Awesome CSS** - [beautiful CSS theme](https://jothepro.github.io/doxygen-awesome-css/index.html)
  that makes Doxygen documentation look visually pleasing

# About author

David Jaromír Šebánek (e-mail: `djsebofficial@gmail.com`). Direct all inquiries about this plugin there.

# License

This plugin uses [MIT licensing](./LICENSE). KernelShark uses LGPL-2.1.

