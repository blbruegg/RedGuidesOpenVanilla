# MQ2CharNotes
# Ported from a 2005-2008-era forum plugin (macroquest2.com/phpBB3).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.

include("../../Plugin.cmake")

set(MQ2CharNotes_SOURCES
    "MQ2CharNotes.cpp"
)

source_groups("Source Files" ${MQ2CharNotes_SOURCES})

add_library(MQ2CharNotes SHARED
    ${MQ2CharNotes_SOURCES}
)

set_target_properties(MQ2CharNotes PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2CharNotes)

target_link_libraries(MQ2CharNotes PRIVATE MQ2Main)
