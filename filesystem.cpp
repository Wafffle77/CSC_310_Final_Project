#include "filesystem.h"
#include <cstring>

#include <unordered_map>
#include <vector>

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

int comp_allocator_ranges(const void *a, const void *b) {
    return (((allocator_range_t*)a)->start - ((allocator_range_t*)b)->start);
}

void MyFilesystem::defragment() {
    allocator_range_t temp[HEADER_RANGES_SIZE];
    memcpy(temp, disk[0].header.ranges, sizeof(temp));
    qsort(temp + 1, disk[0].header.heap_size - 1, sizeof(allocator_range_t), comp_allocator_ranges);

    for(int i = 1; i <= disk[0].header.heap_size - 1; i++) {
        allocator_range_t *cur       = &disk[0].header.ranges[i];
        allocator_range_t *next      = &disk[0].header.ranges[i+1];

        if(cur->start == 0) continue;

        if(cur->end >= next->start) {
            cur->end = next->end;
            next->start = 0;
        }
    }
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
        defragment();
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
sector_t my_hash(string name) {
        sector_t sum = 0;
        for(int i = 0; i < name.length(); i++){
            sum *= 7;
            sum += name[i];
        }
        return sum % ENTRY_CHILDREN_SIZE;
}

//access element in the hashtable by name
sector_t MyFilesystem::entry_access(name_t name, sector_t block_index) {
	sector_union_t* block = &disk[block_index];
    if(block == nullptr){
        return -1;
    }

	sector_t* children= block->entry.children;
    sector_t index = my_hash(name);

    //linear probing

	if(children[index]==0){
		return -1; // entry does not exist
	}

	name_t name_temp; 
	if(children[index]!=-1){
		strncpy(name_temp, disk[children[index]].entry.name, sizeof(name_t));
	}

	while(children[index]!=0 && strncmp(name_temp, name, sizeof(name_t))!=0){
		index+=1;
		index%=ENTRY_CHILDREN_SIZE;
		if(children[index]==0){
			break;
		}
		//-1 is tombstone
		if(children[index]==-1){
			continue;
		}
		strncpy(name_temp, disk[children[index]].entry.name, sizeof(name_t));
	}

	if(children[index]==0) {
		return -1;
	}
    return index;
}

//insert element in the hashtable by name
//currently only uses name but in the future a type could be useful
int MyFilesystem::entry_insert(entry_t entry, sector_t block_index){

sector_union_t* block = &disk[block_index];

    if(block == nullptr){
        return -1;
    }

	sector_t* children = block->entry.children;
    sector_t index = my_hash(entry.name);

    //linear probing

	while(children[index] != 0 && (children[index] != -1)){
		index += 1;
		index %= ENTRY_CHILDREN_SIZE;
		if(children[index] == 0){
			break;
		}
		//-1 is tombstone
		if(children[index] == -1){
			break;
		}
	}

	children[index] = alloc_sector();
	sector_t macro_index = children[index];
	disk[macro_index].entry = entry;
	
	return 0; // success
}

int MyFilesystem::entry_remove(name_t name,sector_t block_index){

	sector_union_t* block = &disk[block_index];
	sector_t index = entry_access(name,block_index);
	if(index==-1){
		return -1; //throw error
	}

	free_sector(block->entry.children[index]);
	block->entry.children[index] = -1; // place tombstone
	return 0; // success

}
