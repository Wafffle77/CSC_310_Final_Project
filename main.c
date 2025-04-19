#include<stdio.h>
#include "linked_list.h"
#include<string.h>





void test_read_and_write(){
	uint32_t fd = my_open("file.txt",1);
	char* msg = "Happy Easter Ethan!";
	uint8_t write_buffer[BUFFER_SIZE];
	for(int i=0; i<strlen(msg); i++){
		write_buffer[i]=msg[i];
	}

	my_write(fd,write_buffer, strlen(msg));
	my_seek(fd,0,SEEK_SET);

	uint8_t read_buffer[BUFFER_SIZE] = {0};
	uint8_t read_count = strlen(msg);
	my_read(fd,read_buffer,read_count);
	for(int i=0; i<read_count; i++){
		printf("%c",read_buffer[i]);
	}
	printf("\n");
	my_close(fd);

}




int main(){

	test_read_and_write();

	return 0;
}
