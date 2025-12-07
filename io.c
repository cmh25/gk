#include "io.h"
#ifdef _WIN32
#undef rc
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define strtok_r strtok_s
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "b.h"
#include "ms.h"

static K ferr(char *f, i32 e) {
  K r;
  char *s,*b;
#ifdef _WIN32
  char buf[256];
  strerror_s(buf,sizeof(buf),e);
  s=buf;
#else
  s=strerror(e);
#endif
  b=xmalloc(strlen(f)+strlen(s)+3);
  sprintf(b,"%s: %s",f,s);
  r=kerror(b);
  xfree(b);
  return r;
}

#ifdef _WIN32
#define FUZZ_ROOT "C:\\Temp\\gk-fuzz-io"
#define PATH_SEP  '\\'
#else
#define FUZZ_ROOT "/tmp/gk-fuzz-io"
#define PATH_SEP  '/'
#endif

static FILE* fopen__(FILE **fp, const char *p, const char *m) {
#ifdef _WIN32
  if(fopen_s(fp, p, m)) *fp = 0;
#else
  *fp = fopen(p, m);
#endif
  return *fp;
}

#ifdef FUZZING
static const char* fuzz_root(void) {
  const char *env = getenv("GK_FUZZ_IO_DIR");
  return (env && *env) ? env : FUZZ_ROOT;
}
static i32 contains_dotdot(const char *path) { return strstr(path, "..") != 0; }
static void needs_dirs(const char *path) {
  char tmp[1024];
  size_t len = strlen(path);
  if(len >= sizeof(tmp)) return;
  strcpy(tmp, path);
  for(char *p = tmp + 1; *p; p++) {
    if(*p == '/' || *p == '\\') {
      char saved = *p;
      *p = 0;
#ifdef _WIN32
      _mkdir(tmp);
#else
      mkdir(tmp, 0700);
#endif
      *p = saved;
    }
  }
}
static i32 join_sandbox(char *buf, size_t bufsz, const char *filename) {
  if(contains_dotdot(filename)) {
    return -1; // reject unsafe path
  }
  if(filename[0] == '/' || filename[0] == '\\' ||
     (isalpha(filename[0]) && filename[1] == ':' &&
     (filename[2] == '/' || filename[2] == '\\'))) {
    return -1; // reject absolute path
  }
  const char *root = fuzz_root();
  i32 n = snprintf(buf, bufsz, "%s%c%s", root, PATH_SEP, filename);
  if (n < 0 || (size_t)n >= bufsz) return -1; // overflow
  return 0;
}
static K fopen_(K x, const char *m, FILE **fp) {
  char b[2]={0},*s=b;
  char pathbuf[1024];
  if(tx==4) s = sk(x);
  else if(tx==3) b[0] = ck(x);
  else if(tx==-3) s = px(x);
  else return KERR_TYPE;
  if(join_sandbox(pathbuf, sizeof(pathbuf), s) < 0) {
    return KERR_DOMAIN; // invalid path
  }
  needs_dirs(pathbuf);
  *fp = fopen__(fp,pathbuf,m);
  if(!*fp) {
    K err=ferr(s,errno);
    return err;
  }
  return null;
}
#else
static K fopen_(K x, char *m, FILE **fp) {
  char b[2]={0},*s=b;
  if(tx==4) s = sk(x);
  else if(tx==3) b[0] = ck(x);
  else if(tx==-3) s = px(x);
  else return KERR_TYPE;
  *fp = fopen__(fp,s,m);
  if(!*fp) {
    K err=ferr(s,errno);
    return err;
  }
  return null;
}
#endif

