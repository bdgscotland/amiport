/* amiport: replaced <sys/stat.h> with amiport shim */
#include <amiport/sys/stat.h>

#include <errno.h>
/* amiport: removed <fcntl.h> — not needed after replacing open()+fdopen() with fopen() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* amiport: removed <unistd.h> — not needed after replacing open()+fdopen() with fopen() */
#include "jv.h"
#include "jv_unicode.h"

jv jv_load_file(const char* filename, int raw) {
  struct stat sb; /* amiport: struct stat maps to amiport_stat via <amiport/sys/stat.h> */

  /* amiport: replaced open()+fstat()+fdopen() with stat()+fopen() — crash-patterns #12.
   * amiport_open() returns fds from amiport's internal fd table; fdopen() expects libnix
   * fds. Mixing them creates a silent failure: the FILE* reads nothing and returns empty
   * output. Fix: stat() the path first to detect directories, then use fopen() directly. */

  /* Check for directory before opening */
  if (stat(filename, &sb) == 0 && S_ISDIR(sb.st_mode)) {
    return jv_invalid_with_msg(jv_string_fmt("Could not open %s: %s",
                                             filename,
                                             "It's a directory"));
  }

  FILE* file = fopen(filename, "r");
  struct jv_parser* parser = NULL;
  jv data;
  if (!file) {
    return jv_invalid_with_msg(jv_string_fmt("Could not open %s: %s",
                                             filename,
                                             strerror(errno)));
  }
  if (raw) {
    data = jv_string("");
  } else {
    data = jv_array();
    parser = jv_parser_new(0);
  }

  // To avoid mangling UTF-8 multi-byte sequences that cross the end of our read
  // buffer, we need to be able to read the remainder of a sequence and add that
  // before appending.
  const int max_utf8_len = 4;
  /* amiport: large local buffer — made static for non-recursive function (crash-patterns #10) */
  static char buf[4096+4]; /* max_utf8_len=4 hardcoded to avoid VLA */
  while (!feof(file) && !ferror(file)) {
    size_t n = fread(buf, 1, sizeof(buf)-max_utf8_len, file);
    int len = 0;

    if (n == 0)
      continue;
    if (jvp_utf8_backtrack(buf+(n-1), buf, &len) && len > 0 &&
        !feof(file) && !ferror(file)) {
      n += fread(buf+n, 1, len, file);
    }

    if (raw) {
      data = jv_string_append_buf(data, buf, n);
    } else {
      jv_parser_set_buf(parser, buf, n, !feof(file));
      jv value;
      while (jv_is_valid((value = jv_parser_next(parser))))
        data = jv_array_append(data, value);
      if (jv_invalid_has_msg(jv_copy(value))) {
        jv_free(data);
        data = value;
        break;
      }
    }
  }
  if (!raw)
      jv_parser_free(parser);
  int badread = ferror(file);
  if (fclose(file) != 0 || badread) {
    jv_free(data);
    return jv_invalid_with_msg(jv_string_fmt("Error reading from %s",
                                             filename));
  }
  return data;
}
