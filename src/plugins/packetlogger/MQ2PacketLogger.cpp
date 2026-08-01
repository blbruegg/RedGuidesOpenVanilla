/*
 * MQ2PacketLogger
 *
 * Research/RE plugin for the EQProject "later-expansion opcode recovery" workstream.
 * See F:\EQProject\docs\re_progress.md (sections 2b/2c/6) for full background.
 *
 * Purpose: log every network opcode the live client sees, correlated with a timestamp,
 * so opcode values can be identified by playing through known actions (cast a spell, loot
 * an item, open a window, etc.) and matching them up against this log afterward. This is
 * the same fundamental technique EQEmu's own oldest opcode tables were built with (originally
 * via ShowEQ) — see re_progress.md section 3.
 *
 * How it works: CPacketScrambler::ntoh / ::hton are the functions the client calls to
 * byte-swap every inbound/outbound opcode immediately after it comes off the wire (post
 * decrypt/decompress) and immediately before it goes on the wire, respectively. This makes
 * them the narrowest possible choke point for observing every opcode the client processes,
 * without needing to separately handle EQ's packet compression/encryption at all.
 *
 * This mirrors a now-disabled detour that used to exist in MQDetourAPI.cpp
 * (class CPacketScrambler_Detours, #if 0'd out) which hooked ntoh for a single specific
 * opcode (EQ_ASSIST) as of a 2022-03-03 client. The offsets (CPacketScrambler__ntoh /
 * CPacketScrambler__hton in eqlib/game/Globals.h) are still tracked and current in this
 * build's eqgame.h, so no new offset-hunting was needed to stand this plugin up.
 *
 * REPO NOTE: this copy of the plugin lives in F:\MQ2Dev\RedGuidesOpenVanilla, a fresh clone
 * of github.com/RedGuides/openvanilla (src/eqlib submodule -> github.com/redguides/eqlib).
 * IMPORTANT — this repo builds against WHICHEVER CLIENT `src/eqlib` IS CHECKED OUT TO. The
 * offsets below were originally verified for the "live" branch (current-era client,
 * F:\EQProject\client\eqgame.exe, build date "Jul 9 2026"). As of 2026-07-30, this project
 * ALSO targets a second, genuinely different client: F:\EQProject\client_rof2_local\
 * everquest_rof2\eqgame.exe, a classic Rain of Fear 2 -era build (PE timestamp 2013-05-11,
 * 8.7MB vs. the live client's 15.1MB — a different binary entirely, not just a patch level).
 * For THAT client, eqlib must be on the `emu-rof2` branch (`git -C src/eqlib checkout
 * emu-rof2`), built as Win32 (not x64) per the repo's own README ("RoF2 (Emulator) builds
 * use the Win32 platform instead of x64"). A build produced with eqlib on `live`/x64 will
 * NOT load correctly against the RoF2 client and vice versa -- these are two separate build
 * outputs from the same source tree, distinguished only by which eqlib branch/platform was
 * selected at configure time. See re_progress.md and the main EQProject README's
 * "MQ2PacketLogger" sections for the current, dated status of both builds.
 *
 * IMPORTANT CAVEATS (read before trusting output):
 *   - ntoh/hton give the *opcode integer only* -- not the packet payload or length. This
 *     plugin can build an opcode inventory (which numeric values exist, roughly how often
 *     seen, and -- if you note what you were doing at the time -- what they probably mean)
 *     but NOT packet struct layouts. That needs a second, not-yet-found hook further down
 *     the dispatch path. See re_progress.md section 2c for status.
 *   - The calling convention/signature of ntoh/hton is not independently re-verified in this
 *     pass -- it's carried over from the old disabled MQDetourAPI.cpp detour, which used a
 *     plain `int ntoh_Detour(int nopcode)` member-style trampoline. If this doesn't compile
 *     or crashes on load, the signature is the first thing to re-check (try free-function,
 *     __fastcall, or a different argument/return width) -- do not assume it's still right
 *     just because the offset resolved.
 *
 * CONTEXT-CAPTURE DESIGN (added 2026-07-30, see re_progress.md for the full writeup):
 *   The user asked for each logged opcode to be correlated with what the character/world
 *   was doing at that moment (zone, position, target, combat/casting state) so opcode
 *   meaning can be inferred from real game state instead of just timing/proximity guesses.
 *
 *   Deliberately NOT done: calling MQ2's TLO/game-state accessors (GetCharInfo(), pTarget,
 *   pLocalPC, etc.) directly from inside the ntoh/hton detour. Three reasons:
 *     1. Some opcodes are extremely high-frequency (keepalives ~every 600ms, but plenty of
 *        movement/combat opcodes fire far more often than that during real play) -- walking
 *        several pointer-chases and formatting a dozen extra CSV fields on every single one
 *        adds real per-packet overhead to a hook sitting directly in the client's send/recv
 *        path, which is exactly the kind of place added latency is most noticeable/riskiest.
 *     2. Thread safety is not established for this hook. ntoh/hton are reached from the
 *        client's own network processing, and nothing here has independently verified it's
 *        always the main/render thread. MQ2's game-state pointers (pLocalPlayer, pTarget,
 *        pLocalPC, ...) are normally only safe to dereference from the main thread; doing so
 *        from an unverified thread context on every packet is a real crash risk, not a
 *        theoretical one.
 *     3. Most of that state (zone, target, position) simply does not change between one
 *        keepalive and the next -- sampling it at packet-hook frequency would mostly just
 *        write the same values thousands of times over.
 *
 *   Instead: the per-packet CSV (PacketLogger_Opcodes.csv) stays exactly as cheap as before
 *   (unix_ms,direction,opcode_hex,opcode_dec -- one branch, one printf, one flush). A SECOND,
 *   separate log (PacketLogger_Context.csv) is written from OnPulse(), MQ2's normal
 *   once-per-frame-ish main-thread callback (confirmed main-thread-safe usage elsewhere in
 *   this codebase, e.g. MQ2Map.cpp's own OnPulse), throttled to roughly 4 times/second
 *   (250ms) -- frequent enough to correlate against short player actions (a cast, a loot,
 *   a zone-in) without spamming a row for every render frame. Both files share the same
 *   unix_ms epoch, so offline analysis just needs a nearest-timestamp join between the two
 *   CSVs to reconstruct "what was happening" around any given opcode. This is option (c)
 *   from the task brief: full context in a separate, lower-frequency stream, correlated by
 *   timestamp, rather than either (a) paying the cost on every hot-path packet or (b) trying
 *   to selectively enrich only some opcode rows (which would require knowing which opcodes
 *   are "notable" -- the very thing this tool exists to help discover).
 *
 * ACTION-EVENT + CHARACTER-SNAPSHOT LOGS (added 2026-07-31, see re_progress.md for the full
 * writeup and the user feedback that prompted it):
 *   Play-testing revealed the two logs above were not enough to correlate opcodes to actions.
 *   The opcode CSV has zero semantic tagging, and the 250ms context CSV only samples *continuous*
 *   state (position/target/combat) -- it can't tell you the instant a discrete, player-initiated
 *   action happened (started a cast, sat down, jumped, opened a loot window, changed target).
 *   Without that, there is nothing precise to line an opcode up against. Two new logs fix this:
 *
 *   1. PacketLogger_Actions.csv -- an EVENT-DRIVEN log. A row is written the instant an
 *      identifiable discrete action is detected, format `unix_ms,event_type,detail`. Because
 *      MQ2's plugin API exposes NO cast-start/sit/jump/loot callbacks (the only callbacks are
 *      OnPulse / OnZoned / OnBeginZone / OnEndZone / OnSetGameState / OnAddSpawn / OnRemoveSpawn /
 *      OnIncomingChat -- verified in include/mq/api/PluginAPI.h), these are detected by
 *      EDGE-DETECTED polling: OnPulse runs a cheap check at a tight interval (~50ms, far tighter
 *      than the 250ms context sampler) that only WRITES A ROW WHEN A TRACKED VALUE CHANGES from
 *      the previous poll -- never once-per-pulse spam. Tracked transitions: cast start/end,
 *      stand-state changes (stand/sit/duck/feign/bind/dead), combat enter/leave, target change,
 *      loot-window open/close, probable jump (upward Z-velocity spike). All reads go through the
 *      same main-thread game-state pointers the context sampler already uses safely -- this log
 *      does NOT touch the packet-hook path, so none of the thread-safety concerns above apply.
 *
 *   2. PacketLogger_CharSnapshot.csv -- a ONE-TIME-PER-ZONE character identity dump. The original
 *      logs captured no character identity at all (class/level/race/AAs/inventory/skills/keyring).
 *      That data is mostly static within a session, so it is written ONCE on each successful
 *      zone-in (OnZoned) rather than polled: a header block (class, level, race, AA totals) plus
 *      one row per non-empty inventory slot, per keyring item, and per trained skill. Uses MQ2's
 *      own higher-level accessors (GetPcProfile(), ItemContainer::VisitItems, ClassInfo[],
 *      GetSpellByID, pSkillMgr/pStringTable) rather than raw memory reads. See the WriteCharSnapshot
 *      function for exactly what is and isn't captured, and why a few things are deferred.
 *
 *   Deliberately NOT captured in the snapshot (documented so it isn't mistaken for an oversight):
 *   the full per-AA enumeration (every purchased AA by id/rank) -- AA *totals* are captured, but
 *   walking pAltAdvManager for every owned ability is a large, higher-risk enumeration that isn't
 *   needed for the immediate opcode-correlation goal; it's the obvious next extension if wanted.
 *   Character "flags/status" beyond class/level/race/AA/anon/PvP is likewise deferred -- most such
 *   flags (achievement/task/keyring-slot flags) live behind their own manager objects and aren't
 *   readily available as a flat readable value.
 *
 * UNIFIED SINGLE-FILE LOG (added 2026-08-01, replacing the 5-file layout above):
 *   User feedback: cross-referencing "what opcode arrived right when I started casting" required
 *   manually opening two separate CSVs and eyeballing nearest timestamps by hand. All 5 logs above
 *   share the same unix_ms epoch already, so the fix is to just write them all into ONE
 *   timestamp-ordered file -- PacketLogger.csv -- instead of 5 separate ones. Sorting/filtering by
 *   time in Excel or a script then trivially interleaves opcodes with actions/context/spawns/snapshot
 *   without any manual join.
 *
 *   SCHEMA: `unix_ms,log_type,a,b,c,d,e,f,g,h,detail`
 *     - unix_ms   : shared epoch, same as before.
 *     - log_type  : one of opcode|context|action|spawn|snapshot -- what shape the rest of the row is.
 *     - a..h      : 8 generic, positionally-reused columns. Each log_type packs its most
 *                   structured/uniform fields into these left-to-right (documented per type just
 *                   below); anything left over, or anything inherently free-form/ragged (the action
 *                   log's detail string, the snapshot's section/key/value triple), goes in the
 *                   trailing `detail` column as a `key=value key2=value2` blob. This keeps the common
 *                   case (opcode/spawn/context rows, which are already columnar) genuinely readable
 *                   as real spreadsheet columns, while still accommodating the two ragged shapes
 *                   (action, snapshot) without inventing 20+ mostly-empty columns that only apply to
 *                   one log_type each. Option (a) from the task brief (one wide fixed schema with
 *                   many empty columns per row) was considered and rejected: the snapshot log alone
 *                   has ~5 genuinely different "shapes" (char header fields, AA totals, skills,
 *                   inventory slots, keyring entries) with no natural columnar alignment between them
 *                   and the other 4 logs' fields -- forcing all of that into fixed named columns would
 *                   leave the vast majority of any given row blank AND still need a catch-all for the
 *                   inventory/skill/keyring key-value pairs. Reusing 8 generic slots + a detail
 *                   column keeps the file narrow enough to actually read while losing zero data.
 *
 *   Per-log_type column packing (empty column = written as blank field, i.e. just a comma):
 *     opcode   : a=direction(in/out)              b=opcode_hex   c=opcode_dec   detail=(unused)
 *     context  : a=game_state   b=zone_short   c=player_x   d=player_y   e=player_z   f=player_heading
 *                detail="stand_state=.. in_combat=.. casting_spell_id=.. target_id=.. target_type=..
 *                        target_name=.. target_distance=.."
 *                (position gets real columns since it's the field most worth sorting/plotting on;
 *                the rest is lower-cardinality/more occasional, so it goes in detail)
 *     action   : a=event_type                      detail=(the same free-form detail string the
 *                                                            action log always wrote, unchanged)
 *     spawn    : a=event(spawn/despawn)  b=spawn_id  c=name  d=type  e=level  f=x  g=y  h=z
 *                detail="class_id=.. race_id=.. hp_cur=.. hp_max=.. master_id=.. casting_spell_id=.."
 *     snapshot : a=section  b=key                   detail="value=.." (value can itself contain a
 *                                                            CSV-escaped quoted sub-value; unchanged
 *                                                            from the old per-row escaping logic)
 *
 *   All 5 call sites now write through one FILE-pointer/mutex pair (s_logFile/s_logMutex) opened once at
 *   plugin load and closed once at unload, replacing the previous 5 separate FILE* statics and their
 *   5 Open/Close function pairs. The _fsopen(path, "a", _SH_DENYWR) pattern is preserved exactly --
 *   see the historical note directly below on why this project uses _fsopen over fopen_s here.
 */

