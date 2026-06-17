# build_info.py

from pathlib import Path
from datetime import datetime

Import("env")

ROOT = Path(env["PROJECT_DIR"])

CONFIG_H = ROOT / "include" / "config.h"
VERSION_FILE = ROOT / "version"
OUT_CPP = ROOT / "src" / "build_info.cpp"
OUT_H = ROOT / "include" / "build_info.h"


def read_version():
    if VERSION_FILE.exists():
        return VERSION_FILE.read_text().strip()
    return "0.0.0"


def read_project_name():
    # lightweight parse instead of full C preprocessor evaluation
    # expects: #define ORD_NAME "OpenRideDash"
    if not CONFIG_H.exists():
        return "UnknownProject"

    for line in CONFIG_H.read_text().splitlines():
        line = line.strip()
        if line.startswith("#define ORD_NAME"):
            # crude but reliable for simple macro
            parts = line.split(maxsplit=2)
            if len(parts) == 3:
                return parts[2].strip().strip('"')
    return "UnknownProject"


def read_short_name():
    if not CONFIG_H.exists():
        return "proj"
    for line in CONFIG_H.read_text().splitlines():
        line = line.strip()
        if line.startswith("#define ORD_SHORT_NAME"):
            parts = line.split(maxsplit=2)
            if len(parts) == 3:
                return parts[2].strip().strip('"')
    return "proj"


def write_if_changed(path: Path, content: str):
    if path.exists() and path.read_text() == content:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


timestamp = datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
version = read_version()

project_name = read_project_name()
short_name = read_short_name()

whoami = f'{project_name} v{version}-" STR(BUILDTAG) " b{timestamp}'

header = """\
/* 
    build_info.h 
    Automatically generated file.
*/

#pragma once

extern const char* ord_version;
extern const char* build_timestamp;
extern const char* whoami;
"""

cpp = f"""\
/*
    build_info.cpp
    Automatically generated file.
*/

#include "build_info.h"
#include "config.h"

const char* ord_version = "{version}";
const char* build_timestamp = "{timestamp}";
const char* whoami = "{whoami}";
"""

write_if_changed(OUT_H, header)
write_if_changed(OUT_CPP, cpp)

print(f"[build_info updated]")
