
#include "rtl.h"

#include <stdlib.h>
#include <string.h>


extern struct String *String_fromCStringAndLength(const char *, size_t);
extern char *String_asCString(struct String *);


struct Object *Symbol_printString(struct Symbol *self) {
    struct String *result;
    size_t len;
    char *str;
    
    len = strlen(self->str);
    result = String_fromCStringAndLength(0, len + 1);
    if (!result)
        return 0;
    str = String_asCString(result);
    str[0] = '$';
    memcpy(&str[1], self->str, len);
    return (struct Object *)result;
}
