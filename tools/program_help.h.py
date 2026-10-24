#!/usr/bin/env python3

import argparse
import os
import io
import json

parser = argparse.ArgumentParser(
	prog = "flare",
	description = "A command-line utility to crop white gutters off images.",
	allow_abbrev = False,
	add_help = False,
	epilog = "Note: options that take a value must use an equal sign (e.g. --input=INPUT)."
)

parser.add_argument(
	"-h",
	"--help",
	required = False,
	action = "store_true",
	help = "Display this help text and exit."
)

parser.add_argument(
	"-v",
	"--version",
	required = False,
	action = "store_true",
	help = "Print version information and exit."
)

parser.add_argument(
	"-i",
	"--input",
	metavar = "INPUT",
	required = False,
	help = "Path to the input image, video, or animation to process."
)

parser.add_argument(
	"-o",
	"--output",
	metavar = "OUTPUT",
	required = False,
	help = "Path where the cropped PNG will be saved. Defaults to '<input>_nogutter.png'."
)

parser.add_argument(
	"-m",
	"--minimum",
	metavar = "MINIMUM",
	required = False,
	help = "Amount of white, in pixels, to leave for each gutter. Gutters wider than this are cropped down to it, narrower ones are extended with white, and places without a gutter are left as-is. Defaults to 0.")

parser.add_argument(
	"-s",
	"--sides",
	required = False,
	action = "store_true",
	help = "Also crop the left and right white gutters."
)

os.environ["LINES"] = "1000"
os.environ["COLUMNS"] = "1000"

file = io.StringIO()
parser.print_help(file = file)
file.seek(0, io.SEEK_SET)

text = file.read()

header = """/*
This file is auto-generated. Use the tool at ../tools/program_help.h.py to regenerate.
*/

#if !defined(PROGRAM_HELP_H)
#define PROGRAM_HELP_H

#define PROGRAM_HELP \\\n\
"""

for line in text.splitlines():
	line = json.dumps(obj = line + "\n")
	header += '\t%s\\\n' % line

header += "\n#endif\n"

destination = os.path.join(
	os.path.dirname(
		p = os.path.dirname(
			p = os.path.realpath(
				filename = __file__
			)
		)
	),
	"src/program_help.h"
)

print("Saving to '%s'" % (destination))

with open(file = destination, mode = "w") as file:
	file.write(header)
