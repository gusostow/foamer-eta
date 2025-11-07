#!/usr/bin/env python3
"""
Build script to embed JSON config profile and splash image into firmware.
Reads config from firmware/profiles/{PROFILE}.json and generates src/config_data.h
Processes static/transit-logo.png and generates src/splash.h
"""

import json
import os
import sys
from pathlib import Path
from functools import partial
from PIL import Image

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

# Process splash image
splash_input = project_dir / "static" / "transit-logo.png"
splash_output = project_dir / "src" / "splash.h"

log(f"processing splash image: {splash_input}")

img = Image.open(splash_input)
img = img.convert("RGB")

# Scale to fit 96x48 display (centered)
display_width = 96
display_height = 48

# Calculate scaling to fit the circle into the display with padding
# Leave 2-3px margin on top and bottom, then scale down by 15%
target_height = (display_height - 4) * 0.85  # ~37px with extra margin
scale_factor = target_height / img.height
new_size = (int(img.width * scale_factor), int(img.height * scale_factor))
img = img.resize(new_size, Image.Resampling.LANCZOS)

# Center horizontally and vertically
x_offset = (display_width - new_size[0]) // 2
y_offset = (display_height - new_size[1]) // 2

# Convert to RGB565 and generate C array
pixels = []
for y in range(display_height):
    for x in range(display_width):
        if (
            x < x_offset
            or x >= x_offset + new_size[0]
            or y < y_offset
            or y >= y_offset + new_size[1]
        ):
            # Black padding
            pixels.append(0x0000)
        else:
            r, g, b = img.getpixel((x - x_offset, y - y_offset))
            # Convert to RGB565: 5 bits red, 6 bits green, 5 bits blue
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            pixels.append(rgb565)

# Generate splash header
splash_header = f"""// AUTO-GENERATED FILE - DO NOT EDIT
// Generated from: {splash_input}

#ifndef SPLASH_H
#define SPLASH_H

#include <stdint.h>

const uint16_t SPLASH_WIDTH = {display_width};
const uint16_t SPLASH_HEIGHT = {display_height};

const uint16_t SPLASH_BITMAP[{display_width * display_height}] = {{
"""

# Write pixels in rows of 12 for readability
for i in range(0, len(pixels), 12):
    row = pixels[i : i + 12]
    splash_header += "  " + ", ".join(f"0x{p:04X}" for p in row)
    if i + 12 < len(pixels):
        splash_header += ","
    splash_header += "\n"

splash_header += """};

#endif // SPLASH_H
"""

with splash_output.open("w") as f:
    f.write(splash_header)
log(f"generated: {splash_output}")
