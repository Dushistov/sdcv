/* dictziplib.c --
 * http://stardict.sourceforge.net
 * Copyright (C) 2003-2003 Hu Zheng <huzheng_001@163.com>
 * This file is a modify version of dictd-1.9.7's data.c
 *
 * data.c -- 
 * Created: Tue Jul 16 12:45:41 1996 by faith@dict.org
 * Revised: Sat Mar 30 10:46:06 2002 by faith@dict.org
 * Copyright 1996, 1997, 1998, 2000, 2002 Rickard E. Faith (faith@dict.org)
 * 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

//#define HAVE_MMAP //it will defined in config.h. this can be done by configure.in with a AC_FUNC_MMAP.
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/stat.h>


#include "dictziplib.hpp"

#define USE_CACHE 1

#define BUFFERSIZE 10240

/* 
 * Output buffer must be greater than or
 * equal to 110% of input buffer size, plus
 * 12 bytes. 
*/
#define OUT_BUFFER_SIZE 0xffffL

#define IN_BUFFER_SIZE ((unsigned long)((double)(OUT_BUFFER_SIZE - 12) * 0.89))

/* For gzip-compatible header, as defined in RFC 1952 */

				/* Magic for GZIP (rfc1952)                */
#define GZ_MAGIC1     0x1f	/* First magic byte                        */
#define GZ_MAGIC2     0x8b	/* Second magic byte                       */

				/* FLaGs (bitmapped), from rfc1952         */
#define GZ_FTEXT      0x01	/* Set for ASCII text                      */
#define GZ_FHCRC      0x02	/* Header CRC16                            */
#define GZ_FEXTRA     0x04	/* Optional field (random access index)    */
#define GZ_FNAME      0x08	/* Original name                           */
#define GZ_COMMENT    0x10	/* Zero-terminated, human-readable comment */
#define GZ_MAX           2	/* Maximum compression                     */
#define GZ_FAST          4	/* Fasted compression                      */

				/* These are from rfc1952                  */
#define GZ_OS_FAT        0	/* FAT filesystem (MS-DOS, OS/2, NT/Win32) */
#define GZ_OS_AMIGA      1	/* Amiga                                   */
#define GZ_OS_VMS        2	/* VMS (or OpenVMS)                        */
#define GZ_OS_UNIX       3      /* Unix                                    */
#define GZ_OS_VMCMS      4      /* VM/CMS                                  */
#define GZ_OS_ATARI      5      /* Atari TOS                               */
#define GZ_OS_HPFS       6      /* HPFS filesystem (OS/2, NT)              */
#define GZ_OS_MAC        7      /* Macintosh                               */
#define GZ_OS_Z          8      /* Z-System                                */
#define GZ_OS_CPM        9      /* CP/M                                    */
#define GZ_OS_TOPS20    10      /* TOPS-20                                 */
#define GZ_OS_NTFS      11      /* NTFS filesystem (NT)                    */
#define GZ_OS_QDOS      12      /* QDOS                                    */
#define GZ_OS_ACORN     13      /* Acorn RISCOS                            */
#define GZ_OS_UNKNOWN  255      /* unknown                                 */

#define GZ_RND_S1       'R'	/* First magic for random access format    */
#define GZ_RND_S2       'A'	/* Second magic for random access format   */

#define GZ_ID1           0	/* GZ_MAGIC1                               */
#define GZ_ID2           1	/* GZ_MAGIC2                               */
#define GZ_CM            2	/* Compression Method (Z_DEFALTED)         */
#define GZ_FLG	         3	/* FLaGs (see above)                       */
#define GZ_MTIME         4	/* Modification TIME                       */
#define GZ_XFL           8	/* eXtra FLags (GZ_MAX or GZ_FAST)         */
#define GZ_OS            9	/* Operating System                        */
#define GZ_XLEN         10	/* eXtra LENgth (16bit)                    */
#define GZ_FEXTRA_START 12	/* Start of extra fields                   */
#define GZ_SI1          12	/* Subfield ID1                            */
#define GZ_SI2          13      /* Subfield ID2                            */
#define GZ_SUBLEN       14	/* Subfield length (16bit)                 */
#define GZ_VERSION      16      /* Version for subfield format             */
#define GZ_CHUNKLEN     18	/* Chunk length (16bit)                    */
#define GZ_CHUNKCNT     20	/* Number of chunks (16bit)                */
#define GZ_RNDDATA      22	/* Random access data (16bit)              */