#include <mq/Plugin.h>

#include <eqlib/game/Globals.h>

#include <cstdio>
#include <cmath>
#include <chrono>
#include <mutex>
#include <string>
#include <atomic>
#include <share.h>
#include <unordered_map>
#include <climits>

using namespace mq;

PreSetup("MQ2PacketLogger");

//----------------------------------------------------------------------------
// Unified log file. All five log "shapes" (opcode/context/action/spawn/snapshot) share one FILE*
// and one CSV, so the whole session is a single timestamp-sorted stream -- see the UNIFIED
// SINGLE-FILE LOG note at the top of this file for the schema rationale. One mutex/FILE* pair,
// one Open/Close pair, replacing the previous five.

static FILE* s_logFile = nullptr;
static std::mutex s_logMutex;

// Only ever touched from OnPulse (main thread), so no lock needed for the throttles themselves --
// the mutex above only guards the FILE* against ShutdownPlugin() racing a pulse.
static std::chrono::steady_clock::time_point s_lastContextSample{};
static constexpr auto kContextSampleInterval = std::chrono::milliseconds(250);

// Edge-detector interval. Deliberately much tighter than the 250ms context sampler so short
// actions (a fast cast, a quick sit/stand) aren't missed, but still throttled off raw pulse
// frequency so the check itself is cheap.
static std::chrono::steady_clock::time_point s_lastActionPoll{};
static constexpr auto kActionPollInterval = std::chrono::milliseconds(50);

