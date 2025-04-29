#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

using namespace std;

#include "types.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>
#include <fstream>



#include <vector>

class MyFilesystem {
    private:
        sector_union_t* disk;
        int fd;
        uint64_t size;

        // Resolving paths
        sector_t resolve(const char *path, sector_t entry);

        // Returns the number of characters
        static inline int str2name(string in, name_t out) {
            int i;
            for(i = 0; i < in.length() && i < sizeof(name_t); i++)
                out[i] = in[i];
            return i;
        }

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
        void debug_tree(string path);

        // Resolving paths
        sector_t resolve(string path);

        sector_t create(string name, sector_t dir);
        sector_t mkdir(string name, sector_t dir);


        //Hash Table functions
        sector_t entry_access(char* name, sector_t block_index);
        sector_t entry_insert(entry_t entry, sector_t block_index);
        sector_t entry_remove(char* name, sector_t block_index);




vector<entry_t>	 readdir(string path);
    };

#endif // _FILESYSTEM_H
