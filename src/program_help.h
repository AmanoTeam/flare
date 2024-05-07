/*
This file is auto-generated. Use the tool at ../tools/program_help.h.py to regenerate.
*/

#if !defined(PROGRAM_HELP_H)
#define PROGRAM_HELP_H

#define PROGRAM_HELP \
	"usage: flare [-h] [-v] [-i INPUT] [-o OUTPUT] [-m MINIMUM] [-s]\n"\
	"\n"\
	"A command-line utility to crop white and black gutters off images.\n"\
	"\n"\
	"options:\n"\
	"  -h, --help            Display this help text and exit.\n"\
	"  -v, --version         Print version information and exit.\n"\
	"  -i, --input INPUT     Path to the input image, video, or animation to process.\n"\
	"  -o, --output OUTPUT   Path where the cropped PNG will be saved. Defaults to '<input>_nogutter.png'.\n"\
	"  -m, --minimum MINIMUM\n"\
	"                        Amount of white or black, in pixels, to leave for each gutter. Gutters wider than this are cropped down to it, narrower ones are extended with their own color, and places without a gutter are left as-is. Defaults to 0.\n"\
	"  -s, --sides           Also crop the left and right gutters.\n"\
	"\n"\
	"Note: options that take a value must use an equal sign (e.g. --input=INPUT).\n"\

#endif
