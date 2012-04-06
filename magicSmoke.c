#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <time.h>

static bool verbose=false;

/**
 * How do I get used?
 */
void usage(void) {
	printf("magicSmoke - Random access disk speed tester\n");
	printf("magicSmoke (-r|-w|-rw) [options] (file)\n");
	printf("magicSmoke perfomrs operations in the file specified by file.  If file exists and is not a block device, magicSmoke fails.  If file does not exist, it is created\n");
	printf("-r,--read	perform random reads\n");
	printf("-w,--write	perform random writes\n");
	printf("-s #,--size=#	if a file is created, make it of size blocks default 10.\n");
	printf("-z #,--raw-size=#	if a file is created, make it of # bytes\n");
	printf("-a # --actions=#	perform # reads/writes [default: %u]\n", UINT_MAX);
	printf("-v, --verbose	write out progress output as we run\n");
}

void get_options(int argc, char **argv, bool *read, bool *write, 
	unsigned int *size, unsigned int *rawsize, unsigned int  *actions, 
	char **file) {
	
	//give some reasonable defaults for things
	*read=false;
	*write=false;
	*actions=UINT_MAX;
	*size=10;
	*rawsize=0;
	
	//Set up options array
	static struct option opts[] = {
		{"verbose",	no_argument,		(int *)&verbose,	true},
		{"write",	no_argument, 		0,		'w'},
		{"read",	no_argument,		0,		'r'},
		{"size",	required_argument,	0,		's'},
		{"rawsize",	required_argument,	0,		'z'},
		{"actions",	required_argument,	0,		'a'},
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
	
	*file=argv[optind];
	
	if (verbose) {
		printf("Arguments from getopt:\n\tVerbose: %i\n\tDo writes:%i\n\tDo reads: %i\n\tSize: %u blocks\n\tRawsize: %u bytes\n\tActions: %u\n\tFile: %s\n"
		, verbose, (int)*write, (int)*read, *size, *rawsize, *actions,*file);
	}
}

bool file_exists(char * file, int perms) {
	errno=0;
	int ifile;
	if(-1==(ifile=open(file, perms))) {
		if (errno==ENOENT) {
			//whoo! no file!
			return false;
		} else
		if (errno==EACCES) {
			printf("Access denied to %s.  You probably need to be root to write to a block device...\n",file);
			exit(EXIT_FAILURE);
		} else {
			printf("An error occurred: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	close(ifile);
	return true;
}

bool is_block_device(char *file) {
	errno=0;
	struct stat stats;
	
	if(-1==stat(file, &stats)) {
		printf("An error occurred: %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	if (S_ISBLK(stats.st_mode)) {
		return true;
	} else {
		return false;
	}
}

int parse_perms(bool read, bool write) {
	if (read && write) {
		return O_RDWR;
	} else if (read) {
		return O_RDONLY;
	} else if (write) {
		return O_WRONLY;
	} else {
		return 0; //what are you doing?
	}
}

unsigned long get_block_size(char *file) {
	struct stat stats;
	
	errno=0;
	
	char *mfile;
	
	if(!(mfile = calloc(strlen(file),sizeof(char)))) {
		printf("magicSmoke: get_block_size(): %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	strcpy(mfile,file);
	mfile = dirname(mfile);
	
	if(-1==stat(mfile, &stats)) {
		printf("magicSmoke: get_block_size(): %s with filename %s\n",strerror(errno),mfile);
		exit(EXIT_FAILURE);
	}
	
	free(mfile);
	
	return (int)stats.st_blksize;
}

unsigned long get_block_device_size(char * file) {
	errno=0;
	unsigned long size;
	
	int fp;
	if (-1==(fp=open(file,O_RDONLY))) {
		printf("magicSmoke: get_block_device_size(): %s with file %s\n",strerror(errno),file);
		exit(EXIT_FAILURE);
	}
	
	if (-1==ioctl(fp, BLKGETSIZE, &size)) {
		printf("magicSmoke: get_block_device_size(): %s with filename %s\n",strerror(errno),file);
		exit(EXIT_FAILURE);
	}
	
	close(fp);
	return size;

}

bool make_file(char * file, unsigned long size) {
	errno=0;
	
	int fp;
	if (-1==(fp=open(file, O_RDWR|O_CREAT|O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO))) {
		printf("magicSmoke: make_file(): %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	int crand=0;
	for (unsigned int i=0; i<size; i+=1) {
		crand=rand();
		if (-1==write(fp, &crand, 1)) {
			printf("magicSmoke: make_file(): %s\nYou'll probably want to manually clean up test file %s\n",strerror(errno),file);
			exit(EXIT_FAILURE);
		}
	}
	
	return true;
}

unsigned long get_random_offset(unsigned long filesize) {
	//scale the random int form rand() into [0,filesize-1]
	//this is not quite an evenmapping.  But assuming blocks are < 
	// RAND_MAX it should at least give decent coverage
	return rand() % (filesize - 1);
}

//proc_time is in jiffies, we convert internally.  Yes, this is inconsistant
void calc_stats(unsigned long actions, unsigned int blocksize, int proc_time, float real_time) {
	printf("%lu %u-byte blocks read in:\n%f seconds proc time\n%f real time\n", actions, blocksize, ((float)proc_time)/CLOCKS_PER_SEC, real_time);
	float baseb = ((float)actions/real_time)*blocksize;
	printf("%f blocks/sec\n%f B/sec\n%f KB/sec\n%f MB/sec\n\n",
	actions/real_time, baseb, baseb / 1024, baseb/1024/1024);
}

void last_chance() {
	printf("***WARNING***\n\nWriting to a block device WILL DESTROY any data on the device.  Are you sure you want to do this? [y/N]:");
	char c = getchar();
	if (c!='y' && c!='Y') {
		printf("Okay, exiting!\n");
		exit(EXIT_SUCCESS);
	}
}

//Dont forget - file size is in blocks!  We read/write with respect to blocks
void do_writes(int fd, unsigned long count, unsigned int blocksize, unsigned long filesize) {
	errno=0;
	void *buffer = malloc(blocksize);
	
	if(!buffer) {
		printf("magicSmoke: do_writes(): %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	printf("Beginning write cycle!\n");
	
	//record start time
	int proc_time_s = clock();
	
	struct timespec starttime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	//time_t real_time_s = time(NULL);
	
	for (unsigned long i=0; i<count; i++) {
		lseek(fd, get_random_offset(filesize), SEEK_SET);
		write(fd, buffer, blocksize);
	}
	
	//record end time
	int proc_time_f = clock();
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	//time_t real_time_f = time(NULL);
	
	float real_time = endtime.tv_sec - starttime.tv_sec;
	real_time += (endtime.tv_nsec - starttime.tv_nsec)/1000000000;
	
	printf("Done write cycle\n");
	
	printf("Write statistics:\n");
	calc_stats(count, blocksize, 
		proc_time_f - proc_time_s,
		real_time);
}

//Dont forget - file size is in blocks!  We read/write with respect to blocks
void do_reads(int fd, unsigned long count, unsigned int blocksize, unsigned long filesize) {
	errno=0;
	void *buffer = malloc(blocksize);
	
	if(!buffer) {
		printf("magicSmoke: do_reads(): %s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	printf("Beginning read cycle!\n");
	
	//record start time
	int proc_time_s = clock();
	
	struct timespec starttime;
	clock_gettime(CLOCK_REALTIME, &starttime);
	//time_t real_time_s = time(NULL);
	
	for (unsigned long i=0; i<count; i++) {
		lseek(fd, get_random_offset(filesize), SEEK_SET);
		read(fd, buffer, blocksize);
	}
	
	//record end time
	int proc_time_f = clock();
	struct timespec endtime;
	clock_gettime(CLOCK_REALTIME, &endtime);
	//time_t real_time_f = time(NULL);
	
	float real_time = endtime.tv_sec - starttime.tv_sec;
	real_time += (endtime.tv_nsec - starttime.tv_nsec)/1000000000;
	
	printf("Done read cycle\n");
	
	printf("Read statistics:\n");
	calc_stats(count, blocksize, 
		proc_time_f - proc_time_s,
		real_time);
}

int main(int argc, char **argv) {
	//Lets:
	//set up variables (default values set by getting options
	bool read;
	bool write;
	unsigned int size;
	unsigned int rawsize;
	unsigned int actions;
	char *file=NULL;
	//Parse arguments
	get_options(argc, argv, &read, &write, &size, &rawsize,
		&actions, &file);
	
	//do some sanity checks
	if (!(read || write)) {
		printf("You must specify one of read or write! (or both)\n");
		usage();
		exit(EXIT_FAILURE);
	}
	
	//parse out what permissions we want on the file when we open it
	int perms = parse_perms(read,write);
	
	//represents the size of this file, as the FS sees it.  In blocks
	unsigned long realsize;
	unsigned int blocksize;
	//If using files, create file or fail
	if (file_exists(file,perms)) {
		if (!is_block_device(file)) {
			printf("File %s already exists! I refuse to overwrite an existing file!\n\n",file);
			usage();
			exit(EXIT_FAILURE);
			//dont want to overwrite a file
		}
		//file is a block device.  We'll get size to use
		realsize = get_block_device_size(file);
		blocksize = get_block_size(file);
		//lets give a final chance to back out...
		if (write)
			last_chance();
	} else {
		//create a file
		if (rawsize!=0) {
			//use rawsize
			realsize=rawsize / get_block_size(file);
		} else {
			//get system blocksize
			realsize=size;
		}
		
		blocksize = get_block_size(file);
		
		if (verbose)
			printf("Writing %lu bytes to %s...\n",realsize * blocksize,file);
		make_file(file, realsize * blocksize);
		if (verbose)
			printf(" Done!\n");
	}
	
	if (verbose)
		printf("Using file/disk of size %lu blocks with block size of %u bytes\n",realsize, blocksize);
	//finally actually open our file
	errno=0;
	int fh;
	if (-1==(fh=open(file, perms|O_SYNC|O_DIRECT))) {
		printf("magicSmoke: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	//mark already run time
	
	//Do random reads/writes
	if (read)
		do_reads(fh, actions, blocksize, realsize);
	if (write)
		do_writes(fh, actions, blocksize, realsize);
	
	close(fh);
	
	//clean up if file was a created file
	if (!is_block_device(file)) {
		errno=0;
		if (-1==remove(file)) {
			printf("Failed to remove %s! You'll probably want to manually clean this up.\n",file);
		}
	}
	
	return EXIT_SUCCESS;
}