#ifdef FUZZING
#ifdef _WIN32
static K fsize_(K x, size_t *bs) {
  K r=null;
  char b[2]={0},*s=b,pathbuf[1024];
  struct _stat64 t;
  if(tx==4) s=sk(x);
  else if(tx==3) b[0]=ck(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  if(join_sandbox(pathbuf, sizeof(pathbuf), s) < 0) r=6;
  else if(_stat64(pathbuf, &t) == -1) r=ferr(pathbuf, errno);
  else *bs = t.st_size;
  return r;
}
#else
static K fsize_(K x, size_t *bs) {
  K r=null;
  char b[2]={0},*s=b,pathbuf[1024];
  struct stat t;
  if(tx==4) s=sk(x);
  else if(tx==3) b[0]=ck(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  if(join_sandbox(pathbuf, sizeof(pathbuf), s) < 0) r=6;
  else if(stat(pathbuf, &t) == -1) r=ferr(pathbuf, errno);
  else *bs = t.st_size;
  return r;
}
#endif
#else
#ifdef _WIN32
static K fsize_(K x, size_t *bs) {
  K r=null;
  char b[2]={},*s=b;
  struct _stat64 t;
  if(tx==4) s=sk(x);
  else if (tx==3) s[0]=ck(x);
  else if (tx==-3) s=px(x);
  else return KERR_TYPE;
  if(_stat64(s,&t) == -1) r=ferr(s,errno);
  else *bs=t.st_size;
  return r;
}
#else
static K fsize_(K x, size_t *bs) {
  K r=null;
  char b[2]={0},*s=b;
  struct stat t;
  if(tx==4) s=sk(x);
  else if(tx==3) s[0]=ck(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  if(stat(s,&t) == -1) r=ferr(s,errno);
  else *bs=t.st_size;
  return r;
}
#endif
#endif

static K b0colon(K x) {
  K *pxk,r=null;
  char *pc;
  if(!x) return KERR_TYPE;
  switch(tx) {
  case  3: printf("%c",ck(x)); break;
  case -3: pc=px(x); i(nx,putchar(pc[i])) break;
  case  0:
    PXK;
    i(nx,if(-3!=T(pxk[i])) return KERR_TYPE)
    i(nx,pc=px(pxk[i]);j(n(pxk[i]),putchar(pc[j])) putchar('\n'))
    break;
  default: r=KERR_TYPE;
  }
  return r;
}

static char* readstdin(u64 *n) {
  u64 max = 1024, m = 0;
  char *b = xmalloc(max);
  int c;
  while((c = getchar()) != '\n') {
    if(m + 1 >= max) {
      max<<=1; b = xrealloc(b, max);
    }
    b[m++] = (char)c;
  }
  b[m] = '\0';
  if (n) *n = m;
  return b;
}
static K zerocolon1(K x) {
  K r=0,e,*prk;
  FILE *fp;
  size_t bs=0, n;
  char *b, *line, *end;
  u32 i=0, m=2;

  if(tx!=3 && tx!=-3 && tx!=4) return KERR_TYPE;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) {
    u64 n;
    char *s=readstdin(&n);
    r=tnv(3,n,s);
    return r;
  }
  EC(fopen_(x,"rb",&fp));
  EC(fsize_(x,&bs));
  VSIZE((i64)bs);
  b=xmalloc(bs);
  if((n=fread(b,1,bs,fp))!=bs) {
    int ioe=ferror(fp);
    fclose(fp);
    xfree(b);
    if(ioe) return kerror("io");
    return KERR_LENGTH;
  }
  fclose(fp);

  PRK(m);
  line=b;
  end=b+n;
  while(line<end) {
    char *nl=memchr(line,'\n',end-line);
    size_t len=nl?(size_t)(nl-line):(size_t)(end-line);
    /* normalize DOS endings: strip trailing '\r' before '\n' */
    if(len>0 && line[len-1]=='\r') len--;
    if(i==m) { m<<=1; r=kresize(r,m); prk=px(r); }
    prk[i++]=tnv(3,len,xmemdup(line,len));
    line=nl?nl+1:end;
  }
  xfree(b);
  n(r)=i;
  return r;
cleanup:
  return e;
}

static K zerocolon2(K a, K x) {
  K r=null,e=0,p=0,q=0,ff=0,*prk,*pak,*pxk;
  FILE *fp=0;
  size_t B=0,N=0,NN=0;
  u32 i,j,k,n,m,L=0,u;
  char *pxc,*ppc,*z,*pz,*z0=0,g;
  i32 *pqi;

  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return b0colon(x);

  switch(ta) {
  case -3: case 3: case 4:  /* file write */
    switch(tx) {
    case 0:  /* mixed list of lines */
      EC(fopen_(a,"wb",&fp));
      pxk=px(x);
      for(i=0;i<nx;i++) {
        if(T(pxk[i])!=-3) { fclose(fp); return KERR_TYPE; }
        ppc=px(pxk[i]);
        j(n(pxk[i]),fputc(ppc[j],fp))
        fputc('\n',fp);
      }
      break;
    case -3:  /* single string */
      EC(fopen_(a,"wb",&fp));
      pxc=px(x);
      i(nx,fputc(pxc[i],fp))
      fputc('\n',fp);
      break;
    default: return KERR_TYPE;
    } break;
  case 0:  /* file read with schema */
    if(na!=2) return KERR_LENGTH;
    pak=px(a);
    p=pak[0]; q=pak[1];
    if(T(p)!=-3) return KERR_TYPE;
    if(T(q)!=-1) return KERR_TYPE;
    if(n(p)!=n(q)) return KERR_LENGTH;

    ppc=px(p);
    pqi=px(q);
    i(n(p),if(!strchr("IFCS ",ppc[i])) return KERR_TYPE)

    n=0; i(n(p),if(ppc[i]!=' ') ++n)
    L=0; i(n(q),if(pqi[i]<0||pqi[i]==INT32_MAX) return KERR_INT; L+=pqi[i])
    VSIZE((i32)n);

    if(tx==0) {
      if(nx!=3) return KERR_TYPE;
      pxk=px(x);
      if(T(pxk[0])!=-3 && T(pxk[0])!=3 && T(pxk[0])!=4) return KERR_TYPE;
      if(T(pxk[1])!=1 && T(pxk[1])!=2) return KERR_TYPE;
      if(T(pxk[2])!=1 && T(pxk[2])!=2) return KERR_TYPE;
      if(T(pxk[1])==1) B=ik(pxk[1]);
      else {
        double d=fk(pxk[1]);
        if(!isfinite(d) || d<0 || d>(double)SIZE_MAX) { e=KERR_DOMAIN; goto cleanup; }
        B=d;
      }
      if(T(pxk[2])==1) N=ik(pxk[2]);
      else {
        double d=fk(pxk[2]);
        if(!isfinite(d) || d<0 || d>(double)SIZE_MAX) { e=KERR_DOMAIN; goto cleanup; }
        N=d;
      }
      ff=pxk[0];
    }
    else {
      ff=x;
      EC(fsize_(ff,&NN));
      B=0; N=NN;
    }
    VSIZE((i64)N);

    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    //if(B+N>NN) { fclose(fp); return KERR_LENGTH; }
    if(N>NN || B>NN-N) { e = KERR_LENGTH; goto cleanup; }

    z0=z=xmalloc(N+1); z[N]=0;
    fseek(fp,B,SEEK_SET);
    if(fread(z,1,N,fp)!=N) {
      int ioe=ferror(fp);
      fclose(fp);
      xfree(z0);
      if(ioe) return kerror("io");
      return KERR_LENGTH;
    }

    if(!L) { fclose(fp); xfree(z0); return KERR_TYPE; }
    m=N/L;
    j=0;
    r=tn(0,n); prk=px(r);
    for(i=0;i<n(p);i++) {
      if(ppc[i]=='I') prk[j++] = tn(1,m);
      else if(ppc[i]=='F') prk[j++] = tn(2,m);
      else if(ppc[i]=='C') prk[j++] = tn(0,m);
      else if(ppc[i]=='S') prk[j++] = tn(4,m);
    }

    k=0;
    while(z<z0+N) {
      if(*z=='\n') { ++z; continue; }

      char *nl = memchr(z, '\n', (z0 + N) - z);  /* find newline */
      if(!nl) nl = z0 + N;
      size_t rowlen = nl - z;
      // normalize DOS endings: trim trailing '\r' before '\n'
      if(rowlen > 0 && z[rowlen-1] == '\r') { rowlen--; nl--; }
      if(rowlen != L) { e=KERR_LENGTH; goto cleanup; }  /* length, row too short */

      z[L]=0;
      for(i=0,j=0;i<n(q);i++) {
        u=pqi[i];
        g=z[u];
        z[u]=0;
        if(ppc[i]=='I') {
          pz=z+pqi[i]-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz)) ++pz;
          i32 *pi=px(prk[j]);
          pi[k]=xatoi(pz);
          j++;
        }
        else if(ppc[i]=='F') {
          pz=z+pqi[i]-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz)) ++pz;
          double *pd=px(prk[j]);
          pd[k]=xstrtod(pz);
          j++;
        }
        else if(ppc[i]=='C') {
          K *pcell=px(prk[j]);
          pcell[k]=tnv(3,pqi[i],xmemdup(z,pqi[i]));
          j++;
        }
        else if(ppc[i]=='S') {
          pz=z+pqi[i]-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz)) ++pz;
          char **psym=px(prk[j]);
          psym[k]=sp(pz);
          j++;
        }
        z[u]=g;
        z+=pqi[i];
      }
      ++z;
      ++k;
    }
    j=0;
    i(n(p),if(ppc[i]!=' ') n(prk[j++])=k)
    break;
  default: r=KERR_TYPE;
  }

  if(fp) fclose(fp);
  if(z0) xfree(z0);
  return r;
