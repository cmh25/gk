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
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

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
  else if(at==3) { s=xstrdup(" "); s[0]=a3; }
  else if(at==-3) s=xstrndup(v3(a),ac);
  else return kerror("type");
  *fp=fopen(s,m);
  if(!*fp) return ferr(s,errno);
  if(at==-3) xfree(s);
  return null;
}

#ifdef _WIN32
static K* fsize_(K *a, size_t *bs) {
  K *r=null;
  char *s;
  struct _stat64 t;
  if(at==4) s=a->v;
  else if(at==3) { s=xstrdup(" "); s[0]=a3; }
  else if(at==-3) s=xstrndup(v3(a),ac);
  else return kerror("type");
  if(-1==_stat64(s,&t)) r=ferr(s,errno);
  *bs=t.st_size;
  if(at==-3) xfree(s);
  return r;
}
#else
static K* fsize_(K *a, size_t *bs) {
  K *r=null;
  char *s;
  struct stat t;
  if(at==4) s=a->v;
  else if(at==3) { s=xstrdup(" "); s[0]=a3; }
  else if(at==-3) s=xstrndup(v3(a),ac);
  else return kerror("type");
  if(-1==stat(s,&t)) r=ferr(s,errno);
  *bs=t.st_size;
  if(at==-3) xfree(s);
  return r;
}
#endif

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
  K *r=null,*p,*q,*ff;
  FILE *fp=0;
  size_t B,N,NN,s;
  unsigned int i,j,k,n,m,L=0,u;
  char *z,*pz,*e,*z0,g;
  switch(at) {
  case -3: case 3: case 4:
    EC(fopen_(a,"w",&fp));
    switch(bt) {
    case  0: DO(bc,if(v0(b)[i]->t!=-3)return kerror("type"))
             DO(bc,p=v0(b)[i];DO2(p->c,fputc(v3(p)[j],fp));fputc('\n',fp)) break;
    case -3: DO(bc,fputc(v3(b)[i],fp)); fputc('\n',fp); break;
    default: return kerror("type");
    } break;
  case 0:
    if(ac!=2) return kerror("length");
    if(v0(a)[0]->t!=-3) return kerror("type");
    if(v0(a)[1]->t!=-1) return kerror("type");
    if(v0(a)[0]->c!=v0(a)[1]->c) return kerror("length");
    p=v0(a)[0]; q=v0(a)[1];
    DO(p->c,if(!strchr("IFCS ",v3(p)[i]))return kerror("type"))
    n=0;DO(p->c,if(v3(p)[i]!=' ')++n)
    DO(q->c,L+=v1(q)[i])

    if(bt==0) { /* ("IFCS";7 9 10 3)0:("a.txt";30;30) */
      if(bc!=3) return kerror("type");
      if(v0(b)[0]->t!=-3 && v0(b)[0]->t!=3 && v0(b)[0]->t!=4) return kerror("type");
      if(v0(b)[1]->t!=1 && v0(b)[1]->t!=2) return kerror("type");
      if(v0(b)[2]->t!=1 && v0(b)[2]->t!=2) return kerror("type");
      if(v0(b)[1]->t==1) B=v0(b)[1]->i; else B=v0(b)[1]->f;
      if(v0(b)[2]->t==1) N=v0(b)[2]->i; else N=v0(b)[2]->f;
      ff=v0(b)[0];
    }
    else ff=b;  /* ("IFCS";7 9 10 3)0:"a.txt" */
    r=kv0(n);

    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    if(bt!=0) { B=0; N=NN; }
    if(B+N>(size_t)NN) return kerror("length");

    z0=z=xmalloc(N);
    fseek(fp,B,SEEK_SET);
    if((s=fread(z,1,N,fp))!=N) return kerror("length");

    m=N/L;
    for(i=0,j=0;i<p->c;i++) {
      if(v3(p)[i]=='I') v0(r)[j++]=kv1(m);
      else if(v3(p)[i]=='F') v0(r)[j++]=kv2(m);
      else if(v3(p)[i]=='C') v0(r)[j++]=kv0(m);
      else if(v3(p)[i]=='S') v0(r)[j++]=kv4(m);
    }
    j=0; k=0; e=z+N-L;
    while(z<e) {
      if(*z=='\n')++z;
      z[L]=0;
      u=0;
      for(i=0,j=0;i<q->c;i++) {
        u=v1(q)[i];
        g=z[u];
        z[u]=0;
        if(v3(p)[i]=='I') {
          pz=z+strlen(z)-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz))++pz;
          v1(v0(r)[j++])[k]=xatoi(pz);
        }
        else if(v3(p)[i]=='F') {
          pz=z+strlen(z)-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz))++pz;
          v2(v0(r)[j++])[k]=xstrtod(pz);
        }
        else if(v3(p)[i]=='C') v0(v0(r)[j++])[k]=knew(-3,v1(q)[i],z,0,0,0);
        else if(v3(p)[i]=='S') {
          pz=z+strlen(z)-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz))++pz;
          v4(v0(r)[j++])[k]=sp(pz);
        }
        z[u]=g;
        z+=v1(q)[i];
      }
      ++z;
      ++k;
    }
    free(z0);
    for(i=0,j=0;i<p->c;i++) {
      if(v3(p)[i]==' ') continue;
      v0(r)[j++]->c=k;
    }
    break;
  default: return kerror("type");
  }
  if(fp) fclose(fp);
  return r;
}

