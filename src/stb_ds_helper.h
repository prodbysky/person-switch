#ifndef STB_DS_HELPER_H
#define STB_DS_HELPER_H
#include <stb_ds.h>
#define STB_DS_ARRAY_CLEAN(array, condition) \
    for (ptrdiff_t i = stbds_arrlen(array) - 1; i >= 0; i--) {\
        if (array[i]condition) {\
            stbds_arrdel(array, i); \
        }\
    }
#endif
