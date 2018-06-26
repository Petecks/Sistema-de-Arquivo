
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include "disk.h"

const unsigned int DISK_MAGIC = 0xdeadbeef;

static FILE *diskfile;
static int nblocks = 0;
static int nreads = 0;
static int nwrites = 0;

int disk_init (const char *filename, int n) {
	diskfile = fopen(filename, "r+");
	if(!diskfile) diskfile = fopen(filename, "w+");
	if(!diskfile) return 0;

	ftruncate(fileno(diskfile), n*DISK_BLOCK_SIZE);

	nblocks = n;
	nreads = 0;
	nwrites = 0;

	return 1;
}

int disk_size () {
	return nblocks;
}

static void sanity_check (int blocknum, const void *data) {
	if(blocknum < 0) {
		std::cout << "ERROR: blocknum (" << blocknum << ") is negative!" << std::endl;
		abort ();
	}

	if(blocknum >= nblocks) {
		std::cout << "ERROR: blocknum ("<< blocknum <<") is too big!" << std::endl;
		abort ();
	}

	if(!data) {
		std::cout << "ERROR: null data pointer!" << std::endl;
		abort ();
	}
}

void disk_read (int blocknum, char *data) {
	sanity_check (blocknum,data);

	fseek (diskfile, blocknum *DISK_BLOCK_SIZE, SEEK_SET);

	if(fread (data, DISK_BLOCK_SIZE, 1, diskfile) == 1) {
		nreads++;
	} else {
		std::cout << "ERROR: couldn't access simulated disk: " << strerror (errno) << std::endl;
		abort ();
	}
}

void disk_write (int blocknum, const char *data) {
	sanity_check (blocknum, data);

	fseek (diskfile, blocknum *DISK_BLOCK_SIZE, SEEK_SET);

	if(fwrite (data, DISK_BLOCK_SIZE, 1, diskfile) == 1) {
		nwrites++;
	} else {
		std::cout << "ERROR: couldn't access simulated disk: " << strerror (errno) << std::endl;
		abort ();
	}
}

void disk_close () {
	if (diskfile) {
		std::cout << nreads << " disk block reads" << std::endl;
		std::cout << nwrites << " disk block writes" << std::endl;
		fclose (diskfile);
		diskfile = 0;
	}
}
