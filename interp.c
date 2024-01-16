#include "interp.h"
#include "x.h"
#include "timer.h"
#include "p.h"
#include "ops.h"
#include "scope.h"
#include "fn.h"
#include <errno.h>
#include <string.h>

int ecount;

K* reduce(node *a, pgs *s, int top) {
  K *r;
  int i;
  for(i=a->v-1;i>=0;i--) {
    r=node_reduce(a->a[i],0);
    if(A) {
      if(ecount) { --ecount; dset(Z,"s",null); }
      else { help1_(0,0); }
      if(!DEPTH) A=0;
      else { node_free(a); pgfree(s); kfree(r); return null; }
    }
    if(!DEPTH) {
      if(fret) { fret=0; if(!top) { node_free(a); pgfree(s); return r; } }
      if(rt==7&&((fn*)r->v)->i==':'&&!top) { node_free(a); pgfree(s); return r; }
    }
    if(btime) { btime=0; quiet=0; printf("%.3f\n",timer_stop()); }
    if((!a->a[i]->q||fret)&&!quiet2&&!quiet) kprint(r,"",0,1);
    node_free(a->a[i]);
    quiet2=0;
    if(r->t==98) { kfree(r); --i; while(i>=0) node_free(a->a[i--]); break; }
    kfree(r);
  }
  return 0;
}

void load(char *fnm) {
  FILE *fp=0;
  int i,j=0,c,f,m=256;
  node *a;
  pgs *s;
  char *od;
  fp=fopen(fnm,"r");
  if(!fp) { fprintf(stderr,"%s: %s\n",fnm,strerror(errno)); return; }
  if(!fnames) {
    fnamesm=2;
    fnames=xmalloc(fnamesm*sizeof(char*));
    fnames[fnamesi++]=xstrdup(fnm);
  }
  s=pgnew();
  s->fni=fnamesi-1;
  f=1; i=0;
  s->p=xmalloc(m+2);
  while((c=fgetc(fp))!=EOF) {
    s->p[i++]=c;
    if(i==m) { m<<=1; s->p=xrealloc(s->p,m+2); }
    if(f==1&&c=='\\') { j=i-1; ++f; }
    else if(f==2&&c=='\\') f=0;
    else if(f==2&&c=='\n') { s->p[j]=0; break; }
    if(c!=' '&&c!='\t'&&c!='\\') f=0;
    if(c=='\n') f=1;
  }
  s->p[i]=0;
  a=pgparse(s);
  if(!a) return;
  od=D->v;
  reduce(a,s,0);
  D->v=od;
  a->v=0; node_free(a);
  fclose(fp);
  pgfree(s);
}

K* interp(int top) {
  K *r;
  node *a;
  int i,c,m=256,pcount=0,scount=0,ccount=0,qcount=0,f,abort=0,spc=0,cmt=0;
  pgs *s=pgnew();
  char *prompt="  ";

  s->p=xmalloc(m+2);

  if(!quiet) { DO(ecount,putc('>',stderr)); fprintf(stderr,"%s",prompt); }
  while(1) {
    for(i=0;;i=0) {
      pcount=scount=ccount=qcount=0;
      while(1) {
        f=1;spc=0;cmt=1;
        while((c=fgetc(stdin))!=EOF&&c!='\n') {
          s->p[i++]=c;
          if(spc&&c=='/') cmt=1;
          if(cmt) continue;
          if(i==m) { m<<=1; s->p=xrealloc(s->p,m+2); }
          if(!qcount) {
            if(c=='(') ++pcount; else if(c==')') --pcount;
            else if(c=='[') ++scount; else if(c==']') --scount;
            else if(c=='{') ++ccount; else if(c=='}') --ccount;
            else if(c=='"') qcount^=1;
            else if(f&&c=='\\') ++f;
            if(c!=' '&&c!='\t'&&c!='\\') f=0;
          }
          else if(c=='"') qcount^=1;
          if(c==' '||c=='\t') spc=1; else spc=0;
        }
        if(c==EOF) exit(0);
        if(f==3) exit(0);
        if(f==2) { abort=1; break; }
        if(!pcount&&!scount&&!ccount&&!qcount) break;
        else if(pcount<0||scount<0||ccount<0) { printf("unmatched\n  "); break; }
        s->p[i++]='\n';
        DO(pcount+scount+ccount+qcount+ecount,putc('>',stderr))
        fprintf(stderr,"%s",prompt);
      }
      s->p[i++]='\n'; s->p[i]=0;
      if(abort) {
        abort=0;
        if(pcount|scount|ccount|qcount) {
          pcount=scount=ccount=qcount=0;
          DO(ecount,putc('>',stderr));
          fprintf(stderr,"%s",prompt);
          continue;
        }
      }
      if(pcount<0||scount<0||ccount<0) { pcount=scount=ccount=0; continue; }
      a=pgparse(s);
      if(!a) { fprintf(stderr,"%s",prompt); continue; }
      r=reduce(a,s,top);
      if(r) return r;
      a->v=0; node_free(a);
      if(!quiet) { DO(ecount,putc('>',stderr)); fprintf(stderr,"%s",prompt); }
    }
  }
  pgfree(s);
  return r;
}