cleanup:
  if(fp) fclose(fp);
  if(z0) xfree(z0);
  if(r) _k(r);
  return e;
}

K zerocolon(K a, K x) {
  K r;
  if(a) r=zerocolon2(a,x);
  else r=zerocolon1(x);
  return r;
}

#if defined(_WIN32) || defined(FUZZING)
static K twocolon1(K x);
static K onecolon1(K x) {
  return twocolon1(x);
}
#else
static K twocolon1(K x);
static K onecolon1(K x) {
  K r;
  i32 fd,t,st,pad;
  u64 c;
  char *s,h[4];
  void *v;
  size_t len;
  ssize_t n;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if(tx==4) s=sk(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  fd=open(s,O_RDONLY);
  if(fd==-1) return ferr(s,errno);
  n=read(fd,h,4); if(n==-1) return ferr(s,errno);
  if(h[0]!=2) { close(fd); return kerror("header"); }
  n=read(fd,&t,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&st,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&pad,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&c,sizeof(u64)); if(n==-1) return ferr(s,errno);
  if(t!=-1&&t!=-2&&t!=-3) { close(fd); return twocolon1(x); }
  if(t==-1) len=c*sizeof(int);
  else if(t==-2) len=c*sizeof(double);
  else if(t==-3) len=c*sizeof(char);
  else return KERR_TYPE;
  v=mmap(0,len,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
  if(v==(void*)-1) { r=ferr(s,errno); return r; }
  r=tnv(-t,c,24+(char*)v);
  ((ko*)(b(48)&r))->r=-2;
  n(r)=c;
  close(fd);
  return r;
}
#endif

static K onecolon2(K a, K x) {
  K r=null,e,p,q,ff,*prk,*pak,*pxk;
  FILE *fp=0;
  size_t B=0,N=0,NN=0;
  u32 i,j,k,n,m,L=0;
  char *z,*pz,*z0=0,*ze,g;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  switch(ta) {
  case -3: case 4:
    if(ta!=-3&&ta!=4) { e=KERR_TYPE; goto cleanup; }
    EC(fopen_(a,"wb",&fp));
    r=bd_(x); EC(r);
    if(E(r)) { fclose(fp); e=r; goto cleanup; }
    fwrite((void*)px(r),1,n(r),fp);
    fclose(fp); fp=0;
    _k(r); r=null;
    break;
  case 0:
    if(na!=2) { e=KERR_LENGTH; goto cleanup; }
    PAK;
    if(T(pak[0])!=-3) { e=KERR_TYPE; goto cleanup; }
    if(T(pak[1])!=-1) { e=KERR_TYPE; goto cleanup; }
    if(n(pak[0])!=n(pak[1])) { e=KERR_LENGTH; goto cleanup; }
    p=pak[0]; q=pak[1];
    char *ppc=px(p); int *pqi=px(q);
    i(n(p),if(!strchr("cbsifd CS",ppc[i])) { e=KERR_TYPE; goto cleanup; })
    n=0;i(n(p),if(ppc[i]!=' ')++n)
    VSIZE((i32)n);
    i(n(q),L+=pqi[i])

    for(u64 t= 0; t< n(p); ++t) {
      int w = pqi[t];
      switch (ppc[t]) {
      case 'c': case 'b': if(w!=1) { e = KERR_LENGTH; goto cleanup; } break;
      case 's': if(w!=2) { e = KERR_LENGTH; goto cleanup; } break;
      case 'i': case 'f': if(w!=4) { e = KERR_LENGTH; goto cleanup; } break;
      case 'd': if(w!=8) { e = KERR_LENGTH; goto cleanup; } break;
      case 'C': case 'S': if(w<=0 || w > INT_MAX) { e = KERR_LENGTH; goto cleanup; } break;
      case ' ': if(w<0 || w > INT_MAX) { e = KERR_LENGTH; goto cleanup; } break;
      default: e=KERR_TYPE; goto cleanup;
      }
    }

    if(tx==0) { /* ("id";4 8)1:("b0";0;48) */
      PXK;
      if(nx!=3) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[0])!=-3 && T(pxk[0])!=3 && T(pxk[0])!=4) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[1])!=1 && T(pxk[1])!=2) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[2])!=1 && T(pxk[2])!=2) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[1])==1) B=ik(pxk[1]); else B=fi(pxk[1]);
      if(T(pxk[2])==1) N=ik(pxk[2]); else N=fi(pxk[2]);
      ff=pxk[0];
    }
    else ff=x;  /* ("id";4 8)1:"b0" */
    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    if(tx!=0) { B=0; N=NN; }
    //if(B+N>(size_t)NN) { e=KERR_LENGTH; goto cleanup; }
    if(N>NN || B>NN-N) { e = KERR_LENGTH; goto cleanup; }
    VSIZE((i64)N);
    z0=z=xmalloc(1+N); ze=z0+N;
    fseek(fp,B,SEEK_SET);
    if(fread(z,1,N,fp)!=N) {
      if(ferror(fp)) e=kerror("io");
      else e=KERR_LENGTH;
      goto cleanup;
    }
    if(!L) { e=KERR_LENGTH; goto cleanup; }
    if(N%L) { e=KERR_LENGTH; goto cleanup; }
    m=N/L;
    PRK(n);
    // ("cid S";1 4 8 4 5)1:("b2";22;66)
    for(i=0,j=0;i<n(p);i++) {
      if(ppc[i]=='c') prk[j++]=tn(3,m);
      else if(ppc[i]=='b') prk[j++]=tn(1,m);
      else if(ppc[i]=='s') prk[j++]=tn(1,m);
      else if(ppc[i]=='i') prk[j++]=tn(1,m);
      else if(ppc[i]=='f') prk[j++]=tn(2,m);
      else if(ppc[i]=='d') prk[j++]=tn(2,m);
      else if(ppc[i]=='C') prk[j++]=tn(0,m);
      else if(ppc[i]=='S') prk[j++]=tn(4,m);
    }
    for(i=0;i<m;i++) {
      for(j=0,k=0;j<n(p);j++) {
        if(ppc[j]==' ') { z+=pqi[j]; continue; }
        K rk=prk[k++];
        if(ppc[j]=='c') {
          if(z+1>ze) { e=KERR_LENGTH; goto cleanup; }
          ((char*)px(rk))[i]=(char)*z;
        }
        else if(ppc[j]=='b') {
          if(z+1>ze) { e=KERR_LENGTH; goto cleanup; }
          ((i32*)px(rk))[i]=(char)*z;
        }
        else if(ppc[j]=='s') {
          if(z+sizeof(short)>ze) { e=KERR_LENGTH; goto cleanup; }
          short tmp; memcpy(&tmp, z, sizeof(tmp)); ((i32*)px(rk))[i] = (i32)tmp;
        }
        else if(ppc[j]=='i') {
          if(z+sizeof(i32)>ze) { e=KERR_LENGTH; goto cleanup; }
          i32 tmp; memcpy(&tmp, z, sizeof(tmp)); ((i32*)px(rk))[i] = tmp;
        }
        else if(ppc[j]=='f') {
          if(z+sizeof(float)>ze) { e=KERR_LENGTH; goto cleanup; }
          float tmp; memcpy(&tmp, z, sizeof(tmp)); ((double*)px(rk))[i] = tmp;
        }
        else if(ppc[j]=='d') {
          if(z+sizeof(double)>ze) { e=KERR_LENGTH; goto cleanup; }
          double tmp; memcpy(&tmp, z, sizeof(tmp)); ((double*)px(rk))[i] = tmp;
        }
        else if(ppc[j]=='C') {
          if(z+pqi[j]>ze) { e=KERR_LENGTH; goto cleanup; }
          char *tmp=xmalloc(pqi[j]); memcpy(tmp,z,pqi[j]); ((K*)px(rk))[i]=tnv(3,pqi[j],tmp);
        }
        else if(ppc[j]=='S') {
          if((z+pqi[j])>=ze+1) { e=KERR_LENGTH; goto cleanup; }
          g=z[pqi[j]];
          z[pqi[j]]=0;
          pz=z+strlen(z)-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz))++pz;
          ((char**)px(rk))[i]=sp(pz);
          z[pqi[j]]=g;
        }
        z+=pqi[j];
      }
    }
    break;
  case 3:
    if(tx==0) { /* "c"1:("b3";0;48) */
      PXK;
      if(nx!=3) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[0])!=-3 && T(pxk[0])!=3 && T(pxk[0])!=4) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[1])!=1 && T(pxk[1])!=2) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[2])!=1 && T(pxk[2])!=2) { e=KERR_TYPE; goto cleanup; }
      if(T(pxk[1])==1) B=ik(pxk[1]);
      else {
        double d=fk(pxk[1]);
        if(!isfinite(d) || d<0 || d>(double)SIZE_MAX) { e=KERR_DOMAIN; goto cleanup; }
        B=d;
      }
      if(T(pxk[2])==1) N=ik(pxk[2]);
      else {
        double d=fk(pxk[2]);
        if(!isfinite(d) || d<0 || d>(double)SIZE_MAX) { e=KERR_DOMAIN; goto cleanup; }
        N=d;
      }
      ff=pxk[0];
    }
    else ff=x;  /* "c"1:"b3" */
    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    if(tx!=0) { B=0; N=NN; }
    //if(B+N>(size_t)NN) { e=KERR_LENGTH; goto cleanup; }
    if(N>NN || B>NN-N) { e = KERR_LENGTH; goto cleanup; }
    fseek(fp,B,SEEK_SET);
    if(ck(a)=='c') {
      VSIZE((i64)N);
      r=tn(3,N);
    }
    else if(ck(a)=='i') {
      if(N%sizeof(i32)) { e=KERR_LENGTH; goto cleanup; }
      else { VSIZE((i64)(N/sizeof(int))); r=tn(1,N/sizeof(int)); }
    }
    else if(ck(a)=='d') {
      if(N%sizeof(double)) { e=KERR_LENGTH; goto cleanup; }
      else { VSIZE((i64)(N/sizeof(double))); r=tn(2,N/sizeof(double)); }
    }
    else { e=KERR_DOMAIN; goto cleanup; }
    if(fread((void*)px(r),1,N,fp)!=N) {
      if(ferror(fp)) e=kerror("io");
      else e=KERR_LENGTH;
      goto cleanup;
    }
    break;
  default: r=KERR_TYPE;
  }
  if(fp) fclose(fp);
  xfree(z0);
  return r;
