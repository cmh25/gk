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
#include <signal.h>
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include "b.h"
#include "ms.h"
#include "ipc.h"

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

static FILE* fopen__(FILE **fp, const char *p, const char *m) {
#ifdef _WIN32
  if(fopen_s(fp, p, m)) *fp = 0;
#else
  *fp = fopen(p, m);
#endif
  return *fp;
}

/* Write the whole buffer, then flush+close, surfacing a short write or a
   deferred flush failure (typically ENOSPC -- disk full) as an io error
   instead of silently truncating the file. A single fwrite of a >2GB buffer
   is fine (glibc loops internally), so a short return is a real device
   error; with buffered stdio the error often only shows up at fclose, so
   close here and check it. *fp is always consumed (closed). Returns null on
   success, an error K otherwise. */
static K fwclose_(const void *p, size_t n, FILE *fp) {
  int err = (n && fwrite(p,1,n,fp)!=n) || ferror(fp);
  int eno = err ? errno : 0;
  if(fclose(fp) && !err) { err=1; eno=errno; }
  return err ? ferr("write",eno) : null;
}

#ifdef FUZZING
/* Under FUZZING, all filesystem-touching verbs are stubbed. Rationale: we're
 * not trying to fuzz libc fopen/fread/stat; we want to fuzz the interpreter.
 * Sandboxing real FS calls caused sporadic SEGVs across parallel afl-fuzz
 * workers racing on shared state (unlink/rename/stat interleaving), and those
 * crashes never reproduced in single-process replay. Stubbing eliminates the
 * whole race surface with negligible coverage loss. */
