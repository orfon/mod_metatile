#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "metatile.h"
#include "config.h"

// Returns the path to the meta-tile and the offset within the meta-tile
int xyz_to_meta(char *path, size_t len, const char *tile_dir, int x, int y, int z) {
    unsigned char i, hash[5], offset, mask;

    // Each meta tile winds up in its own file, with several in each leaf directory
    // the .meta tile name is beasd on the sub-tile at (0,0)
    mask = METATILE - 1;
    offset = (x & mask) * METATILE + (y & mask);
    x &= ~mask;
    y &= ~mask;

    for (i=0; i<5; i++) {
        hash[i] = ((x & 0x0f) << 4) | (y & 0x0f);
        x >>= 4;
        y >>= 4;
    }
    snprintf(path, len, "%s/%d/%u/%u/%u/%u/%u.meta", tile_dir, z, hash[4], hash[3], hash[2], hash[1], hash[0]);
    return offset;
}

int file_tile_read(const char *tile_dir, int x, int y, int z, char *buf, size_t sz, char * log_msg) {

    char path[META_PATH_MAX];
    int meta_offset, fd;
    unsigned int pos;
    unsigned int header_len = sizeof(struct meta_layout) + METATILE*METATILE*sizeof(struct entry);
    struct meta_layout *m = (struct meta_layout *)malloc(header_len);
    size_t file_offset, tile_size;

    meta_offset = xyz_to_meta(path, sizeof(path), tile_dir, x, y, z);

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        snprintf(log_msg, META_PATH_MAX - 1, "Could not open metatile %s. Reason: %s\n", path, strerror(errno));
        free(m);
        return -1;
    }

    pos = 0;
    while (pos < header_len) {
        size_t len = header_len - pos;
        int got = read(fd, ((unsigned char *) m) + pos, len);
        if (got < 0) {
            snprintf(log_msg, META_PATH_MAX - 1, "Failed to read complete header for metatile %s Reason: %s\n", path, strerror(errno));
            close(fd);
            free(m);
            return -2;
        } else if (got > 0) {
            pos += got;
        } else {
            break;
        }
    }
    if (pos < header_len) {
        snprintf(log_msg, META_PATH_MAX - 1, "Meta file %s too small to contain header\n", path);
        close(fd);
        free(m);
        return -3;
    }

    // Currently this code only works with fixed metatile sizes (due to xyz_to_meta above)
    if (m->count != (METATILE * METATILE)) {
        snprintf(log_msg, META_PATH_MAX - 1, "Meta file %s header bad count %d != %d\n", path, m->count, METATILE * METATILE);
        free(m);
        close(fd);
        return -5;
    }

    file_offset = m->index[meta_offset].offset;
    tile_size   = m->index[meta_offset].size;

    free(m);

    if (tile_size > sz) {
        snprintf(log_msg, META_PATH_MAX - 1, "Truncating tile %zd to fit buffer of %zd\n", tile_size, sz);
        tile_size = sz;
        close(fd);
        return -6;
    }

    if (lseek(fd, file_offset, SEEK_SET) < 0) {
        snprintf(log_msg, META_PATH_MAX - 1, "Meta file %s seek error: %s\n", path, strerror(errno));
        close(fd);
        return -7;
    }

    pos = 0;
    while (pos < tile_size) {
        size_t len = tile_size - pos;
        int got = read(fd, buf + pos, len);
        if (got < 0) {
            snprintf(log_msg, META_PATH_MAX - 1, "Failed to read data from file %s. Reason: %s\n", path, strerror(errno));
            close(fd);
            return -8;
        } else if (got > 0) {
            pos += got;
        } else {
            break;
        }
    }
    close(fd);
    return pos;
}