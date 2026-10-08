#include "archive.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* --- Exact-size decompression via the compress native engine ---
   The three helpers below are provided by the compress package C engine
   and linked transitively through the pinned `compress` dependency (see
   [dependencies] in alya.toml). Only fixed-buffer variants are reused:
   archive payloads always carry their exact decompressed size (gzip ISIZE
   footer, ZIP central directory record), so output growth is never needed
   and the auto-allocating variants are deliberately avoided. */
int alya_gzip_uncompressed_len(const uint8_t *src, size_t src_len, size_t *out_len);
int alya_gzip_decompress(const uint8_t *src, size_t src_len, uint8_t *dst, size_t *dst_len);
int alya_deflate_decompress(const uint8_t *src, size_t src_len, uint8_t *dst, size_t *dst_len);

int32_t alya_archive_write_bytes(const char *path, const void *data, int64_t len) {
    FILE *f;
    size_t written;

    if (path == 0 || len < 0) {
        return -1;
    }
    f = fopen(path, "wb");
    if (f == 0) {
        return -2;
    }
    if (len > 0) {
        if (data == 0) {
            fclose(f);
            return -3;
        }
        written = fwrite(data, 1, (size_t)len, f);
        fclose(f);
        return (written == (size_t)len) ? 0 : -4;
    }
    fclose(f);
    return 0;
}

int32_t alya_archive_read_bytes(const char *path, void **out_buf, int64_t *out_len) {
    FILE *f;
    long n;
    void *buf;
    size_t got;

    if (path == 0 || out_buf == 0 || out_len == 0) {
        return -1;
    }
    *out_buf = 0;
    *out_len = 0;
    f = fopen(path, "rb");
    if (f == 0) {
        return -2;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -3;
    }
    n = ftell(f);
    if (n < 0) {
        fclose(f);
        return -3;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return -3;
    }
    buf = malloc((size_t)n + 1);
    if (buf == 0) {
        fclose(f);
        return -4;
    }
    got = (n > 0) ? fread(buf, 1, (size_t)n, f) : 0;
    fclose(f);
    if ((long)got != n) {
        free(buf);
        return -5;
    }
    ((char *)buf)[n] = 0;
    *out_buf = buf;
    *out_len = (int64_t)n;
    return 0;
}

int32_t alya_archive_free_buffer(void *p) {
    if (p != 0) {
        free(p);
    }
    return 0;
}

/* Return codes: 0 on success, -11 invalid arguments, -12 expected size
   unavailable (empty stream or unreadable header), -13 caller buffer size
   disagrees with the stream, otherwise the decompressor's own code. */
int32_t alya_archive_inflate_exact(const void *src, int64_t src_len, void *dst, int64_t dst_cap, int64_t *out_actual) {
    size_t have;
    int rc;

    if (src == 0 || dst == 0 || out_actual == 0 || src_len < 0 || dst_cap < 0) {
        return -11;
    }
    *out_actual = 0;
    if (dst_cap == 0) {
        return 0;
    }
    if (src_len == 0) {
        return -12;
    }
    have = (size_t)dst_cap;
    rc = alya_deflate_decompress((const uint8_t *)src, (size_t)src_len, (uint8_t *)dst, &have);
    if (rc != 0) {
        return rc;
    }
    if ((int64_t)have != dst_cap) {
        return -13;
    }
    *out_actual = (int64_t)have;
    return 0;
}

int32_t alya_archive_gunzip_exact(const void *src, int64_t src_len, void *dst, int64_t dst_cap, int64_t *out_actual) {
    size_t want = 0;
    size_t have;
    int rc;

    if (src == 0 || dst == 0 || out_actual == 0 || src_len < 0 || dst_cap < 0) {
        return -11;
    }
    *out_actual = 0;
    rc = alya_gzip_uncompressed_len((const uint8_t *)src, (size_t)src_len, &want);
    if (rc != 0) {
        return -12;
    }
    if ((int64_t)want != dst_cap) {
        return -13;
    }
    have = (size_t)dst_cap;
    rc = alya_gzip_decompress((const uint8_t *)src, (size_t)src_len, (uint8_t *)dst, &have);
    if (rc != 0) {
        return rc;
    }
    *out_actual = (int64_t)have;
    return 0;
}

int64_t alya_archive_crc32(int64_t crc_init, const void *buf, int64_t len) {
    static uint32_t table[256];
    static int ready = 0;
    uint32_t crc;
    const unsigned char *p;
    int64_t i;
    uint32_t v;
    int k;

    if (ready == 0) {
        for (v = 0; v < 256; v++) {
            uint32_t c = v;
            for (k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            table[v] = c;
        }
        ready = 1;
    }
    crc = ((uint32_t)crc_init) ^ 0xFFFFFFFFu;
    p = (const unsigned char *)buf;
    for (i = 0; i < len; i++) {
        crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    }
    return (int64_t)(crc ^ 0xFFFFFFFFu);
}
