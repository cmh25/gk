#include "pnp.h"
#include "p.h"
#include <stdio.h>
#include <string.h>

static const char* pn_get_type_name(int t) {
  static char buf[32];
  switch(t) {
  case 1: return "verb/expr";
  case 2: return "noun/expr";
  case 4: return "plist";
  case 5: return "klist";
  case 104: return "special";
  default:
    snprintf(buf, sizeof(buf), "type_%d", t);
    return buf;
  }
}

/* Get a short display string for a node */
static void pn_get_display_string(pn *a, char *buf, int bufsize) {
  if(!a) {
    snprintf(buf, bufsize, "NULL");
    return;
  }

  /* For different node types, show meaningful representation */
  switch(a->t) {
  case 1: /* verb/expression */
    if(a->v == 0) {
      snprintf(buf, bufsize, "NULL");
    }
    else if(a->v == 255) {
      snprintf(buf, bufsize, "255");
    }
    else {
      /* Check if it's a subtype 7 or 8 (string pointer) */
      int sx = (a->v >> 48) & 0xFF;
      K px = a->v & ((1ULL << 48) - 1);
      if((sx == 7 || sx == 8) && px != 0) {
        char *str = (char*)px;
        snprintf(buf, bufsize, "%s", str);
      }
      else if(a->v < 256 && (a->v >= 32 && a->v <= 126)) { /* printable ASCII */
        snprintf(buf, bufsize, "%c", (char)a->v);
      }
      else if(a->v < 1000) {
        snprintf(buf, bufsize, "%lld", (long long)a->v);
      }
      else {
        snprintf(buf, bufsize, "V:%llx", (unsigned long long)a->v);
      }
    }
    break;
  case 2: /* e - expression/noun */
    if(a->n == 0) {
      snprintf(buf, bufsize, "0");
    }
    else if(a->n < 1000) {
      snprintf(buf, bufsize, "%lld", (long long)a->n);
    }
    else {
      snprintf(buf, bufsize, "N:%llx", (unsigned long long)a->n);
    }
    break;
  case 14: /* N - number */
    if(a->n == 0) {
      snprintf(buf, bufsize, "0");
    }
    else if(a->n < 1000) {
      snprintf(buf, bufsize, "%lld", (long long)a->n);
    }
    else {
      snprintf(buf, bufsize, "N:%llx", (unsigned long long)a->n);
    }
    break;
  case 4: /* plist */
    if(a->v == 0) {
      snprintf(buf, bufsize, "4");
    }
    else if(a->v < 1000) {
      snprintf(buf, bufsize, "%lld", (long long)a->v);
    }
    else {
      snprintf(buf, bufsize, "4:%llx", (unsigned long long)a->v);
    }
    break;
  case 15: /* V - verb */
    if(a->v == 0) {
      snprintf(buf, bufsize, "NULL");
    }
    else if(a->v < 256 && (a->v >= 32 && a->v <= 126)) { /* printable ASCII */
      snprintf(buf, bufsize, "%c", (char)a->v);
    }
    else if(a->v < 1000) {
      snprintf(buf, bufsize, "%lld", (long long)a->v);
    }
    else {
      snprintf(buf, bufsize, "V:%llx", (unsigned long long)a->v);
    }
    break;
  default:
    /* Use the type name for other types */
    snprintf(buf, bufsize, "%s", pn_get_type_name(a->t));
    break;
  }
}

/* Helper function to print tree recursively */
static void _pnp(pn *a, char *prefix, int is_last, int is_root) {
  if(!a) {
    printf("%sNULL\n", prefix);
    return;
  }

  char display[64];
  pn_get_display_string(a, display, sizeof(display));

  if(is_root) {
    printf("%s\n", display);
  }
  else {
    printf("%s%s%s\n", prefix, is_last ? "└── " : "├── ", display);
  }

  if(a->m > 0 && a->a) {
    char new_prefix[256];
    if(is_root) {
      new_prefix[0] = '\0';
    }
    else {
      snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, is_last ? "    " : "│   ");
    }

    /* Count non-NULL children first */
    int non_null_count = 0;
    for(int i = 0; i < a->m; i++) {
      if(a->a[i]) non_null_count++;
    }

    /* Print only non-NULL children */
    int printed = 0;
    for(int i = 0; i < a->m; i++) {
      if(a->a[i]) {
        _pnp(a->a[i], new_prefix, printed == non_null_count - 1, 0);
        printed++;
      }
    }
  }
}

/* Visual tree printer - shows tree structure like file trees */
void pnp(pn *a) {
  if(!a) {
    printf("NULL\n");
    return;
  }
  _pnp(a, "", 1, 1);
}
