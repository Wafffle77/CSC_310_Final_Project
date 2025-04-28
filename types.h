#ifndef _FS_TYPES_H
#define _FS_TYPES_H

#include <stdint.h>

// Types and sizes
#define SECTOR_SIZE 4096
typedef uint32_t sector_t;
typedef char name_t[32];

// Computerd Constants
#define FILE_NODE_DATA_SIZE (SECTOR_SIZE - 2 * sizeof(sector_t))
#define ENTRY_DATA_SIZE (SECTOR_SIZE - sizeof(sector_t) - sizeof(uint64_t) - sizeof(e_entry_type_t) - sizeof(name_t))
#define ENTRY_CHILDREN_SIZE (ENTRY_DATA_SIZE / sizeof(sector_t))
#define HEADER_RANGES_SIZE ((SECTOR_SIZE - sizeof(uint32_t) - sizeof(uint32_t)) / sizeof(allocator_range_t))

// Enums
typedef enum {
    ENTRY_TYPE_NOTHING,
    ENTRY_TYPE_DIRECTORY,
    ENTRY_TYPE_FILE,

    TOTAL_ENTRY_TYPES
} e_entry_type_t;

// Structs
typedef struct {
    sector_t prev, next;
    uint8_t data[FILE_NODE_DATA_SIZE];
} file_node_t;

typedef struct {
    sector_t start, end;
    uint64_t size;
} file_info_t;

typedef struct {
    sector_t parent;
    uint64_t metadata;
    e_entry_type_t type;
    name_t name;
    union {
        uint8_t data[ENTRY_DATA_SIZE];

        // Directories
        sector_t children[ENTRY_CHILDREN_SIZE];

        // Files
        file_info_t file;
    };
} entry_t;

typedef struct {
    sector_t start, end;
} allocator_range_t;

typedef struct {
    sector_t root_entry;
    uint32_t heap_size;
    allocator_range_t ranges[HEADER_RANGES_SIZE];
} header_t;

typedef union {
    uint8_t raw[SECTOR_SIZE];
    entry_t entry;
    file_node_t file_node;
    header_t header;
} sector_union_t;

typedef struct{
	uint8_t busy;
	uint8_t flags;
	uint32_t inode;
	uint32_t offset;
} Process;


// Make sure that sizeof(sector_union_t) <= SECTOR_SIZE
typedef uint8_t sector_union_larger_than_sector_size[SECTOR_SIZE - sizeof(sector_union_t)];

#endif // _FS_TYPES_H
