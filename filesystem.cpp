#include "filesystem.h"
#include <cstring>

#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>

#include "avl.h"

using namespace std;

MyFilesystem::MyFilesystem(sector_union_t *disk_ptr, uint64_t disk_size)
{
	disk = disk_ptr;
	size = disk_size;
	fd = -1;
}

MyFilesystem::MyFilesystem(sector_t sectors)
{
	disk = new sector_union_t[sectors];
	size = sectors * SECTOR_SIZE;
	fd = -1;
}

// TODO: use exceptions in this
MyFilesystem::MyFilesystem(string path)
{
	fd = open(path.c_str(), O_RDWR);
	if (fd == -1)
	{
		perror("Unable to open disk");
		exit(1);
	}

	size = lseek(fd, 0, SEEK_END);
	if (size == (off_t)-1)
	{
		perror("Unable to get disk size");
		exit(1);
	}
	lseek(fd, 0, SEEK_SET);

	disk = (sector_union_t *)mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (disk == MAP_FAILED)
	{
		perror("Unable to map disk");
		exit(1);
	}
}

MyFilesystem::~MyFilesystem()
{
	if (fd != -1)
	{
		int status = munmap(disk, size);
		if (status != 0)
		{
			perror("Unable to unmap disk");
			exit(1);
		}

		status = close(fd);
		if (status != 0)
		{
			perror("Unable to close disk");
			exit(1);
		}
	}
}

void MyFilesystem::format()
{
	disk[0].header = {
		.root_entry = 1,
		.heap_size = 1,
		.ranges = {
			{},
			{.start = 2,
			 .end = (sector_t)(size / SECTOR_SIZE)}}};

	disk[1].entry = {
		.parent = 0,
		.type = ENTRY_TYPE_DIRECTORY,
		.metadata = 0,
		.name = "ROOT_DIR",
		.children = {0},
	};
}

int comp_allocator_ranges(const void *a, const void *b)
{
	return (((allocator_range_t *)a)->start - ((allocator_range_t *)b)->start);
}
// goal is to move zeros to the right
int comp_allocator_ranges_clean(const void *a, const void *b)
{
	// return (((allocator_range_t*)a)->start < ((allocator_range_t*)b)->start);
	return ((allocator_range_t *)a)->start == 0;
}

void MyFilesystem::defragment()
{
	// initialize variables
	int limit = disk[0].header.heap_size - 1;
	allocator_range_t *cur = &disk[0].header.ranges[0];
	allocator_range_t *next = &disk[0].header.ranges[1];
	int cur_i = 0;
	int next_i = 1;
	// march through the array

	for (cur_i = 0; cur_i < limit - 1; cur_i++)
	{
		for (next_i = 0; next_i < limit; next_i += 1)
		{
			cur = &disk[0].header.ranges[cur_i];
			next = &disk[0].header.ranges[next_i];
			// overlap case
			// cur start must be less than next start
			if (cur->end >= next->start && cur->start < next->start)
			{
				// ignore cur = (0,0)
				if (cur->start == 0 && cur->end == 0)
				{
					continue;
				}
				// ignore next = (0,0)
				if (next->start == 0 && next->end == 0)
				{
					continue;
				}
				cur->end = next->end;

				// set next to 'null'-ish
				next->start = 0;
				next->end = 0;
			}
		}
	}

	// move all the (0,0) to the end
	qsort(disk[0].header.ranges, disk[0].header.heap_size - 1, sizeof(allocator_range_t), comp_allocator_ranges_clean);
}

