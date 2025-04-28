#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

using namespace std;

#include "types.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>
#include <fstream>

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

        void defragment();
        sector_t alloc_sector();
        void free_sector(sector_t i);

        // Debugging
        void debug_heap(string path);




	//Hash Table functions
	sector_t entry_access(char* name, sector_t block_index);
	int entry_insert(entry_t entry, sector_t block_index);
	int entry_remove(char* name, sector_t block_index);
};

#endif // _FILESYSTEM_H
