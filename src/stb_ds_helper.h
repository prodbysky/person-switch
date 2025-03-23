#ifndef STB_DS_HELPER_H
#define STB_DS_HELPER_H
#include <stb_ds.h>
#define STB_DS_ARRAY_CLEAN(array, condition) \
    for (ptrdiff_t i = stbds_arrlen(array) - 1; i >= 0; i--) {\
        if (condition) {\
            stbds_arrdel(array, i); \
        }\
    }
#define STB_DS_ARRAY_RESET(array) stbds_arrsetlen(array, 0)
#endif