cleanup:
  if(fp) fclose(fp);
  xfree(z0);
  _k(r);
  return e;
}

K onecolon(K a, K x) {
  K r;
  if(a) r=onecolon2(a,x);
  else r=onecolon1(x);
  return r;
}

static K twocolon1(K x) {
  K r=0,e,p=0;
  FILE *fp;
  char *b;
  size_t n,bs=0;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if(tx!=-3&&tx!=4) return KERR_TYPE;
  EC(fopen_(x,"rb",&fp));
  EC(fsize_(x,&bs));
  VSIZE((i64)bs);
  b=xmalloc(bs);
  if((n=fread(b,1,bs,fp))!=bs) {
    int ioe=ferror(fp);
    fclose(fp);
    xfree(b);
    if(ioe) return kerror("io");
    return KERR_LENGTH;
  }
  fclose(fp);
  if(b[0]!=2) { xfree(b); return kerror("header"); }
  p=tnv(3,n,b);
  r=db_(p);
  _k(p);
  return r;
cleanup:
  _k(r);
  return e;
}

static K twocolon2(K a, K x) {
  (void)a; (void)x;
  return null;
}

K twocolon(K a, K x) {
  K r;
  if(a) r=twocolon2(a,x);
  else r=twocolon1(x);
  return r;
}