static K fopen_(K x, const char *m, FILE **fp) {
  (void)m;
  if(tx!=3 && tx!=-3 && tx!=4) return KERR_TYPE;
  *fp = 0;
  return KERR_DOMAIN;
}
static K fsize_(K x, size_t *bs) {
  (void)bs;
  if(tx!=3 && tx!=-3 && tx!=4) return KERR_TYPE;
  return KERR_DOMAIN;
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
  u32 i,j,n,L=0,u;   /* field-level: i,j field index, n field count, L row width */
  u64 k,m;           /* row index / row count -- must be 64-bit (>2^32 rows) */
  char *ppc,*z,*pz,*z0=0,g;
  i32 *pqi;

  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return b0colon(x);

  switch(ta) {
  case -3: case 3: case 4:  /* file write */
    switch(tx) {
    case 0:  /* mixed list of lines */
      EC(fopen_(a,"wb",&fp));
      pxk=px(x);
      /* fwrite the whole string + \n in one or two calls instead of
       * fputc per byte. Cuts the syscall count dramatically when stdout
       * is _IONBF (interactive case), which in turn reduces interleaving
       * between forked-server children sharing the same tty. */
      for(i=0;i<nx;i++) {
        if(T(pxk[i])!=-3) { fclose(fp); return KERR_TYPE; }
        fwrite(px(pxk[i]),1,n(pxk[i]),fp);
        fputc('\n',fp);
      }
      e=fwclose_(0,0,fp); fp=0;  /* flush+close; catch disk full */
      if(E(e)) return e;
      break;
    case -3:  /* single string */
      EC(fopen_(a,"wb",&fp));
      fwrite(px(x),1,nx,fp);
      fputc('\n',fp);
      e=fwclose_(0,0,fp); fp=0;  /* flush+close; catch disk full */
      if(E(e)) return e;
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
    i(n(p),if(!strchr("IJEFCS ",ppc[i])) return KERR_TYPE)

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
    m=0;
    for(char *zr=z0; zr<z0+N;) {
      if(*zr=='\n') { ++zr; continue; }
      char *nl=memchr(zr,'\n',(z0+N)-zr);
      if(!nl) nl=z0+N;
      ++m;
      zr=(nl<z0+N)?nl+1:nl;
    }
    if(m>=(u64)VMAX) { e=KERR_WSFULL; goto cleanup; } /* row count -> column length */
    j=0;
    r=tn(0,n); prk=px(r);
    for(i=0;i<n(p);i++) {
      if(ppc[i]=='I') prk[j++] = tn(1,m);
      else if(ppc[i]=='F') prk[j++] = tn(2,m);
      else if(ppc[i]=='J') prk[j++] = tn(8,m);
      else if(ppc[i]=='E') prk[j++] = tn(9,m);
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
        else if(ppc[i]=='J') {
          pz=z+pqi[i]-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz)) ++pz;
          i64 *pj=px(prk[j]);
          pj[k]=xatol(pz);
          j++;
        }
        else if(ppc[i]=='E') {
          pz=z+pqi[i]-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz)) ++pz;
          float *pe=px(prk[j]);
          pe[k]=(float)xstrtod(pz);
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
  if(h[0]!=3 && h[0]!=2) { close(fd); return kerror("header"); } /* v3 + v1's v2 (additive) */
  n=read(fd,&t,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&st,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&pad,sizeof(i32)); if(n==-1) return ferr(s,errno);
  n=read(fd,&c,sizeof(u64)); if(n==-1) return ferr(s,errno);
  if(t!=-1&&t!=-2&&t!=-3) { close(fd); return twocolon1(x); }
  if(t==-1) len=c*sizeof(int);
  else if(t==-2) len=c*sizeof(double);
  else if(t==-3) len=c*sizeof(char);
  else return KERR_TYPE;
  /* the file is [24-byte header][len data]; the vector points at v+24, so the
     mapping must span header+data (24+len). Mapping only `len` left the tail
     24 bytes of data unmapped whenever len was page-aligned (big vectors) -> SEGV.
     __k frees this via munmap(k->v-24, 24+len) -- keep the two in lockstep. */
  v=mmap(0,24+len,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
  if(v==(void*)-1) { r=ferr(s,errno); return r; }
  r=tnv(-t,c,24+(char*)v);
  ((ko*)(b(48)&r))->m=1;
  n(r)=c;
  close(fd);
  return r;
}
#endif

static K onecolon2(K a, K x) {
  K r=null,e,p,q,ff,*prk,*pak,*pxk;
  FILE *fp=0;
  size_t B=0,N=0,NN=0;
  u32 j,k,n,L=0;   /* field-level: j,k field index, n field count, L row width */
  u64 i,m;         /* row index / row count -- must be 64-bit (>2^32 rows) */
  char *z,*pz,*z0=0,*ze,g;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  switch(ta) {
  case -3: case 4:
    if(ta!=-3&&ta!=4) { e=KERR_TYPE; goto cleanup; }
    EC(fopen_(a,"wb",&fp));
    r=bd_(x); EC(r);
    if(E(r)) { fclose(fp); e=r; goto cleanup; }
    e=fwclose_((void*)px(r),n(r),fp); fp=0;  /* consumes fp; catches disk full */
    _k(r); r=null;
    if(E(e)) goto cleanup;
    break;
  case 0:
    if(na!=2) { e=KERR_LENGTH; goto cleanup; }
    PAK;
    if(T(pak[0])!=-3) { e=KERR_TYPE; goto cleanup; }
    if(T(pak[1])!=-1) { e=KERR_TYPE; goto cleanup; }
    if(n(pak[0])!=n(pak[1])) { e=KERR_LENGTH; goto cleanup; }
    p=pak[0]; q=pak[1];
    char *ppc=px(p); int *pqi=px(q);
    i(n(p),if(!strchr("cbsijef CS",ppc[i])) { e=KERR_TYPE; goto cleanup; })
    n=0;i(n(p),if(ppc[i]!=' ')++n)
    VSIZE((i32)n);
    i(n(q),L+=pqi[i])

    for(u64 t= 0; t< n(p); ++t) {
      int w = pqi[t];
      switch (ppc[t]) {
      case 'c': case 'b': if(w!=1) { e = KERR_LENGTH; goto cleanup; } break;
      case 's': if(w!=2) { e = KERR_LENGTH; goto cleanup; } break;
      case 'i': case 'e': if(w!=4) { e = KERR_LENGTH; goto cleanup; } break;
      case 'j': case 'f': if(w!=8) { e = KERR_LENGTH; goto cleanup; } break;
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
    if(m>=(u64)VMAX) { e=KERR_WSFULL; goto cleanup; } /* row count -> column length */
    PRK(n);
    // ("cid S";1 4 8 4 5)1:("b2";22;66)
    for(i=0,j=0;i<n(p);i++) {
      if(ppc[i]=='c') prk[j++]=tn(3,m);
      else if(ppc[i]=='b') prk[j++]=tn(1,m);
      else if(ppc[i]=='s') prk[j++]=tn(1,m);
      else if(ppc[i]=='i') prk[j++]=tn(1,m);
      else if(ppc[i]=='j') prk[j++]=tn(8,m);
      else if(ppc[i]=='e') prk[j++]=tn(9,m);
      else if(ppc[i]=='f') prk[j++]=tn(2,m);
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
        else if(ppc[j]=='j') {
          if(z+sizeof(i64)>ze) { e=KERR_LENGTH; goto cleanup; }
          i64 tmp; memcpy(&tmp, z, sizeof(tmp)); ((i64*)px(rk))[i] = tmp;
        }
        else if(ppc[j]=='e') {
          if(z+sizeof(float)>ze) { e=KERR_LENGTH; goto cleanup; }
          float tmp; memcpy(&tmp, z, sizeof(tmp)); ((float*)px(rk))[i] = tmp;
        }
        else if(ppc[j]=='f') {
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
    else if(ck(a)=='j') {
      if(N%sizeof(i64)) { e=KERR_LENGTH; goto cleanup; }
      else { VSIZE((i64)(N/sizeof(i64))); r=tn(8,N/sizeof(i64)); }
    }
    else if(ck(a)=='e') {
      if(N%sizeof(float)) { e=KERR_LENGTH; goto cleanup; }
      else { VSIZE((i64)(N/sizeof(float))); r=tn(9,N/sizeof(float)); }
    }
    else if(ck(a)=='f') {
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
  if(b[0]!=3 && b[0]!=2) { xfree(b); return kerror("header"); } /* v3 + v1's v2 (additive) */
  p=tnv(3,n,b);
  r=db_(p);
  _k(p);
  return r;
cleanup:
  _k(r);
  return e;
}

/* Maximum valence of a 2:-linked external function.  linkcall() dispatches
   through a fixed switch (no libffi), so this bounds the arity it can call. */
#define LINK_MAXV 8

/* One function-pointer type per supported arity.  linkcall() memcpy's dlsym's
   void* into the right one and calls it -- memcpy is the one void*->function
   conversion ISO C allows, and using the exact type means no function-pointer
   cast (so neither -Wpedantic nor -Wcast-function-type fires). */
typedef K (*kf0)(void);
typedef K (*kf1)(K);
typedef K (*kf2)(K,K);
typedef K (*kf3)(K,K,K);
typedef K (*kf4)(K,K,K,K);
typedef K (*kf5)(K,K,K,K,K);
typedef K (*kf6)(K,K,K,K,K,K);
typedef K (*kf7)(K,K,K,K,K,K,K);
typedef K (*kf8)(K,K,K,K,K,K,K,K);

/* Pull a NUL-terminated C string out of a char-vector (-3), symbol (4), or
   char atom (3).  Returns a malloc'd copy the caller must xfree, or 0 on a
   type mismatch. */
static char* kcstr(K x) {
  if(T(x)==4) return xstrdup(sk(x));
  if(T(x)==-3) { u64 n=nx; char *s=xmalloc(n+1); memcpy(s,px(x),n); s[n]=0; return s; }
  if(T(x)==3) { char *s=xmalloc(2); s[0]=(char)ck(x); s[1]=0; return s; }
  return 0;
}

#ifndef FUZZING
/* Open a shared object by exact path; returns an opaque handle, or 0. */
static void *linkdlopen(const char *p) {
#ifdef _WIN32
  return (void*)LoadLibraryA(p);
#else
  return dlopen(p,RTLD_NOW|RTLD_LOCAL); /* LOCAL: the .so still resolves gk's
    exported gk_* ABI, but its own symbols stay private (no collisions) */
#endif
}
#endif

/* f 2:(e;t) -- Link Object Code. */
static K twocolon2(K a, K x) {
#ifdef FUZZING
  (void)a; (void)x;
  return KERR_DOMAIN; /* no dlopen under the fuzzer (see fopen_ rationale) */
#else
  K r,*pxk,ke,kt;
  char *fn,*en;
  i32 val;
  void *h,*fp;
  if(ta!=-3&&ta!=4&&ta!=3) return KERR_TYPE;
  if(T(x)!=0||s(x)||nx!=2) return KERR_TYPE; /* (e;t) */
  pxk=px(x); ke=pxk[0]; kt=pxk[1];
  if(T(kt)!=1) return KERR_TYPE;
  val=ik(kt);
  if(val<0||val>LINK_MAXV) return KERR_DOMAIN;
  fn=kcstr(a);  if(!fn) return KERR_TYPE;
  en=kcstr(ke); if(!en) { xfree(fn); return KERR_TYPE; }
  /* Resolve the library. A bare name with no path separator (e.g. "mylib.so")
     is, by a script's intent, the .so in the working directory -- but dlopen()/
     LoadLibrary search the system library path, NOT the cwd. So try "./<name>"
     first, then fall back to the bare name so a genuine system library (e.g.
     "libm.so.6") still resolves. A name that already carries a separator
     (relative-with-dir or absolute) is used verbatim. */
  h=0;
#ifdef _WIN32
  if(!strpbrk(fn,"/\\")) {
#else
  if(!strchr(fn,'/')) {
#endif
    size_t fl=strlen(fn);
    char *rel=xmalloc(fl+3);
    rel[0]='.'; rel[1]='/'; memcpy(rel+2,fn,fl+1);
    h=linkdlopen(rel);
    xfree(rel);
  }
  if(!h) h=linkdlopen(fn);
  if(!h) { xfree(fn); xfree(en); return KERR_DOMAIN; }
#ifdef _WIN32
  fp=(void*)GetProcAddress((HMODULE)h,en);
#else
  fp=dlsym(h,en);
#endif
  /* The handle is intentionally left open for the life of the process: a
     linked function can be called any time after `f 2: ...` returns, so the
     library must stay mapped.  K processes never unlink object code. */
  if(!fp) { xfree(fn); xfree(en); return KERR_DOMAIN; }
  r=tn(0,3);
  K *pf=px(r);
  pf[0]=tj((i64)(intptr_t)fp); /* function pointer */
  pf[1]=t(1,(u32)val);         /* valence */
  pf[2]=t(4,sp(en));           /* symbol name, for printing */
  r=st(0xdc,r);
  xfree(fn); xfree(en);
  return r;
#endif
}

/* Apply a 2:-linked function (subtype 0xdc).  x is a type-0 argument list
   (a 0x81 bracket plist or a plain list from `.`); each element is one
   borrowed argument.  Frees x before returning. */
K linkcall(K f, K x) {
  K r,*pf=px(f),*av;
  void *fp;
  kf0 g0;kf1 g1;kf2 g2;kf3 g3;kf4 g4;kf5 g5;kf6 g6;kf7 g7;kf8 g8;
  i32 val,argc;
  if(T(x)!=0) { _k(f); _k(x); return KERR_TYPE; }
  fp=(void*)(intptr_t)jk(pf[0]);
  val=ik(pf[1]);
  av=px(x);
  argc=(i32)nx;
  if(argc==1&&val==0&&(av[0]==null||av[0]==inull)) argc=0; /* g[] passes a lone nul */
  if(argc!=val) { _k(f); _k(x); return KERR_VALENCE; }
  switch(val) {
  case 0: memcpy(&g0,&fp,sizeof g0); r=g0(); break;
  case 1: memcpy(&g1,&fp,sizeof g1); r=g1(av[0]); break;
  case 2: memcpy(&g2,&fp,sizeof g2); r=g2(av[0],av[1]); break;
  case 3: memcpy(&g3,&fp,sizeof g3); r=g3(av[0],av[1],av[2]); break;
  case 4: memcpy(&g4,&fp,sizeof g4); r=g4(av[0],av[1],av[2],av[3]); break;
  case 5: memcpy(&g5,&fp,sizeof g5); r=g5(av[0],av[1],av[2],av[3],av[4]); break;
  case 6: memcpy(&g6,&fp,sizeof g6); r=g6(av[0],av[1],av[2],av[3],av[4],av[5]); break;
  case 7: memcpy(&g7,&fp,sizeof g7); r=g7(av[0],av[1],av[2],av[3],av[4],av[5],av[6]); break;
  case 8: memcpy(&g8,&fp,sizeof g8); r=g8(av[0],av[1],av[2],av[3],av[4],av[5],av[6],av[7]); break;
  default: _k(f); _k(x); return KERR_VALENCE;
  }
  _k(f); _k(x); /* frees the arg list and its (borrowed) elements */
  return r;
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
    if(h[0]!=3 && h[0]!=2) { fclose(fp); return kerror("header"); } /* v3 + v1's v2 (additive) */
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
    /* append data; close+check catches disk full (p skips header,type,subtype,pad,count) */
    e=fwclose_(24+(char*)px(p),n(p)-24,fp); fp=0;
    if(E(e)) goto cleanup;
    EC(fopen_(a,"rb+",&fp));
    fseek(fp,sizeof(int)*4,SEEK_SET); /* skip header,type,subtype,pad */
    n=c+((tx>0||s(x))?1:nx);
    fwrite(&n,sizeof(u64),1,fp); /* update count */
    e=fwclose_(0,0,fp); fp=0;
    if(E(e)) goto cleanup;
    r=n>(size_t)INT32_MAX?tj((i64)n):t(1,n); /* big count: long atom, not i32-truncated */
  }
  else {
    p=bd_(x);
    if(E(p)) { fclose(fp); e=p; goto cleanup; }
    e=fwclose_((char*)px(p),n(p),fp); fp=0;
    if(E(e)) goto cleanup;
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
    EC(fopen_(pak[0],"ab",&fp));   /* binary: 6: writes raw bytes; text-mode "a"
                                      would CRLF-translate \n on Windows (the
                                      fresh-write path below already uses "wb") */
  }
  else EC(fopen_(a,"wb",&fp));
  PXC;
  fwrite(pxc,1,nx,fp);
  return fwclose_(0,0,fp);  /* flush+close; catch disk full */
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
  if(tx!=4 && tx!=-3) return KERR_TYPE;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  return KERR_DOMAIN;
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
  if(ta!=4 && ta!=-3) return KERR_TYPE;
  if(tx!=4 && tx!=-3) return KERR_TYPE;
  if((ta==4&&!strlen(sk(a))) || (ta==-3&&!na)) return KERR_LENGTH;
  if((tx==4&&!strlen(sk(x))) || (tx==-3&&!nx)) return KERR_LENGTH;
  return KERR_DOMAIN;
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
  /* Block SIGCHLD across fork+waitpid so a SIGCHLD reaper installed
   * elsewhere (e.g. ipc.c's fork-mode listener) cannot race in and
   * reap our shell child before our explicit waitpid sees its status. */
  sigset_t blk, prev;
  sigemptyset(&blk); sigaddset(&blk, SIGCHLD);
  sigprocmask(SIG_BLOCK, &blk, &prev);
  if(!(p=fork())) { /* child */
    close(out[0]);
    dup2(out[1],1);
    close(out[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  sigprocmask(SIG_SETMASK, &prev, NULL);
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
  /* See b4colon for why we block SIGCHLD across fork+waitpid. */
  sigset_t blk, prev;
  sigemptyset(&blk); sigaddset(&blk, SIGCHLD);
  sigprocmask(SIG_BLOCK, &blk, &prev);
  if(!(p=fork())) { /* child */
    close(out[0]); close(err[0]);
    dup2(out[1],1); dup2(err[1],2);
    close(out[1]); close(err[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  sigprocmask(SIG_SETMASK, &prev, NULL);
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

/* `a` looks like (host;port): 2-elt mixed list of sym|chars + int atom. */
static int is_hostport(K a) {
  if(ta != 0 || n(a) != 2) return 0;
  K *pk = px(a);
  K host = pk[0], port = pk[1];
  return (T(host)==4 || T(host)==-3) && T(port)==1;
}

/* monadic 3:
 *   3:(host;port)   -> ipc_open  (returns int handle)
 *   3:w             -> ipc_close (w is an int handle from a prior open)
 *   else            -> null      (existing nop) */
static K threecolon1(K x) {
  if(tx==1) return ipc_close((int)ik(x));
  if(is_hostport(x)) return ipc_open(x);
  return null;
}

/* dyadic a 3: x
 *   ""        3: cmd -> b3colon (shell, existing)
 *   w         3: msg -> ipc_send_async (w is an int handle)
 *   (host;p)  3: msg -> ipc_open + ipc_send_async (handle stays open,
 *                       dedup'd next time; user closes via 3:h if wanted)
 *   else              -> null (existing nop) */
static K threecolon2(K a, K x) {
  if((ta==4&&sp("")==sk(a)) ||(ta==-3&&sp("")==sp(px(a)))) return b3colon(x);
  if(ta==1) return ipc_send_async((int)ik(a), x);
  if(is_hostport(a)) {
    K h = ipc_open(a);
    if(E(h)) return h;
    return ipc_send_async((int)ik(h), x);
  }
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

/* dyadic a 4: x
 *   ""        4: cmd -> b4colon (shell-with-output, existing)
 *   w         4: msg -> ipc_send_sync (blocks for response)
 *   (host;p)  4: msg -> ipc_open + ipc_send_sync (handle stays open,
 *                       dedup'd next time; user closes via 3:h if wanted)
 *   else              -> null (existing nop) */
static K fourcolon2(K a, K x) {
  if((ta==4&&sp("")==sk(a)) ||(ta==-3&&sp("")==sp(px(a)))) return b4colon(x);
  if(ta==1) return ipc_send_sync((int)ik(a), x);
  if(is_hostport(a)) {
    K h = ipc_open(a);
    if(E(h)) return h;
    return ipc_send_sync((int)ik(h), x);
  }
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
  if(0xc3==s(x)) r=t(1,7); /* 0xc4 retired in Pass 4 */
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
