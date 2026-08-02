# MQ2AutoAccept
# Vendored from the current, actively-maintained github.com/RedGuides/MQ2AutoAccept source
# (via local RazzrrMacroq checkout, F:\MQ2Dev\RazzrrMacroq\plugins\MQ2AutoAccept\MQ2AutoAccept.cpp),
# NOT the obsolete 2008 forum "AutoAcceptTrades" copy preserved under
# src/plugins/autoaccepttrades/original_2005-2013_source/ for reference only.
# Auto-accepts group/raid/fellowship invites, trades, and various dialog boxes
# (real estate anchor ports, translocates, etc) -- unrelated to the old
# trade-window-only forum plugin that name superficially resembles.

include("../../Plugin.cmake")

set(MQ2AutoAccept_SOURCES
    "MQ2AutoAccept.cpp"
)

source_groups("Source Files" ${MQ2AutoAccept_SOURCES})

add_library(MQ2AutoAccept SHARED
    ${MQ2AutoAccept_SOURCES}
)

set_target_properties(MQ2AutoAccept PROPERTIES FOLDER "core/plugins")

target_Plugin_props(MQ2AutoAccept)

target_link_libraries(MQ2AutoAccept PRIVATE MQ2Main)
