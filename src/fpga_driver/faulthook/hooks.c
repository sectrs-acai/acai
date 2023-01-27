#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

pthread_mutex_t lock;
#define SIZE 4096

extern char *program_invocation_name;
extern char *program_invocation_short_name;

static void *(*real_malloc)(size_t) =NULL;

static void (*real_free)(void *) =NULL;

static void *(*real_calloc)(size_t nmemb, size_t size) = NULL;

static void *(*real_realloc)(void *ptr, size_t size) = NULL;

static void *map = NULL;
static void *map_start;
static size_t map_size;

static inline int is_map_address(void *p)
{
    return p >= map_start && p < (map_start + map_size);
}

#define PAGE_SIZE (4096)
#define FILE_PATH "/tmp/hooks_mmap"
#define MMAP_PATH "/hooks_file"

static void inline mmap_shm(void)
{
    // shm_unlink(MMAP_PATH);
    /* Create a new memory object */
    int fd = shm_open(MMAP_PATH, O_RDWR | O_CREAT, 0777);
    if (fd==-1) {
        fprintf(stderr, "Open failed:%s\n", strerror(errno));
        exit(1);
    }
    if (ftruncate(fd, map_size)==-1) {
        fprintf(stderr, "ftruncate: %s\n",
                strerror(errno));
        exit(1);
    }

    pthread_mutex_lock(&lock);
    map = mmap(NULL,
               map_size,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               0);

    if (map==MAP_FAILED) {
        fprintf(stderr, "MAP_FAILED: %s\n", dlerror());
        exit(1);
    }
    map_start = map;
    pthread_mutex_unlock(&lock);
}

static int stat_file_done = 0;

static void inline stat_file()
{
    if (stat_file_done) {
        return;
    }
    stat_file_done = 1;
    char buffer[128];
    pid_t pid = getpid();
    sprintf(buffer, "%s_%d", FILE_PATH, pid);
    printf("opening file %s\n", buffer);
    FILE *f = fopen(buffer, "w");

    if (f==NULL) {
        fprintf(stderr, "fail to open file: %s %s\n", buffer, dlerror());
        exit(1);
    }
    sprintf(buffer, "0x%lx-0x%lx;%d", map_start, map_start + map_size, pid);
    fwrite(buffer, 1, strlen(buffer), f);
    fclose(f);

}

#define ENV_FAULTHOOK_ENABLE "faulthook_enable"
static int env_faulthook_enable = 0;

#define ENV_FAULTHOOK_HEAP_GB "faulthook_heap_gb"
static int env_faulthook_heap_gb = 2;

static void read_env_vars(void)
{
    const int buf_len = 256;
    char buf[buf_len];
    return;

    if (getenv(ENV_FAULTHOOK_ENABLE)) {
        printf("1\n");
        if (snprintf(buf, buf_len, "%d", getenv(ENV_FAULTHOOK_ENABLE)) < buf_len) {
            printf("3\n");
            env_faulthook_enable = (strtol(buf, NULL, 10)==1);
            printf("Setting %s=%d\n", ENV_FAULTHOOK_ENABLE, env_faulthook_enable);
        }
    }

    if (getenv(ENV_FAULTHOOK_HEAP_GB)) {
        printf("2\n");
        if (snprintf(buf, buf_len, "%d", getenv(ENV_FAULTHOOK_HEAP_GB)) < buf_len) {
            printf("4\n");
            env_faulthook_heap_gb = strtol(buf, NULL, 10);
            printf("Setting %s=%d\n", ENV_FAULTHOOK_HEAP_GB, env_faulthook_heap_gb);
        }
    }

    printf("%s=%d\n", ENV_FAULTHOOK_ENABLE, env_faulthook_enable);
    printf("%s=%d\n", ENV_FAULTHOOK_HEAP_GB, env_faulthook_heap_gb);
}

static void inline mmap_annon()
{
    pthread_mutex_lock(&lock);
    map = mmap(NULL,
               map_size,
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS,
               -1,
               0);

    if (map==MAP_FAILED) {
        fprintf(stderr, "MAP_FAILED: %s\n", dlerror());
        exit(1);
    }

    printf("mmap %lx(%lx) for %s/ %s\n",
           map,
           map_size,
           program_invocation_name,
           program_invocation_short_name);
    printf("pid: %d\n", getpid());
    map_start = map;
    pthread_mutex_unlock(&lock);
}

static int mtrace_init_done = 0;
static void mtrace_init(void)
{
    if (mtrace_init_done) {
        return;
    }

    if (pthread_mutex_init(&lock, NULL)!=0) {
        printf("\n mutex init has failed\n");
        exit(1);
    }
    printf("here\n");
    read_env_vars();
    map_size = 1024L * 1024L * 1024L * env_faulthook_heap_gb;

    real_malloc = dlsym(RTLD_NEXT, "malloc");
    if (NULL==real_malloc) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
    real_free = dlsym(RTLD_NEXT, "free");
    if (NULL==real_free) {
        fprintf(stderr, "Error for free in `dlsym`: %s\n", dlerror());
    }
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    if (NULL==real_calloc) {
        fprintf(stderr, "Error for calloc in `dlsym`: %s\n", dlerror());
    }
    real_realloc = dlsym(RTLD_NEXT, "realloc");
    if (NULL==real_realloc) {
        fprintf(stderr, "Error for realloc in `dlsym`: %s\n", dlerror());
    }

    // mmap_shm();
    mmap_annon();
    stat_file();
    mtrace_init_done = 1;
}

unsigned long c = 0;

void *malloc(size_t size)
{

    if (real_malloc==NULL) {
        mtrace_init();
    }
    if (!env_faulthook_enable) {
        return real_malloc(size);
    }
    void *p = NULL;
    if (size==4128) {

        pthread_mutex_lock(&lock);
        void *p = map;
        void *a = map + PAGE_SIZE;
        // map += 2 * PAGE_SIZE;
        map += 1 * PAGE_SIZE;
        pthread_mutex_unlock(&lock);
        assert(map < map + map_size);
        // memset(a, 'A', PAGE_SIZE);
        fprintf(stderr, "page_malloc(%d)=%p, %ld\n", size, p, ++c);
        return p;
    }

    p = real_malloc(size);
    if (size >= SIZE) {
//        fprintf(stderr, "malloc(%d)=%p\n", size, p);
        // fprintf(stderr, "%p\n", p);
    }
    return p;
}


void *calloc(size_t nmemb, size_t size)
{
    if (real_calloc==NULL) {
        mtrace_init();
    }
    if (!env_faulthook_enable) {
        return real_calloc(nmemb, size);
    }

    void *p = NULL;
    p = real_calloc(nmemb, size);
    if (size >= SIZE) {
//        fprintf(stderr, "calloc(%d, %d)=%p\n", nmemb, size, p);
        // fprintf(stderr, "%p\n", p);
    }
    return p;
}

void *realloc(void *ptr, size_t size)
{

    if (real_realloc==NULL) {
        mtrace_init();
    }
    if (!env_faulthook_enable) {
        return real_realloc(ptr, size);
    }

    void *p = NULL;
    p = real_realloc(ptr, size);
    if (size >= SIZE) {
        // fprintf(stderr, "realloc(%p, %d)=%p\n", ptr, size, p);
        // fprintf(stderr, "%p\n", p);
    }
    return p;
}


void free(void *p)
{
    if (real_free==NULL) {
        mtrace_init();
    }
    if (!env_faulthook_enable) {
        free(p);
    }
    // fprintf(stderr, "free(%p)\n", p);
    if (!is_map_address(p)) {
        real_free(p);
    }
}