// Last-seen values for edge detection. Sentinels chosen so the very first poll after login emits
// a baseline row for each (INT_MIN / "impossible" values won't equal any real state).
static int   s_lastStandState   = INT_MIN;
static int   s_lastCastingSpell  = INT_MIN;   // -1 == not casting
static int   s_lastInCombat      = INT_MIN;   // 0/1
static unsigned int s_lastTargetId = 0xFFFFFFFFu;
static int   s_lastLootOpen       = INT_MIN;  // 0/1
static bool  s_wasAirborne        = false;    // for jump edge detection

// Per-spawn last-seen casting spell id, keyed by spawn id, for NPC (and other-player) cast
// edge-detection -- see the NPC casting scan in PollAndLogActions(). Only touched from OnPulse
// (main thread), so no lock needed.
static std::unordered_map<unsigned int, int> s_spawnCastState;

static void OpenLogFile()
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	if (s_logFile)
		return;

	// gPathLogs is MQ's standard logs directory (already an absolute path, e.g.
	// F:\MacroQuest2\Logs) -- same convention other MQ logging plugins/tools use.
	std::string path = std::string(gPathLogs) + "\\PacketLogger.csv";

	// Append mode: we want a running history across sessions/relogs so opcode coverage
	// accumulates over multiple play sessions rather than being wiped each time.
	// _fsopen with _SH_DENYWR (not fopen_s, which defaults to a share mode that blocks
	// concurrent readers) so the file can be tailed/opened read-only by a text editor or
	// `tail`-equivalent while the plugin is still writing to it, instead of only becoming
	// readable after the plugin unloads and closes its handle.
	s_logFile = _fsopen(path.c_str(), "a", _SH_DENYWR);

	if (s_logFile)
	{
		// Header only written once conceptually, but harmless if repeated across appended
		// sessions -- makes each session's data easy to spot when eyeballing the file.
		fprintf(s_logFile, "# ---- MQ2PacketLogger session start ----\n");
		fprintf(s_logFile, "unix_ms,log_type,a,b,c,d,e,f,g,h,detail\n");
		fflush(s_logFile);
	}
}

static void CloseLogFile()
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	if (s_logFile)
	{
		fprintf(s_logFile, "# ---- MQ2PacketLogger session end ----\n");
		fclose(s_logFile);
		s_logFile = nullptr;
	}
}

// Maps the spawn Type byte to a readable label (SPAWN_* from Constants.h).
static const char* SpawnTypeName(int t)
{
	switch (t)
	{
	case SPAWN_PLAYER: return "pc";
	case SPAWN_NPC:    return "npc";
	case SPAWN_CORPSE: return "corpse";
	default:           return "other";
	}
}

// Human-readable name for a StandState value (STANDSTATE_* from EQData.h).
static const char* StandStateName(int s)
{
	switch (s)
	{
	case STANDSTATE_STAND:   return "standing";
	case STANDSTATE_CASTING: return "casting";
	case STANDSTATE_BIND:    return "binding_wound";
	case STANDSTATE_SIT:     return "sitting";
	case STANDSTATE_DUCK:    return "ducking";
	case STANDSTATE_FEIGN:   return "feign_death";
	case STANDSTATE_DEAD:    return "dead";
	default:                 return "unknown";
	}
}

// Escapes a value for safe embedding in a CSV field (wraps in quotes, doubles any embedded
// quotes) -- needed for any free-text field (names, detail blobs) that could contain a comma or
// quote; numeric/enum fields don't need it.
static std::string CsvEscape(const char* value)
{
	if (!value || !*value)
		return std::string();

	std::string out = "\"";
	for (const char* p = value; *p; ++p)
	{
		if (*p == '"')
			out += '"';
		out += *p;
	}
	out += '"';
	return out;
}

