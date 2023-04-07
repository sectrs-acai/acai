#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "fh_def.h"
#include "usr_manager.h"
#include "drm.h"

int main(int argc, char *argv[])
{
    int fd = open("/dev/dri/renderD128", O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    struct drm_version version;
    version.name = NULL;
    version.date = NULL;
    version.desc = NULL;

    if (ioctl(fd, DRM_IOCTL_VERSION, &version) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    if (version.name_len)
        version.name    = malloc(version.name_len + 1);
    if (version.date_len)
        version.date    = malloc(version.date_len + 1);
    if (version.desc_len)
        version.desc    = malloc(version.desc_len + 1);

    if (ioctl(fd, DRM_IOCTL_VERSION, &version) < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    /* The results might not be null-terminated strings, so terminate them. */
    if (version.name_len) version.name[version.name_len] = '\0';
    if (version.date_len) version.date[version.date_len] = '\0';
    if (version.desc_len) version.desc[version.desc_len] = '\0';

    printf("Version: %d.%d.%d\n", version.version_major, version.version_minor, version.version_patchlevel);
    printf("Name: %s\n", version.name);
    printf("Date: %s\n", version.date);
    printf("Description: %s\n", version.desc);

    close(fd);
    return 0;

}