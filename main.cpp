#include "filesystem.h"

#include <vector>

int main() {
	MyFilesystem fs("test_disk.img");
	fs.format();

	vector<sector_t> allocated_sectors;

	for(int i = 0; i < 16; i++)
		allocated_sectors.push_back(fs.alloc_sector());

	for(int i = 0; i < 16; i++) {
		fs.free_sector(allocated_sectors.back());
		allocated_sectors.pop_back();
	}

	return 0;
}
