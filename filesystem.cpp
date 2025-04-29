#include "filesystem.h"
#include <stdlib.h>

#include <cstring>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>

#include "avl.h"

using namespace std;

#define MIN(a,b) (a<b?a:b)

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
	fd = ::open(path.c_str(), O_RDWR);
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

		status = ::close(fd);
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
			{
				.start = 2,
				.end = (sector_t)(size / SECTOR_SIZE)
			}
		}
	};

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
	if(sector == 0) return;
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

		if (entry.type == ENTRY_TYPE_DIRECTORY)
		{
			for (int i = 0; i < ENTRY_CHILDREN_SIZE; i++)
			{
				if (entry.children[i] == 0 || entry.children[i] == -1)
					continue;
				q.push(entry.children[i]);
				f << "\t" << cur << " -> " << entry.children[i] << endl;
			}
		}

		if (entry.type == ENTRY_TYPE_FILE) {
			if(entry.file.start != 0)
				f << "\t" << cur << " -> " << entry.file.start << endl;
			if(entry.file.end != 0)
				f << "\t" << cur << " -> " << entry.file.end << endl;
			for(sector_t cur = entry.file.start; cur != 0; cur = disk[cur].file_node.next) {
				f << "\t" << cur << " [color=\"magenta\"]" << endl;
				sector_t n = disk[cur].file_node.next;
				if(n != 0)
					f << "\t" << cur << " -> " << n << endl;
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


vector<entry_t> MyFilesystem::readdir(string path) {
	sector_t sector = resolve(path);
	return readdir(sector);
}

#define CHILDREN disk[sector].entry.children[index]
vector<entry_t> MyFilesystem::readdir(sector_t sector)
{
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


int MyFilesystem::open(string path) {
	sector_t f = resolve(path);
	if(f == 0 || disk[f].entry.type != ENTRY_TYPE_FILE) {
		return -1;
	}

	open_file_t entry = {
		.file = f,
		.cur = disk[f].entry.file.start,
		.pos = 0,
	};

	int fd = rand();
	fd_table[fd] = entry;
	return fd;
}

void MyFilesystem::close(int fd) {
	fd_table.erase(fd);
}

// Truncates a file to end at the current position
void MyFilesystem::trunc(int fd) {
	open_file_t* f = &fd_table[fd];
	entry_t* entry = &disk[f->file].entry;
	entry->file.size = f->pos;
	entry->file.end = f->cur;
	sector_t cur = disk[f->cur].file_node.next;
	while(cur != 0) {
		free_sector(cur);
		cur = disk[cur].file_node.next;
	}
}

uint64_t MyFilesystem::read(int fd, uint8_t *buf, uint64_t len) {
	open_file_t* f = &fd_table[fd];
	entry_t* entry = &disk[f->file].entry;
	uint64_t bytes_read = 0;
	while(len > 0 && f->pos < entry->file.size && f->cur != 0) {
		uint64_t in_node_pos = f->pos % FILE_NODE_DATA_SIZE;
		uint64_t copy_size = FILE_NODE_DATA_SIZE - in_node_pos;
		if(len < copy_size)
			copy_size = len;
		if(entry->file.size - f->pos < copy_size)
			copy_size = entry->file.size - f->pos;
		memcpy(buf, disk[f->cur].file_node.data + in_node_pos, copy_size);
		buf += copy_size;
		bytes_read += copy_size;
		len -= copy_size;
		f->pos += copy_size;
		if(len > 0)
			f->cur = disk[f->cur].file_node.next;
	}
	return bytes_read;
}

uint64_t MyFilesystem::write(int fd, uint8_t *buf, uint64_t len) {
	open_file_t* f = &fd_table[fd];
	entry_t* entry = &disk[f->file].entry;
	uint64_t bytes_read = 0;
	while(len > 0 && f->pos < entry->file.size && f->cur != 0) {
		uint64_t in_node_pos = f->pos % FILE_NODE_DATA_SIZE;
		uint64_t copy_size = FILE_NODE_DATA_SIZE - in_node_pos;
		memcpy(disk[f->cur].file_node.data + in_node_pos, buf, copy_size);
		buf += copy_size;
		len -= copy_size;
		bytes_read += copy_size;
		f->pos += copy_size;
		f->cur = disk[f->cur].file_node.next;
	}

	while(len > 0) {
		sector_t new_end = alloc_sector();
		disk[new_end].file_node.prev = entry->file.end;
		disk[entry->file.end].file_node.next = new_end;
		disk[new_end].file_node.next = 0;

		uint64_t copy_size = FILE_NODE_DATA_SIZE;
		if(FILE_NODE_DATA_SIZE > len)
			copy_size = len;

		memcpy(disk[new_end].file_node.data, buf, copy_size);

		buf += copy_size;
		len -= copy_size;
		bytes_read += copy_size;
		f->pos += copy_size;
		f->cur = disk[new_end].file_node.next;
		
		entry->file.size = f->pos + 1;
		entry->file.end = new_end;
		if(entry->file.start == 0)
			entry->file.start = new_end;
	}

	return bytes_read;
}

uint64_t MyFilesystem::seek(int fd, int64_t offset, int whence) {
	open_file_t* f = &fd_table[fd];
	entry_t* entry = &disk[f->file].entry;

	switch(whence) {
		case MY_SEEK_END:
			offset = entry->file.size - offset;
			// Look ma, no break!
		case MY_SEEK_SET:
			offset -= f->pos;
			break;
		case MY_SEEK_CUR:
			offset = f->pos + offset;
			break;
	}

	bool forward = offset > 0;

	uint64_t pos_in_node = f->pos % FILE_NODE_DATA_SIZE;
	offset += pos_in_node;
	f->pos -= pos_in_node;

	while(f->cur != 0 && offset >= FILE_NODE_DATA_SIZE) {
		if(forward)
			f->cur = disk[f->cur].file_node.next;
		else
			f->cur = disk[f->cur].file_node.prev;
		f->pos += FILE_NODE_DATA_SIZE;
		offset -= FILE_NODE_DATA_SIZE;
	}
	
	f->pos += offset;
	return f->pos;
}

bool MyFilesystem::eof(int fd) {
	return fd_table[fd].pos == disk[fd_table[fd].file].entry.file.size;
}