sector_t MyFilesystem::alloc_sector()
{
	// Increment the lowest range (and remember the sector)
	sector_t ret = disk->header.ranges[1].start++;

	// If the range is empty, pop and discard it
	if (disk->header.ranges[1].start >= disk->header.ranges[1].end)
	{
		uint32_t i = --disk->header.heap_size;
		disk->header.ranges[1] = disk->header.ranges[i];

		// Percolate
		while (i <= disk[0].header.heap_size)
		{
			allocator_range_t *parent = &disk->header.ranges[i];
			allocator_range_t *right = &disk->header.ranges[2 * i + 1];
			allocator_range_t *left = &disk->header.ranges[2 * i];
			allocator_range_t *child;

			if (2 * i > HEADER_RANGES_SIZE)
			{
				break;
				// TODO: trigger defrag
			}

			allocator_range_t temp = *parent;

			if (left->start < right->start)
			{
				child = left;
				i = 2 * i;
			}
			else
			{
				child = right;
				i = 2 * i + 1;
			}

			if (child->start < parent->start)
			{
				*parent = *child;
				*child = temp;
			}
			else
			{
				break;
			}
		}
	}
	return ret;
}

void MyFilesystem::free_sector(sector_t sector)
{
	if (disk->header.heap_size + 1 >= HEADER_RANGES_SIZE)
	{
		defragment();
	}

	uint32_t i = ++disk->header.heap_size;
	disk->header.ranges[i] = {
		.start = sector,
		.end = sector + 1};

	// Percolate
	while (i >= 1)
	{
		allocator_range_t *parent = &disk->header.ranges[i / 2];
		allocator_range_t *child = &disk->header.ranges[i];

		if (parent->start > child->start)
		{
			allocator_range_t temp = *child;
			*child = *parent;
			*parent = temp;

			i /= 2;
		}
		else
		{
			break;
		}
	}
}

void MyFilesystem::debug_heap(string path)
{
	ofstream f(path);
	f << "digraph {" << endl;
	f << "\t" << disk[0].header.ranges[1].start << " [label=\"" << disk[0].header.ranges[1].start << "-" << disk[0].header.ranges[1].end << "\"]" << endl;
	for (int i = 2; i < disk[0].header.heap_size; i++)
	{
		f << "\t" << disk[0].header.ranges[i].start << " [label=\"" << disk[0].header.ranges[i].start << "-" << disk[0].header.ranges[i].end << "\"]" << endl;
		f << "\t" << disk[0].header.ranges[i].start << " -> " << disk[0].header.ranges[i / 2].start << endl;
	}
	f << "}" << endl;
}

unordered_map<e_entry_type_t, string> COLOR_MAP = {
	{ENTRY_TYPE_DIRECTORY, "blue"},
	{ENTRY_TYPE_FILE, "green"},
	{ENTRY_TYPE_NOTHING, "red"},
};

void MyFilesystem::debug_tree(string path)
{
	ofstream f(path);
	queue<sector_t> q;
	q.push(disk[0].header.root_entry);

	f << "digraph {" << endl;
	while (q.size() > 0)
	{
		sector_t cur = q.front();
		entry_t entry = disk[cur].entry;
		q.pop();

		f << "\t" << cur << " [label=\"" << entry.name << "\" color=\"" << COLOR_MAP[entry.type] << "\"]" << endl;

		if (entry.type = ENTRY_TYPE_DIRECTORY)
		{
			for (int i = 0; i < ENTRY_CHILDREN_SIZE; i++)
			{
				if (entry.children[i] == 0 || entry.children[i] == -1)
					continue;
				q.push(entry.children[i]);
				f << "\t" << cur << " -> " << entry.children[i] << endl;
			}
		}
	}

	// for(int i = 2; i < disk[0].header.heap_size; i++) {
	//     f << "\t" << disk[0].header.ranges[i].start << " [label=\"" << disk[0].header.ranges[i].start << "-" << disk[0].header.ranges[i].end << "\"]" << endl;
	//     f << "\t" << disk[0].header.ranges[i].start << " -> " << disk[0].header.ranges[i / 2].start << endl;
	// }
	f << "}" << endl;
}

sector_t my_hash(string name)
{
	sector_t sum = 0;
	for (int i = 0; i < name.length(); i++)
	{
		sum *= 7;
		sum += name[i];
	}
	return sum % ENTRY_CHILDREN_SIZE;
}

