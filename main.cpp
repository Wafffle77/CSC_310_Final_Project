#include "filesystem.h"

#include <vector>
#include <cstring>

#include "avl.h"

#include <sstream>
#include <iostream>
using namespace std;

void test_avl()
{
	name_t words[] = {"hello", "math", "chicken", "wolf", "dolphin", "memes", "apple", "banana", "whale", "zebra"};

	AVL avl;
	for (int i = 0; i < sizeof(words); i++)
	{
		entry_t entry;
		strncpy(entry.name, words[i], sizeof(name_t));
		avl.Insert(entry);
	}

	vector<entry_t> nums = avl.Sort();

	for (int i = 0; i < nums.size(); i++)
	{
		cout << nums[i].name << " ";
	}
}

int main()
{
	MyFilesystem fs("test_disk.img");
	fs.format();
	vector<sector_t> allocated_sectors;
	sector_t root = fs.resolve("/");

	sector_t etc = fs.mkdir("etc", root);
	sector_t bin = fs.mkdir("bin", root);
	sector_t usr = fs.mkdir("usr", root);

	sector_t dir = fs.mkdir("dir", root);

	fs.create("cat", bin);
	fs.create("tee", bin);
	fs.create("ls", bin);
	fs.create("pwd", bin);
	fs.create("lolcat", bin);
	fs.create("cowsay", bin); // the GOAT

	// sector_t s = fs.resolve("/dir/zxcv");
	// cout << zxcv << endl << s << endl;
	// vector<entry_t> entries = fs.readdir("/bin");
	// for (int i = 0; i < entries.size(); i++)
	// {
	// 	cout << entries[i].name << endl;
	// }

	fs.create("filesystem.cpp", root);


	int fd = fs.open("/filesystem.cpp");
	ifstream f("filesystem.cpp");
	stringstream stsr;
	stsr << f.rdbuf();
	string data = stsr.str();
	
	cout << fs.write(fd, (uint8_t*) data.c_str(), data.length() - 1) << endl;
	fs.close(fd);

	fd = fs.open("/filesystem.cpp");
	uint8_t test_buffer[4096] = {0};
	while(!fs.eof(fd)) {
		uint64_t bytes_read = fs.read(fd, test_buffer, sizeof(test_buffer));
		write(1, test_buffer, bytes_read);
		// cout << (char*) test_buffer << endl;
	}
	fs.close(fd);

	fs.defragment();
	fs.debug_heap("heap2.dot");
	fs.debug_tree("fs.dot");

	return 0;
}
