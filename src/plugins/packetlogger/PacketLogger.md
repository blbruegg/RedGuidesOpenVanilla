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

In addition to the raw opcode stream, the plugin writes four correlated kinds
of rows so opcodes can actually be tied to what you were doing (added
2026-07-31 after play-testing showed the opcode log alone wasn't
correlatable). **As of 2026-08-01, all five row kinds are written into a
single unified file, `PacketLogger.csv`**, instead of five separate CSVs —
see "Output" below for the schema. This replaced the previous 5-file layout
so everything can be sorted/filtered by timestamp in one place (Excel, a
script, etc.) without a manual cross-file join.

| `log_type` value | What it captures | Written from |
|---|---|---|
| `opcode` | Every inbound/outbound opcode (the raw firehose). | ntoh/hton detours |
| `context` | Continuous state (zone, position, target, combat, casting) sampled ~4×/sec. | OnPulse (250ms) |
| `action` | **Discrete, edge-triggered player actions** — cast start/end, sit/stand/duck, combat enter/leave, target change, loot open/close, jump, and observed NPC casts. A row is written the instant a value *changes*, not on a timer. | OnPulse (50ms edge-detect) |
| `snapshot` | **One-time character identity dump per zone-in** — name, class, level, race, deity, AA totals, all trained skills, full inventory (every non-empty slot), and keyrings (on clients that have them). | OnZoned |
| `spawn` | **NPC/PC spawn + despawn events** — name, spawn id, type, level, class, race, HP, pet master id, casting spell id, position. | OnAddSpawn/OnRemoveSpawn |

All five share the same `unix_ms` epoch and now live in one timestamp-ordered
file, so offline analysis is just filtering/sorting `PacketLogger.csv` by
`unix_ms` — no manual cross-file join needed. The `action` rows are the ones
that fix "I can't tell if I'm casting/sitting/jumping from the opcodes" — they
give a precise, semantically tagged timestamp for each discrete action to line
opcodes up against.

**On NPC abilities/spells:** the client holds no roster of what an NPC *can*
cast — spell/skill lists exist only for the local player (`PcProfile`), never
for remote spawns. An NPC's kit is server-authoritative and only visible at the
moment it casts. So the `action` rows infer NPC abilities behaviorally:
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
3. You should see a chat message naming the unified log file
   (`Logs\PacketLogger.csv`, covering opcodes, context, actions, char
   snapshot, and spawns) it writes to.
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

Single CSV file at `<MacroQuest install>\Logs\PacketLogger.csv`
(MacroQuest's standard logs directory — same convention other MQ logging
plugins use). All five row kinds (opcode/context/action/spawn/snapshot,
formerly five separate files) are written into this one file, ordered by
write time. The file is **opened in append mode**, so it accumulates history
across sessions/relogs rather than being overwritten. Delete it manually if
you want to start fresh.

Schema: `unix_ms,log_type,a,b,c,d,e,f,g,h,detail`

- `unix_ms` — wall-clock timestamp (ms since Unix epoch) of the write. Shared
  across all row kinds, so sorting/filtering the whole file by this column
  interleaves opcodes with actions/context/spawns/snapshot chronologically.
- `log_type` — one of `opcode`, `context`, `action`, `spawn`, `snapshot`.
  Determines how the rest of the row is populated.
- `a`..`h` — 8 generic, positionally-reused columns. Each `log_type` packs its
  most structured/uniform fields into these left-to-right; unused columns are
  blank for a given row.
- `detail` — trailing free-form `key=value key2=value2 ...` blob for
  whatever didn't fit in `a`..`h` (and the naturally ragged shapes — action
  detail text, snapshot section/key/value). Always CSV-quoted as a whole
  field, since it may embed an already-quoted sub-value (an item/spawn name
  containing a comma, for instance).

Per-`log_type` column packing:

| `log_type` | a | b | c | d | e | f | g | h | detail |
|---|---|---|---|---|---|---|---|---|---|
| `opcode` | direction (`in`/`out`) | opcode_hex | opcode_dec | | | | | | *(unused)* |
| `context` | game_state | zone_short | player_x | player_y | player_z | player_heading | | | `stand_state=.. in_combat=.. casting_spell_id=.. target_id=.. target_type=.. target_name=.. target_distance=..` |
| `action` | event_type | | | | | | | | free-form detail text (unchanged from the old per-file action log) |
| `spawn` | event (`spawn`/`despawn`) | spawn_id | name | type | level | x | y | z | `class_id=.. race_id=.. hp_cur=.. hp_max=.. master_id=.. casting_spell_id=..` |
| `snapshot` | section | key | | | | | | | `value=..` |

Example:

```
# ---- MQ2PacketLogger session start ----
unix_ms,log_type,a,b,c,d,e,f,g,h,detail
1769649023123,opcode,in,0x0042,66,,,,,,
1769649023200,action,cast_start,,,,,,,,"spell_id=1234 name=""Minor Heal"" actor=self"
1769649023456,opcode,out,0x00A1,161,,,,,,
1769649023500,context,3,soldunga,102.30,-45.10,12.00,180.00,,,"stand_state=0 in_combat=1 casting_spell_id=1234 target_id=501 target_type=0 target_name=""a_skeleton"" target_distance=15.20"
# ---- MQ2PacketLogger session end ----
```

Why this schema (not one of the alternatives): a single wide fixed-column
schema with a named column per field across all five logs was considered and
rejected — the snapshot log alone has ~5 genuinely different row shapes
(char header, AA totals, skills, inventory slots, keyring entries) with no
natural columnar alignment against the other four logs' fields, so most of
any given row would be blank regardless. Reusing 8 generic positional columns
plus one trailing `detail` blob keeps the common, already-columnar rows
(opcode/spawn/context) genuinely readable as real spreadsheet columns while
still accommodating the ragged shapes (action, snapshot) without losing data.

Every write is flushed immediately (correctness over throughput) — safe to
tail the file live while playing:

```
Get-Content -Path "Logs\PacketLogger.csv" -Wait -Tail 20
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
