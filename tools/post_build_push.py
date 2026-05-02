"""PlatformIO post-build hook: ship the firmware to Zac's MacBook.

Triggered after a successful link of env:cardputer_adv. Pushes the merged
firmware.bin + a SHA256SUMS line to the Mac at ~/yui-builds/ via scp,
routed through the R620 (this CT can't reach the MBP's subnet directly).

Set YUI_NO_PUSH=1 in the env to skip — useful when iterating fast.
"""

import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

Import("env")  # type: ignore  # PlatformIO injects this


# ─── Config ────────────────────────────────────────────────────────────────
JUMP_HOST   = "192.168.5.1"     # R620 (has LAN visibility to 192.168.4/24)
JUMP_USER   = "root"
JUMP_PASS   = "packard00"
MAC_HOST    = "192.168.4.31"
MAC_USER    = "zac"
MAC_PASS    = "packard00"
MAC_FOLDER  = "/Users/zac/Desktop/yui-builds"


def _git_short_sha() -> str:
    # Resolve from the project root (one up from this script's tools/ dir)
    # so PlatformIO's working directory doesn't trip the lookup.
    repo = Path(__file__).resolve().parent.parent
    try:
        return subprocess.check_output(
            ["git", "-C", str(repo), "rev-parse", "--short", "HEAD"],
            stderr=subprocess.DEVNULL,
        ).decode().strip() or "nogit"
    except Exception:
        return "nogit"


def _run(cmd: list[str]) -> tuple[int, str]:
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        return proc.returncode, (proc.stdout + proc.stderr).strip()
    except Exception as exc:
        return 99, repr(exc)


def push_to_mac(source, target, env):
    if os.environ.get("YUI_NO_PUSH"):
        print("[push] YUI_NO_PUSH set — skipping Mac upload")
        return

    if not shutil.which("sshpass"):
        print("[push] sshpass not in PATH — skipping (install with apt)")
        return

    bin_path = Path(str(target[0]))
    if not bin_path.exists():
        print(f"[push] {bin_path} missing — skipping")
        return

    sha = _git_short_sha()
    stamp = time.strftime("%Y%m%d-%H%M%S")
    versioned = f"yui-{stamp}-{sha}.bin"

    # Stage payload locally first.
    stage = Path("/tmp/yui-push")
    stage.mkdir(exist_ok=True)
    for old in stage.glob("*"):
        old.unlink()
    shutil.copy2(bin_path, stage / "firmware.bin")
    shutil.copy2(bin_path, stage / versioned)
    sha_out = subprocess.check_output(
        ["sha256sum", "firmware.bin", versioned], cwd=stage
    ).decode()
    (stage / "SHA256SUMS").write_text(sha_out)

    # Make sure the target folder exists on the Mac (idempotent).
    mkdir_remote = (
        f"sshpass -p '{MAC_PASS}' ssh -o StrictHostKeyChecking=no "
        f"-o ConnectTimeout=5 {MAC_USER}@{MAC_HOST} "
        f"'mkdir -p {MAC_FOLDER}'"
    )
    rc, out = _run([
        "sshpass", "-p", JUMP_PASS,
        "ssh", "-o", "StrictHostKeyChecking=no", "-o", "ConnectTimeout=5",
        f"{JUMP_USER}@{JUMP_HOST}", mkdir_remote,
    ])
    if rc != 0:
        print(f"[push] mkdir on Mac failed (rc={rc}): {out}")
        return

    # rsync the staged dir through the jump host. -e tunnels the inner ssh
    # via sshpass so we don't need keys on either leg.
    rsync_remote = (
        f"sshpass -p '{JUMP_PASS}' rsync -az --progress "
        f"-e \"sshpass -p '{MAC_PASS}' ssh -o StrictHostKeyChecking=no\" "
        f"/tmp/yui-push/ {MAC_USER}@{MAC_HOST}:{MAC_FOLDER}/"
    )

    # Run rsync from the jump host (which can both read /tmp/yui-push? — no,
    # /tmp is local to this CT). Easier: scp staged files individually
    # through the jump host using ssh ProxyJump.
    targets = [
        ("firmware.bin", stage / "firmware.bin"),
        (versioned,      stage / versioned),
        ("SHA256SUMS",   stage / "SHA256SUMS"),
    ]
    failures = 0
    for name, path in targets:
        # ProxyCommand: open a TCP tunnel via the jump host for the inner
        # scp. Two-leg sshpass: outer JUMP_PASS authenticates the
        # ProxyCommand, inner MAC_PASS authenticates the destination.
        proxy_cmd = (
            f"sshpass -p '{JUMP_PASS}' ssh -o StrictHostKeyChecking=no "
            f"-W %h:%p {JUMP_USER}@{JUMP_HOST}"
        )
        rc, out = _run([
            "sshpass", "-p", MAC_PASS,
            "scp", "-o", "StrictHostKeyChecking=no", "-o", "ConnectTimeout=8",
            "-o", f"ProxyCommand={proxy_cmd}",
            str(path),
            f"{MAC_USER}@{MAC_HOST}:{MAC_FOLDER}/{name}",
        ])
        if rc != 0:
            failures += 1
            print(f"[push] scp {name} failed (rc={rc}): {out[:200]}")
        else:
            print(f"[push] {name} → {MAC_HOST}:{MAC_FOLDER}/{name}")

    if failures == 0:
        print(f"[push] done · sha={sha} stamp={stamp}")
    else:
        print(f"[push] {failures} file(s) failed — see above")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", push_to_mac)
