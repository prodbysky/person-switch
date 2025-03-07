#include "timing_utilities.h"
#include <raylib.h>

double time_delta(double t) {
    return GetTime() - t;
}