#define DICT_UNKNOWN    0
#define DICT_TEXT       1
#define DICT_GZIP       2
#define DICT_DZIP       3


int dictData::read_header(const std::string &fname, int computeCRC)
{
	FILE          *str;
	int           id1, id2, si1, si2;
	char          buffer[BUFFERSIZE];
	int           extraLength, subLength;
	int           i;
	char          *pt;
	int           c;
	struct stat   sb;
	unsigned long crc   = crc32( 0L, Z_NULL, 0 );
	int           count;
	unsigned long offset;
	
	if (!(str = fopen(fname.c_str(), "rb"))) {
		//err_fatal_errno( __FUNCTION__,
		//       "Cannot open data file \"%s\" for read\n", filename );
	}	

	this->headerLength = GZ_XLEN - 1;
	this->type         = DICT_UNKNOWN;
   
	id1                  = getc( str );
	id2                  = getc( str );
	
	if (id1 != GZ_MAGIC1 || id2 != GZ_MAGIC2) {
		this->type = DICT_TEXT;
		fstat( fileno( str ), &sb );
		this->compressedLength = this->length = sb.st_size;
		this->origFilename     = fname;
		this->mtime            = sb.st_mtime;
		if (computeCRC) {
			rewind( str );
			while (!feof( str )) {
				if ((count = fread( buffer, 1, BUFFERSIZE, str ))) {
					crc = crc32(crc, (Bytef *)buffer, count);
				}
			}
		}
		this->crc = crc;
		fclose( str );
		return 0;
	}
	this->type = DICT_GZIP;
  
	this->method       = getc( str );
	this->flags        = getc( str );
	this->mtime        = getc( str ) <<  0;
	this->mtime       |= getc( str ) <<  8;
	this->mtime       |= getc( str ) << 16;
	this->mtime       |= getc( str ) << 24;
	this->extraFlags   = getc( str );
	this->os           = getc( str );
  
	if (this->flags & GZ_FEXTRA) {
		extraLength          = getc( str ) << 0;
		extraLength         |= getc( str ) << 8;
		this->headerLength += extraLength + 2;
		si1                  = getc( str );
		si2                  = getc( str );
    
		if (si1 == GZ_RND_S1 || si2 == GZ_RND_S2) {
			subLength            = getc( str ) << 0;
			subLength           |= getc( str ) << 8;
			this->version      = getc( str ) << 0;
			this->version     |= getc( str ) << 8;
			
			if (this->version != 1) {
				//err_internal( __FUNCTION__,
				//	  "dzip header version %d not supported\n",
				//	  this->version );
			}
			
			this->chunkLength  = getc( str ) << 0;
			this->chunkLength |= getc( str ) << 8;
			this->chunkCount   = getc( str ) << 0;
			this->chunkCount  |= getc( str ) << 8;
			
			if (this->chunkCount <= 0) {
				fclose( str );
				return 5;
			}
			this->chunks = (int *)malloc(sizeof( this->chunks[0] )
																		 * this->chunkCount );
			for (i = 0; i < this->chunkCount; i++) {
				this->chunks[i]  = getc( str ) << 0;
				this->chunks[i] |= getc( str ) << 8;
			}
			this->type = DICT_DZIP;
		} else {
			fseek( str, this->headerLength, SEEK_SET );
		}
	}
	
	if (this->flags & GZ_FNAME) { /* FIXME! Add checking against header len */
		pt = buffer;
		while ((c = getc( str )) && c != EOF)
			*pt++ = c;
		*pt = '\0';
		
		this->origFilename = buffer;
		this->headerLength += this->origFilename.length() + 1;
	} else {
		this->origFilename = "";
	}
   
   if (this->flags & GZ_COMMENT) { /* FIXME! Add checking for header len */
      pt = buffer;
      while ((c = getc( str )) && c != EOF)
	 *pt++ = c;
      *pt = '\0';
      comment = buffer;
      headerLength += comment.length()+1;
   } else {
      comment = "";
   }

   if (this->flags & GZ_FHCRC) {
      getc( str );
      getc( str );
      this->headerLength += 2;
   }

   if (ftell( str ) != this->headerLength + 1) {
      //err_internal( __FUNCTION__,
		//    "File position (%lu) != header length + 1 (%d)\n",
		  //  ftell( str ), this->headerLength + 1 );
   }

   fseek( str, -8, SEEK_END );
   this->crc     = getc( str ) <<  0;
   this->crc    |= getc( str ) <<  8;
   this->crc    |= getc( str ) << 16;
   this->crc    |= getc( str ) << 24;
   this->length  = getc( str ) <<  0;
   this->length |= getc( str ) <<  8;
   this->length |= getc( str ) << 16;
   this->length |= getc( str ) << 24;
   this->compressedLength = ftell( str );

				/* Compute offsets */
   this->offsets = (unsigned long *)malloc( sizeof( this->offsets[0] )
																							* this->chunkCount );
   for (offset = this->headerLength + 1, i = 0;
	i < this->chunkCount;
	i++) {
      this->offsets[i] = offset;
      offset += this->chunks[i];
   }

   fclose( str );
   return 0;
}

