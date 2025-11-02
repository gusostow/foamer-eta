#!/usr/bin/env python3
"""
Build script to embed JSON config profile into firmware.
Reads config from firmware/profiles/{PROFILE}.json and generates src/config_data.h
"""

import json
import os
import sys
from pathlib import Path
from functools import partial

log = partial(print, file=sys.stderr)

profile = os.environ.get("CONFIG_PROFILE", "dev")

# Get project directory - __file__ is not available when run by PlatformIO
try:
    project_dir = Path(__file__).parent.parent
except NameError:
    # When run by PlatformIO, cwd is the project directory
    project_dir = Path.cwd()

profiles_dir = project_dir.parent / "profiles"

config_file = (profiles_dir / profile).with_suffix(".json")
output_file = project_dir / "src" / "config_data.h"

log(f"embedding config profile: {profile}")
log(f"config file: {config_file}")

# check if config file exists
if not config_file.is_file():
    log(f"ERROR: Config file not found: {config_file}")
    log(f"available profiles in {profiles_dir}:")
    profiles = [f[:-5] for f in os.listdir(profiles_dir) if f.endswith(".json")]
    for p in profiles_dir.glob("*"):
        p = p.with_suffix("")
        log(f"  - {p}")
    sys.exit(1)

with config_file.open("r") as f:
    config_data = json.load(f)

# re-serialize to ensure valid JSON
config_json = json.dumps(config_data)

# Generate header file with embedded JSON
header_content = f"""// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from profile: {profile}
// Source: {config_file}

#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

const char* CONFIG_JSON = R"({config_json})";

#endif // CONFIG_DATA_H
"""

# Write header file
with output_file.open("w") as f:
    f.write(header_content)
log(f"generated: {output_file}")
