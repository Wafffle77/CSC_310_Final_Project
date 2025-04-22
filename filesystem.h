#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

using namespace std;

#include "types.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>

class MyFilesystem {
    private:
        sector_union_t* disk;
        int fd;
        uint64_t size;

    public:
        MyFilesystem(sector_union_t* disk, uint64_t disk_size);
        MyFilesystem(string path);
        MyFilesystem(sector_t sectors);

        ~MyFilesystem();

        void format();

        sector_t alloc_sector();
        void free_sector(sector_t i);
};

#endif // _FILESYSTEM_H