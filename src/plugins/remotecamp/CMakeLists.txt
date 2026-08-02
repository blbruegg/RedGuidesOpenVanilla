# MQ2RemoteCamp
# Ported from a 2005-2008-era forum plugin (macroquest2.com/phpBB3).
# See F:\EQProject\docs\re_progress.md for the forum-plugins-2026-08-02 write-up.

include("../../Plugin.cmake")

set(MQ2RemoteCamp_SOURCES
    "MQ2RemoteCamp.cpp"
)

source_groups("Source Files" ${MQ2RemoteCamp_SOURCES})

add_library(MQ2RemoteCamp SHARED
    ${MQ2RemoteCamp_SOURCES}
)

set_target_properties(MQ2RemoteCamp PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2RemoteCamp)

target_link_libraries(MQ2RemoteCamp PRIVATE MQ2Main)
