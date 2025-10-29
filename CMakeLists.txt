# CMake config
cmake_minimum_required(VERSION 3.1.2 FATAL_ERROR)
project(naps)

# Plugin meta
set(PLUGIN_NAME "naps")
set(NAPS_VERSION "1.2.0")

# KernelShark meta
set(KS_VERSION "2.6.0-numatv")

# Ensure a default build type
## For customisability by the user
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif ()

message("[INFO] Modified KernelShark source version of software will be built.")

# Build the plugin itself
add_subdirectory("./src")

# If documentation was specified, attempt to find Doxygen and if found, add
# documentation build instructions to the Makefile.
# Otherwise, the user is free to generate the documentation themselves.
if (_DOXYGEN_DOC)
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
    message("[INFO] Adding documentation instructions into Makefile...")

    set(DOX_CONFIG "${CMAKE_SOURCE_DIR}/doc/Doxyfile")
    set(DOX_BUILD_LOG "${CMAKE_SOURCE_DIR}/doc/docs_build.log")

    add_custom_target(docs ALL)
    add_custom_command(TARGET docs
                           COMMAND doxygen ${DOX_CONFIG} > ${DOX_BUILD_LOG}
                           WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/doc")
    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
                    "${CMAKE_SOURCE_DIR}/docs_build.log"
                    "${CMAKE_SOURCE_DIR}/technical/html")
  else()
    message("[ERROR] Doxygen not found, can't generate build instructions.")
  endif()
endif()
