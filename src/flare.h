#if !defined(FLARE_H)
#define FLARE_H

#include "argparse.h"

#define PROJECT_NAME "flare"
#define PROJECT_NAME_LOWERCASE "flare"
#define PROJECT_VERSION "0.1"
#define PROJECT_REPOSITORY "https://github.com/AmanoTeam/flare"

int flare_bands(const char* const path, int** const bands, int* const count);

int flare_main(int argc, argv_t** argv);

#endif