static K fivecolon1(K x) {
  K r;
  char *prc;
  mreset();
  const char *s=kprint_(x,"","","");
  PRC(strlen(s));
  i(n(r),prc[i]=s[i])
  return r;
}

static K fivecolon2(K a, K x) {
  K r=0,e,p=0;
  FILE *fp;
  int t,st,pad;
  char h[4];
  size_t n,c=0;
  if(ta!=-3&&ta!=4) return KERR_TYPE;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  if(tx>0||s(x)) return KERR_TYPE;
  EC(fopen_(a,"ab+",&fp));
  fseek(fp,0,SEEK_END);
  if(ftell(fp)) {                /* was file opened or created? */
    fseek(fp,0,SEEK_SET);
    if(fread(&h,1,4,fp)!=4) {
      int ioe=ferror(fp);
      fclose(fp);
      if(ioe) return ferr("",errno);
      return KERR_LENGTH; 
    }
    if(h[0]!=2) { fclose(fp); return kerror("header"); }
    if(fread(&t,sizeof(int),1,fp)!=1) {
      int ioe=ferror(fp);
      fclose(fp);
      if(ioe) return ferr("",errno);
      return KERR_LENGTH; 
    }
    if(t>0) { fclose(fp); return KERR_TYPE; }
    if(t!=tx) { fclose(fp); return KERR_TYPE; }
    if(fread(&st,sizeof(int),1,fp)!=1) {
      int ioe=ferror(fp);
      fclose(fp);
      if(ioe) return ferr("",errno);
      return KERR_LENGTH; 
    }
    if(fread(&pad,sizeof(int),1,fp)!=1) {
      int ioe=ferror(fp);
      fclose(fp);
      if(ioe) return ferr("",errno);
      return KERR_LENGTH; 
    }
    if(fread(&c,sizeof(u64),1,fp)!=1) {
      int ioe=ferror(fp);
      fclose(fp);
      if(ioe) return ferr("",errno);
      return KERR_LENGTH; 
    }
    VSIZE((i64)c);
    p=bd_(x);
    if(E(p)) { fclose(fp); e=p; goto cleanup; }
    fseek(fp,0,SEEK_END);
    fwrite(24+(char*)px(p),1,n(p)-24,fp); /* p skip header,type,subtype,pad,count */
    fclose(fp);                  /* close and reopen to edit count */
    EC(fopen_(a,"rb+",&fp));
    fseek(fp,sizeof(int)*4,SEEK_SET); /* skip header,type,subtype,pad */
    n=c+((tx>0||s(x))?1:nx);
    fwrite(&n,sizeof(u64),1,fp); /* update count */
    fclose(fp);
    r=t(1,n);
  }
  else {
    p=bd_(x);
    if(E(p)) { fclose(fp); e=p; goto cleanup; }
    fwrite((char*)px(p),1,n(p),fp);
    fclose(fp);
    r=null;
  }
  _k(p);
  return r;
cleanup:
  _k(p);
  _k(r);
  return e;
}

