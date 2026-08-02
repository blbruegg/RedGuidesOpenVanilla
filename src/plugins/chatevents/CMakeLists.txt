# MQ2ChatEvents
# Ported from a 2006-2013-era forum plugin (macroquest2.com/phpBB3), authors
# Persnickety/BustedPretext. See F:\EQProject\docs\re_progress.md for the
# forum-plugins-2026-08-02 write-up.

include("../../Plugin.cmake")

set(MQ2ChatEvents_SOURCES
    "MQ2ChatEvents.cpp"
)

source_groups("Source Files" ${MQ2ChatEvents_SOURCES})

add_library(MQ2ChatEvents SHARED
    ${MQ2ChatEvents_SOURCES}
)

set_target_properties(MQ2ChatEvents PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2ChatEvents)

target_link_libraries(MQ2ChatEvents PRIVATE MQ2Main)

target_link_libraries(MQ2ChatEvents PRIVATE winmm)
