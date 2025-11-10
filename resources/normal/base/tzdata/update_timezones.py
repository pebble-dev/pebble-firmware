#!/usr/bin/env python
# SPDX-FileCopyrightText: 2024 Google LLC
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2025 Joshua Wise
# SPDX-License-Identifier: GPL-3.0-or-later


import os
import tempfile

import sh

TZDATA_DIR = os.path.dirname(os.path.abspath(__file__))

tmp_dir = tempfile.mkdtemp()
os.chdir(tmp_dir)

print("Downloading timezone data to {}".format(tmp_dir))

sh.wget("https://ftp.iana.org/tz/tzdata-latest.tar.gz")

print("Download complete")
print(sh.ls("-la", "tzdata-latest.tar.gz").strip())

sh.tar("-xvzf", "tzdata-latest.tar.gz")

tz_file = os.path.join(TZDATA_DIR, "timezones_olson.txt")

# backward goes last so we can just always do backreferences for links
sh.awk(
    "-v", "DATAFORM=rearguard",
    "-v", "PACKRATDATA=",
    "-v", "PACKRATLIST=",
    "-f", "ziguard.awk",
    "africa",
    "antarctica",
    "asia",
    "australasia",
    "europe",
    "etcetera",
    "northamerica",
    "southamerica",
    "backward",
    _out=tz_file,
)
print("Updated database written to {}".format(tz_file))
