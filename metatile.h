#ifndef METATILE_H
#define METATILE_H

int xyz_to_meta(char *path, size_t len, const char *tile_dir, int x, int y, int z);

int file_tile_read(const char *tile_dir, int x, int y, int z, char *buf, size_t sz, char * log_msg);

struct entry {
    int offset;
    int size;
};

struct meta_layout {
    char magic[4];
    int count; // METATILE ^ 2
    int x, y, z; // lowest x,y of this metatile, plus z
    struct entry index[]; // count entries
    // Followed by the tile data
    // The index offsets are measured from the start of the file
};

#endif