static long long NowUnixMs()
{
	auto now = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

// Writes one row into the unified log. a..h are the 8 generic positional columns (pass "" for any
// this log_type doesn't use); detail is the trailing free-form key=value blob (also "" if unused).
// See the UNIFIED SINGLE-FILE LOG note at the top of this file for what each log_type packs into
// a..h vs. detail. Every write is flushed immediately, matching the previous per-log behavior.
//
// detail is always wrapped via CsvEscape before being written, regardless of caller, because it's
// a free-form blob that may embed an already-CsvEscape'd sub-value (e.g. an item/spawn name that
// itself contains a comma or quote) -- without an outer quote+escape pass, an embedded comma would
// silently shift every column after it. a..h are simple positional fields (ids, enums, coordinates,
// or already-escaped names) and are written as-is, matching the original per-file behavior.
static void LogRow(long long unixMs, const char* logType,
	const std::string& a, const std::string& b, const std::string& c, const std::string& d,
	const std::string& e, const std::string& f, const std::string& g, const std::string& h,
	const std::string& detail)
{
	std::lock_guard<std::mutex> lock(s_logMutex);
	if (!s_logFile)
		return;

	std::string safeDetail = detail.empty() ? std::string() : CsvEscape(detail.c_str());

	fprintf(s_logFile, "%lld,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
		unixMs, logType,
		a.c_str(), b.c_str(), c.c_str(), d.c_str(),
		e.c_str(), f.c_str(), g.c_str(), h.c_str(),
		safeDetail.c_str());
	fflush(s_logFile);
}

// Convenience overload for the action log, which only ever uses column a (event_type) + detail.
static void LogAction(const char* eventType, const std::string& detail)
{
	LogRow(NowUnixMs(), "action", eventType, "", "", "", "", "", "", "", detail);
}

// Samples current game/character/target state and writes one row. Called only from
// OnPulse (main thread) -- safe to dereference MQ2's game-state pointers here, unlike from
// the ntoh/hton detour. Every pointer is null-checked since most of this state legitimately
// doesn't exist outside of GAMESTATE_INGAME (character select, loading screens, etc.) and
// we still want a row in those states (game_state alone is useful context), just with the
// character/target fields blank.
static void SampleAndLogContext()
{
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	int gameState = GetGameState();

	const char* zoneShort = "";
	float px = 0.f, py = 0.f, pz = 0.f, pheading = 0.f;
	int standState = -1;
	int inCombat = -1;
	int castingSpellId = -1;

	if (pLocalPC)
		zoneShort = GetShortZone(pLocalPC->zoneId);

	if (pLocalPlayer)
	{
		px = pLocalPlayer->X;
		py = pLocalPlayer->Y;
		pz = pLocalPlayer->Z;
		pheading = pLocalPlayer->Heading;
		standState = pLocalPlayer->StandState;

		// CastingData lives on PlayerClient (any spawn can be casting, not just the local
		// PC), not on PcClient -- confirmed against both the live and emu-rof2 eqlib
		// branches. LaunchSpellData::SpellID is documented "-1 = not casting a spell";
		// logged as-is, no translation needed.
		castingSpellId = pLocalPlayer->CastingData.SpellID;
	}

	if (pLocalPC)
	{
		inCombat = pLocalPC->IsInCombat() ? 1 : 0;
	}

	int targetId = 0;
	int targetType = -1;
	std::string targetName;
	float targetDistance = -1.f;

	if (pTarget)
	{
		targetId = pTarget->SpawnID;
		targetType = pTarget->Type;
		targetName = CsvEscape(pTarget->Name);

		if (pLocalPlayer)
		{
			float dx = pTarget->X - pLocalPlayer->X;
			float dy = pTarget->Y - pLocalPlayer->Y;
			float dz = pTarget->Z - pLocalPlayer->Z;
			targetDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
		}
	}

	// context row: a=game_state b=zone_short c/d/e=x/y/z f=heading; the rest (stand_state,
	// in_combat, casting_spell_id, target_*) is lower-cardinality/occasional, so it goes in
	// detail as key=value pairs -- see the UNIFIED SINGLE-FILE LOG note at the top of the file.
	std::string detail = "stand_state=" + std::to_string(standState)
		+ " in_combat=" + std::to_string(inCombat)
		+ " casting_spell_id=" + std::to_string(castingSpellId)
		+ " target_id=" + std::to_string(targetId)
		+ " target_type=" + std::to_string(targetType)
		+ " target_name=" + targetName
		+ " target_distance=" + std::to_string(targetDistance);

	char xb[32], yb[32], zb[32], hb[32];
	sprintf_s(xb, "%.2f", px);
	sprintf_s(yb, "%.2f", py);
	sprintf_s(zb, "%.2f", pz);
	sprintf_s(hb, "%.2f", pheading);

	LogRow(static_cast<long long>(ms), "context",
		std::to_string(gameState), zoneShort, xb, yb, hb, "", "", "", detail);
}

// Writes one spawn-event row (called from OnAddSpawn/OnRemoveSpawn). Reads only the readily
// available spawn-struct fields -- see the WHAT IS/ISN'T OBSERVABLE note above the spawn log.
static void LogSpawnEvent(const char* event, PlayerClient* pSpawn)
{
	if (!pSpawn)
		return;

	// HP is int64 on the live eqlib branch and int32 on emu-rof2 -- cast to long long so one
	// format works for both builds.
	long long hpCur = static_cast<long long>(pSpawn->HPCurrent);
	long long hpMax = static_cast<long long>(pSpawn->HPMax);

	std::string name = CsvEscape(pSpawn->Name);

	// spawn row: a=event b=spawn_id c=name d=type e=level f/g/h=x/y/z; class/race/hp/master/cast
	// go in detail -- see the UNIFIED SINGLE-FILE LOG note at the top of the file.
	std::string detail = "class_id=" + std::to_string(pSpawn->GetClass())
		+ " race_id=" + std::to_string(pSpawn->GetRace())
		+ " hp_cur=" + std::to_string(hpCur)
		+ " hp_max=" + std::to_string(hpMax)
		+ " master_id=" + std::to_string(pSpawn->MasterID)
		+ " casting_spell_id=" + std::to_string(pSpawn->CastingData.SpellID);

	char xb[32], yb[32], zb[32];
	sprintf_s(xb, "%.2f", pSpawn->X);
	sprintf_s(yb, "%.2f", pSpawn->Y);
	sprintf_s(zb, "%.2f", pSpawn->Z);

	LogRow(NowUnixMs(), "spawn",
		event, std::to_string(pSpawn->SpawnID), name, SpawnTypeName(pSpawn->Type),
		std::to_string(pSpawn->Level), xb, yb, zb, detail);
}

// One helper row for the snapshot's long-format table: a=section b=key, detail="value=..".
static void SnapshotRow(const char* section, const char* key, const std::string& value)
{
	LogRow(NowUnixMs(), "snapshot", section, key, "", "", "", "", "", "", "value=" + value);
}
static void SnapshotRow(const char* section, const char* key, int value)
{
	SnapshotRow(section, key, std::to_string(value));
}

// Writes a one-time character identity snapshot. Called from OnZoned (once per zone-in). Reads via
// MQ2's own higher-level accessors -- GetPcProfile()/ClassInfo[]/ItemContainer::VisitItems/
// GetSpellByID/pSkillMgr -- rather than raw memory. Every pointer is null-checked; if we're not
// fully in-game yet the snapshot is simply skipped (OnZoned can fire slightly before all pointers
// are populated). See the CHARACTER-SNAPSHOT note at the top of the file for what's deferred.
static void WriteCharSnapshot()
{
	// No lock here -- SnapshotRow()/LogRow() each take s_logMutex internally per row (std::mutex
	// is non-recursive, so holding it across the whole function would deadlock on the first
	// SnapshotRow call below). A quick unlocked peek at s_logFile is fine as a cheap early-out;
	// LogRow() re-checks it under the lock on every row anyway.
	if (!s_logFile)
		return;

	PcProfile* pProfile = GetPcProfile();
	if (!pProfile || !pLocalPC || !pLocalPlayer)
		return;

	// ---- identity header -------------------------------------------------
	SnapshotRow("char", "name", CsvEscape(pLocalPlayer->Name));
	SnapshotRow("char", "level", pProfile->Level);

	int classId = pProfile->Class;
	const char* className = (classId >= 0 && classId < (int)(sizeof(ClassInfo) / sizeof(ClassInfo[0])))
		? ClassInfo[classId].LongName : "unknown";
	SnapshotRow("char", "class_id", classId);
	SnapshotRow("char", "class", CsvEscape(className));

	SnapshotRow("char", "race_id", pProfile->Race);
	if (pEverQuest)
		SnapshotRow("char", "race", CsvEscape(pEverQuest->GetRaceDesc((EQRace)pProfile->Race)));

	SnapshotRow("char", "gender", (int)pProfile->Gender);
	SnapshotRow("char", "deity", pLocalPlayer->Deity);

	int shortZone = pLocalPC->zoneId;
	SnapshotRow("char", "zone_id", shortZone);
	SnapshotRow("char", "zone_short", CsvEscape(GetShortZone(shortZone)));

	// ---- AA totals (per-AA enumeration deferred, see header note) --------
	SnapshotRow("aa", "unspent", pProfile->AAPoints);
	SnapshotRow("aa", "spent", pProfile->AAPointsSpent);
	SnapshotRow("aa", "total", pProfile->AAPointsSpent + pProfile->AAPoints);

	// ---- skills (name + trained value, only ones the char actually has) --
	if (pSkillMgr && pStringTable)
	{
		for (int i = 0; i < NUM_SKILLS; ++i)
		{
			int value = pProfile->Skill[i];
			if (value <= 0)
				continue; // untrained/not-applicable -- skip to keep the file readable
			EQ_Skill* pSkill = pSkillMgr->pSkill[i];
			const char* skillName = (pSkill) ? pStringTable->getString(pSkill->nName) : nullptr;
			std::string key = skillName ? skillName : ("skill_" + std::to_string(i));
			SnapshotRow("skill", key.c_str(), value);
		}
	}

	// ---- inventory (one row per non-empty slot) --------------------------
	// VisitItems walks the possessions container to full depth (bags-within-bags). Each visited
	// item gives id / name / stack count / slot location. Best-effort: some exotic nested layouts
	// may not be reached, but the common inventory + worn + bag contents are.
	int itemCount = 0;
	pProfile->InventoryContainer.VisitItems(-1, [&](const ItemPtr& item, const ItemIndex& location)
	{
		if (!item)
			return;
		++itemCount;
		char key[64];
		sprintf_s(key, "slot_%d_%d_%d",
			location.GetSlot(0), location.GetSlot(1), location.GetSlot(2));
		std::string val = CsvEscape(item->GetName());
		val += " (id=" + std::to_string(item->GetID())
			+ ",qty=" + std::to_string(item->GetItemCount()) + ")";
		// val already CSV-quoted from CsvEscape, so re-escape the whole assembled string.
		SnapshotRow("inventory", key, CsvEscape(val.c_str()));
	});
	SnapshotRow("inventory", "count", itemCount);

	// ---- keyrings (mounts/illusions/familiars/hero-forge/teleport/etc.) --
	// The whole keyring subsystem is a newer-client feature. On the RoF2/emu-rof2 client eqlib's
	// GetKeyRingItems() accessor and the individual keyring ItemContainers are commented out
	// entirely (that era's client had no keyring window), so gate the ENTIRE block on
	// HAS_KEYRING_WINDOW -- true on the live client, false on RoF2 -- to keep both builds compiling.
	// Within the block, the activated-item and equipment keyrings are further-newer features gated
	// behind their own HAS_* macros the same way eqlib gates the enum values themselves.
#if HAS_KEYRING_WINDOW
	static const struct { KeyRingType type; const char* name; } kKeyRings[] = {
		{ eMount,            "mount" },
		{ eIllusion,         "illusion" },
		{ eFamiliar,         "familiar" },
		{ eHeroForge,        "heroforge" },
		{ eTeleportationItem,"teleport" },
#if HAS_ACTIVATED_ITEM_KEYRING
		{ eActivatedItem,    "activated" },
#endif
#if HAS_EQUIPMENT_KEYRING
		{ eEquipmentKeyRing, "equipment" },
#endif
	};
	for (const auto& kr : kKeyRings)
	{
		ItemContainer& ring = pLocalPC->GetKeyRingItems(kr.type);
		ring.VisitItems(-1, [&](const ItemPtr& item, const ItemIndex& /*location*/)
		{
			if (!item)
				return;
			std::string val = CsvEscape(item->GetName());
			val += " (id=" + std::to_string(item->GetID()) + ")";
			SnapshotRow("keyring", kr.name, CsvEscape(val.c_str()));
		});
	}
#else
	SnapshotRow("keyring", "note", CsvEscape("keyrings not available on this client (pre-keyring-window era, e.g. RoF2)"));
#endif

	{
		std::lock_guard<std::mutex> lock(s_logMutex);
		if (s_logFile)
		{
			fprintf(s_logFile, "# ---- end snapshot ----\n");
			fflush(s_logFile);
		}
	}

	WriteChatf("\ayMQ2PacketLogger\ax: wrote character snapshot (%s, level %d %s) to "
		"Logs\\PacketLogger.csv", pLocalPlayer->Name, pProfile->Level, className);
}

// Edge-detected action poller. Runs from OnPulse at kActionPollInterval. Reads current game state
// via the same main-thread pointers the context sampler uses, and writes an action row ONLY when a
// tracked value differs from its last-seen value. Cheap on the common no-change path (a few pointer
// derefs and integer compares, no file I/O).
static void PollAndLogActions()
{
	if (!pLocalPlayer || !pLocalPC)
		return;

	// --- stand state (stand/sit/duck/feign/dead/bind) -------------------
	int standState = pLocalPlayer->StandState;
	if (standState != s_lastStandState)
	{
		if (s_lastStandState != INT_MIN) // don't emit the initial baseline as a "transition"
		{
			std::string detail = "from=" + std::string(StandStateName(s_lastStandState))
				+ " to=" + StandStateName(standState);
			LogAction("stand_state", detail);
		}
		s_lastStandState = standState;
	}

	// --- casting (start / end) ------------------------------------------
	int castingSpell = pLocalPlayer->CastingData.SpellID; // -1 = not casting
	if (castingSpell != s_lastCastingSpell)
	{
		if (s_lastCastingSpell != INT_MIN)
		{
			// actor=self tag distinguishes the player's own casts from NPC casts (below).
			const char* actorTag = " actor=self";
			if (castingSpell != -1 && s_lastCastingSpell != -1 && castingSpell != s_lastCastingSpell)
			{
				// cast id changed directly to another cast id (rare): treat as a new start.
				const char* spellName = "";
				if (EQ_Spell* pSpell = GetSpellByID(castingSpell))
					spellName = pSpell->Name;
				LogAction("cast_start", "spell_id=" + std::to_string(castingSpell)
					+ " name=" + CsvEscape(spellName) + actorTag);
			}
			else if (castingSpell != -1)
			{
				// went from not-casting to casting: cast start
				const char* spellName = "";
				if (EQ_Spell* pSpell = GetSpellByID(castingSpell))
					spellName = pSpell->Name;
				LogAction("cast_start", "spell_id=" + std::to_string(castingSpell)
					+ " name=" + CsvEscape(spellName) + actorTag);
			}
			else
			{
				// went from casting to not-casting: cast end (complete OR interrupted --
				// ntoh/hton and the surrounding opcode timing disambiguate which).
				LogAction("cast_end", "spell_id=" + std::to_string(s_lastCastingSpell) + actorTag);
			}
		}
		s_lastCastingSpell = castingSpell;
	}

	// --- combat enter/leave ---------------------------------------------
	int inCombat = pLocalPC->IsInCombat() ? 1 : 0;
	if (inCombat != s_lastInCombat)
	{
		if (s_lastInCombat != INT_MIN)
			LogAction(inCombat ? "combat_enter" : "combat_leave", std::string());
		s_lastInCombat = inCombat;
	}

	// --- target change --------------------------------------------------
	unsigned int targetId = pTarget ? pTarget->SpawnID : 0u;
	if (targetId != s_lastTargetId)
	{
		if (s_lastTargetId != 0xFFFFFFFFu)
		{
			if (targetId == 0)
			{
				LogAction("target_change", "cleared=1");
			}
			else
			{
				std::string detail = "target_id=" + std::to_string(targetId);
				if (pTarget)
				{
					detail += " name=" + CsvEscape(pTarget->Name);
					detail += " type=" + std::to_string(pTarget->Type);
				}
				LogAction("target_change", detail);
			}
		}
		s_lastTargetId = targetId;
	}

	// --- loot window open/close -----------------------------------------
	int lootOpen = (pLootWnd && pLootWnd->IsVisible()) ? 1 : 0;
	if (lootOpen != s_lastLootOpen)
	{
		if (s_lastLootOpen != INT_MIN)
			LogAction(lootOpen ? "loot_open" : "loot_close", std::string());
		s_lastLootOpen = lootOpen;
	}

	// --- jump (heuristic: upward Z-velocity spike, edge-triggered) ------
	// No clean jump callback/flag exists. SpeedZ spiking positive while it wasn't the previous
	// poll is a decent proxy for a jump press. Edge-triggered on the rising edge so a single jump
	// yields one row, not one per airborne pulse. Deliberately a heuristic -- flagged as such in
	// the event detail so downstream analysis knows it's inferred, not authoritative.
	bool airborneUp = pLocalPlayer->SpeedZ > 0.5f;
	if (airborneUp && !s_wasAirborne)
		LogAction("jump", "heuristic=1 speedz=" + std::to_string(pLocalPlayer->SpeedZ));
	s_wasAirborne = airborneUp;

	// --- NPC / other-spawn cast detection (behavioral inference) ---------
	// The client has no roster of what an NPC CAN cast (see the spawn-log note), but it DOES expose
	// each spawn's live CastingData.SpellID. Walk the spawn list and edge-detect casting-state
	// transitions per spawn id -- this accumulates a real "spawn X was observed casting spell Y"
	// history from live play, which is the only way to build NPC ability data client-side.
	// Skips the local player (handled above) and only reports NPCs + other players' casts here.
	if (pSpawnList)
	{
		unsigned int selfId = pLocalPlayer->SpawnID;
		for (PlayerClient* pSpawn = pSpawnList; pSpawn; pSpawn = pSpawn->GetNext())
		{
			unsigned int id = pSpawn->SpawnID;
			if (id == selfId)
				continue;

			int cast = pSpawn->CastingData.SpellID; // -1 = not casting
			auto it = s_spawnCastState.find(id);
			int prev = (it == s_spawnCastState.end()) ? -1 : it->second;

			if (cast != prev)
			{
				if (cast != -1)
				{
					const char* spellName = "";
					if (EQ_Spell* pSpell = GetSpellByID(cast))
						spellName = pSpell->Name;
					std::string detail = "spell_id=" + std::to_string(cast)
						+ " name=" + CsvEscape(spellName)
						+ " actor_id=" + std::to_string(id)
						+ " actor_name=" + CsvEscape(pSpawn->Name)
						+ " actor_type=" + SpawnTypeName(pSpawn->Type);
					LogAction("npc_cast_start", detail);
				}
				else
				{
					std::string detail = "spell_id=" + std::to_string(prev)
						+ " actor_id=" + std::to_string(id)
						+ " actor_name=" + CsvEscape(pSpawn->Name)
						+ " actor_type=" + SpawnTypeName(pSpawn->Type);
					LogAction("npc_cast_end", detail);
				}
				s_spawnCastState[id] = cast;
			}
		}
	}
}

static void LogOpcode(const char* direction, int opcode)
{
	auto now = std::chrono::system_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

	std::lock_guard<std::mutex> lock(s_logMutex);
	if (!s_logFile)
		return;

	// This is the hottest call site in the plugin (every single inbound/outbound packet), so it
	// writes directly with one fprintf rather than going through LogRow()'s string-building --
	// same perf characteristics as the original per-file opcode log. opcode row shape:
	// log_type=opcode a=direction b=opcode_hex c=opcode_dec (d..h/detail left blank).
	// opcode is logged as both hex and decimal for convenience -- EQEmu's patch_*.conf
	// files use hex (0xHHHH), but decimal is sometimes easier to eyeball for small/special
	// values.
	fprintf(s_logFile, "%lld,opcode,%s,0x%04X,%d,,,,,,\n",
		static_cast<long long>(ms), direction, static_cast<unsigned int>(opcode) & 0xFFFF, opcode);

	// Flush every write for now -- correctness/durability over throughput while we're still
	// validating the hook even fires at all. Revisit (buffer + periodic flush on OnPulse)
	// once this is confirmed working and capture volume becomes a concern.
	fflush(s_logFile);
}

//----------------------------------------------------------------------------
// Detours on CPacketScrambler::ntoh / ::hton
//
// Signatures are carried over as-is from the old disabled detour in MQDetourAPI.cpp.

// Forward declaration -- defined after the class below (it needs eqlib::CPacketScrambler__ntoh
// / __hton which are declared via the <eqlib/game/Globals.h> include above), but is called
// from inside the class's __except handlers.
static void OnTrampolineFault(const char* which);

//
// CRASH INVESTIGATION (2026-07-31): this hook crashed the RoF2/emu-rof2 client twice during
// live play-testing (STATUS_ACCESS_VIOLATION, attempted execution at a garbage/heap address --
// confirmed via minidump analysis both times: exception code 0xC0000005, parameter[0]=8 which
// is DEP/no-execute, i.e. an indirect call/jump through a corrupted or invalid function
// pointer). Two things were tried:
//   1. #pragma optimize("", off) around this class, matching a comment on the original
//      (never-shipped) disabled detour in MQDetourAPI.cpp: "ntoh_detour actually climbs into
//      the stack and pulls data out from the caller's stack frame... keep optimizations off
//      for this function or it will break." Applied here, but the SECOND crash still happened
//      after this fix was in place, with the same signature -- so this alone was not
//      sufficient (may still be a real, latent issue described by that comment, but it isn't
//      the whole story for this build).
//   2. Verified CPacketScrambler__ntoh_x / __hton_x (0x7D03B0 / 0x7D03C0 in this eqlib
//      branch) resolve to real, valid, in-bounds addresses inside eqgame.exe at runtime (the
//      hook installs without error, WriteChatf confirms both offsets are non-null, and the
//      plugin DID capture several genuine-looking opcodes across two separate sessions before
//      each crash -- e.g. 0x7A09/31241 -- which are plausible EQ opcode values, not obviously
//      garbage). This means the hook mechanism and offsets are very likely fundamentally
//      correct, and the crash is either (a) the "climbs the stack" issue the original author
//      already knew about, not fully solved by the optimize pragma alone, or (b) a narrower
//      edge case (e.g. a specific opcode value or packet timing) that only manifests some of
//      the time -- both crashes happened after some number of successful captures, not on the
//      very first call in the first session, though the second session crashed with an empty
//      opcode CSV, suggesting it CAN also happen on/near the first call.
//
// SAFETY FIX applied here: wrap the trampoline call in SEH (__try/__except), matching the
// pattern already used elsewhere in this codebase for exactly this kind of defensive
// wrapping (see CrashHandler.cpp, MQ2DeveloperTools.cpp). If the underlying call does fault,
// this catches it, logs a warning once, and permanently disables further packet logging for
// the rest of the session (removing the detour) rather than crashing the client. This is a
// pragmatic containment measure given the exact root cause inside the client's own compiled
// code is not something we can fix (we don't control eqgame.exe) -- the goal is "never take
// down the user's play session," even if that means this plugin sometimes stops capturing
// early. A capped per-session capture is far more useful for RE purposes than a crash.
#pragma optimize("", off)
class CPacketScrambler_Detours
{
public:
	DETOUR_TRAMPOLINE_DEF(int, ntoh_Trampoline, (int))
	int ntoh_Detour(int nopcode)
	{
		int hopcode = nopcode;
		bool ok = true;

		__try
		{
			hopcode = ntoh_Trampoline(nopcode);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ok = false;
		}

		if (!ok)
		{
			OnTrampolineFault("ntoh");
			return nopcode;
		}

		LogOpcode("in", hopcode);
		return hopcode;
	}

	DETOUR_TRAMPOLINE_DEF(int, hton_Trampoline, (int))
	int hton_Detour(int hopcode)
	{
		int nopcode = hopcode;
		bool ok = true;

		__try
		{
			nopcode = hton_Trampoline(hopcode);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			ok = false;
		}

		if (!ok)
		{
			OnTrampolineFault("hton");
			return hopcode;
		}

		LogOpcode("out", hopcode);
		return nopcode;
	}
};
#pragma optimize("", on)

static CPacketScrambler_Detours* s_packetScramblerDetours = nullptr;
static std::atomic<bool> s_disablingAfterFault = false;

// Called from inside the SEH __except handler above (ntoh_Detour/hton_Detour) when the
// underlying trampoline call faults. SEH handler bodies run with the stack already unwound
// past the point of the exception, so it's safe to do normal work here (unlike inside the
// __except filter expression itself). Removes both detours so the plugin stops touching the
// packet path for the rest of this session -- safer than retrying, since whatever caused the
// fault (bad offset resolution for a particular code path, corrupted state, etc.) is likely
// to recur on the next packet too, and repeated SEH recovery on a hot path is itself risky.
static void OnTrampolineFault(const char* which)
{
	bool expected = false;
	if (!s_disablingAfterFault.compare_exchange_strong(expected, true))
		return; // already handled by a concurrent/earlier fault

	WriteChatf("\arMQ2PacketLogger: CPacketScrambler::%s trampoline call faulted -- disabling "
		"packet capture for the rest of this session to avoid crashing the client. See "
		"re_progress.md / README.md for the known-issue writeup.", which);

	if (eqlib::CPacketScrambler__ntoh)
		RemoveDetour(eqlib::CPacketScrambler__ntoh);
	if (eqlib::CPacketScrambler__hton)
		RemoveDetour(eqlib::CPacketScrambler__hton);
}

//----------------------------------------------------------------------------
// Plugin lifecycle

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2PacketLogger");

	OpenLogFile();

	s_packetScramblerDetours = new CPacketScrambler_Detours();

	if (eqlib::CPacketScrambler__ntoh)
	{
		EzDetour(eqlib::CPacketScrambler__ntoh,
			&CPacketScrambler_Detours::ntoh_Detour,
			&CPacketScrambler_Detours::ntoh_Trampoline);
	}
	else
	{
		WriteChatf("\arMQ2PacketLogger: CPacketScrambler__ntoh offset is null, inbound opcode logging disabled.");
	}

	if (eqlib::CPacketScrambler__hton)
	{
		EzDetour(eqlib::CPacketScrambler__hton,
			&CPacketScrambler_Detours::hton_Detour,
			&CPacketScrambler_Detours::hton_Trampoline);
	}
	else
	{
		WriteChatf("\arMQ2PacketLogger: CPacketScrambler__hton offset is null, outbound opcode logging disabled.");
	}

	WriteChatf("\ayMQ2PacketLogger\ax loaded -- opcodes/context/actions/char snapshot/spawns all "
		"-> Logs\\PacketLogger.csv (unified log, log_type column distinguishes row kinds)");

	// If we loaded while already in-game (plugin reload mid-session), grab a snapshot immediately
	// rather than waiting for the next zone. OnZoned won't fire until the next zone otherwise.
	if (GetGameState() == GAMESTATE_INGAME)
		WriteCharSnapshot();
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2PacketLogger");

	if (eqlib::CPacketScrambler__ntoh)
		RemoveDetour(eqlib::CPacketScrambler__ntoh);
	if (eqlib::CPacketScrambler__hton)
		RemoveDetour(eqlib::CPacketScrambler__hton);

	delete s_packetScramblerDetours;
	s_packetScramblerDetours = nullptr;

	CloseLogFile();
}

