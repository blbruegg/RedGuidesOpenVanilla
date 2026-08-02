#!/usr/bin/env python3
"""
rebuild_all.py - "patch day" rebuild script for RedGuidesOpenVanilla + friends.

Manual script. The user runs this themselves after a game patch (or whenever
they want to refresh from upstream and rebuild). It does NOT set up any
scheduled/automatic triggering.

What it does, in order:
  1. Discovers every git submodule under this repo that has an "upstream"
     remote configured (checked dynamically via `git submodule status` +
     `git remote` per submodule -- NOT hardcoded, so new submodules that get
     an upstream remote later are picked up automatically), plus the
     standalone `rgmercs` clone and this repo itself (RedGuidesOpenVanilla's
     own "upstream" = RedGuides/openvanilla). For each: fetch upstream, and
     fast-forward the local branch if that's a clean, safe fast-forward.
     If the branches have diverged, do NOT auto-merge -- just report how many
     new commits are available and tell the user to review/merge manually.
  2. Rebuilds the working plugin set (the confirmed-working plugin targets)
     via MSBuild, run inside a single continuous vcvarsall-sourced dev
     environment invocation -- this project's documented working build
     pattern on this machine (see F:\\EQProject\\docs\\re_progress.md,
     sections 9 / 25.3 / 26). A bare `cmake --preset` invocation from a
     plain shell has repeatedly failed on this machine because the compiler
     environment isn't sourced; MSBuild against the already-configured
     `build\\` tree, launched from inside one continuous
     vcvarsall-sourced batch process, is what has actually worked.
  3. Deploys the resulting DLL+PDB pairs, and rgmercs' Lua directory, to
     every MQ2 install location that's actually present on this machine
     (checks C:\\MacroQuest2, F:\\MacroQuest2, F:\\MacroQuest2_RoF2).
  4. Prints a clear, structured summary: what was updated from upstream,
     what built, what failed (with error detail), what was deployed where.

Usage:
    python rebuild_all.py             # do everything
    python rebuild_all.py --no-fetch  # skip the upstream-sync step
    python rebuild_all.py --no-build  # skip the build step
    python rebuild_all.py --no-deploy # skip the deploy step

Never a .ps1 file -- this project has a standing rule against PowerShell
script files (they trip the user's antivirus). This script only ever shells
out to git.exe, cmd.exe (to source vcvarsall.bat), and msbuild.exe.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parent
BUILD_DIR = REPO_ROOT / "build"

# Separate, non-submodule repo (see Part 2 of the porting session).
RGMERCS_DIR = Path(r"F:\EQProject\re\rgmercs")

# VS2026 (v18) on this machine; see re_progress.md section 9 for why this is
# hardcoded rather than relying on PATH/vcvars being pre-sourced.
VCVARSALL = Path(
    r"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

# The plugin targets known to build cleanly as of the forum-plugins-2026-08-02
# session. Each entry is (display name, path to .vcxproj relative to BUILD_DIR).
# NOTE: unlike the upstream-remote discovery below, this list IS hardcoded --
# there is no reliable way to auto-discover "which plugins are expected to
# build cleanly" from the tree alone (many sourced-but-not-ported plugins sit
# unregistered in src/plugins/ on purpose, see re_progress.md section 26.3).
# Update this list by hand as more plugins get ported/confirmed.
PLUGIN_TARGETS: list[tuple[str, str]] = [
    ("MQ2ChatEvents", r"src\plugins\chatevents\MQ2ChatEvents.vcxproj"),
    ("MQ2GemTimer", r"src\plugins\gemtimer\MQ2GemTimer.vcxproj"),
    ("MQ2Pop", r"src\plugins\pop\MQ2Pop.vcxproj"),
    ("MQ2CharNotes", r"src\plugins\charnotes\MQ2CharNotes.vcxproj"),
    ("MQ2RemoteCamp", r"src\plugins\remotecamp\MQ2RemoteCamp.vcxproj"),
    ("MQ2ScreenShot", r"src\plugins\screenshot\MQ2ScreenShot.vcxproj"),
    ("MQ2AutoAccept", r"src\plugins\autoaccept\MQ2AutoAccept.vcxproj"),
    ("MQ2LinkDB", r"src\plugins\linkdb_current\MQ2LinkDB.vcxproj"),
]

MQ2_INSTALL_ROOTS = [
    Path(r"C:\MacroQuest2"),
    Path(r"F:\MacroQuest2"),
    Path(r"F:\MacroQuest2_RoF2"),
]

BUILT_PLUGINS_DIR = BUILD_DIR / "bin" / "release" / "plugins"


# ---------------------------------------------------------------------------
# Small helpers
# ---------------------------------------------------------------------------


def run(cmd: list[str], cwd: Path | None = None, timeout: int = 120) -> subprocess.CompletedProcess:
    """Run a command, always capturing output, never raising on nonzero exit."""
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def git(args: list[str], cwd: Path, timeout: int = 120) -> subprocess.CompletedProcess:
    return run(["git", *args], cwd=cwd, timeout=timeout)


def section(title: str) -> None:
    print()
    print("=" * 78)
    print(title)
    print("=" * 78)


# ---------------------------------------------------------------------------
# Part 1: upstream sync
# ---------------------------------------------------------------------------


@dataclass
class SyncResult:
    name: str
    path: Path
    ok: bool
    detail: str
    fast_forwarded: bool = False
    diverged_ahead: int = 0  # commits upstream has that local doesn't
    diverged_behind: int = 0  # commits local has that upstream doesn't


def repo_has_remote(repo_path: Path, remote_name: str) -> bool:
    result = git(["remote"], cwd=repo_path)
    return remote_name in result.stdout.split()


def discover_submodules_with_upstream(repo_root: Path) -> list[tuple[str, Path]]:
    """Return (name, absolute path) for every submodule that has an
    'upstream' remote configured. Discovered dynamically from
    `git submodule status`, not hardcoded, so newly-added submodules with
    an upstream remote are picked up automatically."""
    result = git(["submodule", "status"], cwd=repo_root)
    found: list[tuple[str, Path]] = []
    if result.returncode != 0:
        print(f"  ! could not enumerate submodules: {result.stderr.strip()}")
        return found

    for line in result.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        # Format: [+-U ]<sha> <path> (<describe>)
        parts = line.split()
        if len(parts) < 2:
            continue
        sub_path_str = parts[1]
        sub_path = (repo_root / sub_path_str).resolve()
        if not sub_path.is_dir():
            continue
        if repo_has_remote(sub_path, "upstream"):
            found.append((sub_path_str, sub_path))
    return found


def sync_repo_with_upstream(name: str, repo_path: Path) -> SyncResult:
    """Fetch 'upstream' and fast-forward the current local branch if safe.
    Never merges/rebases across real divergence -- reports it instead."""
    if not repo_path.is_dir():
        return SyncResult(name, repo_path, ok=False, detail="path does not exist")

    if not repo_has_remote(repo_path, "upstream"):
        return SyncResult(name, repo_path, ok=False, detail="no 'upstream' remote configured")

    fetch = git(["fetch", "upstream"], cwd=repo_path, timeout=180)
    if fetch.returncode != 0:
        return SyncResult(name, repo_path, ok=False, detail=f"fetch failed: {fetch.stderr.strip()}")

    branch_result = git(["rev-parse", "--abbrev-ref", "HEAD"], cwd=repo_path)
    local_branch = branch_result.stdout.strip()
    if not local_branch or local_branch == "HEAD":
        return SyncResult(name, repo_path, ok=False, detail="not on a named branch (detached HEAD) -- skipped")

    # Figure out which upstream branch corresponds to local. Try the same
    # name first (most submodules track a branch of the same name), then
    # fall back to upstream's default branch.
    upstream_ref = f"upstream/{local_branch}"
    check = git(["rev-parse", "--verify", upstream_ref], cwd=repo_path)
    if check.returncode != 0:
        head_result = git(["symbolic-ref", "refs/remotes/upstream/HEAD"], cwd=repo_path)
        if head_result.returncode == 0 and head_result.stdout.strip():
            upstream_ref = head_result.stdout.strip().replace("refs/remotes/", "", 1)
        else:
            return SyncResult(
                name, repo_path, ok=False,
                detail=f"no upstream branch matching local '{local_branch}' and no upstream/HEAD default found",
            )

    ahead_behind = git(["rev-list", "--left-right", "--count", f"{upstream_ref}...HEAD"], cwd=repo_path)
    if ahead_behind.returncode != 0:
        return SyncResult(name, repo_path, ok=False, detail=f"rev-list failed: {ahead_behind.stderr.strip()}")

    try:
        new_upstream, new_local = ahead_behind.stdout.split()
        new_upstream, new_local = int(new_upstream), int(new_local)
    except ValueError:
        return SyncResult(name, repo_path, ok=False, detail=f"could not parse rev-list output: {ahead_behind.stdout!r}")

    if new_upstream == 0:
        return SyncResult(name, repo_path, ok=True, detail="already up to date with upstream")

    if new_local > 0:
        # Real divergence -- do not auto-merge.
        return SyncResult(
            name, repo_path, ok=True,
            detail=(f"{new_upstream} new commits available upstream for {name}, "
                    f"but local has {new_local} commit(s) not in upstream -- "
                    f"review and merge manually, not auto-merged"),
            diverged_ahead=new_upstream, diverged_behind=new_local,
        )

    # Clean fast-forward case.
    ff = git(["merge", "--ff-only", upstream_ref], cwd=repo_path)
    if ff.returncode != 0:
        return SyncResult(
            name, repo_path, ok=False,
            detail=(f"{new_upstream} new commits available upstream for {name}, "
                    f"fast-forward attempt failed unexpectedly: {ff.stderr.strip()} "
                    f"-- review and merge manually"),
        )

    return SyncResult(
        name, repo_path, ok=True,
        detail=f"fast-forwarded {new_upstream} new commit(s) from upstream",
        fast_forwarded=True,
    )


def do_upstream_sync() -> list[SyncResult]:
    section("PART 1: Syncing from upstream")
    results: list[SyncResult] = []

    # 1a. The repo itself.
    print(f"\n[{REPO_ROOT.name}] fetching upstream...")
    r = sync_repo_with_upstream(REPO_ROOT.name, REPO_ROOT)
    results.append(r)
    print(f"  -> {r.detail}")

    # 1b. Every submodule with an upstream remote (discovered dynamically).
    submodules = discover_submodules_with_upstream(REPO_ROOT)
    if not submodules:
        print("\n(no submodules with an 'upstream' remote were found)")
    for sub_name, sub_path in submodules:
        print(f"\n[{sub_name}] fetching upstream...")
        r = sync_repo_with_upstream(sub_name, sub_path)
        results.append(r)
        print(f"  -> {r.detail}")

    # 1c. rgmercs (separate clone, not a submodule).
    print(f"\n[rgmercs] fetching upstream...")
    r = sync_repo_with_upstream("rgmercs", RGMERCS_DIR)
    results.append(r)
    print(f"  -> {r.detail}")

    return results


# ---------------------------------------------------------------------------
# Part 2: build
# ---------------------------------------------------------------------------


@dataclass
class BuildResult:
    name: str
    ok: bool
    detail: str
    dll_path: Path | None = None


def build_plugins() -> list[BuildResult]:
    section("PART 2: Rebuilding plugins")

    if not BUILD_DIR.is_dir():
        print(f"! build directory not found at {BUILD_DIR} -- run a CMake configure first.")
        return [BuildResult(name, False, "build/ directory not configured") for name, _ in PLUGIN_TARGETS]

    if not VCVARSALL.is_file():
        print(f"! vcvarsall.bat not found at {VCVARSALL} -- adjust VCVARSALL in this script for this machine.")
        return [BuildResult(name, False, "vcvarsall.bat not found") for name, _ in PLUGIN_TARGETS]

    results: list[BuildResult] = []

    # Build everything inside ONE continuous vcvarsall-sourced cmd.exe
    # invocation -- this is the documented-working pattern on this machine
    # (bare `cmake --preset`/`msbuild` from a plain shell fails to find a
    # working compiler because the VS dev environment isn't sourced).
    batch_path = BUILD_DIR / "_rebuild_all_msbuild.bat"
    log_dir = BUILD_DIR / "_rebuild_all_logs"
    log_dir.mkdir(exist_ok=True)

    lines = [
        "@echo off",
        f'call "{VCVARSALL}" x64',
        "if errorlevel 1 (",
        "  echo VCVARS_FAILED",
        "  exit /b 1",
        ")",
        "echo VCVARS_OK",
        f"cd /d {BUILD_DIR}",
    ]
    for name, rel_vcxproj in PLUGIN_TARGETS:
        log_file = log_dir / f"{name}.log"
        lines.append(f"echo ==BUILD_START {name}==")
        lines.append(
            f'msbuild "{rel_vcxproj}" /p:Configuration=Release /p:Platform=x64 /m '
            f'> "{log_file}" 2>&1'
        )
        lines.append(f"echo BUILD_EXIT_{name}=%errorlevel%")
    batch_path.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"Running MSBuild for {len(PLUGIN_TARGETS)} plugin target(s) "
          f"inside one vcvarsall-sourced process...")
    proc = run(["cmd.exe", "/c", str(batch_path)], cwd=BUILD_DIR, timeout=1800)
    stdout = proc.stdout

    if "VCVARS_FAILED" in stdout or "VCVARS_OK" not in stdout:
        print("! Failed to source the VS developer environment (vcvarsall.bat).")
        print(stdout[-2000:])
        return [BuildResult(name, False, "vcvarsall.bat failed to initialize") for name, _ in PLUGIN_TARGETS]

    for name, rel_vcxproj in PLUGIN_TARGETS:
        exit_marker = f"BUILD_EXIT_{name}="
        exit_code = None
        for line in stdout.splitlines():
            if line.startswith(exit_marker):
                try:
                    exit_code = int(line[len(exit_marker):].strip())
                except ValueError:
                    pass
                break

        log_file = log_dir / f"{name}.log"
        log_text = log_file.read_text(encoding="utf-8", errors="replace") if log_file.is_file() else ""
        succeeded = "Build succeeded." in log_text and exit_code == 0

        dll_path = BUILT_PLUGINS_DIR / f"{name}.dll"
        if succeeded and dll_path.is_file():
            results.append(BuildResult(name, True, "built cleanly (0 errors)", dll_path))
            print(f"  [OK]   {name} -> {dll_path}")
        else:
            # Pull out the real error lines for the summary, not the whole log.
            error_lines = [
                ln for ln in log_text.splitlines()
                if " error " in ln.lower() or ln.strip().lower().startswith("error")
            ]
            detail = "; ".join(error_lines[:5]) if error_lines else (
                f"MSBuild exit code {exit_code}, no DLL produced" if not dll_path.is_file()
                else f"MSBuild exit code {exit_code}"
            )
            results.append(BuildResult(name, False, detail))
            print(f"  [FAIL] {name}: {detail}")

    return results


# ---------------------------------------------------------------------------
# Part 3: deploy
# ---------------------------------------------------------------------------


@dataclass
class DeployResult:
    target: str
    ok: bool
    detail: str


def deploy_files(build_results: list[BuildResult]) -> list[DeployResult]:
    section("PART 3: Deploying")
    results: list[DeployResult] = []

    successful_dlls: list[Path] = [r.dll_path for r in build_results if r.ok and r.dll_path]

    present_roots = [root for root in MQ2_INSTALL_ROOTS if root.is_dir()]
    if not present_roots:
        print("! No MQ2 install locations found among the known candidates:")
        for root in MQ2_INSTALL_ROOTS:
            print(f"    {root}")
        return results

    for root in present_roots:
        plugins_dir = root / "plugins"
        lua_dir = root / "lua"

        # DLLs + PDBs
        if plugins_dir.is_dir():
            copied = 0
            for dll_path in successful_dlls:
                pdb_path = dll_path.with_suffix(".pdb")
                for src in (dll_path, pdb_path):
                    if not src.is_file():
                        continue
                    dest = plugins_dir / src.name
                    try:
                        shutil.copy2(src, dest)
                        copied += 1
                    except OSError as exc:
                        results.append(DeployResult(str(dest), False, f"copy failed: {exc}"))
                        print(f"  [FAIL] {dest}: {exc}")
            if copied:
                results.append(DeployResult(str(plugins_dir), True, f"{copied} file(s) copied"))
                print(f"  [OK]   {plugins_dir}: {copied} file(s) copied")
        else:
            print(f"  (no plugins/ dir under {root}, skipped)")

        # rgmercs Lua directory
        if lua_dir.is_dir() and RGMERCS_DIR.is_dir():
            dest_dir = lua_dir / "rgmercs"
            try:
                if dest_dir.is_dir():
                    shutil.rmtree(dest_dir)
                shutil.copytree(
                    RGMERCS_DIR, dest_dir,
                    ignore=shutil.ignore_patterns(".git", ".git*"),
                )
                results.append(DeployResult(str(dest_dir), True, "rgmercs Lua tree deployed"))
                print(f"  [OK]   {dest_dir}: rgmercs Lua tree deployed")
            except OSError as exc:
                results.append(DeployResult(str(dest_dir), False, f"copy failed: {exc}"))
                print(f"  [FAIL] {dest_dir}: {exc}")
        elif not RGMERCS_DIR.is_dir():
            print(f"  (rgmercs not found at {RGMERCS_DIR}, skipped Lua deploy)")
        else:
            print(f"  (no lua/ dir under {root}, skipped)")

    return results


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------


def print_summary(
    sync_results: list[SyncResult],
    build_results: list[BuildResult],
    deploy_results: list[DeployResult],
) -> None:
    section("SUMMARY")

    print("\n-- Upstream sync --")
    if not sync_results:
        print("  (skipped)")
    for r in sync_results:
        status = "OK" if r.ok else "FAIL"
        print(f"  [{status}] {r.name}: {r.detail}")

    print("\n-- Build --")
    if not build_results:
        print("  (skipped)")
    ok_builds = [r for r in build_results if r.ok]
    fail_builds = [r for r in build_results if not r.ok]
    for r in ok_builds:
        print(f"  [OK]   {r.name}: {r.detail}")
    for r in fail_builds:
        print(f"  [FAIL] {r.name}: {r.detail}")
    if build_results:
        print(f"\n  {len(ok_builds)}/{len(build_results)} plugin target(s) built successfully.")

    print("\n-- Deploy --")
    if not deploy_results:
        print("  (skipped or nothing to deploy)")
    for r in deploy_results:
        status = "OK" if r.ok else "FAIL"
        print(f"  [{status}] {r.target}: {r.detail}")

    print()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--no-fetch", action="store_true", help="skip the upstream-sync step")
    parser.add_argument("--no-build", action="store_true", help="skip the build step")
    parser.add_argument("--no-deploy", action="store_true", help="skip the deploy step")
    args = parser.parse_args()

    sync_results: list[SyncResult] = []
    build_results: list[BuildResult] = []
    deploy_results: list[DeployResult] = []

    if not args.no_fetch:
        sync_results = do_upstream_sync()
    else:
        print("(--no-fetch: skipping upstream sync)")

    if not args.no_build:
        build_results = build_plugins()
    else:
        print("(--no-build: skipping build)")

    if not args.no_deploy:
        deploy_results = deploy_files(build_results if build_results else [
            BuildResult(name, True, "assumed already built (build step skipped)",
                        BUILT_PLUGINS_DIR / f"{name}.dll")
            for name, _ in PLUGIN_TARGETS
            if (BUILT_PLUGINS_DIR / f"{name}.dll").is_file()
        ])
    else:
        print("(--no-deploy: skipping deploy)")

    print_summary(sync_results, build_results, deploy_results)

    any_build_failed = any(not r.ok for r in build_results)
    return 1 if any_build_failed else 0


if __name__ == "__main__":
    sys.exit(main())