#define CHILDREN disk[block_index].entry.children[index]
// access element in the hashtable by name
sector_t MyFilesystem::entry_access(name_t name, sector_t block_index)
{
	sector_t index = my_hash(name);

	// linear probing

	if (CHILDREN == 0)
	{
		return -1; // entry does not exist
	}

	name_t name_temp;
	if (CHILDREN != -1)
	{
		strncpy(name_temp, disk[CHILDREN].entry.name, sizeof(name_t));
	}

	while (CHILDREN != 0 && strncmp(name_temp, name, sizeof(name_t)) != 0)
	{
		index += 1;
		index %= ENTRY_CHILDREN_SIZE;
		if (CHILDREN == 0)
		{
			break;
		}
		//-1 is tombstone
		if (CHILDREN == -1)
		{
			continue;
		}
		strncpy(name_temp, disk[CHILDREN].entry.name, sizeof(name_t));
	}

	if (CHILDREN == 0)
	{
		return -1;
	}
	return CHILDREN;
}

// insert element in the hashtable by name
// currently only uses name but in the future a type could be useful
sector_t MyFilesystem::entry_insert(entry_t entry, sector_t block_index)
{
	if (block_index == 0)
		return 0;
	sector_t index = my_hash(entry.name);

	// linear probing

	while (CHILDREN != 0 && (CHILDREN != -1))
	{
		index += 1;
		index %= ENTRY_CHILDREN_SIZE;
		if (CHILDREN == 0)
		{
			break;
		}
		//-1 is tombstone
		if (CHILDREN == -1)
		{
			break;
		}
	}

	CHILDREN = alloc_sector();
	sector_t macro_index = CHILDREN;
	disk[macro_index].entry = entry;

	return macro_index; // success
}
#undef CHILDREN

sector_t MyFilesystem::entry_remove(name_t name, sector_t block_index)
{
	sector_union_t *block = &disk[block_index];
	sector_t index = entry_access(name, block_index);
	if (index == -1)
	{
		return 0; // throw error
	}

	free_sector(block->entry.children[index]);
	block->entry.children[index] = -1; // place tombstone
	return index;					   // success
}

// Path resolution

sector_t MyFilesystem::resolve(string path)
{
	if (path == "/")
		return disk[0].header.root_entry;
	return resolve(path.c_str(), disk[0].header.root_entry);
}

sector_t MyFilesystem::resolve(const char *path, sector_t entry)
{
	name_t name = {0};
	int i;

	// Assuming the path starts with a separator
	for (i = 1; path[i] != '/' && path[i] != 0; i++)
	{
		name[i - 1] = path[i];
	}

	sector_t child = entry_access(name, entry);
	if (child == 0)
		return 0;
	if (path[i] == 0)
		return child;
	return resolve(path + i, child);
}

sector_t MyFilesystem::create(string name, sector_t dir)
{
	// TODO: Throw exception
	if (disk[dir].entry.type != ENTRY_TYPE_DIRECTORY)
		return 0;

	entry_t child = {
		.parent = dir,
		.type = ENTRY_TYPE_FILE,
		.file = {
			.start = 0,
			.end = 0,
			.size = 0,
		}};
	str2name(name, child.name);

	return entry_insert(child, dir);
}

sector_t MyFilesystem::mkdir(string name, sector_t dir)
{
	// TODO: Throw exception
	if (disk[dir].entry.type != ENTRY_TYPE_DIRECTORY)
		return 0;

	entry_t child = {
		.parent = dir,
		.type = ENTRY_TYPE_DIRECTORY,
		.data = {0},
	};
	str2name(name, child.name);

	return entry_insert(child, dir);
}

bool compare_entries(entry_t entry_one, entry_t entry_two)
{
	return strncmp(entry_one.name, entry_two.name, sizeof(name_t)) < 0;
}

#define CHILDREN disk[sector].entry.children[index]
vector<entry_t> MyFilesystem::readdir(string path)
{
	sector_t sector = resolve(path);

	AVL avl;
	for (int index = 0; index < ENTRY_CHILDREN_SIZE; index++)
	{
		if (CHILDREN != 0)
		{
			avl.Insert(disk[CHILDREN].entry);
		}
	}
	return avl.Sort();
}
#undef CHILDREN
