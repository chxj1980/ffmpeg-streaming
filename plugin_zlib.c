#include <stdio.h>
#include <zlib.h>
#include "config.h"
#include "compact.h"
#include "plugin_zlib.h"

typedef struct{
	z_stream z_st;
	uint8_t *z_buf;
	int z_size;
}zlib_t;

#define BUF_SIZE (256 * 1024)
#define windowBits 15
#define GZIP_ENCODING 16

void* zlib_init(void)
{
	zlib_t *zl = av_mallocz(sizeof(*zl));
	zl->z_size = BUF_SIZE;
	zl->z_buf = av_mallocz(BUF_SIZE);
	if(!zl->z_buf){
		printf("cant alloc for zlib\n");
		return NULL;
	}

	deflateInit2(&zl->z_st, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
			windowBits | GZIP_ENCODING, 8,
			Z_DEFAULT_STRATEGY);

	return zl;
}

int zlib_destroy(void *z)
{
	zlib_t *zl = (zlib_t*)z;
	deflateEnd(&zl->z_st);
	av_freep(&zl->z_buf);
	av_freep(&zl);
	return 0;
}

int zlib_read_compress(read_fn fn, int fd, void *z, uint8_t *buf, int size)
{/*return >0 -- ok, 0 -- eof, -1 -- err, -11 -- eagain*/
	int ret, rd, flag = Z_NO_FLUSH; 
	zlib_t *zl = (zlib_t*)z;
	z_stream *st = &zl->z_st;

	if (st->avail_in == 0){
		rd = fn(fd, zl->z_buf, zl->z_size);
		if (rd < 0){
			return rd;
		}else if(rd > 0){
			st->next_in  = zl->z_buf;
			st->avail_in = rd;
		}else if(0 == rd){
			flag = Z_FINISH;
		}
	}
	st->avail_out = size;
	st->next_out  = buf;
	ret = deflate(st, flag);
	
	if (Z_STREAM_ERROR == ret){
		printf("zlib ret %d\n", ret);
		return -1;
	}else if(Z_STREAM_END != ret && size <= st->avail_out){
		printf("zlib again %d\n", ret);
		return -11;
	}

	return size - st->avail_out;	
}


