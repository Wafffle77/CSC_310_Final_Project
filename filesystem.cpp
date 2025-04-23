#include "filesystem.h"

using namespace std;

MyFilesystem::MyFilesystem(sector_union_t* disk_ptr, uint64_t disk_size) {
    disk = disk_ptr;
    size = disk_size;
    fd = -1;
}

MyFilesystem::MyFilesystem(sector_t sectors) {
    disk = new sector_union_t[sectors];
    size = sectors * SECTOR_SIZE;
    fd = -1;
}

// TODO: use exceptions in this
MyFilesystem::MyFilesystem(string path) {
    fd = open(path.c_str(), O_RDWR);
    if(fd == -1) {
        perror("Unable to open disk");
        exit(1);
    }

    size = lseek(fd, 0, SEEK_END);
    if(size == (off_t) -1) {
        perror("Unable to get disk size");
        exit(1);
    }
    lseek(fd, 0, SEEK_SET);

    disk = (sector_union_t*) mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("Unable to map disk");
        exit(1);
    }
}

MyFilesystem::~MyFilesystem() {
    if(fd != -1) {
        int status = munmap(disk, size);
        if(status != 0) {
            perror("Unable to unmap disk");
            exit(1);
        }

        status = close(fd);
        if(status != 0) {
            perror("Unable to close disk");
            exit(1);
        }
    }
}

void MyFilesystem::format() {
    disk[0].header = {
        .root_entry = 1,
        .heap_size = 1,
        .ranges = {
            {},
            {
                .start = 2,
                .end = (sector_t) (size / SECTOR_SIZE)
            }
        }
    };

    disk[1].entry = {
        .parent = 0,
        .metadata = 0,
        .type = ENTRY_TYPE_DIRECTORY,
        .name = "ROOT_DIR",
        .children = {0},
    };
}

void MyFilesystem::defragment() {

}

sector_t MyFilesystem::alloc_sector() {
    // Increment the lowest range (and remember the sector)
    sector_t ret = disk->header.ranges[1].start++;

    // If the range is empty, pop and discard it
    if(disk->header.ranges[1].start >= disk->header.ranges[1].end) {
        uint32_t i = --disk->header.heap_size;
        disk->header.ranges[1] = disk->header.ranges[i];

        // Percolate
        while(i <= disk[0].header.heap_size) {
            allocator_range_t *parent = &disk->header.ranges[i];
            allocator_range_t *right  = &disk->header.ranges[2 * i + 1];
            allocator_range_t *left   = &disk->header.ranges[2 * i];
            allocator_range_t *child;
            
            if(2 * i > HEADER_RANGES_SIZE) {
                break;
                // TODO: trigger defrag
            }

            allocator_range_t temp = *parent;

            if(left->start < right->start) {
                child = left;
                i = 2 * i;
            } else {
                child = right;
                i = 2 * i + 1;
            }

            if(child->start < parent->start) {
                *parent = *child;
                *child = temp;
            } else {
                break;
            }
        }
    }
    return ret;
}


void MyFilesystem::free_sector(sector_t sector) {
    if(disk->header.heap_size + 1 >= HEADER_RANGES_SIZE) {
        // Defrag (expensive)

        // Eh I'll finish this later when it's an issue
        // for(uint32_t i = 0; i < HEADER_RANGES_SIZE; i++) {
        //     disk->header.ranges[i]
        // }
    }

    uint32_t i = ++disk->header.heap_size;
    disk->header.ranges[i] = {
        .start = sector,
        .end = sector + 1
    };

    // Percolate
    while(i >= 1) {
        allocator_range_t *parent = &disk->header.ranges[i / 2];
        allocator_range_t *child  = &disk->header.ranges[i];

        if(parent->start > child->start) {
            allocator_range_t temp = *child;
            *child = *parent;
            *parent = temp;

            i /= 2;
        } else {
            break;
        }
    }
}


void MyFilesystem::debug_heap(string path) {
    ofstream f(path);
    f << "digraph {" << endl;
    f << "\t" << disk[0].header.ranges[1].start << " [label=\"" << disk[0].header.ranges[1].start << "-" << disk[0].header.ranges[1].end << "\"]" << endl;
    for(int i = 2; i < disk[0].header.heap_size; i++) {
        f << "\t" << disk[0].header.ranges[i].start << " [label=\"" << disk[0].header.ranges[i].start << "-" << disk[0].header.ranges[i].end << "\"]" << endl;
        f << "\t" << disk[0].header.ranges[i].start << " -> " << disk[0].header.ranges[i / 2].start << endl;
    }
    f << "}" << endl;
}