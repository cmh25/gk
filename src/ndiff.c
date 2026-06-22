/* ndiff -- "newline diff": compare two files ignoring CR ('\r'), so a CRLF file
 * and the same LF file compare equal.  The test harness's portable,
 * dependency-free replacement for `diff`/`comp`/`fc`:
 *   - builds with the same C compiler as gk (if that fails, gk itself wouldn't
 *     build, so the suite would never run -- zero added risk),
 *   - behaves identically on Linux/macOS/MSYS/native-Windows,
 *   - is small enough to audit at a glance, so it stays an oracle that is
 *     INDEPENDENT of the gk under test.
 *
 * The PASS/FAIL decision is an exact byte compare (after stripping CR) -- that
 * is the part that must be trustworthy.  On a mismatch it ALSO prints a
 * `diff -u`-style line-by-line view to help debugging.  The view is naive
 * (position-by-position, no LCS), so an inserted/deleted line desyncs it into
 * noisy-but-correct output; for a clean minimal diff in that case, fall back to
 * a system tool, e.g.  diff -u rNNN NNN.
 *
 * exit 0 = files equal (ignoring CR)
 * exit 1 = files differ (prints the diff)
 * exit 2 = I/O error -- a missing/unreadable file is never a silent pass
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const unsigned char *p; long len; } Line;

static long slurp(const char *path, unsigned char **buf) {
  FILE *f = fopen(path, "rb");
  if (!f) return -1;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  unsigned char *b = malloc(sz > 0 ? (size_t)sz : 1);
  if (!b) { fclose(f); return -1; }
  long n = (long)fread(b, 1, (size_t)(sz > 0 ? sz : 0), f);
  fclose(f);
  *buf = b;
  return n;
}

/* drop every '\r' in place; return the new length */
static long strip_cr(unsigned char *b, long n) {
  long j = 0;
  for (long i = 0; i < n; i++) if (b[i] != '\r') b[j++] = b[i];
  return j;
}

/* split buf into lines (content excluding '\n').  Only the final line can lack
 * a terminator; *ends_nl reports whether the buffer ended on '\n'. */
static long split(const unsigned char *b, long n, Line *out, int *ends_nl) {
  long nl = 0, start = 0;
  for (long i = 0; i < n; i++)
    if (b[i] == '\n') { out[nl].p = b + start; out[nl].len = i - start; nl++; start = i + 1; }
  if (start < n) { out[nl].p = b + start; out[nl].len = n - start; nl++; *ends_nl = 0; }
  else *ends_nl = (n > 0);
  return nl;
}

static void emit(char prefix, const Line *l, int term) {
  putchar(prefix);
  fwrite(l->p, 1, (size_t)l->len, stdout);
  putchar('\n');
  if (!term) fputs("\\ No newline at end of file\n", stdout);
}

int main(int argc, char **argv) {
  if (argc != 3) { fprintf(stderr, "usage: %s file1 file2\n", argv[0]); return 2; }
  unsigned char *a = NULL, *b = NULL;
  long na = slurp(argv[1], &a);
  long nb = slurp(argv[2], &b);
  if (na < 0) { free(b); fprintf(stderr, "ndiff: cannot read %s\n", argv[1]); return 2; }
  if (nb < 0) { free(a); fprintf(stderr, "ndiff: cannot read %s\n", argv[2]); return 2; }
  na = strip_cr(a, na);
  nb = strip_cr(b, nb);

  /* the verdict: exact byte compare (CR already stripped) */
  if (na == nb && memcmp(a, b, (size_t)na) == 0) { free(a); free(b); return 0; }

  /* differ: render a naive unified-style diff */
  Line *la = malloc(sizeof(Line) * (size_t)(na + 1));
  Line *lb = malloc(sizeof(Line) * (size_t)(nb + 1));
  if (!la || !lb) { free(la); free(lb); fprintf(stderr, "ndiff: out of memory\n"); return 2; }
  int aenl, benl;
  long nla = split(a, na, la, &aenl);
  long nlb = split(b, nb, lb, &benl);

  printf("--- %s\n", argv[1]);
  printf("+++ %s\n", argv[2]);
  long m = nla > nlb ? nla : nlb;
  for (long i = 0; i < m; i++) {
    int hasa = i < nla, hasb = i < nlb;
    int aterm = hasa ? (i < nla - 1 ? 1 : aenl) : 1;
    int bterm = hasb ? (i < nlb - 1 ? 1 : benl) : 1;
    int eq = hasa && hasb && aterm == bterm &&
             la[i].len == lb[i].len &&
             memcmp(la[i].p, lb[i].p, (size_t)la[i].len) == 0;
    if (eq) emit(' ', &la[i], aterm);
    else { if (hasa) emit('-', &la[i], aterm); if (hasb) emit('+', &lb[i], bterm); }
  }
  free(la); free(lb); free(a); free(b);
  return 1;
}
