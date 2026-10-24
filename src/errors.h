#if !defined(ERRORS_H)
#define ERRORS_H

#define FLAREERR_SUCCESS 0 /* Success */

#define FLAREERR_ARGPARSE_ARGUMENT_EMPTY -1 /* Got an empty argument while parsing the command-line arguments */
#define FLAREERR_ARGPARSE_ARGUMENT_INVALID -2 /* Got an invalid argument while parsing the command-line arguments */
#define FLAREERR_ARGPARSE_ARGUMENT_VALUE_MISSING -3 /* This argument requires a value but none was supplied */
#define FLAREERR_ARGPARSE_INVALID_UINT -4 /* Got an invalid integer value for this argument */
#define FLAREERR_ARGPARSE_VALUE_UNEXPECTED -5 /* Got an unexpected value while parsing the command-line arguments */

#define FLAREERR_DECODER_OPEN_FAILURE -6 /* Could not open a decoder for the input file */

#define FLAREERR_DECODE_FAILED -7 /* Could not decode a frame from the input file */

#define FLAREERR_ENCODER_OPEN_FAILURE -8 /* Could not open the PNG encoder */

#define FLAREERR_IMAGE_ENTIRELY_WHITE -9 /* The input image is entirely white */

#define FLAREERR_INPUT_MISSING -10 /* No input file was supplied */
#define FLAREERR_INPUT_OPEN_FAILURE -11 /* Could not open the input file */

#define FLAREERR_MEM_ALLOC_FAILURE -12 /* Could not allocate memory */

#define FLAREERR_OUTPUT_CONTEXT_FAILURE -13 /* Could not allocate the output format context */
#define FLAREERR_OUTPUT_FILE_OPEN_FAILURE -14 /* Could not open the output file */

#define FLAREERR_VIDEO_STREAM_MISSING -15 /* The input file does not contain a video stream */

#define FLAREERR_WRITE_HEADER_FAILURE -16 /* Could not write the output file header */

const char* flare_getmessage(const int code);

#endif
