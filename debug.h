#ifndef _DEBUG_H
#define _DEBUG_H

/**
 * @author	alper akcan <alper.akcan@airties.com>
 * @date	09.2016
 */

enum ptun_debug_level {
	ptun_debug_level_silent,
	ptun_debug_level_error,
	ptun_debug_level_info,
	ptun_debug_level_debug,
};

#define ptun_debugf(a...) { \
	if (ptun_debug_get_level() >= ptun_debug_level_debug) { \
		fprintf(stderr, "ptun-debug: "); \
		fprintf(stderr, a); \
		fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	} \
}

#define ptun_infof(a...) { \
	if (ptun_debug_get_level() >= ptun_debug_level_info) { \
		fprintf(stderr, "ptun-info: "); \
		fprintf(stderr, a); \
		fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	} \
}

#define ptun_errorf(a...) { \
	if (ptun_debug_get_level() >= ptun_debug_level_error) { \
		fprintf(stderr, "ptun-error: "); \
		fprintf(stderr, a); \
		fprintf(stderr, " (%s %s:%d)\n", __FUNCTION__, __FILE__, __LINE__); \
	} \
}

enum ptun_debug_level ptun_debug_get_level (void);
void ptun_debug_set_level (enum ptun_debug_level level);
enum ptun_debug_level ptun_debug_level_from_string (const char *string);
#endif
