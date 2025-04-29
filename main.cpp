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

	sector_t etc = fs.mkdir("etc", root);
	sector_t bin = fs.mkdir("bin", root);
	sector_t usr = fs.mkdir("usr", root);

	sector_t dir = fs.mkdir("dir", root);


	sector_t cat = fs.create("cat", bin);
	sector_t tee = fs.create("tee", bin);
	sector_t ls = fs.create("ls", bin);
	sector_t pwd= fs.create("pwd", bin);
	sector_t lolcat = fs.create("lolcat", bin);
	sector_t cowsay = fs.create("cowsay", bin); //the GOAT

	sector_t s = fs.resolve("/dir/zxcv");
	//cout << zxcv << endl << s << endl;
	vector<entry_t> entries = fs.readdir("/bin");
	for(int i=0; i< entries.size(); i++){
		cout<<entries[i].name<<endl;
	}

	fs.defragment();
	fs.debug_heap("heap2.dot");
	fs.debug_tree("fs.dot");

	return 0;
}
