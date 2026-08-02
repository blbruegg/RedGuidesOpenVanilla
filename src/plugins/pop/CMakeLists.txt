# MQ2Pop
# Ported from a 2006-era forum plugin (macroquest2.com/phpBB3 t=12799).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.

# ---------------------------------------------------------------------
# Props file includes
# ---------------------------------------------------------------------
include("../../Plugin.cmake")

# ---------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------
set(MQ2Pop_SOURCES
    "MQ2Pop.cpp"
)

source_groups("Source Files" ${MQ2Pop_SOURCES})

# ---------------------------------------------------------------------
# Target definition
# ---------------------------------------------------------------------
add_library(MQ2Pop SHARED
    ${MQ2Pop_SOURCES}
)

set_target_properties(MQ2Pop PROPERTIES FOLDER "core/plugins")

# ---------------------------------------------------------------------
# Apply props file configurations
# ---------------------------------------------------------------------
target_Plugin_props(MQ2Pop)

# ---------------------------------------------------------------------
# Project dependencies
# ---------------------------------------------------------------------
target_link_libraries(MQ2Pop PRIVATE MQ2Main)
