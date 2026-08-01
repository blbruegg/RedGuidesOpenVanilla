# MQ2PacketLogger
# Research plugin: logs client network opcodes (via CPacketScrambler::ntoh/hton hooks)
# for the EQProject later-expansion opcode-recovery workstream.
# See F:\EQProject\docs\re_progress.md sections 2b/2c/6.

# ---------------------------------------------------------------------
# Props file includes
# ---------------------------------------------------------------------
include("../../Plugin.cmake")

# ---------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------
set(MQ2PacketLogger_SOURCES
    "MQ2PacketLogger.cpp"
)

source_groups("Source Files" ${MQ2PacketLogger_SOURCES})

# ---------------------------------------------------------------------
# Target definition
# ---------------------------------------------------------------------
add_library(MQ2PacketLogger SHARED
    ${MQ2PacketLogger_SOURCES}
)

set_target_properties(MQ2PacketLogger PROPERTIES FOLDER "core/plugins")

# ---------------------------------------------------------------------
# Apply props file configurations
# ---------------------------------------------------------------------
target_Plugin_props(MQ2PacketLogger)

# ---------------------------------------------------------------------
# Project dependencies
# ---------------------------------------------------------------------
target_link_libraries(MQ2PacketLogger PRIVATE MQ2Main)
