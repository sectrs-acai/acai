#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE (4096)
#define FILE_PATH "/tmp/hooks_mmap"

#define HOOK_ENABLE 1
#define HOOK_TRACE 1

#define ENV_HOOK_ENABLE "fvp_libchook_enable"

#define HERE fprintf(stderr, "%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define HERE_NUMBER(i) fprintf(stderr, "%s/%s: %d (%ld)\n", __FILE__, __FUNCTION__, __LINE__, i)

static void *(*real_malloc)(size_t) =NULL;

static void (*real_free)(void *) =NULL;

static void *(*real_calloc)(size_t nmemb, size_t size) = NULL;

static void *(*real_realloc)(void *ptr, size_t size) = NULL;

extern char *program_invocation_name;
extern char *program_invocation_short_name;

pthread_mutex_t lock;
static void *map = NULL;
static void *map_start;

/*
 * max size: does not MAP_POPULATE at creation time
 * Increase if you need more
 */
static size_t map_size = 1024L * 1024L * 1024L * 12L;

static int is_do_init = 0;
static int do_malloc_init = 0;
static int do_calloc_init = 0;
static size_t malloc_count = 0;

inline static void *init_symbol(const char *name)
{
    void *s = dlsym(RTLD_NEXT, name);
    if (s == NULL)
    {
        fprintf(stderr, "Error in `dlsym` for symbol %s: %s\n", name, dlerror());
        exit(10);
    }
    return s;
}

/*
 * XXX: Chicken Egg problem
 * Invocation of dlsym itself needs memory.
 * Bootstrap invocations with stack memory until things are set up.
 */
static size_t _buffer_used = 0;
static size_t _buffer_size = 40960;
static char _buffer[40960];

static void *mm_alloc(size_t size)
{
    if (_buffer_used + size <= _buffer_size)
    {
        void *res = _buffer + _buffer_used;
        _buffer_used += size;
        return res;
    } else
    {
        fprintf(stderr, "no mem left in mm_alloc: %ld\n", size);
        exit(10);
    }
}

static int mm_is_address(void *p)
{
    void *b = (void *) _buffer;
    return p >= b && p < b + _buffer_size;
}


static void inline stat_file()
{
    char buffer[128];
    pid_t pid = getpid();
    sprintf(buffer, "%s_%d", FILE_PATH, pid);
    printf("opening file %s\n", buffer);
    FILE *f = fopen(buffer, "w");

    if (f == NULL)
    {
        fprintf(stderr, "fail to open file: %s %s\n", buffer, dlerror());
        exit(1);
    }
    sprintf(buffer, "0x%lx-0x%lx;%d",
            (unsigned long) map_start,
            (unsigned long) map_start + map_size, pid);
    fwrite(buffer, 1, strlen(buffer), f);
    fclose(f);

}

static void inline mmap_annon()
{
    int ret;
    pthread_mutex_lock(&lock);
    map = mmap(NULL,
               map_size,
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS,
               - 1,
               0);

    if (map == MAP_FAILED)
    {
        fprintf(stderr, "MAP_FAILED: %s\n", dlerror());
        exit(1);
    }

    printf("mmap %lx(%lx) for %s/ %s\n",
           (unsigned long) map,
           map_size,
           program_invocation_name,
           program_invocation_short_name);
    printf("pid: %d\n", getpid());
    map_start = map;

    /*
     * Lock memory pages (and further page faults)
     * such that they are not paged out when we alias them in another process
     */
    printf("locking all pages in range %p+%lx\n", map_start, map_size);
    ret = mlock2(map_start, map_size, MLOCK_ONFAULT);
    if (ret < 0)
    {
        perror("mlock failed. Are you running FVP as root? "
               "This is required to pin memory pages\n");
    }
    pthread_mutex_unlock(&lock);
}

static inline int is_map_address(void *p)
{
    void *buf = (void *) map_start;
    void *buf_end = (void *) map_start + map_size;
    return p >= buf && p < buf_end;
}


static inline void do_init(void)
{
    if (is_do_init)
    {
        return;
    }

    is_do_init = 1;
    mmap_annon();
    stat_file();
}

void *malloc(size_t size)
{
    if (real_malloc == NULL)
    {
        if (do_malloc_init)
        {
            return mm_alloc(size);
        }
        do_malloc_init = 1;
        real_malloc = init_symbol("malloc");
        HERE_NUMBER(size);
    }
    #if HOOK_ENABLE
    do_init();

    /*
     * XXX: Magic number,
     * FVP_Base_RevC-2xAEMvA_11.20_15_Linux64 specific.
     *
     * If we run into issues here we can allow other sizes to be served from mmap
     * too. For instance everything > 4kb
     * */
    if (size == 4128)
    {
        pthread_mutex_lock(&lock);
        /*
         * TODO: use 2 pages instead not to skip unknown holes
         */
        void *res = map;
        map += 1 * PAGE_SIZE;
        pthread_mutex_unlock(&lock);
        if (map >= map_start + map_size)
        {
            fprintf(stderr, "mmap region too small, libc hook\n");
            exit(1);
        }
        #if HOOK_TRACE
        if (++ malloc_count % 10000 == 0)
        {
            fprintf(stderr, "page_malloc(%ld)=%p, %ld\n",
                    size, res, malloc_count);
        }
        #endif
        return res;
    }
    #endif
    return real_malloc(size);
}

void *calloc(size_t nmemb, size_t size)
{
    if (real_calloc == NULL)
    {
        if (do_calloc_init)
        {
            char *p = mm_alloc(nmemb * size);
            for (size_t i = 0; i < nmemb * size; i ++)
            {
                *(p + i) = 0;
            }
            return p;
        }
        do_calloc_init = 1;
        real_calloc = init_symbol("calloc");
        HERE_NUMBER(size * nmemb);
    }
    #if HOOK_ENABLE
    do_init();
    #endif
    return real_calloc(nmemb, size);

}

void *realloc(void *ptr, size_t size)
{
    if (real_realloc == NULL)
    {
        real_realloc = init_symbol("realloc");
    }
    #if HOOK_ENABLE
    do_init();
    #endif
    return real_realloc(ptr, size);
}

void free(void *p)
{
    if (mm_is_address(p))
    {
        HERE;
        return;
    }
    if (real_free == NULL)
    {
        real_free = init_symbol("free");
    }
    #if HOOK_ENABLE
    do_init();
    if (is_map_address(p))
    {
        return;
    }
    #endif
    real_free(p);
}
