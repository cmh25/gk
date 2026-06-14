/* msgbox.c -- a native Win32 modal dialog exposed to gk via `2:` link object
 * code. About the simplest possible Windows GUI extension: one user32 call,
 * no window class, no message loop, no frameworks.
 *
 *   box :"msgbox.dll" 2:("box";1)
 *   box "hello from gk"    pops a modal OK dialog, blocks until dismissed,
 *                          and returns the clicked-button id (IDOK = 1).
 *
 * The argument may be a gk string (char vector) or a symbol. MessageBoxA takes
 * a byte string, which matches gk's char strings directly (use MessageBoxW only
 * if you need true Unicode -- not barebones).
 *
 * Build (from a Visual Studio "x64 Native Tools" command prompt):  make.bat
 * or directly:
 *   cl /nologo /O2 /LD msgbox.c gk.lib user32.lib /Femsgbox.dll
 * (gk.lib is the import library emitted next to gk.exe when gk is built on
 *  Windows; a client DLL must link it so the gk_* imports bind.)
 */
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "gk.h"

GK_FN K box(K x) {
  if (gk_t(x) == GK_CHARV) {
    /* a gk char vector isn't NUL-terminated, so copy out its n bytes */
    int n = (int)gk_n(x);
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) return gk_err("wsfull");
    memcpy(buf, gk_cv(x), (size_t)n);
    buf[n] = 0;
    int id = MessageBoxA(NULL, buf, "gk", MB_OK);
    free(buf);
    return gk_mki(id);
  }
  if (gk_t(x) == GK_SYM)          /* symbols are interned, NUL-terminated */
    return gk_mki(MessageBoxA(NULL, gk_s(x), "gk", MB_OK));
  return gk_err("type");
}
