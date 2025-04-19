#include <stdint.h>
#include <unistd.h>

#include "types.h"


// Reads count bytes from the file at the current position into buf
// Returns bytes read
uint64_t my_read(int fd, uint8_t* buf, uint64_t count);

// Write count bytes to the file from buf at the current position
// This will overwrite data if it's in the middle of the file and
// append data if it's at the end of the file.
// Returns bytes written
uint64_t my_write(int fd, uint8_t buf, uint64_t count);

// Seek moves the current position in the file
// Values for whence:
// - SEEK_SET is the start
// - SEEK_END is the end
// - SEEK_CUR is the current position (can be negative)
uint64_t my_seek(int fd, uint64_t offset, int whence);