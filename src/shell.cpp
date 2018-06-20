
#include "fs.h"
#include "disk.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>

static int do_copyin( const char *filename, int inumber );
static int do_copyout( int inumber, const char *filename );

int main( int argc, char *argv[] )
{
	char line[1024];
	char cmd[1024];
	char arg1[1024];
	char arg2[1024];
	int inumber, result, args;

	if(argc!=3) {
		std::cout << "use: " << argv[0] << " <diskfile> <nblocks>" << std::endl;
		return 1;
	}

	if(!disk_init(argv[1],atoi(argv[2]))) {
		std::cout << "couldn't initialize " << argv[1] << ":" << strerror(errno) << std::endl;
		return 1;
	}

	std::cout << "opened emulated disk image " << argv[1] << " with " << disk_size() << " blocks" << std::endl;

	while(1) {
		std::cout << " simplefs> ";
		fflush(stdout);

		if(!fgets(line,sizeof(line),stdin)) break;

		if(line[0]=='\n') continue;
		line[strlen(line)-1] = 0;

		args = sscanf(line,"%s %s %s",cmd,arg1,arg2);
		if(args==0) continue;

		if(!strcmp(cmd,"format")) {
			if(args==1) {
				if(fs_format()) {
					std::cout << "disk formatted." << std::endl;
				} else {
					std::cout << "format failed!" << std::endl;
				}
			} else {
				std::cout << "use: format" << std::endl;
			}
		} else if(!strcmp(cmd,"mount")) {
			if(args==1) {
				if(fs_mount()) {
					std::cout << "disk mounted." << std::endl;
				} else {
					std::cout << "mount failed!" << std::endl;
				}
			} else {
				std::cout << "use: mount" << std::endl;
			}
		} else if(!strcmp(cmd,"debug")) {
			if(args==1) {
				fs_debug();
			} else {
				std::cout << "use: debug" << std::endl;
			}
		} else if(!strcmp(cmd,"getsize")) {
			if(args==2) {
				inumber = atoi(arg1);
				result = fs_getsize(inumber);
				if(result>=0) {
					std::cout << "inode " << inumber << " has size " << result << std::endl;
				} else {
					std::cout << "getsize failed!" << std::endl;
				}
			} else {
				std::cout << "use: getsize <inumber>" << std::endl;
			}

		} else if(!strcmp(cmd,"create")) {
			if(args==1) {
				inumber = fs_create();
				if(inumber>0) {
					std::cout << "created inode "<< inumber << std::endl;
				} else {
					std::cout << "create failed!" << std::endl;
				}
			} else {
				std::cout << "use: create" << std::endl;
			}
		} else if(!strcmp(cmd,"delete")) {
			if(args==2) {
				inumber = atoi(arg1);
				if(fs_delete(inumber)) {
					std::cout << "inode " << inumber << " deleted." << std::endl;
				} else {
					std::cout << "delete failed!" << std::endl;
				}
			} else {
				std::cout << "use: delete <inumber>" << std::endl;
			}
		} else if(!strcmp(cmd,"cat")) {
			if(args==2) {
				inumber = atoi(arg1);
				if(!do_copyout(inumber,"/dev/stdout")) {
					std::cout << "cat failed!" << std::endl;
				}
			} else {
				std::cout << "use: cat <inumber>" << std::endl;
			}

		} else if(!strcmp(cmd,"copyin")) {
			if(args==3) {
				inumber = atoi(arg2);
				if(do_copyin(arg1,inumber)) {
					std::cout << "copied file " << arg1 << " to inode " << inumber << std::endl;
				} else {
					std::cout << "copy failed!" << std::endl;
				}
			} else {
				std::cout << "use: copyin <filename> <inumber>" << std::endl;
			}

		} else if(!strcmp(cmd,"copyout")) {
			if(args==3) {
				inumber = atoi(arg1);
				if(do_copyout(inumber,arg2)) {
					std::cout << "copied inode " << inumber << " to file " << arg2 << std::endl;
				} else {
					std::cout << "copy failed!" << std::endl;
				}
			} else {
				std::cout << "use: copyout <inumber> <filename>" << std::endl;
			}

		} else if(!strcmp(cmd,"help")) {
			std::cout << "Commands are:" << std::endl;
			std::cout << "    format" << std::endl;
			std::cout << "    mount" << std::endl;
			std::cout << "    debug" << std::endl;
			std::cout << "    create" << std::endl;
			std::cout << "    delete  <inode>" << std::endl;
			std::cout << "    cat     <inode>" << std::endl;
			std::cout << "    copyin  <file> <inode>" << std::endl;
			std::cout << "    copyout <inode> <file>" << std::endl;
			std::cout << "    help" << std::endl;
			std::cout << "    quit" << std::endl;
			std::cout << "    exit" << std::endl;
		} else if(!strcmp(cmd,"quit")) {
			break;
		} else if(!strcmp(cmd,"exit")) {
			break;
		} else {
			std::cout << "unknown command: " << cmd << std::endl;
			std::cout << "type 'help' for a list of commands." << std::endl;
			result = 1;
		}
	}

	std::cout << "closing emulated disk." << std::endl;
	disk_close();

	return 0;
}

static int do_copyin( const char *filename, int inumber )
{
	FILE *file;
	int offset=0, result, actual;
	char buffer[16384];

	file = fopen(filename,"r");
	if(!file) {
		std::cout << "couldn't open " << filename << ": " << strerror(errno) << std::endl;
		return 0;
	}

	while(1) {
		result = fread(buffer,1,sizeof(buffer),file);
		if(result<=0) break;
		if(result>0) {
			actual = fs_write(inumber,buffer,result,offset);
			if(actual<0) {
				std::cout << "ERROR: fs_write return invalid result" << actual << std::endl;
				break;
			}
			offset += actual;
			if(actual!=result) {
				std::cout << "WARNING: fs_write only wrote " << actual << " bytes, not "<< result << " bytes" << std::endl;
				break;
			}
		}
	}

	std::cout << offset << " bytes copied" << std::endl;

	fclose(file);
	return 1;
}

static int do_copyout( int inumber, const char *filename )
{
	FILE *file;
	int offset=0, result;
	char buffer[16384];

	file = fopen(filename,"w");
	if(!file) {
		std::cout << "couldn't open "<< filename <<": " << strerror(errno) << std::endl;
		return 0;
	}

	while(1) {
		result = fs_read(inumber,buffer,sizeof(buffer),offset);
		if(result<=0) break;
		fwrite(buffer,1,result,file);
		offset += result;
	}

	std::cout << offset << " bytes copied" << std::endl;

	fclose(file);
	return 1;
}
