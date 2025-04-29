#include "filesystem.h"

#include <vector>
#include <cstring>

#include<iostream>
using namespace std;

int main() {
	MyFilesystem fs("test_disk.img");
	fs.format();
	vector<sector_t> allocated_sectors;
	sector_t root = fs.resolve("/");

	sector_t asdf = fs.create("asdf", root);
	sector_t qwer = fs.create("qwer", root);

	sector_t dir = fs.mkdir("dir", root);
	sector_t zxcv = fs.create("zxcv", dir);

	sector_t s = fs.resolve("/dir/zxcv");
	cout << zxcv << endl << s << endl;

	fs.defragment();
	fs.debug_heap("heap2.dot");
	fs.debug_tree("fs.dot");

	return 0;
}
