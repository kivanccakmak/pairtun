#ifndef _COMMONS_H
#define _COMMONS_H

#define sfree(_ptr)         \
    do {                    \
        if (_ptr) {         \
            free(_ptr);     \
            _ptr = NULL;    \
        }                   \
    } while (0)             \

#endif
