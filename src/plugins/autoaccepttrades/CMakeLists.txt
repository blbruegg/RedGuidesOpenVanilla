# MQ2AutoAcceptTrades
# Ported from a 2005-2008-era forum plugin (macroquest2.com/phpBB3).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.

include("../../Plugin.cmake")

set(MQ2AutoAcceptTrades_SOURCES
    "MQ2AutoAcceptTrades.cpp"
)

source_groups("Source Files" ${MQ2AutoAcceptTrades_SOURCES})

add_library(MQ2AutoAcceptTrades SHARED
    ${MQ2AutoAcceptTrades_SOURCES}
)

set_target_properties(MQ2AutoAcceptTrades PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2AutoAcceptTrades)

target_link_libraries(MQ2AutoAcceptTrades PRIVATE MQ2Main)
