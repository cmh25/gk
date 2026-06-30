#include "repl.h"
#include "p.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0
#define GKPATHSEP ';'  /* ':' is a drive-letter separator on Windows */
#else
#include <unistd.h>
#include <poll.h>
#define GKPATHSEP ':'
#endif
#include "timer.h"
#include "scope.h"
#include "h.h"
#include "b.h"
#include "ipc.h"
#include "tmr.h"

#ifdef _WIN32
#define repl_getc() ipc_stdin_getc()
#else
/* poll_getc()
 *
 * One-byte reader for the interactive repl. Polls stdin plus any fds the
 * ipc layer is currently interested in (listen socket + active connections),
 * then does a 1-byte raw read(2) on stdin once it's ready. Bypassing stdio
 * is required so we don't over-read past a newline while ipc fds are also
 * in the poll set.
 *
 * EOF is reported as the literal value EOF (-1), matching fgetc().
 *
 * NOTE: stdio buffering on stdin must remain empty across calls. The only
 * other stdin reader in the tree is readstdin() in io.c (used by 0:""),
 * which is called from the evaluator *after* the repl has consumed the
 * trailing '\n', so stdio's buffer starts empty there. If anything else
 * starts pulling bytes from stdin, this comment needs revisiting.
 */
#define POLL_MAX 258                       /* 1 stdin + 1 listen + 256 conns */
static int poll_getc(void) {
  struct pollfd pfd[POLL_MAX];
  for(;;) {
    pfd[0].fd      = STDIN_FILENO;
    pfd[0].events  = POLLIN;
    pfd[0].revents = 0;
    int n_extra = ipc_extra_pollfds(pfd + 1, POLL_MAX - 1);
    int n_total = 1 + n_extra;

    int n = poll(pfd, n_total, tmr_timeout_ms());
    if(n < 0) {
      if(errno == EINTR) continue;
      return EOF;
    }

    /* dispatch any ipc events first; they don't affect stdin state */
    for(int i = 1; i < n_total; i++) {
      if(pfd[i].revents) ipc_handle_pollfd(pfd[i].fd, pfd[i].revents);
    }

    /* fire the recurring timer if its slot has come up. No-op if
     * disabled, in a sub-repl, or already inside a tick. */
    tmr_maybe_fire();

    if(pfd[0].revents & (POLLIN|POLLHUP|POLLERR)) {
      unsigned char c;
      ssize_t r = read(STDIN_FILENO, &c, 1);
      if(r == 0) return EOF;
      if(r < 0) {
        if(errno == EINTR) continue;
        return EOF;
      }
      return (int)c;
    }
    /* no stdin yet; loop and poll again (ipc events were dispatched above) */
  }
}
#define repl_getc() poll_getc()
#endif

int ecount;
static int pcount,scount,ccount,qcount;
int DEPTH;
int REPL;
int LOADLINE;

static FILE *ofopen(const char *p) {
  FILE *f;
#ifdef _WIN32
  if(fopen_s(&f,p,"r")) f=0;
#else
  f=fopen(p,"r");
#endif
  return f;
}

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&_S_IFDIR)!=0)  /* MSVC <sys/stat.h> lacks the macro */
#endif

/* A directory on the load path is a "package": loading it really loads its
   entry script <dir>/GKPKGENTRY (the Python `__init__.py` / Node `index.js`
   idea). The entry uses .z.filepath to find its own dir and 2:-link siblings.
   This also closes a latent trap -- fopen() succeeds on a directory, so without
   this a `\l somedir` would silently "load" nothing. */
#define GKPKGENTRY "load.k"

/* Open `path` as a script. If `path` names a directory, open its package entry
   <path>/load.k instead, writing the resolved path into `buf` (capacity `n`).
   *out is set to the path actually opened (== `path`, or `buf` for a package),
   so pfile / .z.filepath report the real file. Returns the FILE*, or 0. */
static FILE *ofopen_script(char *path, char *buf, size_t n, char **out) {
  struct stat st;
  *out = path;
  if(!stat(path,&st) && S_ISDIR(st.st_mode)) {
    if((size_t)snprintf(buf,n,"%s/%s",path,GKPKGENTRY) >= n) return 0;
    FILE *f = ofopen(buf);
    if(f) *out = buf;
    return f;
  }
  return ofopen(path);
}

