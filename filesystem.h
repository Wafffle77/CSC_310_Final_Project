#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

using namespace std;

#include "types.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <string>
#include <fstream>
#include <unordered_map>


#include <vector>

typedef struct {
    sector_t file;
    sector_t cur;
    uint64_t pos;
} open_file_t;

typedef enum {
    MY_SEEK_SET,
    MY_SEEK_END,
    MY_SEEK_CUR,
} e_whence_t;

class MyFilesystem {
    private:
        sector_union_t* disk;
        int fd;
        uint64_t size;

        unordered_map<int,open_file_t> fd_table;

        // Resolving paths
        sector_t resolve(const char *path, sector_t entry);

        // Seeking
        uint64_t seek_back(int fd, int64_t offset);
        uint64_t seek_forward(int fd, int64_t offset);

        // Returns the number of characters
        static inline int str2name(string in, name_t out) {
            int i;
            for(i = 0; i < in.length() && i < sizeof(name_t); i++)
                out[i] = in[i];
            return i;
        }

        vector<entry_t>	 readdir(sector_t dir);

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

        // File Handles
        int open(string path);
        void close(int fd);
        void trunc(int fd);
        uint64_t read (int fd, uint8_t *buf, uint64_t count);
        uint64_t write(int fd, uint8_t *buf, uint64_t count);
        uint64_t seek(int fd, int64_t offset, int whence);
        bool eof(int fd);

        //Hash Table functions
        sector_t entry_access(char* name, sector_t block_index);
        sector_t entry_insert(entry_t entry, sector_t block_index);
        sector_t entry_remove(char* name, sector_t block_index);

        vector<entry_t>	 readdir(string path);
    };

#endif // _FILESYSTEM_H
