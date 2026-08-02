# MQ2ScreenShot
# Ported from a 2005-2008-era forum plugin (macroquest2.com/phpBB3).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.

include("../../Plugin.cmake")

set(MQ2ScreenShot_SOURCES
    "MQ2ScreenShot.cpp"
)

source_groups("Source Files" ${MQ2ScreenShot_SOURCES})

add_library(MQ2ScreenShot SHARED
    ${MQ2ScreenShot_SOURCES}
)

set_target_properties(MQ2ScreenShot PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2ScreenShot)

target_link_libraries(MQ2ScreenShot PRIVATE MQ2Main)
