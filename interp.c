#include "interp.h"
#include "x.h"
#include "timer.h"
#include "p.h"
#include "ops.h"
#include "scope.h"
#ifdef _WIN32
#include "unistd.h"
#else
#include <unistd.h>
#endif
#include "fn.h"

int ecount;

K* interp(char *fnm, int interactive, int top) {
  FILE *fp=0;
  K *r;
  node *a;
  int i,j,c,m=256,pcount=0,scount=0,ccount=0,qcount=0,f,abort=0;
  pgs *s=pgnew();
  char *prompt="  ";

  if(fnm) {
    if(access(fnm,F_OK)) fprintf(stderr,"%s: file not found\n",fnm);
    else if(access(fnm,R_OK)) fprintf(stderr,"%s: permission denied\n",fnm);
    else fp=fopen(fnm,"r");
    fnamesm=2;
    fnames=xmalloc(fnamesm*sizeof(char*));
    fnames[fnamesi++]=xstrdup(fnm);
    s->fni=fnamesi-1;
  }
  if(fp) interactive=0;
  else fp=stdin;

  s->p=xmalloc(m+2);
  if(!quiet&&interactive) { DO(ecount,putc('>',stderr)); fprintf(stderr,"%s",prompt); }
  while(1) {
    for(i=0;;i=0) {
      if(interactive) {
        pcount=scount=ccount=qcount=0;
        while(1) {
          f=1;
          while((c=fgetc(fp))!=EOF&&c!='\n') {
            s->p[i++]=c;
            if(i==m) { m<<=1; s->p=xrealloc(s->p,m+2); }
            if(c=='(') ++pcount; else if(c==')') --pcount;
            else if(c=='[') ++scount; else if(c==']') --scount;
            else if(c=='{') ++ccount; else if(c=='}') --ccount;
            else if(c=='"') qcount^=1;
            else if(f&&c=='\\') ++f;
            if(c!=' '&&c!='\t'&&c!='\\') f=0;
          }
          if(f==3) exit(0);
          if(f==2) { abort=1; break; }
          if(!pcount&&!scount&&!ccount&&!qcount) break;
          else if(pcount<0||scount<0||ccount<0) { printf("unmatched\n  "); break; }
          s->p[i++]='\n';
          for(j=0;j<pcount;j++) fprintf(stderr,">");
          for(j=0;j<scount;j++) fprintf(stderr,">");
          for(j=0;j<ccount;j++) fprintf(stderr,">");
          for(j=0;j<qcount;j++) fprintf(stderr,">");
          DO(ecount,putc('>',stderr))
          fprintf(stderr,"%s",prompt);
        }
        s->p[i++]='\n'; s->p[i]=0;
      }
      else {
        f=1;
        while((c=fgetc(fp))!=EOF) {
          s->p[i++]=c;
          if(i==m) { m<<=1; s->p=xrealloc(s->p,m+2); }
          if(f==1&&c=='\\') { j=i-1; ++f; }
          else if(f==2&&c=='\\') f=0;
          else if(f==2&&c=='\n') { abort=1; s->p[j]=0; break; }
          if(c!=' '&&c!='\t'&&c!='\\') f=0;
          if(c=='\n') f=1;
        }
        s->p[i]=0;
      }
      if(abort) {
        abort=0;
        if(interactive) {
          if(pcount|scount|ccount|qcount) {
            pcount=scount=ccount=qcount=0;
            DO(ecount,putc('>',stderr));
            fprintf(stderr,"%s",prompt);
            continue;
          }
        }
        else { fclose(fp); interactive=1; fp=stdin; }
      }
      if(pcount<0||scount<0||ccount<0) { pcount=scount=ccount=0; continue; }
      a=pgparse(s);
      if(!a) { fprintf(stderr,"%s",prompt); continue; }
      for(i=a->v-1;i>=0;i--) {
        r=node_reduce(a->a[i]);
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
      a->v=0; node_free(a);
      if(!quiet&&interactive) { DO(ecount,putc('>',stderr)); fprintf(stderr,"%s",prompt); }
      if(!interactive) break;
    }
    if(!interactive) { fclose(fp); interactive=1; fp=stdin; fprintf(stderr,"%s",prompt); }
  }
  pgfree(s);
  return r;
}
