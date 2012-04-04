#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

static bool verbose=false;

/**
 * How do I get used?
 */
void usage(void) {
	printf("magicSmoke - Random access hdparm\n");
	printf("magicSmoke (-r|-w|-rw) [options] (file)\n");
	printf("magicSmoke perfomrs operations in the file specified by file.  If file exists and is not a block device, magicSmoke fails.  If file does not exist, it is created\n");
	printf("-r,--read	perform random reads\n");
	printf("-w,--write	perform random writes\n");
	printf("-b,--rwsize=#	do read/writes of # bytes [default: %lu]\n", sizeof(int));
	printf("-s #,--size=#	if a file is created, make it of size blocks default 10.\n");
	printf("-z #,--raw-size=#	if a file is created, make it of # bytes\n");
	printf("-a # --actions=#	perform # reads/writes [default: %u]\n", UINT_MAX);
	printf("-v, --verbose	write out progress output as we run\n");
}

/**
 * Make a file to do reads and/or writes to
 */
bool makeFile(char * filename, int size) {
	return false;
}

void get_options(int argc, char **argv, bool *read, bool *write, 
	unsigned int *size, unsigned int *rawsize, unsigned int  *actions, 
	unsigned int *rwsize, char *file) {
	
	//give some reasonable defaults for things
	*read=false;
	*write=false;
	*actions=UINT_MAX;
	*size=10;
	*rawsize=0;
	*rwsize=sizeof(int);
	
	//Set up options array
	static struct option opts[] = {
		{"verbose",	no_argument,		(int *)&verbose,	true},
		{"write",	no_argument, 		0,		'w'},
		{"read",	no_argument,		0,		'r'},
		{"size",	required_argument,	0,		's'},
		{"rawsize",	required_argument,	0,		'z'},
		{"actions",	required_argument,	0,		'a'},
		{"rwsize",	required_argument,	0,		'b'},
		{"help",	no_argument,		0,		'h'},
		{0,0,0,0}
	};
	
	int c;
	int option_index=0;
	while ((c=getopt_long(argc, argv, "vwrhs:z:a:", 
		opts, &option_index))!=-1) {

		switch (c) {
			case 0:
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
				break;
			case 'v':
				verbose=true;
				break;
			case 'r':
				*read=true;
				break;
			case 'w':
				*write=true;
				break;
			case 's':
				*size=atoi(optarg);
				break;
			case 'z':
				*rawsize=atoi(optarg);
				break;
			case 'a':
				*actions=atoi(optarg);
				break;
			case 'b':
				*rwsize=atoi(optarg);
				break;
			case '?':
				break;
			default:
				break;
		}
	}
	
	//okay, last param shoudl be filename
	if (optind+1!=argc) {
		printf("You must specify a (single) file or disk!\n\n");
		usage();
		exit(EXIT_FAILURE);
	}
	
	file=argv[optind];
	
	if (verbose) {
		printf("Arguments from getopt:\n\tVerbose: %i\n\tDo writes:%i\n\tDo reads: %i\n\tRead-Write sizeL %i\n\tSize: %u\n\tRawsize: %u\n\tActions: %u\n\tFile: %s\n"
		, verbose, (int)*write, (int)*read, *rwsize, *size, *rawsize, *actions,file);
	}
}

int main(int argc, char **argv) {
	//Lets:
	//set up variables (default values set by getting options
	bool read;
	bool write;
	unsigned int size;
	unsigned int rawsize;
	unsigned int actions;
	unsigned int rwsize;
	char *file=NULL;
	//Parse arguments
	get_options(argc, argv, &read, &write, &size, &rawsize,
		&actions, &rwsize, file);
	
	//represents the size of this file, as the FS sees it
	unsigned long realsize;
	//If using files, create file or fail
	if (file_exists(file)) {
		if (!is_block_device(file)) {
			printf("File %s already exists! I refuse to overwrite an existing file!\n\n",file);
			usage();
			exit(EXIT_FAILURE);
			//dont want to overwrite a file
		}
		//file is a block device.  We'll get size to use
		get_block_device_size(file);
		
		//lets give a final chance to back out...
		last_chance();
	} else {
		//create a file
		if (rawsize!=0) {
			//use rawsize
			realsize=rawsize;
		} else {
			//get system blocksize
			realsize=get_block_size(file) * size;
		}
		make_file(realsize);
	}
	//mark already run time
	//Do random reads/writes
	//calculate result for both real and process time
	return EXIT_SUCCESS;
}
