#pragma once

#include <ctime>
#include <string>
#include <zlib.h>

#include "mapfile.hpp"


#define DICT_CACHE_SIZE 5

struct dictCache {
	int           chunk;
	char          *inBuffer;
	int           stamp;
	int           count;
};

struct dictData {
	dictData() {}
	bool open(const std::string& filename, int computeCRC);
	void close();
	void read(char *buffer, unsigned long start, unsigned long size);
	~dictData() { close(); }
private:
	const char    *start;	/* start of mmap'd area */
	const char    *end;		/* end of mmap'd area */
	unsigned long size;		/* size of mmap */
	
	int           type;
	z_stream      zStream;
	int           initialized;
  
	int           headerLength;
	int           method;
	int           flags;
	time_t        mtime;
	int           extraFlags;
	int           os;
	int           version;
	int           chunkLength;
	int           chunkCount;
	int           *chunks;
	unsigned long *offsets;	/* Sum-scan of chunks. */
	std::string    origFilename;
	std::string    comment;
	unsigned long crc;
	unsigned long length;
	unsigned long compressedLength;
	dictCache     cache[DICT_CACHE_SIZE];
	MapFile mapfile;

	int read_header(const std::string &filename, int computeCRC);
};

