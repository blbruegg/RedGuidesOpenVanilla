# MQ2GemTimer
# Ported from a 2008-2009-era forum plugin (macroquest2.com/phpBB3).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.
# NOTE: the aura-timer-bar feature and the spellstate-based "confirmed" cast
# gate are disabled/simplified in this port -- see header comment in
# MQ2GemTimer.cpp for details. Gem recast timers work; aura timers do not.

include("../../Plugin.cmake")

set(MQ2GemTimer_SOURCES
    "MQ2GemTimer.cpp"
)

source_groups("Source Files" ${MQ2GemTimer_SOURCES})

add_library(MQ2GemTimer SHARED
    ${MQ2GemTimer_SOURCES}
)

set_target_properties(MQ2GemTimer PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2GemTimer)

target_link_libraries(MQ2GemTimer PRIVATE MQ2Main)
