"""Build the release zip for ShroudMenu.

Takes the built .asi, the release .ini and the release readme and zips them
with the files at the archive root, the way manual installs and Vortex expect:

  python tools/make_release.py

Run build.bat first. The version comes from kVersion in src/script.cpp.
"""

import argparse
import re
import shutil
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def version_of(script: Path) -> str:
    m = re.search(r'kVersion\s*=\s*"([^"]+)"', script.read_text(encoding="utf-8"))
    if not m:
        raise SystemExit("could not find kVersion in src/script.cpp")
    return m.group(1)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=ROOT / "release")
    args = ap.parse_args(argv)

    version = version_of(ROOT / "src" / "script.cpp")
    asi = ROOT / "build" / "ShroudMenu.asi"
    ini = ROOT / "dist" / "ShroudMenu.ini"
    readme = ROOT / "dist" / "README_release.txt"

    problems = [f"missing {f}" for f in (asi, ini, readme) if not f.exists()]
    ini_text = ini.read_text(encoding="utf-8-sig") if ini.exists() else ""
    if re.search(r"^LogLevel\s*=\s*2", ini_text, re.M):
        problems.append("dist/ShroudMenu.ini has LogLevel=2 (debug logging) - set it to 1 for a release")
    if version not in ini_text.splitlines()[0]:
        problems.append(f"first line of dist/ShroudMenu.ini does not mention version {version}")
    if version not in readme.read_text(encoding="utf-8").splitlines()[0]:
        problems.append(f"first line of dist/README_release.txt does not mention version {version}")
    if problems:
        print("\n".join(problems), file=sys.stderr)
        return 1

    name = f"ShroudMenu-{version}"
    staging = args.out / name
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    shutil.copy2(asi, staging / asi.name)
    shutil.copy2(ini, staging / ini.name)
    shutil.copy2(readme, staging / "README.txt")

    archive = args.out / f"{name}.zip"
    if archive.exists():
        archive.unlink()
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(staging.rglob("*")):
            if f.is_file():
                z.write(f, f.relative_to(staging))

    print(f"{archive}  ({archive.stat().st_size / 1e3:.0f} KB), version {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
