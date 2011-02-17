#include <stdlib.h>
#include "imagedb.h"

extern "C" {
    void imagedb_wrap(void* data, const char* format) {
        imagedb(data, format);
    }
}