bool dictData::open(const std::string& fname, int computeCRC)
{
	struct stat sb;
	int         j;
	int fd;

	this->initialized = 0;

	if (stat(fname.c_str(), &sb) || !S_ISREG(sb.st_mode)) {
		//err_warning( __FUNCTION__,
		//   "%s is not a regular file -- ignoring\n", fname );
		return false;
	}
   
	if (read_header(fname, computeCRC)) {
		//err_fatal( __FUNCTION__,
		// "\"%s\" not in text or dzip format\n", fname );
		return false;
	}
   
	if ((fd = ::open(fname.c_str(), O_RDONLY )) < 0) {
		//err_fatal_errno( __FUNCTION__,
		//       "Cannot open data file \"%s\"\n", fname );
		return false;
   }
   if (fstat(fd, &sb)) {
		 //err_fatal_errno( __FUNCTION__,
		 //       "Cannot stat data file \"%s\"\n", fname );
		 return false;
   }

   this->size = sb.st_size;
	 ::close(fd);
	 if (!mapfile.open(fname.c_str(), size))
		 return false;		

	 this->start=mapfile.begin();
   this->end = this->start + this->size;

   for (j = 0; j < DICT_CACHE_SIZE; j++) {
		 cache[j].chunk    = -1;
		 cache[j].stamp    = -1;
		 cache[j].inBuffer = NULL;
		 cache[j].count    = 0;
   }
   
   return true;
}

void dictData::close()
{
	int i;   
	
	if (this->chunks)
		free(this->chunks);
	if (this->offsets)
		free(this->offsets);

	if (this->initialized) {
		if (inflateEnd( &this->zStream )) {
			//err_internal( __FUNCTION__,
			//       "Cannot shut down inflation engine: %s\n",
		  //     this->zStream.msg );
	  }
	}

	for (i = 0; i < DICT_CACHE_SIZE; ++i){
		if (this -> cache [i].inBuffer)
			free (this -> cache [i].inBuffer);
	}
}

