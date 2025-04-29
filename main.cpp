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

	for(int i = 0; i < 200; i++)
		allocated_sectors.push_back(fs.alloc_sector());
	sector_t asdf = fs.create("asdf", root);
	sector_t qwer = fs.create("qwer", root);

	sector_t dir = fs.mkdir("dir", root);
	sector_t zxcv = fs.create("zxcv", dir);

	sector_t s = fs.resolve("/dir/zxcv");
	cout << zxcv << endl << s << endl;

	int input = 0;

	while(input!=-1){
		cout<<"What do you want to do?\n0=insert\n1=access\n2=delete\n-1=quit"<<endl;
		cin>>input;
		if(input==-1){
			continue;
		}
		cout<<"enter name"<<endl;
		name_t name;
		for(int i=0; i<strlen(name); i++){
			if(name[i]=='\n'){
				cout<<"CRINGE"<<endl;
			}
		}
		cin>>name;

		if(input==0){
			entry_t entry;
			strncpy(entry.name,name,sizeof(name_t));
			int success = fs.entry_insert(entry,1);
			if(success== -1){
				cout<<"insertion failed :( "<<endl;
			}
			else{
				cout<<"insertion successful :)" <<endl;
			}

		}
		if(input==1){
			sector_t index = fs.entry_access(name,1);
			if(index== (uint32_t)-1){
				cout<<"sorry that name is not found in this sector union :( "<<endl;
			}
			else{
				cout<<"name found at index "<<index<<" in sector union 1"<<endl;
			}
		}
		if(input==2){
			int success = fs.entry_remove(name,1);
			if(success== -1){
				cout<<"removal failed :( "<<endl;
			}
			else{
				cout<<"removal successful :)" <<endl;
			}
		}

		cout<<endl;
		cout<<endl;
		cout<<endl;

	}
	for(int i = 0; i < 200; i++) {
		fs.free_sector(allocated_sectors.back());
		allocated_sectors.pop_back();
	}
	fs.defragment();
	fs.debug_heap("heap2.dot");
	fs.debug_tree("fs.dot");

	return 0;
}