K fivecolon(K a, K x) {
  K r;
  if(a) r=fivecolon2(a,x);
  else r=fivecolon1(x);
  return r;
}

static K sixcolon1(K x) {
  K r=0,e;
  FILE *fp;
  char *b;
  size_t n=0,bs=0;
  if(tx!=-3&&tx!=4) return KERR_TYPE;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  EC(fopen_(x,"rb",&fp));
  EC(fsize_(x,&bs));
  b=xmalloc(bs);
  VSIZE((i64)bs);
  if((n=fread(b,1,bs,fp))!=bs) {
    int ioe=ferror(fp);
    fclose(fp);
    xfree(b);
    if(ioe) return kerror("io");
    return KERR_LENGTH;
  }
  fclose(fp);
  r=tnv(3,n,b);
  return r;
cleanup:
  _k(r);
  return e;
}

static K sixcolon2(K a, K x) {
  K *pak,e;
  char *pxc;
  FILE *fp;
  if(ta!=-3&&ta!=4&&ta!=0) return KERR_TYPE;
  if(tx!=-3) return KERR_TYPE;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  if(!ta) { /* append */
    PAK;
    if(!na||(T(pak[0])!=-3&&T(pak[0])!=4)) return KERR_TYPE;
    EC(fopen_(pak[0],"a",&fp));
  }
  else EC(fopen_(a,"wb",&fp));
  PXC;
  i(nx,fputc(pxc[i],fp));
  fclose(fp);
  return null;
cleanup:
  return e;
}

K sixcolon(K a, K x) {
  K r;
  if(a) r=sixcolon2(a,x);
  else r=sixcolon1(x);
  return r;
}