K* zerocolon1_(K *a) {
  K *r=0;
  FILE *fp;
  unsigned int n,i,m=2;
  char *b,*p,*s;
  size_t bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"r",&fp));
  EC(fsize_(a,&bs));
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
  K *r=null,*p,*q,*ff;
  FILE *fp=0;
  size_t B,N,NN,s;
  unsigned int i,j,k,n,m,L=0;
  char *z,*pz,*z0,g;
  switch(at) {
  case -3: case 4:
    if(at!=-3&&at!=4) return kerror("type");
    EC(fopen_(a,"wb",&fp));
    r=bd1_(b);
    fwrite(r->v,1,r->c,fp);
    fclose(fp);
    kfree(r);
    return null;
    break;
  case 0:
    if(ac!=2) return kerror("length");
    if(v0(a)[0]->t!=-3) return kerror("type");
    if(v0(a)[1]->t!=-1) return kerror("type");
    if(v0(a)[0]->c!=v0(a)[1]->c) return kerror("length");
    p=v0(a)[0]; q=v0(a)[1];
    DO(p->c,if(!strchr("cbsifd CS",v3(p)[i]))return kerror("type"))
    n=0;DO(p->c,if(v3(p)[i]!=' ')++n)
    DO(q->c,L+=v1(q)[i])

    if(bt==0) { /* ("id";4 8)1:("b0";0;48) */
      if(bc!=3) return kerror("type");
      if(v0(b)[0]->t!=-3 && v0(b)[0]->t!=3 && v0(b)[0]->t!=4) return kerror("type");
      if(v0(b)[1]->t!=1 && v0(b)[1]->t!=2) return kerror("type");
      if(v0(b)[2]->t!=1 && v0(b)[2]->t!=2) return kerror("type");
      if(v0(b)[1]->t==1) B=v0(b)[1]->i; else B=v0(b)[1]->f;
      if(v0(b)[2]->t==1) N=v0(b)[2]->i; else N=v0(b)[2]->f;
      ff=v0(b)[0];
    }
    else ff=b;  /* ("id";4 8)1:"b0" */
    r=kv0(n);
    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    if(bt!=0) { B=0; N=NN; }
    if(B+N>(size_t)NN) return kerror("length");
    z0=z=xmalloc(1+N);
    fseek(fp,B,SEEK_SET);
    if((s=fread(z,1,N,fp))!=N) return kerror("length");
    if(N%L) return kerror("length");
    m=N/L;
    for(i=0,j=0;i<p->c;i++) {
      if(v3(p)[i]=='c') v0(r)[j++]=kv3(m);
      else if(v3(p)[i]=='b') v0(r)[j++]=kv1(m);
      else if(v3(p)[i]=='s') v0(r)[j++]=kv1(m);
      else if(v3(p)[i]=='i') v0(r)[j++]=kv1(m);
      else if(v3(p)[i]=='f') v0(r)[j++]=kv2(m);
      else if(v3(p)[i]=='d') v0(r)[j++]=kv2(m);
      else if(v3(p)[i]=='C') v0(r)[j++]=kv0(m);
      else if(v3(p)[i]=='S') v0(r)[j++]=kv4(m);
    }
    for(i=0;i<m;i++) {
      for(j=0,k=0;j<p->c;j++) {
        if(v3(p)[j]=='c') v3(v0(r)[k++])[i]=*(unsigned char*)z;
        else if(v3(p)[j]=='b') v1(v0(r)[k++])[i]=(int)*z;
        else if(v3(p)[j]=='s') v1(v0(r)[k++])[i]=(int)*(short*)z;
        else if(v3(p)[j]=='i') v1(v0(r)[k++])[i]=*(int*)z;
        else if(v3(p)[j]=='f') v2(v0(r)[k++])[i]=*(float*)z;
        else if(v3(p)[j]=='d') v2(v0(r)[k++])[i]=*(double*)z;
        else if(v3(p)[j]=='C') v0(v0(r)[k++])[i]=knew(-3,v1(q)[j],z,0,0,0);
        else if(v3(p)[j]=='S') {
          g=z[v1(q)[j]];
          z[v1(q)[j]]=0;
          pz=z+strlen(z)-1; while(pz>z && isspace(*pz)) --pz; pz[1]=0;
          pz=z; while(isspace(*pz))++pz;
          v4(v0(r)[k++])[i]=sp(pz);
          z[v1(q)[j]]=g;
        }
        z+=v1(q)[j];
      }
    }
    xfree(z0);
    break;
  case 3:
    if(bt==0) { /* "c"1:("b3";0;48) */
      if(bc!=3) return kerror("type");
      if(v0(b)[0]->t!=-3 && v0(b)[0]->t!=3 && v0(b)[0]->t!=4) return kerror("type");
      if(v0(b)[1]->t!=1 && v0(b)[1]->t!=2) return kerror("type");
      if(v0(b)[2]->t!=1 && v0(b)[2]->t!=2) return kerror("type");
      if(v0(b)[1]->t==1) B=v0(b)[1]->i; else B=v0(b)[1]->f;
      if(v0(b)[2]->t==1) N=v0(b)[2]->i; else N=v0(b)[2]->f;
      ff=v0(b)[0];
    }
    else ff=b;  /* "c"1:"b3" */
    EC(fopen_(ff,"rb",&fp));
    EC(fsize_(ff,&NN));
    if(bt!=0) { B=0; N=NN; }
    if(B+N>(size_t)NN) return kerror("length");
    fseek(fp,B,SEEK_SET);
    if(a->i=='c') r=kv3(N);
    else if(a->i=='i') { if(N%sizeof(int)) return kerror("length"); r=kv1(N/sizeof(int)); }
    else if(a->i=='d') { if(N%sizeof(double)) return kerror("length"); r=kv2(N/sizeof(double)); }
    else return kerror("domain");
    if((s=fread(r->v,1,N,fp))!=N) return kerror("length");
    break;
  default: return kerror("type");
  }
  if(fp) fclose(fp);
  return r;
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
  size_t bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"rb",&fp));
  EC(fsize_(a,&bs));
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
  size_t bs;
  if(at!=-3&&at!=4) return kerror("type");
  EC(fopen_(a,"rb",&fp));
  EC(fsize_(a,&bs));
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
