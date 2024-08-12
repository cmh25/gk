#include "io.h"
#ifdef _WIN32
  #define strtok_r strtok_s
#else
  #include <fcntl.h>
  #include <unistd.h>
  #include <sys/mman.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "x.h"
#include "sym.h"
#include "ops.h"

extern K* null;

static K* ferr(char *f, int e) {
  K *r;
  char *s,*b;
  s=strerror(e);
  b=xmalloc(strlen(f)+strlen(s)+3);
  sprintf(b,"%s: %s",f,s);
  r=kerror(b);
  xfree(b);
  return r;
}

static K* fopen_(K *a, char *m, FILE **fp) {
  char *s;
  if(at==4) s=a->v;
  else s=xstrndup(v3(a),ac);
  *fp=fopen(s,m);
  if(!*fp) return ferr(s,errno);
  if(at==-3) xfree(s);
  return null;
}

static K* fsize_(FILE *fp, long *bs) {
  if(fseek(fp,0L,SEEK_END)) return kerror("io");
  *bs=ftell(fp); if(*bs==-1) return kerror("io");
  if(fseek(fp,0L,SEEK_SET)) return kerror("io");
  return null;
}

K* b0colon1_(K *a) {
  char *s=0;
  if(at==3) printf("%c", a->i);
  else if(at==-3) DO(ac,putchar(v3(a)[i]))
  else if(at==0) {
    DO(ac, if(v0(a)[i]->t!=-3) return kerror("type"))
    DO(ac, s=v3(v0(a)[i]); DO2(v0(a)[i]->c, putchar(s[j]));putchar('\n'))
  }
  else return kerror("type");
  return null;
}

K* zerocolon2_(K *a, K *b) {
  FILE *fp;
  K *p;
  if(bt!=-3&&bt!=4&&bt!=0) return kerror("type");
  EC(fopen_(a,"w",&fp));
  switch(bt) {
  case  0: DO(bc,p=v0(b)[i];DO2(p->c,fputc(v3(p)[j],fp));fputc('\n',fp)) break;
  case -3: DO(bc,fputc(v3(b)[i],fp)); fputc('\n',fp); break;
  }
  fclose(fp);
  return null;
}

K* zerocolon1_(K *a) {
  K *r=0;
  FILE *fp;
  unsigned int n=0,i,m=2;
  char *b,*p,*s;
  long bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"r",&fp));
  EC(fsize_(fp,&bs));
  b=xmalloc(bs+1);
  n=fread(b,1,bs,fp);
  if(ferror(fp)) return kerror("io");
  b[n]=0;
  fclose(fp);
  r=kv0(m);
  i=0;
  p=strtok_r(b,"\n",&s);
  do{
    if(i==m) { m<<=1; r->v=xrealloc(r->v,m*sizeof(K*)); }
    v0(r)[i++]=knew(-3,strlen(p),p,0,0,0);
  }while((p=strtok_r(0,"\n",&s)));
  xfree(b);
  r->c=i;
  return r;
}

K* onecolon2_(K *a, K *b) {
  FILE *fp;
  K *r;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"wb",&fp));
  r=bd1_(b);
  fwrite(r->v,1,r->c,fp);
  fclose(fp);
  kfree(r);
  return null;
}

