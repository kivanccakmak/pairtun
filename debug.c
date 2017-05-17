#include <stdlib.h>
#include <strings.h>

#include "debug.h"

enum ptun_debug_level g_ptun_debug_level = ptun_debug_level_debug;

enum ptun_debug_level ptun_debug_get_level (void)
{
    return g_ptun_debug_level;
}

void ptun_debug_set_level (enum ptun_debug_level level)
{
    g_ptun_debug_level = level;
}

enum ptun_debug_level ptun_debug_level_from_string (const char *string)
{
    if (string == NULL) {
        return ptun_debug_level_silent;
    }
    if (strcasecmp(string, "silent") == 0) {
        return ptun_debug_level_silent;
    }
    if (strcasecmp(string, "error") == 0) {
        return ptun_debug_level_error;
    }
    if (strcasecmp(string, "info") == 0) {
        return ptun_debug_level_info;
    }
    if (strcasecmp(string, "debug") == 0) {
        return ptun_debug_level_debug;
    }
    return ptun_debug_level_silent;
}
