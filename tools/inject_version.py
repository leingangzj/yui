"""Pre-build hook: stamp the firmware with the real git-derived version.

Runs `git describe --tags --dirty --always` from the project root and:
  • adds `-DYUI_VERSION="<v>"` so main.cpp's kVersion reflects reality
  • writes the same string to `$BUILD_DIR/yui_version.txt` so the
    post-build push can name the artifact accordingly

Wired in via `extra_scripts = pre:tools/inject_version.py` per env.
"""

import subprocess
from pathlib import Path

Import("env")  # type: ignore


def _resolve_version() -> str:
    # SCons exec()'s pre-scripts without setting __file__, so lean on
    # PlatformIO's PROJECT_DIR instead.
    repo = Path(env.subst("$PROJECT_DIR"))
    try:
        v = subprocess.check_output(
            ["git", "-C", str(repo),
             "describe", "--tags", "--dirty", "--always"],
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        return v or "dev"
    except Exception:
        return "dev"


version = _resolve_version()
env.Append(CPPDEFINES=[("YUI_VERSION", env.StringifyMacro(version))])

build_dir = Path(env.subst("$BUILD_DIR"))
build_dir.mkdir(parents=True, exist_ok=True)
(build_dir / "yui_version.txt").write_text(version)

print(f"[version] firmware tagged {version}")
