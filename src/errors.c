/*
This file is auto-generated. Use the tool at ../tools/errors.h.py to regenerate.
*/

#include "errors.h"

const char* flare_getmessage(const int code) {

	switch (code) {
		case FLAREERR_SUCCESS:
			return "Success";
		case FLAREERR_ARGPARSE_ARGUMENT_EMPTY:
			return "Got an empty argument while parsing the command-line arguments";
		case FLAREERR_ARGPARSE_ARGUMENT_INVALID:
			return "Got an invalid argument while parsing the command-line arguments";
		case FLAREERR_ARGPARSE_ARGUMENT_VALUE_MISSING:
			return "This argument requires a value but none was supplied";
		case FLAREERR_ARGPARSE_INVALID_UINT:
			return "Got an invalid integer value for this argument";
		case FLAREERR_ARGPARSE_VALUE_UNEXPECTED:
			return "Got an unexpected value while parsing the command-line arguments";
		case FLAREERR_DECODER_OPEN_FAILURE:
			return "Could not open a decoder for the input file";
		case FLAREERR_DECODE_FAILED:
			return "Could not decode a frame from the input file";
		case FLAREERR_ENCODER_OPEN_FAILURE:
			return "Could not open the PNG encoder";
		case FLAREERR_IMAGE_NO_CONTENT:
			return "The input image has no content to keep";
		case FLAREERR_INPUT_MISSING:
			return "No input file was supplied";
		case FLAREERR_INPUT_OPEN_FAILURE:
			return "Could not open the input file";
		case FLAREERR_MEM_ALLOC_FAILURE:
			return "Could not allocate memory";
		case FLAREERR_OUTPUT_CONTEXT_FAILURE:
			return "Could not allocate the output format context";
		case FLAREERR_OUTPUT_FILE_OPEN_FAILURE:
			return "Could not open the output file";
		case FLAREERR_VIDEO_STREAM_MISSING:
			return "The input file does not contain a video stream";
		case FLAREERR_WRITE_HEADER_FAILURE:
			return "Could not write the output file header";
	}

	return "Unknown error";

}
