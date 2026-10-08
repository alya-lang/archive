#ifndef ALYA_ARCHIVE_H
#define ALYA_ARCHIVE_H

#include <stdint.h>

int32_t alya_archive_write_bytes(const char *path, const void *data, int64_t len);
int32_t alya_archive_read_bytes(const char *path, void **out_buf, int64_t *out_len);
int32_t alya_archive_free_buffer(void *p);
int64_t alya_archive_crc32(int64_t crc_init, const void *buf, int64_t len);
int32_t alya_archive_inflate_exact(const void *src, int64_t src_len, void *dst, int64_t dst_cap, int64_t *out_actual);
int32_t alya_archive_gunzip_exact(const void *src, int64_t src_len, void *dst, int64_t dst_cap, int64_t *out_actual);

#endif