void dictData::read(char *buffer, unsigned long start, unsigned long size)
{
	char          *pt;
	unsigned long end;
	int           count;
	char          *inBuffer;
	char          outBuffer[OUT_BUFFER_SIZE];
	int           firstChunk, lastChunk;
	int           firstOffset, lastOffset;
	int           i, j;
	int           found, target, lastStamp;
	static int    stamp = 0;
	
	end  = start + size;
	
	//buffer = malloc( size + 1 );
  
	//PRINTF(DBG_UNZIP,
	// ("dict_data_read( %p, %lu, %lu )\n",
	//h, start, size ));
	
  
	switch (this->type) {
	case DICT_GZIP:
		//err_fatal( __FUNCTION__,
		// "Cannot seek on pure gzip format files.\n"
		// "Use plain text (for performance)"
		// " or dzip format (for space savings).\n" );
		break;
	case DICT_TEXT:
		memcpy( buffer, this->start + start, size );
		//buffer[size] = '\0';
		break;
	case DICT_DZIP:
		if (!this->initialized) {
			++this->initialized;
			this->zStream.zalloc    = NULL;
			this->zStream.zfree     = NULL;
			this->zStream.opaque    = NULL;
			this->zStream.next_in   = 0;
			this->zStream.avail_in  = 0;
			this->zStream.next_out  = NULL;
			this->zStream.avail_out = 0;
			if (inflateInit2( &this->zStream, -15 ) != Z_OK) {
				//err_internal( __FUNCTION__,
				//  "Cannot initialize inflation engine: %s\n",
			  //this->zStream.msg );
			}
		}
		firstChunk  = start / this->chunkLength;
		firstOffset = start - firstChunk * this->chunkLength;
		lastChunk   = end / this->chunkLength;
		lastOffset  = end - lastChunk * this->chunkLength;
		//PRINTF(DBG_UNZIP,
		// ("   start = %lu, end = %lu\n"
		//"firstChunk = %d, firstOffset = %d,"
		//" lastChunk = %d, lastOffset = %d\n",
		//start, end, firstChunk, firstOffset, lastChunk, lastOffset ));
		for (pt = buffer, i = firstChunk; i <= lastChunk; i++) {
			
			/* Access cache */
			found  = 0;
			target = 0;
			lastStamp = INT_MAX;
			for (j = 0; j < DICT_CACHE_SIZE; j++) {
#if USE_CACHE
				if (this->cache[j].chunk == i) {
					found  = 1;
					target = j;
					break;
				}
#endif
				if (this->cache[j].stamp < lastStamp) {
					lastStamp = this->cache[j].stamp;
					target = j;
				}
			}
			
			this->cache[target].stamp = ++stamp;
			if (found) {
				count = this->cache[target].count;
				inBuffer = this->cache[target].inBuffer;
			} else {
				this->cache[target].chunk = i;
				if (!this->cache[target].inBuffer)
					this->cache[target].inBuffer = (char *)malloc( IN_BUFFER_SIZE );
				inBuffer = this->cache[target].inBuffer;
				
				if (this->chunks[i] >= OUT_BUFFER_SIZE ) {
					//err_internal( __FUNCTION__,
					//    "this->chunks[%d] = %d >= %ld (OUT_BUFFER_SIZE)\n",
					//  i, this->chunks[i], OUT_BUFFER_SIZE );
				}
				memcpy( outBuffer, this->start + this->offsets[i], this->chunks[i] );
				
				this->zStream.next_in   = (Bytef *)outBuffer;
				this->zStream.avail_in  = this->chunks[i];
				this->zStream.next_out  = (Bytef *)inBuffer;
				this->zStream.avail_out = IN_BUFFER_SIZE;
				if (inflate( &this->zStream,  Z_PARTIAL_FLUSH ) != Z_OK) {
					//err_fatal( __FUNCTION__, "inflate: %s\n", this->zStream.msg );
				}
				if (this->zStream.avail_in) {
					//err_internal( __FUNCTION__,
					//    "inflate did not flush (%d pending, %d avail)\n",
					//  this->zStream.avail_in, this->zStream.avail_out );
				}
				
				count = IN_BUFFER_SIZE - this->zStream.avail_out;
				
				this->cache[target].count = count;
			}
			
			if (i == firstChunk) {
				if (i == lastChunk) {
					memcpy( pt, inBuffer + firstOffset, lastOffset-firstOffset);
					pt += lastOffset - firstOffset;
				} else {
					if (count != this->chunkLength ) {
						//err_internal( __FUNCTION__,
						//	"Length = %d instead of %d\n",
						//count, this->chunkLength );
					}
					memcpy( pt, inBuffer + firstOffset,
									this->chunkLength - firstOffset );
					pt += this->chunkLength - firstOffset;
				}
			} else if (i == lastChunk) {
				memcpy( pt, inBuffer, lastOffset );
				pt += lastOffset;
			} else {
				assert( count == this->chunkLength );
				memcpy( pt, inBuffer, this->chunkLength );
				pt += this->chunkLength;
			}
		}
		//*pt = '\0';
		break;
	case DICT_UNKNOWN:
		//err_fatal( __FUNCTION__, "Cannot read unknown file type\n" );
		break;
	}
}