K load(char *fn, int load) {
  FILE *fp=0; K r,e=null;
  K cs0,gs0,d0,zfp0=0;
  int c,f,s=0,space=0,op=0;
  size_t i=0,j=0,m=32;
  char *b,*pfile0,*rfn=fn;
  char rbuf[4096],cbuf[4096];
  int fileline0=fileline;
  fileline=0;
  int newfileline=0;
  /* Resolve fn to an openable script (ofopen_script also turns a directory
     into its package entry <dir>/load.k). */
  fp=ofopen_script(fn,rbuf,sizeof rbuf,&rfn);
  /* \l (load==2) consults GKPATH when the literal name isn't found and isn't
     an absolute path: a list of directories searched in order, first hit wins
     (separator ':' on unix, ';' on windows).  fn is repointed at the resolved
     path so pfile and error messages report where it actually loaded from.
     The command-line script arg (load==1) is never searched -- it stays literal. */
  if(!fp && load==2 && fn[0]!='/') {
#ifdef _MSC_VER
    /* MSVC-only: _dupenv_s mallocs, freed below. MinGW/POSIX use getenv (static). */
    char *gp=0; _dupenv_s(&gp,0,"GKPATH");
#else
    char *gp=getenv("GKPATH");
#endif
    if(gp) {
      size_t fl=strlen(fn);
      for(char *p=gp;*p;) {
        char *sep=strchr(p,GKPATHSEP);
        size_t dl=sep?(size_t)(sep-p):strlen(p);
        if(dl && dl+1+fl<sizeof cbuf) {
          memcpy(cbuf,p,dl); cbuf[dl]='/'; memcpy(cbuf+dl+1,fn,fl+1);
          if((fp=ofopen_script(cbuf,rbuf,sizeof rbuf,&rfn))) break;
        }
        if(!sep) break;
        p=sep+1;
      }
#ifdef _MSC_VER
      free(gp);
#endif
    }
  }
  if(!fp) { fprintf(stderr,"%s: %s\n",fn,xstrerror(errno)); fileline=fileline0; return null; }
  fn=rfn;  /* commit the resolved path (package entry, or GKPATH hit) */
  b=xmalloc(m+2);
  pcount=scount=ccount=qcount=0;
  f=1; s=0;
  /* In the WASM REPL leave pfile empty so error context reads like the interactive
   * REPL ("type error / src / ^") instead of leaking the temp filename ("in stdin:1
   * / ..."): printerror prints the "in FILE:N" line only when pfile is non-empty
   * (p.c). .z.filepath still gets the real fn below. */
  pfile0=pfile; pfile = wasm ? "" : fn;
  zfp0=scope_set_z_filepath(fn);  /* .z.filepath; restored at cleanup */
  cs0=k_(cs);
  gs0=k_(gs);
  d0=D;
  while(1) {
    j=i;
    while((c=fgetc(fp))!=EOF&&c!='\n') {
      if(c=='\r') continue;
      if(i==m) { m<<=1; b=xrealloc(b,m+2); }
      b[i++]=c;
      switch(s) {
      case 0:
        if(c=='(') ++pcount; else if(pcount&&c==')') --pcount;
        else if(c=='[') ++scount; else if(scount&&c==']') --scount;
        else if(c=='{') ++ccount; else if(ccount&&c=='}') --ccount;
        else if(c=='"') qcount=s=1;
        else if(f&&c=='\\') ++f;
        else if((space||f==1||op)&&c=='/') s=2; /* op: `/` right after an opener ( [ { is a comment */
        if(!isblank(c)&&c!='\\') f=0;
        break;
      case 1:
        if(c=='"') qcount=s=0;
        else if(c=='\\') s=11;
        break;
      case 11: s=1; break;
      case 2: // comment
        if(c=='\n') s=0;
        break;
      }
      space=isblank(c); op=(c=='('||c=='['||c=='{');
    }
    if(f==2) { break; }
    if(c==EOF) { break; } /* have to unwind to free memory */
    b[i]=0;
    if(!strcmp(b+j,"\\")) { e=kerror("abort"); goto cleanup; }
    if(i==m) { m<<=1; b=xrealloc(b,m+2); }
    b[i++]='\n';
    ++newfileline;
    if(!pcount&&!scount&&!ccount&&!qcount) {
      b[i++]=0;
      r=memchr(b,0,i-1)?KERR_PARSE:pgparse(xstrdup(b),load,0);  /* raw NUL: not valid source */
      if(E(r)) {
        if(r<EMAX) r=kerror(E[r]);
        if(glinep&&gline0p&&strcmp(glinep,gline0p)) {
          if(!wasm) printf("%s ... + %d in %s:%d\n",gline0p,gline0,fn,gline0+gline+1);
          kprint(r,"","\n","");
          printf("%s\n",glinep);
          i(gline0i,putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
          pfile=pfile0;
          if(glinep) { xfree(glinep); glinep=0; }
          if(gline0p) { xfree(gline0p); gline0p=0; }
          if(!wasm) fprintf(stderr,"load: %s ... + %d\n",fn,gline0+gline+1);
        }
        else {
          if(!wasm) printf("in %s:%d\n",fn,fileline+1);
          kprint(r,"","\n","");
          if(glinep) printf("%s\n",glinep);
          i(gline0i,putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
          pfile=pfile0;
          if(glinep) { xfree(glinep); glinep=0; }
          if(gline0p) { xfree(gline0p); gline0p=0; }
          if(!wasm) fprintf(stderr,"load: %s ... + %d\n",fn,fileline+1);
        }
        e=kerror("domain");
        goto cleanup;
      }
      else if(r) {
#ifdef FUZZING
        gk_budget=GK_BUDGET;
        gk_alloc_budget=GK_ALLOC_BUDGET;
#endif
        ++DEPTH;
        K rr=pgreduce(r,1);
        --DEPTH;
        if(E(rr)&&EFLAG) {
          if(!wasm) fprintf(stderr,"load: %s ... + %d\n",fn,newfileline);
          prfree(r);
          e=rr;
          goto cleanup;
        }
        prfree(r);
      }
      i=0;
      fileline=newfileline;
    }
    f=1; s=0;
    if(EXIT) { break; }
  }
  D=d0;
  _k(gs); gs=gs0;
  _k(cs); cs=cs0;
  gcache_clear(); /* gs may have changed during load */
  pfile=pfile0;
  if(EXIT) { e=kerror("abort"); goto cleanup; }
  if(pcount||scount||ccount||qcount) {
    if(!wasm) fprintf(stderr,"load: %s ... + %d\n",fn,newfileline);
    fprintf(stderr,"open error\n");
    b[i]=0;
    if(i) --i;
    if(b[i]=='\n') while(i && b[i]=='\n') --i; // remove trailing nl's
    b[i+1]=0;
    int z;
    for(z=i;z>=0;--z) if(b[z]=='\n') break;
    printf("%s\n",b+z+1);
    i(strlen(b+z+1),putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
  }
cleanup:
  scope_restore_z_filepath(zfp0);
  fclose(fp);
  fileline=fileline0;
  xfree(b);
  return e;
}

static int H;
int wasm=0;
static K repl_(void) {
  int c,f,s=0,space=0,op=0; size_t i=0,j=0,m=32; char *b; K r;
  const char *prompt="  ";
  /* WASM (no stdin): an error opened this subconsole IN PLACE; don't print a prompt
   * or block on stdin -- signal EXIT so the in-flight statement unwinds cleanly. The
   * error was already printed by printerror before we got here. */
  if(wasm) { if(!EXIT) EXIT=t(1,0); return null; }
  b=xmalloc(m+2);
  pcount=scount=ccount=qcount=0;
  f=1; s=0;
  while(1) {
    j=i;
    i(ecount+pcount+scount+ccount+qcount,putc('>',stderr))
    fputs(prompt,stderr);
    while((c=repl_getc())!=EOF&&c!='\n') {
      if(c=='\r') continue;
      if(i==m) { m<<=1; b=xrealloc(b,m+2); }
      b[i++]=c;
      switch(s) {
      case 0:
        if(c=='(') ++pcount; else if(pcount&&c==')') --pcount;
        else if(c=='[') ++scount; else if(scount&&c==']') --scount;
        else if(c=='{') ++ccount; else if(ccount&&c=='}') --ccount;
        else if(c=='"') qcount=s=1;
        else if(f&&c=='\\') ++f;
        else if((space||f==1||op)&&c=='/') s=2; /* op: `/` right after an opener ( [ { is a comment */
        if(!isblank(c)&&c!='\\') f=0;
        break;
      case 1:
        if(c=='"') qcount=s=0;
        else if(c=='\\') s=11;
        break;
      case 11: s=1; break;
      case 2: // comment
        if(c=='\n') s=0;
        break;
      }
      space=isblank(c); op=(c=='('||c=='['||c=='{');
    }
    if(isatty(STDIN_FILENO) && i>=4095) {
        xfree(b);
        if(EFLAG) ++ecount;
        return kerror("len");
    }
    if(f==2) {
      if(DEPTH) { xfree(b); return kerror("abort"); }
      if(ecount) --ecount;
      else if(cs==gs && !pcount&&!scount&&!ccount&&!qcount) help(0);
      xfree(b);
      return null;
    }
    if(c==EOF) { xfree(b); EXIT=t(1,0); return null; } /* have to unwind to free memory */
    b[i]=0;
    if(!strcmp(b+j,"\\")) { xfree(b); H=0; return kerror("abort"); }
    if(!pcount&&!scount&&!ccount&&!qcount) break;
    if(i==m) { m<<=1; b=xrealloc(b,m+2); }
    b[i++]='\n';
    f=1; s=0;
  }
  if(i==m) { m<<=1; b=xrealloc(b,m+2); }
  b[i++]='\n'; b[i]=0;
  int opencode0=opencode; opencode=1;
  //K d0=D; K cs0=cs; K gs0=gs;
  /* A raw NUL is never valid in source (escaped "\0" is the two bytes \ 0, not
     a NUL byte).  The byte-level bracket counter above can balance a line whose
     embedded NUL would truncate the C-string the lexer sees, handing the
     open-code parser an unterminated group -> spin.  Reject it as a parse
     error here, while we still know the real length. */
  if(memchr(b,0,i)) { xfree(b); r=KERR_PARSE; } /* NUL-reject must still free the line buffer that pgparse() would otherwise consume */
  else r=pgparse(b,0,0);
  //D=d0; cs=cs0; gs=gs0;
  opencode=opencode0;
  if(E(r)) {
    if(r<EMAX) r=kerror(E[r]);
     kprint(r,"","\n","");
     if(glinep) printf("%s\n",glinep);
     i(gline0i,putc(' ',stderr))
     putc('^',stderr);
     putc('\n',stderr);
     if(glinep) { xfree(glinep); glinep=0; }
     if(gline0p) { xfree(gline0p); gline0p=0; }
     r=null;
  }
  else if(r) {
#ifdef FUZZING
    gk_budget=GK_BUDGET;
    gk_alloc_budget=GK_ALLOC_BUDGET;
#endif
    int opencode0=opencode; opencode=1;
    K t=pgreduce(r,0);
    opencode=opencode0;
    prfree(r);
    r=t;
  }
  return r;
}

K repl(void) {
  K r; //,os=0;
  int repl0;
  while(1) {
    ++DEPTH; repl0=REPL; REPL=1; H=1;
    //if(cs!=gs) { os=cs; cs=gs; } // don't open repl in local scope
    r=repl_();
    //if(os) cs=os;
    --DEPTH; REPL=repl0;
    if(DEPTH) {
      if(0xc0==s(r) && 32==ck(r)) { /* > : (return from subconsole) */
        if(ecount) --ecount;
        return null;
      }
      if(EXIT) return r; /* have to unwind to free memory */
      if(E(r)&&sk(r)==sp("abort")) {
        H=0;
        if(ecount) --ecount;
        if(ecount==0) return r;
      }
      if(SIGNAL) {
        SIGNAL=0;
        if(ecount) --ecount;
        return r;
      }
      if(RETURN) {
        RETURN=0;
        if(ecount) --ecount;
        return r;
      }
    }
    else {
      if(EXIT) {
        exit__(EXIT); /* nothing left to unwind */
      }
      if(E(r)&&sk(r)==sp("abort")) {
        if(ecount) --ecount;
        if(H) help(0);
      }
      if(SIGNAL) {
        SIGNAL=0;
        if(ecount) --ecount;
      }
      if(RETURN) {
        RETURN=0;
        if(ecount) --ecount;
      }
    }
    if(r!=null) { if(quiet) _k(r); else kprint(r,"","\n",""); }
  }
  return r;
}
