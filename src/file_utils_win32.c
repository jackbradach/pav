#include <assert.h>
#include <errno.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <windows.h>

#include "file_utils.h"

// XXX - these need to be rewritten for windows!
static void file_mmap(FILE *fp, void **buf, size_t *len);
static bool inflate_buffer(void *src, size_t src_len, void **dst, size_t *dst_len);

int file_load_path(const char *path, void **buf, size_t *len)
{
    FILE *fp = fopen(path, "rb");
    if (NULL == fp) {
        fprintf(stderr, "Unable to open < %s >\n", path);
        return -1;
    }
    return file_load(fp, buf, len);
}

int file_load(FILE *fp, void **buf, size_t *len)
{
    void *src_buf, *dst_buf;
    size_t src_len, dst_len;
    bool compressed;

    if (NULL == fp) {
        fprintf(stderr, "Invalid file handle for load!\n");
        errno = ENOENT;
        return -1;
    }

    /* Map the file into memory, decompressing if necessary.  If source
     * wasn't compressed, copy it over to a buffer and unmap the file.
     */
    file_mmap(fp, &src_buf, &src_len);
    compressed = inflate_buffer(src_buf, src_len, &dst_buf, &dst_len);
    if (!compressed) {
        dst_len = src_len;
        dst_buf = calloc(dst_len, sizeof(uint8_t));
        memcpy(dst_buf, src_buf, src_len);
    }
    munmap(src_buf, src_len);

    *buf = dst_buf;
    *len = dst_len;
    return 0;
}

/* Function: file_mmap
 *
 * Maps a file contents into memory.  Buffer needs to be munmap'ed
 * when done.
 *
 */
static void file_mmap(FILE *fp, void **buf, size_t *len)
{
    struct stat st;
    void *addr;

    fstat(fileno(fp), &st);
    addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fileno(fp), 0);
    *len = st.st_size;
    *buf = addr;
}

/* Attempts to decompress a memmaped gzip file in-place.  If the buffer
 * doesn't contain a GZIP image, it'll be left untouched.  Otherwise,
 * buf and buf_len will be replaced with the decompressed image.
 */
static bool inflate_buffer(void *src, size_t src_len, void **dst, size_t *dst_len)
{
    size_t buf_len = 4 * (1 << 20); // start with 4 megabytes
    void *buf = calloc(buf_len, sizeof(uint8_t));
    z_stream strm  = {
        .next_in = (Bytef *) src,
        .avail_in = src_len,
    };

    int rc;

    /* Note: this bare value is literally what the dang documentation
     * says to put in there for combining window size and whether or not
     * I wants gzip header detection (spointer alert: I does!)
     */
    rc = inflateInit2(&strm, (15 + 32));
    if (Z_OK != rc) {
        return false;
    }

    for (int retries = 10; retries > 0; retries--) {
        buf = realloc(buf, buf_len);
        strm.next_out = (Bytef *) buf;
        strm.avail_out = buf_len;
        rc = inflate(&strm, Z_FINISH);
        assert(rc != Z_STREAM_ERROR);

        if (Z_STREAM_END == rc) {
            break;
        } else if (Z_BUF_ERROR == rc) {
            /* Bump the decompression buffer size */
            buf_len *= 2;
        } else {
            /* Not compressed! */
            inflateEnd(&strm);
            free(buf);
            *dst = src;
            *dst_len = src_len;
            return false;
        }
    }

    *dst = realloc(buf, strm.total_out);
    *dst_len = strm.total_out;
    return true;
}