#ifdef _WIN32
K* onecolon1_(K *a) {
  return twocolon1_(a); /* TODO: map */
}
#else
K* onecolon1_(K *a) {
  K *r;
  int fd,h,t,c;
  char *s;
  void *v;
  size_t len;
  ssize_t n;
  if(at==4) s=a->v;
  else if(at==-3) s=xstrndup(v3(a),ac);
  else return kerror("type");
  fd=open(s,O_RDONLY);
  if(fd==-1) return ferr(s,errno);
  n=read(fd,&h,sizeof(int)); if(n==-1) return ferr(s,errno);
  n=read(fd,&t,sizeof(int)); if(n==-1) return ferr(s,errno);
  n=read(fd,&c,sizeof(int)); if(n==-1) return ferr(s,errno);
  if(at==-3) xfree(s);
  if(t!=-1&&t!=-2&&t!=-3) { close(fd); return twocolon1_(a); }
  if(t==-1) len=12+c*sizeof(int);
  else if(t==-2) len=12+c*sizeof(double);
  else if(t==-3) len=12+c*sizeof(char);
  else return kerror("type");
  v=mmap(0,len,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
  if(v==(void*)-1) { r=ferr(s,errno); return r; }
  r=kv1(1); xfree(r->v);
  r->v=12+(char*)v;
  r->r=-2;
  r->c=c;
  r->t=t;
  close(fd);
  return r;
}
#endif

K* twocolon1_(K *a) {
  K *r=0,*p=0;
  FILE *fp;
  int n;
  char *b;
  long bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"rb",&fp));
  EC(fsize_(fp,&bs));
  b=xmalloc(bs);
  n=fread(b,1,bs,fp);
  if(ferror(fp)) return kerror("io");
  p=kv3(1); xfree(p->v); p->v=b; p->c=n;
  r=db1_(p);
  fclose(fp);
  kfree(p);
  return r;
}

K* fivecolon2_(K *a, K *b) {
  K *r,*p;
  FILE *fp;
  int n,t,c=0;
  size_t m;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"ab+",&fp));
  fseek(fp,0,SEEK_END);
  if(ftell(fp)) {                /* was file opened or created? */
    fseek(fp,4,SEEK_SET);        /* skip header */
    m=fread(&t,sizeof(int),1,fp); if(m<=0&&ferror(fp)) return ferr("",errno);
    m=fread(&c,sizeof(int),1,fp); if(m<=0&&ferror(fp)) return ferr("",errno);
    if(t!=bt) { fclose(fp); return kerror("type"); }
    p=bd1_(b);
    fseek(fp,0,SEEK_END);
    fwrite(12+(char*)p->v,1,p->c-12,fp);
    fclose(fp);                  /* close and reopen to edit count */
    EC(fopen_(a,"rb+",&fp));
    fseek(fp,8,SEEK_SET);
    n=c+bc;
    fwrite(&n,sizeof(int),1,fp); /* update count */
    fclose(fp);
    r=k1(n);
  }
  else {
    p=bd1_(b);
    fwrite(p->v,1,p->c,fp);
    fclose(fp);
    r=null;
  }
  kfree(p);
  return r;
}

K* sixcolon2_(K *a, K *b) {
  FILE *fp;
  if(at!=-3&&at!=4&&at!=0) return kerror("type");
  if(bt!=-3) return kerror("type");
  if(!at) { /* append */
    if(!ac||(v0(a)[0]->t!=-3&&v0(a)[0]->t!=4)) return kerror("type");
    EC(fopen_(v0(a)[0],"a",&fp));
  }
  else EC(fopen_(a,"wb",&fp));
  DO(bc,fputc(v3(b)[i],fp));
  fclose(fp);
  return null;
}

K* sixcolon1_(K *a) {
  K *r=0;
  FILE *fp;
  int n=0;
  char *b;
  long bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"r",&fp));
  EC(fsize_(fp,&bs));
  b=xmalloc(bs);
  n=fread(b,1,bs,fp);
  if(ferror(fp)) return kerror("io");
  fclose(fp);
  r=kv3(1); xfree(r->v); r->v=b; r->c=n;
  return r;
}

K* del1_(K *a) {
  char *s;
  if(at==4) s=a->v;
  else if(at==-3) s=xstrndup(a->v,ac);
  else return kerror("type");
  if((remove(s)==-1)) return ferr(s,errno);
  if(at==-3) xfree(s);
  return null;
}

K* rename2_(K *a, K *b) {
  char *p,*q;
  if(at==4) p=a->v;
  else if(at==-3) p=xstrndup(a->v,ac);
  else return kerror("type");
  if(bt==4) q=b->v;
  else if(bt==-3) q=xstrndup(b->v,bc);
  else return kerror("type");
  if((rename(p,q)==-1)) return kerror(strerror(errno));
  if(at==-3) xfree(p);
  if(bt==-3) xfree(q);
  return null;
}
