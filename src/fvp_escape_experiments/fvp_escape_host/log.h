#ifndef THREADS_LOG_H
#define THREADS_LOG_H

#ifndef LOG_VERBOSE
#define LOG_VERBOSE 1
#endif

enum
{
    LOG_LVL_LOG, // 0
    LOG_LVL_DEBUG, // 1
    LOG_LVL_ERROR, // 2
};

const char *log_level_strings[] = {
        "LOG", // 0
        "DEBUG", // 1
        "ERROR", // 2
};

#define LL_DEBUG(fmt, arg...) LOG( LOG_LVL_DEBUG, fmt, ##arg)
#define LL_LOG(fmt, arg ...) LOG( LOG_LVL_LOG, fmt, ##arg )
#define LL_ERROR(fmt, arg ...) LOG( LOG_LVL_ERROR, fmt, ##arg )
#define LOG(level, fmt, arg...) do { \
    if (level == LOG_LVL_ERROR)      \
    {                                \
    fprintf(stdout, "[%s] %s:%d: " fmt "\n", log_level_strings[level], __FUNCTION__,__LINE__, ##arg); \
        fflush( stdout );                                  \
    }                                           \
    else if if ( LOG_VERBOSE ) { \
        fprintf(stdout, "[%s] %s:%d: " fmt "\n", log_level_strings[level], __FUNCTION__,__LINE__, ##arg); \
        fflush( stdout ); \
    } \
} while(0)

#endif