#ifdef FUZZING
K del_(K x) {
  K r=null;
  char *s,pathbuf[1024];
  if(tx==4) s=sk(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if(join_sandbox(pathbuf, sizeof(pathbuf), s) < 0) r=KERR_TYPE;
  else if((remove(pathbuf)==-1)) r=ferr(s,errno);
  return r;
}
#else
K del_(K x) {
  K r=null;
  char *s;
  if(tx==4) s=sk(x);
  else if(tx==-3) s=px(x);
  else return KERR_TYPE;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if((remove(s)==-1)) r=ferr(s,errno);
  return r;
}
#endif

#ifdef FUZZING
K rename_(K a, K x) {
  K r=null;
  char *p=0,*q=0,pathbufp[1024],pathbufq[1024];
  if(ta==4) p=sk(a);
  else if(ta==-3) p=px(a);
  else return KERR_TYPE;
  if(tx==4) q=sk(x);
  else if(tx==-3) q=px(x);
  else return KERR_TYPE;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if(join_sandbox(pathbufp, sizeof(pathbufp), p) < 0) r=KERR_TYPE;
  else if(join_sandbox(pathbufq, sizeof(pathbufq), q) < 0) r=KERR_TYPE;
  else if((rename(pathbufp,pathbufq)==-1)) {
#ifdef _WIN32
    char b[256]; strerror_s(b,sizeof(b),errno);
    r=kerror(b);
#else
    r=kerror(strerror(errno));
#endif
  }
  return r;
}
#else
K rename_(K a, K x) {
  K r=null;
  char *p,*q;
  if(ta==4) p=sk(a);
  else if(ta==-3) p=px(a);
  else return KERR_TYPE;
  if(tx==4) q=sk(x);
  else if(tx==-3) q=px(x);
  else return KERR_TYPE;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  if((rename(p,q)==-1)) {
#ifdef _WIN32
    char b[256]; strerror_s(b,sizeof(b),errno);
    r=kerror(b);
#else
    r=kerror(strerror(errno));
#endif
  }
  return r;
}
#endif

#ifdef _WIN32
static char* read_(HANDLE f) {
  size_t m=32,n=0;
  char buf[1024],*b;
  DWORD len;
  BOOL res;

  b=xmalloc(m);
  while(1) {
    res=ReadFile(f,buf,1024,&len,0);
    if(!res||!len) break;
    while(m<n+len) { m<<=1; b=xrealloc(b,m); }
    memcpy(b+n,buf,len);
    n+=len;
  }
  b[n]=0;
  return b;
}

static K b3colon(K x) {
#ifdef FUZZING
  return null;
#else
  char *cmd;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;

  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  cmd=xmalloc(nx+8);
  sprintf(cmd,"cmd /c ");
  size_t len = strlen(cmd);
  snprintf(cmd+len, nx+8-len, "%s", (char*)px(x));

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  xfree(cmd);
  if(!res) return KERR_DOMAIN;

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return null;
#endif
}

static K b4colon(K x) {
  K r;
#ifdef FUZZING
  K *prk;
  char *s="fuzz_sandbox_stubbed";
  PRK(1);
  prk[0]=tnv(3,strlen(s),xmemdup(s,1+strlen(s)));
  return r;
#else
  char *cmd,*buf1,*buf2;
  DWORD status;
  HANDLE out[2],err[2];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;
  SECURITY_ATTRIBUTES sa;

  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;

  // Set the bInheritHandle flag so pipe handles are inherited.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = 0;

  if(!CreatePipe(&out[0], &out[1], &sa, 0)) return KERR_DOMAIN;
  if(!SetHandleInformation(out[0], HANDLE_FLAG_INHERIT, 0)) return KERR_DOMAIN;
  if(!CreatePipe(&err[0], &err[1], &sa, 0)) return KERR_DOMAIN;
  if(!SetHandleInformation(err[0], HANDLE_FLAG_INHERIT, 0)) return KERR_DOMAIN;

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdOutput = out[1];
  si.hStdError = err[1];
  si.dwFlags |= STARTF_USESTDHANDLES;

  cmd=xmalloc(nx+8);
  sprintf(cmd,"cmd /c ");
  size_t len = strlen(cmd);
  snprintf(cmd+len, nx+8-len, "%s", (char*)px(x));

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  xfree(cmd);
  if(!res) return KERR_DOMAIN;

  CloseHandle(out[1]);
  CloseHandle(err[1]);
  buf1=read_(out[0]);
  buf2=read_(err[0]);
  fprintf(stderr,"%s",buf2);
  //WaitForSingleObject(pi.hProcess,INFINITE);
  //WaitForSingleObject doesn't work for "cmd1 & cmd2 & cmd3"
  //should be safe to assume process has exited since read_ doesn't return
  //until stdout/stderr is closed
  if(!GetExitCodeProcess(pi.hProcess,&status)) return KERR_DOMAIN;
  if(status) return KERR_DOMAIN;
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  r=ksplit(buf1,"\r\n");
  xfree(buf1);
  return r;
#endif
}

#define BUFSIZE 1024
static K b8colon(K x) {
  K r,*prk;
#ifdef FUZZING
  char *s="fuzz_sandbox_stubbed";
  PRK(3);
  prk[0]=t(1,0);
  prk[1]=tn(0,1); ((K*)px(prk[1]))[0]=tnv(3,strlen(s),xmemdup(s,1+strlen(s)));
  prk[2]=tn(0,0);
  return r;
#else
  char *cmd,*buf1,*buf2;
  DWORD status;
  HANDLE out[2],err[2];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;
  SECURITY_ATTRIBUTES sa;

  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;

  // set the bInheritHandle flag so pipe handles are inherited.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = 0;

  if(!CreatePipe(&out[0], &out[1], &sa, 0)) return KERR_DOMAIN;
  if(!CreatePipe(&err[0], &err[1], &sa, 0)) return KERR_DOMAIN;
  if(!SetHandleInformation(out[0], HANDLE_FLAG_INHERIT, 0)) return KERR_DOMAIN;
  if(!SetHandleInformation(err[0], HANDLE_FLAG_INHERIT, 0)) return KERR_DOMAIN;

  ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
  ZeroMemory( &si, sizeof(STARTUPINFO) );
  si.cb = sizeof(STARTUPINFO);
  si.hStdError = err[1];
  si.hStdOutput = out[1];
  si.dwFlags |= STARTF_USESTDHANDLES;

  cmd=xmalloc(nx+8);
  sprintf(cmd,"cmd /c ");
  size_t len = strlen(cmd);
  snprintf(cmd+len, nx+8-len, "%s", (char*)px(x));

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  if(!res) return KERR_DOMAIN;

  // close handles to the stdout pipe no longer needed by the child process
  CloseHandle(out[1]);
  CloseHandle(err[1]);

  buf1=read_(out[0]);
  buf2=read_(err[0]);

  // close handles to the child process and its primary thread
  WaitForSingleObject(pi.hProcess,INFINITE);
  if(!GetExitCodeProcess(pi.hProcess, &status)) { status = (DWORD)-1; }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  xfree(cmd);
  PRK(3);
  prk[0]=t(1,(u32)status);
  prk[1]=ksplit(buf1,"\r\n");
  prk[2]=ksplit(buf2,"\r\n");
  xfree(buf1);
  xfree(buf2);
  return r;
#endif
}
#else
static char* read_(int f) {
  size_t m=32,n=0;
  char buf[1024],*b;
  ssize_t len;
  b=xmalloc(m);
  while((len=read(f,buf,1024))) {
    while(m<n+len) { m<<=1; b=xrealloc(b,m); }
    memcpy(b+n,buf,len);
    n+=len;
  }
  b[n]=0;
  return b;
}

static K b3colon(K x) {
#ifdef FUZZING
  return null;
#else
  char *argv[4];
  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup((char*)px(x),nx);
  argv[3]=0;
  if(!fork()) execvp(argv[0],argv);
  xfree(argv[2]);
  return null;
#endif
}

static K b4colon(K x) {
  K r;
#ifdef FUZZING
  K *prk;
  char *s="fuzz_sandbox_stubbed";
  PRK(1);
  prk[0]=tnv(3,strlen(s),xmemdup(s,1+strlen(s)));
  return r;
#else
  pid_t p;
  char *argv[4],*buf;
  int out[2],status;
  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;
  if(pipe(out)) return kerror("pipe");
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup(px(x),nx);
  argv[3]=0;
  if(!(p=fork())) { /* child */
    close(out[0]);
    dup2(out[1],1);
    close(out[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  close(out[1]);
  buf=read_(out[0]);
  close(out[0]);
  xfree(argv[2]);
  r=status?KERR_DOMAIN:ksplit(buf,"\r\n");
  xfree(buf);
  return r;
#endif
}

static K b8colon(K x) {
  K r,*prk;
#ifdef FUZZING
  char *s="fuzz_sandbox_stubbed";
  PRK(3);
  prk[0]=t(1,0);
  prk[1]=tn(0,1); ((K*)px(prk[1]))[0]=tnv(3,strlen(s),xmemdup(s,1+strlen(s)));
  prk[2]=tn(0,0);
  return r;
#else
  pid_t p;
  char *argv[4],*buf1,*buf2;
  int out[2],err[2],status;
  if(tx!=-3) return KERR_TYPE;
  if(!nx) return KERR_DOMAIN;
  if(pipe(out)) return kerror("pipe");
  if(pipe(err)) return kerror("pipe");
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup((char*)px(x),nx);
  argv[3]=0;
  if(!(p=fork())) { /* child */
    close(out[0]); close(err[0]);
    dup2(out[1],1); dup2(err[1],2);
    close(out[1]); close(err[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  if(WIFEXITED(status)) status = WEXITSTATUS(status);
  else if(WIFSIGNALED(status)) status = 128 + WTERMSIG(status);
  else status = -1;
  close(out[1]); close(err[1]);
  buf1=read_(out[0]);
  buf2=read_(err[0]);
  close(out[0]); close(err[0]);
  xfree(argv[2]);
  PRK(3);
  prk[0]=t(1,(u32)status);
  prk[1]=ksplit(buf1,"\r\n");
  prk[2]=ksplit(buf2,"\r\n");
  xfree(buf1);
  xfree(buf2);
  return r;
#endif
}
#endif

static K threecolon1(K x) {
  (void)x;
  return null;
}

static K threecolon2(K a, K x) {
  if((ta==4&&sp("")==sk(a)) ||(ta==-3&&sp("")==sp(px(a)))) return b3colon(x);
  return null;
}

K threecolon(K a, K x) {
  K r;
  if(a) r=threecolon2(a,x);
  else r=threecolon1(x);
  return r;
}

static K fourcolon1(K x) {
  K r=t(1,(u32)tx);
  if(ISF(x)) r=t(1,7);
  else if(0x80==s(x)) r=t(1,5);
  return r;
}

static K fourcolon2(K a, K x) {
  if((ta==4&&sp("")==sk(a)) ||(ta==-3&&sp("")==sp(px(a)))) return b4colon(x);
  return null;
}

K fourcolon(K a, K x) {
  K r;
  if(a) r=fourcolon2(a,x);
  else r=fourcolon1(x);
  return r;
}

static K eightcolon1(K x) {
  K r=t(1,(u32)tx);
  if(0xc3==s(x)||0xc4==s(x)) r=t(1,7);
  if(0x80==s(x)) r=t(1,5);
  return r;
}

static K eightcolon2(K a, K x) {
  if((ta==4&&sp("")==sk(a)) ||(ta==-3&&sp("")==sp(px(a)))) return b8colon(x);
  return null;
}

K eightcolon(K a, K x) {
  K r;
  if(a) r=eightcolon2(a,x);
  else r=eightcolon1(x);
  return r;
}