//----------------------------------------------------------------------------
// Spawn add/remove callbacks. MQ2 hands us a fully-populated PlayerClient* for each spawn as it
// appears/disappears -- log both, and keep the NPC cast-state map trimmed on removal so it doesn't
// grow unbounded over a long session.

PLUGIN_API void OnAddSpawn(PlayerClient* pNewSpawn)
{
	LogSpawnEvent("spawn", pNewSpawn);
}

PLUGIN_API void OnRemoveSpawn(PlayerClient* pSpawn)
{
	LogSpawnEvent("despawn", pSpawn);
	if (pSpawn)
		s_spawnCastState.erase(pSpawn->SpawnID);
}

//----------------------------------------------------------------------------
// OnZoned fires once each time the character finishes zoning in. This is where the one-time
// character snapshot is written (gear/AA/skills may have changed since the last zone). Also reset
// the action edge-detector sentinels so the first poll in the new zone re-baselines cleanly rather
// than emitting a spurious "transition" from the previous zone's last state.

PLUGIN_API void OnZoned()
{
	s_lastStandState = INT_MIN;
	s_lastCastingSpell = INT_MIN;
	s_lastInCombat = INT_MIN;
	s_lastTargetId = 0xFFFFFFFFu;
	s_lastLootOpen = INT_MIN;
	s_wasAirborne = false;
	s_spawnCastState.clear(); // spawn ids don't carry across zones

	WriteCharSnapshot();
}

//----------------------------------------------------------------------------
// Periodic context sampling. OnPulse is MQ2's standard main-thread "tick" callback (called
// repeatedly while in game, roughly once per client frame/pulse) -- safe to touch game-state
// pointers here. Throttled internally so we don't write a context row on every single pulse.

PLUGIN_API void OnPulse()
{
	auto now = std::chrono::steady_clock::now();

	// Action edge-detector runs on a tight interval (50ms) so short discrete actions aren't
	// missed. Cheap on the no-change path (integer compares, no I/O unless something transitioned).
	if (now - s_lastActionPoll >= kActionPollInterval)
	{
		s_lastActionPoll = now;
		PollAndLogActions();
	}

	// Continuous-context sampler runs on the original slower interval (250ms).
	if (now - s_lastContextSample >= kContextSampleInterval)
	{
		s_lastContextSample = now;
		SampleAndLogContext();
	}
}
