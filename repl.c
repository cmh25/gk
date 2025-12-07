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
#else
#include <unistd.h>
#endif
#include "timer.h"
#include "scope.h"
#include "h.h"
#include "b.h"

int ecount;
static int pcount,scount,ccount,qcount;
int DEPTH;
int REPL;
int LOADLINE;

K load(char *fn, int load) {
  FILE *fp=0; K r,e=null;
  K cs0,gs0,d0;
  int c,f,s=0,space=0;
  size_t i=0,j=0,m=32;
  char *b,*pfile0;
  int fileline0=fileline;
  fileline=0;
  int newfileline=0;
#ifdef _WIN32
  if(fopen_s(&fp,fn,"r")) fp=0;
#else
  fp=fopen(fn,"r");
#endif
  if(!fp) { fprintf(stderr,"%s: %s\n",fn,xstrerror(errno)); fileline=fileline0; return null; }
  b=xmalloc(m+2);
  pcount=scount=ccount=qcount=0;
  f=1; s=0;
  pfile0=pfile; pfile=fn;
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
        else if((space||f==1)&&c=='/') s=2;
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
      space=isblank(c);
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
      r=pgparse(xstrdup(b),load,0);
      if(E(r)) {
        if(r<EMAX) r=kerror(E[r]);
        if(glinep&&gline0p&&strcmp(glinep,gline0p)) {
          printf("%s ... + %d in %s:%d\n",gline0p,gline0,fn,gline0+gline+1);
          kprint(r,"","\n","");
          printf("%s\n",glinep);
          i(gline0i,putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
          pfile=pfile0;
          if(glinep) { xfree(glinep); glinep=0; }
          if(gline0p) { xfree(gline0p); gline0p=0; }
          fprintf(stderr,"load: %s ... + %d\n",fn,gline0+gline+1);
        }
        else {
          printf("in %s:%d\n",fn,fileline+1);
          kprint(r,"","\n","");
          if(glinep) printf("%s\n",glinep);
          i(gline0i,putc(' ',stderr)); putc('^',stderr); putc('\n',stderr);
          pfile=pfile0;
          if(glinep) { xfree(glinep); glinep=0; }
          if(gline0p) { xfree(gline0p); gline0p=0; }
          fprintf(stderr,"load: %s ... + %d\n",fn,fileline+1);
        }
        e=kerror("domain");
        goto cleanup;
      }
      else if(r) {
        ++DEPTH;
        K rr=pgreduce(r,1);
        --DEPTH;
        if(E(rr)&&EFLAG) {
          fprintf(stderr,"load: %s ... + %d\n",fn,newfileline);
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
  pfile=pfile0;
  if(EXIT) { e=kerror("abort"); goto cleanup; }
  if(pcount||scount||ccount||qcount) {
    fprintf(stderr,"load: %s ... + %d\n",fn,newfileline);
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
  fclose(fp);
  fileline=fileline0;
  xfree(b);
  return e;
}

static int H;
static K repl_(void) {
  int c,f,s=0,space=0; size_t i=0,j=0,m=32; char *b; K r;
  const char *prompt="  ";
  b=xmalloc(m+2);
  pcount=scount=ccount=qcount=0;
  f=1; s=0;
  while(1) {
    j=i;
    i(ecount+pcount+scount+ccount+qcount,putc('>',stderr))
    fputs(prompt,stderr);
    while((c=fgetc(stdin))!=EOF&&c!='\n') {
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
        else if((space||f==1)&&c=='/') s=2;
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
      space=isblank(c);
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
  r=pgparse(b,0,0);
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
