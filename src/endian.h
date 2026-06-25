#ifndef ENDIAN_H
#define ENDIAN_H
#include <stdint.h>
#include <string.h>

/* Canonical on-disk / wire byte order for all of gk's binary I/O
 * (bd/db serialize, the 1: binary reader, the 5: append format, and the
 * ipc wire format) is LITTLE-ENDIAN. That is simply the order every gk
 * build has ever produced, so declaring it canonical keeps every existing
 * file/stream readable.
 *
 * On little-endian hosts (x86, riscv64, arm64, ...) the helpers below are a
 * plain memcpy -- byte-identical output and identical codegen to the
 * pre-endian-aware code, so LE builds are unchanged and stay interchangeable
 * with older binaries. On big-endian hosts (s390x) they byte-swap.
 *
 * IEEE-754 float/double byte order follows integer endianness on every arch
 * gk targets, so the integer swap is correct for f32/f64 too. */

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define GK_BIG_ENDIAN 1
#else
#define GK_BIG_ENDIAN 0
#endif

static inline uint16_t gk_bswap16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static inline uint32_t gk_bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) |
         ((v & 0x00FF0000u) >> 8)  | ((v & 0xFF000000u) >> 24);
}
static inline uint64_t gk_bswap64(uint64_t v) {
  return ((uint64_t)gk_bswap32((uint32_t)v) << 32) | gk_bswap32((uint32_t)(v >> 32));
}

/* Copy n elements of size sz between a native array and a little-endian byte
 * buffer (the transform is its own inverse, so one routine serves load and
 * store). On LE hosts, or for byte data, this is a single memcpy. src==dst is
 * safe (each element is read into a register before being written back). */
static inline void gk_cvt_arr(void *dst, const void *src, size_t n, size_t sz) {
  if (!GK_BIG_ENDIAN || sz == 1) { memmove(dst, src, n * sz); return; }
  const unsigned char *s = (const unsigned char *)src;
  unsigned char *d = (unsigned char *)dst;
  for (size_t i = 0; i < n; i++, s += sz, d += sz) {
    if (sz == 2)      { uint16_t v; memcpy(&v, s, 2); v = gk_bswap16(v); memcpy(d, &v, 2); }
    else if (sz == 4) { uint32_t v; memcpy(&v, s, 4); v = gk_bswap32(v); memcpy(d, &v, 4); }
    else if (sz == 8) { uint64_t v; memcpy(&v, s, 8); v = gk_bswap64(v); memcpy(d, &v, 8); }
    else              { memmove(d, s, sz); }
  }
}

/* store native scalar(s) -> little-endian buffer; load little-endian buffer -> native */
#define gk_st_arr(dst, src, n, sz) gk_cvt_arr((dst), (src), (size_t)(n), (size_t)(sz))
#define gk_ld_arr(dst, src, n, sz) gk_cvt_arr((dst), (src), (size_t)(n), (size_t)(sz))

#endif /* ENDIAN_H */
