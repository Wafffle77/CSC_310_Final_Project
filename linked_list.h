#include <stdint.h>
#include <unistd.h>

#include "types.h"

#define BUFFER_SIZE 5000
#define DISK_SIZE 500
#define NUM_PROCESSES 500
#define NUM_ENTRIES 500

file_node_t files[DISK_SIZE]; //Basically our file 'linked list' where inodes are the indexes
Process processes[NUM_PROCESSES] = {0};  //Simulating how file descriptors work (as far as I can tell lol)

/*
 * not used for anything yet but could be useful to use to keep track of which inodes have already been used 
 * for our custom alloc function for later
 * */
int used_inodes[DISK_SIZE];  //Maybe replace with AVL tree later?


uint64_t my_open(const char* pathname, uint32_t flags){
	uint32_t inode = 0; //replace with our very epic trie/hashtable logic later
	
	uint32_t fd;
	//reserve file descriptor 0,1, and 2 because that is what heros do
	for(uint32_t i=3; i<NUM_PROCESSES; i++){
		if(!processes[i].busy){
			fd=i;
			processes[fd].busy = 1;
			break;
		}
	}

	processes[fd].flags = flags;
	processes[fd].inode = inode;
	processes[fd].offset=0;
	processes[fd].busy = 1;
	return fd;

}
uint64_t my_close(int fd){
	processes[fd].busy=0;
	processes[fd].inode=0;
	processes[fd].flags=0;
	processes[fd].offset=0;
	return 0;
}


// Reads count bytes from the file at the current position into buf
// Returns bytes read
uint64_t my_read(int fd, uint8_t buf[BUFFER_SIZE], uint64_t count) {

	if(!processes[fd].busy){
		printf("INVALID FILE DESCRIPTOR\n");
		return 1;
	}


	uint32_t inode = processes[fd].inode;
	uint16_t index = 0;
	for(int i=0; i<count; i++){
		buf[index] = files[inode].data[processes[fd].offset];
		processes[fd].offset++;
		if (processes[fd].offset>=FILE_NODE_DATA_SIZE){
			if(files[inode].next==0){
				return -1; //eof error :(
			}
			processes[fd].inode = files[inode].next;	
			processes[fd].offset=0;
		}
		inode = processes[fd].inode;
		index++;
	}
	return 0; //success (based)
}

// Write count bytes to the file from buf at the current position
// This will overwrite data if it's in the middle of the file and
// append data if it's at the end of the file.
// Returns bytes written
uint64_t my_write(int fd, uint8_t buf[BUFFER_SIZE], uint64_t count) {
	if(!processes[fd].busy){
		printf("INVALID FILE DESCRIPTOR\n");
		return 1;
	}

	if(processes[fd].flags&1!=1){
		printf("NOT IN WRITE MODE\n");
		return 1;
	}


	uint32_t inode = processes[fd].inode;
	uint16_t index = 0;
	for(int i=0; i<count; i++){
		files[inode].data[processes[fd].offset] = buf[index];
		processes[fd].offset++;
		if (processes[fd].offset>=FILE_NODE_DATA_SIZE){
			if(files[inode].next==0){
				return -1; //eof error, in the future we will replace with our my_alloc logic with hashtable
			}
			processes[fd].inode = files[inode].next;	
			processes[fd].offset=0;
		}
		inode = processes[fd].inode;
		index++;
	}
	return 0; //success (based)
}

// Seek moves the current position in the file
// Values for whence:
// - SEEK_SET is the start
// - SEEK_END is the end
// - SEEK_CUR is the current position (can be negative)
uint64_t my_seek(int fd, int64_t offset, int whence) {
	if(whence==SEEK_SET){
		if(offset<0){
			return -1; //out of bounds error
		}
		int inode = processes[fd].inode;
		while(inode!=files[inode].prev){
			inode = files[inode].prev;
			processes[fd].inode = inode;
		}
		for(int i=0; i<offset/FILE_NODE_DATA_SIZE; i++){
			inode = files[inode].next;
			processes[fd].inode = inode;
		}
		processes[fd].offset = offset%FILE_NODE_DATA_SIZE;
		return 0;
	}
	//I have not tested SEEK_CUR yet...
	else if(whence==SEEK_CUR){
		int inode = processes[fd].inode;


		int32_t sector_offset = offset/FILE_NODE_DATA_SIZE;

		int8_t sign = 1;
		if (sector_offset<0){
			sign=-1;
			sector_offset*=-1;
		}
		for(int i=0;i<sector_offset;  i++){
			if(sign==1){
				if(files[inode].next==0){
					return -1; //out of bounds
				}
				inode = files[inode].next;
			}
			else if (sign==-1){
				if(files[inode].prev==0){
					return -1; //out of bounds
				}
				inode = files[inode].prev;
			}
			processes[fd].inode = inode;
		}
		processes[fd].offset =(processes[fd].offset+ offset%FILE_NODE_DATA_SIZE)%FILE_NODE_DATA_SIZE;
		return 0;
	}
	//todo: make SEEK_END
}
