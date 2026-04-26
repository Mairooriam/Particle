#include "string.h"
#include <string.h>

sv sv_create(const char *string) {
    sv result;
    result.data = string;
    result.lenght = string ? strlen(string) : 0;
    return result;
}
