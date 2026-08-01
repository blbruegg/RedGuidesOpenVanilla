# MQ2PacketLogger

Research/reverse-engineering plugin for the EQProject "later-expansion opcode
recovery" workstream. Logs every network opcode the live EverQuest client
processes to a CSV file, so opcodes can be identified by performing known
actions in-game (cast a spell, loot an item, open a window, etc.) and matching
them against the log afterward.

Background/full context: `F:\EQProject\docs\re_progress.md`, sections 2b/2c/6.

## What it does

The plugin detours two client functions:

- `CPacketScrambler::ntoh` — called on every **inbound** packet, immediately
  after the client decrypts/decompresses it off the wire.
- `CPacketScrambler::hton` — called on every **outbound** packet, immediately
  before it goes on the wire.

Both functions byte-swap the opcode as their whole job, which makes them the
narrowest possible choke point for observing every opcode the client sends or
receives — no need to separately understand EQ's packet compression or
encryption.

In addition to the raw opcode stream, the plugin writes four correlated logs so
opcodes can actually be tied to what you were doing (added 2026-07-31 after
play-testing showed the opcode log alone wasn't correlatable):

| Log file | What it captures | Written from |
|---|---|---|
| `PacketLogger_Opcodes.csv` | Every inbound/outbound opcode (the raw firehose). | ntoh/hton detours |
| `PacketLogger_Context.csv` | Continuous state (zone, position, target, combat, casting) sampled ~4×/sec. | OnPulse (250ms) |
| `PacketLogger_Actions.csv` | **Discrete, edge-triggered player actions** — cast start/end, sit/stand/duck, combat enter/leave, target change, loot open/close, jump, and observed NPC casts. A row is written the instant a value *changes*, not on a timer. | OnPulse (50ms edge-detect) |
| `PacketLogger_CharSnapshot.csv` | **One-time character identity dump per zone-in** — name, class, level, race, deity, AA totals, all trained skills, full inventory (every non-empty slot), and keyrings (on clients that have them). | OnZoned |
| `PacketLogger_Spawns.csv` | **NPC/PC spawn + despawn events** — name, spawn id, type, level, class, race, HP, pet master id, casting spell id, position. | OnAddSpawn/OnRemoveSpawn |

All five share the same `unix_ms` epoch, so offline analysis is a
nearest-timestamp join: find the action/snapshot/spawn row nearest an opcode's
timestamp to reconstruct what was happening when that opcode fired. The
`PacketLogger_Actions.csv` log is the one that fixes "I can't tell if I'm
casting/sitting/jumping from the opcodes" — it gives a precise, semantically
tagged timestamp for each discrete action to line opcodes up against.

**On NPC abilities/spells:** the client holds no roster of what an NPC *can*
cast — spell/skill lists exist only for the local player (`PcProfile`), never
for remote spawns. An NPC's kit is server-authoritative and only visible at the
moment it casts. So `PacketLogger_Actions.csv` infers NPC abilities behaviorally:
it edge-detects each spawn's live `CastingData.SpellID` and logs
`npc_cast_start`/`npc_cast_end` rows tagged with the caster's spawn id. Over
repeated play this accumulates a real "this NPC was seen casting this spell"
history — the only client-side way to build NPC ability data.

## What it does NOT do

- **No payload capture.** Only the opcode integer is logged — not packet
  length or contents. This plugin builds an *opcode inventory* (which numeric
  values exist, how often, and — if you note what you were doing at the
  time — what they probably correspond to), not packet struct layouts.
- **No live UI.** This is a minimal first cut: flat CSV file only, no ImGui
  inspector window, no database sink. (Planned follow-up, see
  `re_progress.md` section 6.)
- **Signature not independently re-verified.** The `ntoh`/`hton` call
  signature (`int fn(int)`, free-function-style trampoline) was carried over
  from an old, disabled MQ2Main detour (`MQDetourAPI.cpp`,
  `CPacketScrambler_Detours`, `#if 0`'d out) written against a much older
  client build. The plugin has been build-tested but **not yet run against
  the live client** — if it fails to load or crashes on `/plugin
  MQ2PacketLogger load`, the calling convention/argument width of `ntoh`/
  `hton` is the first thing to suspect.

## Requirements

- MacroQuest built from `F:\MQ2Dev\RedGuidesOpenVanilla` (the `RedGuides
  OpenVanilla` fork — its `eqlib` submodule is a confirmed **exact** offset
  match for our EQ client build; other forks/upstream will NOT have correct
  offsets for `CPacketScrambler::ntoh`/`::hton`).
- A MacroQuest install with `MQ2Main.dll` and this plugin's DLL in the same
  `plugins\` directory (standard MQ2 layout).

## Installation

Copy both files from the build output into the target MacroQuest install's
`plugins\` folder:

```
build\bin\release\plugins\MQ2PacketLogger.dll
build\bin\release\plugins\MQ2PacketLogger.pdb   (optional, for crash debugging)
```

Example (dev machine, live MQ2 install):

```
copy build\bin\release\plugins\MQ2PacketLogger.dll  F:\MacroQuest2\plugins\
copy build\bin\release\plugins\MQ2PacketLogger.pdb  F:\MacroQuest2\plugins\
```

On a test machine, drop both files into that machine's own MacroQuest
`plugins\` folder the same way (e.g. `C:\MQ2Log\plugins\` if MacroQuest itself
is installed under `C:\MQ2Log`, or wherever the test MQ2 install's `plugins`
folder actually is — the DLL must sit next to `MQ2Main.dll`, not in an
arbitrary folder).

## Operation

1. Launch EverQuest and inject MacroQuest as usual.
2. In an in-game or MQ2 console window, run:
   ```
   /plugin MQ2PacketLogger load
   ```
3. You should see a chat message naming all the log files it writes
   (opcodes, context, actions, char snapshot, spawns) under `Logs\`.
   If instead you see a red warning about `CPacketScrambler__ntoh` or
   `__hton` offset being null, the eqlib offsets don't match this client
   build — stop and re-check the eqlib submodule/client version before
   trusting any output (see "Requirements" above).
4. Play normally, and deliberately perform a variety of actions you want to
   identify the opcodes for: cast a spell, loot a corpse, open the spellbook,
   send a tell, zone, group up, etc. Note roughly when you did each action
   (wall-clock time) so you can correlate it against the log's millisecond
   timestamps afterward.
5. When done, either `/plugin MQ2PacketLogger unload` or just exit — the
   plugin flushes and closes the log cleanly in `ShutdownPlugin`.

## Output

CSV file at `<MacroQuest install>\Logs\PacketLogger_Opcodes.csv`
(MacroQuest's standard logs directory — same convention other MQ logging
plugins use). The file is **opened in append mode**, so it accumulates
history across sessions/relogs rather than being overwritten — opcode
coverage builds up over multiple play sessions. Delete it manually if you
want to start fresh.

Format:

```
# ---- MQ2PacketLogger session start ----
unix_ms,direction,opcode_hex,opcode_dec
1769649023123,in,0x0042,66
1769649023456,out,0x00A1,161
# ---- MQ2PacketLogger session end ----
```

| Column | Meaning |
|---|---|
| `unix_ms` | Wall-clock timestamp (ms since Unix epoch) of the log write, i.e. right after the opcode was un/scrambled. |
| `direction` | `in` (server → client, via `ntoh`) or `out` (client → server, via `hton`). |
| `opcode_hex` | Opcode as 4-digit hex, masked to 16 bits (`0xHHHH`) — matches the format EQEmu's `patch_*.conf` opcode tables use. |
| `opcode_dec` | Same opcode as a plain decimal integer, for quick eyeballing of small/special values. |

Every write is flushed immediately (correctness over throughput) — safe to
tail the file live while playing:

```
Get-Content -Path "Logs\PacketLogger_Opcodes.csv" -Wait -Tail 20
```

## Troubleshooting

- **Plugin won't load / immediately crashes the client**: most likely the
  `ntoh`/`hton` signature assumption is wrong for this client build (see
  "What it does NOT do" above). Check `Logs\` for a MacroQuest crash dump and
  compare the crash address against `CPacketScrambler__ntoh`/`__hton` in
  `eqlib`.
- **"offset is null" warning on load**: the `eqlib` submodule doesn't match
  the client build actually running. Confirm the client's embedded build
  date (visible in `eqgame.h` as `__ClientDate`/`__ExpectedVersionDate`,
  or by checking the literal ASCII build-date string in `eqgame.exe`) matches
  the `eqlib` submodule's tracked client date.
- **No file appears in `Logs\`**: confirm the plugin actually loaded (check
  for the yellow "MQ2PacketLogger loaded" chat message) and that the
  MacroQuest process has write access to its own `Logs\` folder.
- **File grows huge / lots of noise**: expected for busy play sessions — this
  is a raw firehose by design (every opcode, no filtering). Use the
  timestamp column to narrow down to the time window you care about.

## Known limitations / next steps

See `F:\EQProject\docs\re_progress.md` section 6 for the fuller roadmap.
Short version:

- Payload/struct capture needs a second hook further down the dispatch path
  (not yet found/implemented).
- No live in-game inspector (ImGuiWindowBase-based window) yet — CSV only.
- No database sink (SQLite/MySQL) yet — flat file only.
