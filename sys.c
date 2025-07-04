#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
  #include "systime.h"
  #include "unistd.h"
#else
  #include <sys/time.h>
  #include <unistd.h>
#endif
#include "sys.h" /* has to be after systime.h on win32 */
#include <math.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include "av.h"
#include "x.h"
#include "ops.h"
#include "mc.h"
#include "fn.h"
#include "sym.h"
#include "rand.h"
#include "md5.h"
#include "sha1.h"
#include "sha2.h"
#include "aes256.h"
#include "lzw.h"

K* sleep1_(K *a) {
  #ifdef _WIN32
  Sleep(a1);
  #else
  usleep(a1*1000);
  #endif
  return null;
}

K* exit1_(K *a) {
  exit(a->i);
}

K* t0(void) { /* ts */
  return k1(time(0)-2051222400l); /* 2035-1970 */
}

K* tt0(void) { /* TS */
  struct timeval tv;
  gettimeofday(&tv,0);
  double d = tv.tv_sec - 2051222400l; /* 2035-1970 */
  return k2(d/60/60/24);
}

static K* vs2__(K *a, K *b, unsigned int w) {
  K *r=0,*q=0,*p=0,*f=0;
  int i,m;
  uint64_t x,y;

  switch(at) {
  case 1:
    if(a1<=1) return kerror("domain");
    switch(bt) {
    case  1:
      if(b1==0) return kv1(0);
      //if(b1<=0) return kerror("domain");
      x=(unsigned int)b1; y=a1; i=w;
      r=kv1(w);
      while(x>0) { m=x%y; x/=y; v1(r)[--i]=m; }
      while(--i>=0) v1(r)[i]=0;
      break;
    case  0:
      q=kv0(bc);
      DO(q->c, v0(q)[i]=vs2__(a,v0(b)[i],w); EC(v0(q)[i]))
      r=flip_(q);
      kfree(q);
      break;
    case -1:
      q=kv0(bc);
      DO(q->c, p=k1(v1(b)[i]); v0(q)[i]=vs2__(a,p,w); kfree(p); EC(v0(q)[i]))
      r=flip_(q);
      kfree(q);
      break;
    default: return kerror("type");
    } break;
  case -1:
    //DO(ac,if(v1(a)[i]<0) return kerror("domain"))
    //DO(ac,if(i&&v1(a)[i]<1) return kerror("domain"))
    switch(bt) {
    case  1:
      q=reverse_(a);
      f=knew(7,0,fnnew("*"),'*',0,0);
      p=avdom(f,q,"\\"); kfree(q);
      kfree(f);
      q=reverse_(p); kfree(p);
      DO(q->c-1,v1(q)[i]=v1(q)[i+1]) v1(q)[q->c-1]=1;
      x=(unsigned int)b1; i=0; r=kv1(q->c);
      DO(q->c,y=v1(q)[i];m=x/y;x-=m*y;v1(r)[i]=v1(a)[i]?m%v1(a)[i]:m)
      kfree(q);
      break;
    case  0:
      q=kv0(bc);
      DO(q->c, v0(q)[i]=vs2__(a,v0(b)[i],w); EC(v0(q)[i]))
      r=flip_(q); kfree(q);
      break;
    case -1:
      q=kv0(bc);
      DO(q->c, p=k1(v1(b)[i]); v0(q)[i]=vs2__(a,p,w); EC(v0(q)[i]); kfree(p))
      r=flip_(q); kfree(q);
      break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }

  return r->t ? r : knorm(r);
}
static int maxw(K *a, uint64_t b) {
  int w=0,z;
  uint64_t n;
  K *p;
  if(at==1) {
    if(b==1) return b;
    w=1; n=(unsigned int)a1; while(b&&n>=b) { n/=b; ++w; }
  }
  else if(at==-1) { DO(a->c,p=k1(v1(a)[i]);z=maxw(p,b);kfree(p);if(w<z)w=z) }
  else if(at==0) { DO(a->c,z=maxw(v0(a)[i],b);if(w<z)w=z) }
  return w;
}
K * vs2_(K *a, K *b) {
  int w,z;
  if((at==1&&a1<=1)) return kerror("domain");
  if(at==-1) {
    if(!ac) return kerror("domain");
    DO(ac,if(v1(a)[i]<0) return kerror("domain"))
    DO(ac,if(i&&v1(a)[i]<1) return kerror("domain"))
    DO(ac,if(v1(a)[i]==INT_MAX||v1(a)[i]==INT_MIN||v1(a)[i]==INT_MIN+1) return kerror("domain"))
  }
  if(at==1) w=maxw(b,a1);
  else if(at==-1) { w=0; DO(ac,z=maxw(b,v1(a)[i]);if(w<z)w=z) }
  else return kerror("type");
  return vs2__(a,b,w);
}
MC2A(vs2_)

K* sv2_(K *a, K *b) {
  K *r=0,*q=0,*p=0,*f=0;
  unsigned int i,j;
  double jf;

  switch(at) {
  case 1:
    switch(bt) {
    case  1: r=kref(b); break;
    case  0:
      p=flip_(b);
      r=kv0(p->c);
      DO(rc, v0(r)[i]=sv2_(a,v0(p)[i]); EC(v0(r)[i]))
      kfree(p);
      break;
    case -1:
      r=k1(0);
      j=bc?v1(b)[0]:0;
      for(i=1;i<bc;i++) j=j*a1+v1(b)[i];
      r->i=j;
      break;
    case -2:
      r=k2(0);
      jf=bc?v2(b)[0]:0;
      for(i=1;i<bc;i++) jf=jf*a1+v2(b)[i];
      r->f=jf;
      break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1:
      q=reverse_(a);
      f=knew(7,0,fnnew("*"),'*',0,0);
      p=avdom(f,q,"\\"); kfree(q);
      kfree(f);
      p->c--;
      q=reverse_(p); kfree(p);
      r=k1(0); r->i=0;
      for(i=0;i<q->c;i++) r->i+=v1(q)[i]*b1;
      r->i+=b1;
      kfree(q);
      break;
    case  0:
      p=flip_(b);
      r=kv0(p->c);
      DO(rc, v0(r)[i]=sv2_(a,v0(p)[i]); EC(v0(r)[i]))
      kfree(p);
      break;
    case -1:
      if(ac!=bc) return kerror("length");
      q=reverse_(a);
      f=knew(7,0,fnnew("*"),'*',0,0);
      p=avdom(f,q,"\\"); kfree(q);
      kfree(f);
      p->c--;
      q=reverse_(p); kfree(p);
      r=k1(0); r->i=0;
      for(i=0;i<q->c;i++) r->i+=v1(q)[i]*v1(b)[i];
      r->i+=v1(b)[i];
      kfree(q);
      break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }

  return r->t ? r : knorm(r);
}
MC2A(sv2_)

K* draw2_(K *a, K *b) {
  K *r=0,*p=0,*q=0,*f=0;

  if(at && at!=1 && at!=-1) return kerror("type");
  if(at==1 && bt==1 && b1<0 && a1>abs(b1)) return kerror("length");

  switch(at) {
  case 0:
    if(ac) return kerror("type");
    switch(bt) {
    case 1:
      if(b1>0) { p=kv1(1); drawi(p,b1); r=k1(v1(p)[0]); kfree(p); }
      else if(b1<0) { p=kv1(1); deal(p,abs(b1)); r=k1(v1(p)[0]); kfree(p); }
      else { p=kv2(1); drawf(p,1.0); r=k2(v2(p)[0]); kfree(p); }
      break;
    case 2: p=kv2(1); drawf(p,b2); r=k2(v2(p)[0]); kfree(p); break;
    default: return kerror("type");
    } break;
  case 1:
    VSIZE(a1);
    if(a1<0) return kerror("wsfull");
    switch(bt) {
    case 1:
      if(b1>0) { r=kv1(a1); drawi(r,b1); }
      else if(b1<0) { r=kv1(a1); deal(r,abs(b1)); }
      else { r=kv2(a1); drawf(r,1.0); }
      break;
    case 2: r=kv2(a1); drawf(r,b2); break;
    default: return kerror("type");
    } break;
  case -1:
    DO(ac,VSIZE(v1(a)[i]))
    DO(ac,if(v1(a)[i]<0) return kerror("wsfull"))
    switch(bt) {
    case 1:
      f=knew(7,0,fnnew("*"),'*',0,0);
      p=avdom(f,a,"/"); kfree(f);
      if(b1<0 && p->i>abs(b1)) return kerror("length");
      q=b1?kv1(p->i):kv2(p->i); kfree(p);
      if(b1<0) deal(q,abs(b1));
      else if(b1>0) drawi(q,b1);
      else drawf(q,1.0);
      r=take2_(a,q); kfree(q);
      break;
    case 2:
      f=knew(7,0,fnnew("*"),'*',0,0);
      p=avdom(f,a,"/"); kfree(f);
      q=kv2(p->i); kfree(p);
      drawf(q,b2);
      r=take2_(a,q); kfree(q);
      break;
    case -1:
      if(ac!=bc) return kerror("length");
      r=eache(draw2_,a,b);
      break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC2A(draw2_)

K* ci1_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k3(a1%256); break;
  case -1: r=kv3(ac); DO(rc, v3(r)[i]=v1(a)[i]%256); break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(ci1_)

K* ic1_(K *a) {
  K *r=0;
  unsigned char *s=0;
  switch(at) {
  case  3: r=k1(a1); break;
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=ic1_(v0(a)[i]); EC(v0(r)[i])) break;
  case -3: s=a->v; if(ac==1) r=k1(s[0]); else { r=kv1(ac); DO(ac,v1(r)[i]=s[i]) } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(ic1_)

K* dj1_(K *a) {
  K *r=0,*am=0;
  int year,month,day,g,b,c,d,e,m;
  switch(at) {
  case  1:
    g = a1 + 32044 + 2464329; /* 20350101 */
    b = (4*g+3)/146097;
    c = g - (b*146097)/4;
    d = (4*c+3)/1461;
    e = c - (1461*d)/4;
    m = (5*e+2)/153;
    day   = e - (153*m+2)/5 + 1;
    month = m + 3 - 12*(m/10);
    year  = b*100 + d - 4800 + m/10;
    r = k1(year*10000+month*100+day);
    break;
  case -1:
    am=kmix(a);
    r=kv0(am->c); DO(rc,v0(r)[i]=dj1_(v0(am)[i]); EC(v0(r)[i]));
    kfree(am);
    break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(dj1_)

K* jd1_(K *a) {
  K *r=0,*am=0;;
  int year,month,day,y,m,g;
  switch(at) {
  case  1:
    year = a1/10000;
    month = (a1/100)%100;
    day = a1%100;
    g = (14-month)/12;
    y = year+4800-g;
    m = month + 12*g - 3;
    r = k1(day + (153*m+2)/5 + y*365 + y/4 - y/100 + y/400 - 32045 - 2464329);
    break;
  case -1:
    am=kmix(a);
    r=kv0(am->c); DO(rc,v0(r)[i]=jd1_(v0(am)[i]); EC(v0(r)[i]));
    kfree(am);
    break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(jd1_)

K* lt1_(K *a) {
  K *r=0,*am=0;
  time_t t,g,o;
  struct tm *tm;
  switch(at) {
  case  1:
    t = a1+2051222400l; /* 2035-1970 */
    tm = localtime(&t);
    #ifdef _WIN32
    g = _mkgmtime(tm);
    #else
    g = timegm(tm);
    #endif
    o = g-t;
    r = k1(a1+(int)o);
    break;
  case -1:
    am = kmix(a);
    r = kv0(am->c); DO(rc, v0(r)[i]=lt1_(v0(am)[i]); EC(v0(r)[i]));
    kfree(am);
    break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(lt1_)

MC1D(log1_,log,log,2,2,v2,v2)
MC1A(log1_)

MC1D(exp1_,exp,exp,2,2,v2,v2)
MC1A(exp1_)

MC1(abs1_,abs,fabs,1,2,v1,v2)
MC1A(abs1_)

K* sqr1_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k2(I2F(a1)*I2F(a1)); break;
  case  2: r=k2(a2*a2); break;
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=sqr1_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1: r=kv2(ac); DO(ac, v2(r)[i]=v1(a)[i]*I2F(v1(a)[i])) break;
  case -2: r=kv2(ac); DO(ac, v2(r)[i]=v2(a)[i]*v2(a)[i]) break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(sqr1_)

MC1D(sqrt1_,sqrt,sqrt,2,2,v2,v2)
MC1A(sqrt1_)

MC1D(floor1_,floor,floor,2,2,v2,v2)
MC1A(floor1_)

MC1D(ceil1_,ceil,ceil,2,2,v2,v2)
MC1A(ceil1_)

K* ddot2_(K *a, K *b) {
  K *r=0,*q=0,*am=0,*bm=0,*sa=0,*sb=0,*f=0;
  int si=0,ai,bi;
  double sf=0,af,bf;;

  if(at<0 && bt<0 && ac!=bc) return kerror("length");

  switch(at) {
  case  1:
    switch(bt) {
    case  1: r=k1(a1*b1); break;
    case  2: r=k2(I2F(a1)*b2); break;
    case  0:
      sb=shape_(b);
      if(sb->c==1) {
        q=kv0(bc);
        DO(bc, v0(q)[i]=ddot2_(a,v0(b)[i]); EC(v0(q)[i]))
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { bm=flip_(b); r=kv0(bm->c); DO(bm->c, v0(r)[i]=ddot2_(a,v0(bm)[i]); EC(v0(r)[i])) kfree(bm); }
      kfree(sb);
      break;
    case -1: ai=a1; DO(bc, si+=ai*v1(b)[i]) r=k1(si); break;
    case -2: ai=I2F(a1); DO(bc, sf+=ai*v2(b)[i]) r=k2(sf); break;
    default: return kerror("type");
    } break;
  case  2:
    switch(bt) {
    case  1: r=k2(a2*I2F(b1)); break;
    case  2: r=k2(a2*b2); break;
    case  0:
      sb=shape_(b);
      if(sb->c==1) {
        q=kv0(bc);
        DO(bc, v0(q)[i]=ddot2_(a,v0(b)[i]); EC(v0(q)[i]))
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { bm=flip_(b); r=kv0(bm->c); DO(bm->c, v0(r)[i]=ddot2_(a,v0(bm)[i]); EC(v0(r)[i])) kfree(bm); }
      kfree(sb);
      break;
    case -1: af=a2; DO(bc, sf+=af*I2F(v1(b)[i])) r=k2(sf); break;
    case -2: af=a2; DO(bc, sf+=af*v2(b)[i]) r=k2(sf); break;
    default: return kerror("type");
    } break;
  case  0:
    switch(bt) {
    case  1:
    case  2:
      sa=shape_(a);
      if(sa->c==1) {
        q=kv0(ac);
        DO(ac, v0(q)[i]=ddot2_(v0(a)[i],b); EC(v0(q)[i]))
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { am=flip_(a); EC(am); r=kv0(am->c); DO(am->c, v0(r)[i]=ddot2_(v0(am)[i],b); EC(v0(r)[i])) kfree(am); }
      kfree(sa);
      break;
    case  0:
      sa=shape_(a); sb=shape_(b);
      if(sa->c>1) am=flip_(a); else am=kref(a);
      if(sb->c>1) bm=flip_(b); else bm=kref(b);
      if(sa->c>1 && sb->c>1) { r=kv0(bm->c); DO(bm->c, v0(r)[i]=ddot2_(v0(am)[i],v0(bm)[i]); EC(v0(r)[i])) }
      else if(sa->c>1 && sb->c==1) { r=kv0(bc); DO(bc, v0(r)[i]=ddot2_(v0(am)[i],b); EC(v0(r)[i])) }
      else if(sa->c==1 && sb->c>1) { r=kv0(ac); DO(ac, v0(r)[i]=ddot2_(a,v0(bm)[i]); EC(v0(r)[i])) }
      else {
        q=kv0(ac);
        DO(ac, v0(q)[i]=ddot2_(v0(am)[i],v0(bm)[i]); EC(v0(q)[i]))
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      kfree(sa); kfree(sb);
      kfree(am); kfree(bm);
      break;
    case -1:
    case -2:
      am=flip_(a);
      if(am->c==bc) {
        q=kv0(bc);
        bm=kmix(b);
        DO(ac, v0(q)[i]=ddot2_(v0(am)[i],v0(bm)[i]); EC(v0(q)[i]))
        kfree(bm);
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { r=kv0(am->c); DO(am->c, v0(r)[i]=ddot2_(v0(am)[i],b); EC(v0(r)[i])) }
      kfree(am);
      break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: bi=b1; DO(ac, si+=v1(a)[i]*bi) r=k1(si); break;
    case  2: bf=b2; DO(ac, sf+=I2F(v1(a)[i])*bf) r=k2(sf); break;
    case  0:
      bm=flip_(b);
      sb=shape_(bm);
      if(ac==bm->c && sb->c==1) { /* matrix? */
        q=kv0(ac);
        am=kmix(a);
        DO(ac, v0(q)[i]=ddot2_(v0(am)[i],v0(bm)[i]); EC(v0(q)[i]))
        kfree(am);
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { r=kv0(bm->c); DO(bm->c, v0(r)[i]=ddot2_(a,v0(bm)[i]); EC(v0(r)[i])) }
      kfree(bm);
      kfree(sb);
      break;
    case -1: DO(ac, si+=v1(a)[i]*v1(b)[i]) r=k1(si); break;
    case -2: DO(ac, sf+=I2F(v1(a)[i])*v2(b)[i]) r=k2(sf); break;
    default: return kerror("type");
    } break;
  case -2:
    switch(bt) {
    case  1: bi=I2F(b1); DO(ac, sf+=v2(a)[i]*bi) r=k2(sf); break;
    case  2: bf=b2; DO(ac, sf+=v2(a)[i]*bf) r=k2(sf); break;
    case  0:
      bm=flip_(b);
      sb=shape_(bm);
      if(ac==bm->c && sb->c==1) { /* matrix? */
        q=kv0(ac);
        am=kmix(a);
        DO(ac, v0(q)[i]=ddot2_(v0(am)[i],v0(bm)[i]); EC(v0(q)[i]))
        kfree(am);
        f=knew(7,0,fnnew("+"),'+',0,0);
        r=avdom(f,q,"/"); kfree(q);
        kfree(f);
      }
      else { r=kv0(bm->c); DO(bm->c, v0(r)[i]=ddot2_(a,v0(bm)[i]); EC(v0(r)[i])) }
      kfree(bm);
      kfree(sb);
      break;
    case -1: DO(ac, sf+=v2(a)[i]*I2F(v1(b)[i])) r=k2(sf); break;
    case -2: DO(ac, sf+=v2(a)[i]*v2(b)[i]) r=k2(sf); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}

K* ddot2_avopt2(K *a, K *b, char *av) {
  K *r=0,*s,*t,*u,*fb;
  unsigned int i,j,k,am,bm; /* cols */
  double d;
  int g;
  if(!strcmp(av,"\\")||!strcmp(av,"/")) { /* eachleft or eachright */
    if(!strcmp(av,"/")) { t=a; a=b; b=t; } /* eachright */
    if(at==0&&bt==0) { /* ma dot\ mb */
      if(ac==0||bc==0) return 0;
      if(v0(a)[0]->t==-2) {
        DO(ac,if(v0(a)[i]->t!=-2) return 0)
        DO(bc,if(v0(b)[i]->t!=-2) return 0)
        am=v0(a)[0]->c; bm=v0(b)[0]->c;
        DO(ac,if(v0(a)[i]->c!=am) return kerror("length"))
        DO(bc,if(v0(b)[i]->c!=bm) return kerror("length"))
        if(am!=bc) return kerror("length");
        /* ok to multiply */
        r=kv0(ac);
        DO(ac,v0(r)[i]=kv2(bm))
        fb=flip_(b); /* transpose b */
        for(i=0;i<ac;i++) {
          s=v0(a)[i];
          u=v0(r)[i];
          for(j=0;j<bm;j++) {
            d=0;
            t=v0(fb)[j];
            for(k=0;k<am;k++) d+=v2(s)[k]*v2(t)[k];
            v2(u)[j]=d;
          }
        }
        kfree(fb);
      }
      else if(v0(a)[0]->t==-1) {
        DO(ac,if(v0(a)[i]->t!=-1) return 0)
        DO(bc,if(v0(b)[i]->t!=-1) return 0)
        am=v0(a)[0]->c; bm=v0(b)[0]->c;
        DO(ac,if(v0(a)[i]->c!=am) return kerror("length"))
        DO(bc,if(v0(b)[i]->c!=bm) return kerror("length"))
        if(am!=bc) return kerror("length");
        /* ok to multiply */
        r=kv0(ac);
        DO(ac,v0(r)[i]=kv1(bm))
        fb=flip_(b); /* transpose b */
        for(i=0;i<ac;i++) {
          s=v0(a)[i];
          u=v0(r)[i];
          for(j=0;j<bm;j++) {
            g=0;
            t=v0(fb)[j];
            for(k=0;k<am;k++) g+=v1(s)[k]*v1(t)[k];
            v1(u)[j]=g;
          }
        }
        kfree(fb);
      }
    }
  }
  return r;
}

MC1D(sin1_,sin,sin,2,2,v2,v2)
MC1A(sin1_)

MC1D(cos1_,cos,cos,2,2,v2,v2)
MC1A(cos1_)

MC1D(tan1_,tan,tan,2,2,v2,v2)
MC1A(tan1_)

MC1D(asin1_,asin,asin,2,2,v2,v2)
MC1A(asin1_)

MC1D(acos1_,acos,acos,2,2,v2,v2)
MC1A(acos1_)

MC1D(atan1_,atan,atan,2,2,v2,v2)
MC1A(atan1_)

MC1D(sinh1_,sinh,sinh,2,2,v2,v2)
MC1A(sinh1_)

MC1D(cosh1_,cosh,cosh,2,2,v2,v2)
MC1A(cosh1_)

MC1D(tanh1_,tanh,tanh,2,2,v2,v2)
MC1A(tanh1_)

MC2D(atan2_,atan2,atan2,2,2,v2,v2)
MC2A(atan2_)

K* euclid2_(K *a, K *b) {
  K *r=0,*p=0;
  double af,bf;
  int b0=0;

  if(at<0 && bt<0 && ac!=bc) return kerror("length");

  if(at==0 || bt==0) { r=eache(euclid2_,a,b); b0=1; }
  else {
  switch(at) {
  case  1:
    switch(bt) {
    case  1: af=I2F(a1); bf=I2F(b1); r=k2(sqrt(af*af+bf*bf)); break;
    case  2: af=I2F(a1); r=k2(sqrt(af*af+b2*b2)); break;
    case -1: r=kv2(bc); bf=I2F(a1); af=bf*bf; DO(rc, bf=I2F(v1(b)[i]); v2(r)[i]=sqrt(af+bf*bf)) break;
    case -2: r=kv2(bc); bf=I2F(a1); af=bf*bf; DO(rc, bf=v2(b)[i]; v2(r)[i]=sqrt(af+bf*bf)) break;
    default: return kerror("type");
    } break;
  case  2:
    switch(bt) {
    case  1: bf=I2F(b1); r=k2(sqrt(a2*a2+bf*bf)); break;
    case  2: r=k2(sqrt(a2*a2+b2*b2)); break;
    case -1: r=kv2(bc); af=a2*a2; DO(rc, bf=I2F(v1(b)[i]); v2(r)[i]=sqrt(af+bf*bf)) break;
    case -2: r=kv2(bc); af=a2*a2; DO(rc, bf=v2(b)[i]; v2(r)[i]=sqrt(af+bf*bf)) break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: r=kv2(ac); af=I2F(b1); bf=af*af; DO(rc, af=I2F(v1(a)[i]); v2(r)[i]=sqrt(af*af+bf)); break;
    case  2: r=kv2(ac); bf=b2*b2; DO(rc, af=I2F(v1(a)[i]); v2(r)[i]=sqrt(af*af+bf)); break;
    case -1: r=kv2(ac); DO(rc, af=I2F(v1(a)[i]); bf=I2F(v1(b)[i]); v2(r)[i]=sqrt(af*af+bf*bf)) break;
    case -2: r=kv2(ac); DO(rc, af=I2F(v1(a)[i]); bf=v2(b)[i]; v2(r)[i]=sqrt(af*af+bf*bf)) break;
    default: return kerror("type");
    } break;
  case -2:
    switch(bt) {
    case  1: r=kv2(ac); af=I2F(b1); bf=af*af; DO(rc, af=v2(a)[i]; v2(r)[i]=sqrt(af*af+bf)); break;
    case  2: r=kv2(ac); bf=b2*b2; DO(rc, af=I2F(v2(a)[i]); v2(r)[i]=sqrt(af*af+bf)); break;
    case -1: r=kv2(ac); DO(rc, af=v2(a)[i]; bf=I2F(v1(b)[i]); v2(r)[i]=sqrt(af*af+bf*bf)) break;
    case -2: r=kv2(ac); DO(rc, af=v2(a)[i]; bf=v2(b)[i]; v2(r)[i]=sqrt(af*af+bf*bf)) break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  }
  // for some reason, e keeps mixed lists here
  //return r->t ? r : knorm(r);
  if(b0) { p=kmix(r); kfree(r); r=p; }
  return r;
}
MC2A(euclid2_)

MC1D(erf1_,erf,erf,2,2,v2,v2)
MC1A(erf1_)

MC1D(erfc1_,erfc,erfc,2,2,v2,v2)
MC1A(erfc1_)

MC1D(gamma1_,tgamma,tgamma,2,2,v2,v2)
MC1A(gamma1_)

MC1D(lgamma1_,lgamma,lgamma,2,2,v2,v2)
MC1A(lgamma1_)

MC1D(rint1_,rint,rint,2,2,v2,v2)
MC1A(rint1_)

MC1D(trunc1_,trunc,trunc,2,2,v2,v2)
MC1A(trunc1_)

MC2IO(div2_,/,1)
MC2A(div2_)

MC2IO(and2_,&,0)
MC2A(and2_)

MC2IO(or2_,|,0)
MC2A(or2_)

MC2IO(xor2_,^,0)
MC2A(xor2_)

K* not1_(K *a) {
  K *r=0;
  switch(at) {
  case  1: r=k1(~a1); break;
  case  0: r=kv0(ac); DO(ac, v0(r)[i]=not1_(v0(a)[i]); EC(v0(r)[i])) break;
  case -1: r=kv1(ac); DO(ac, v1(r)[i]=~v1(a)[i]) break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC1A(not1_)

#define ROT(a,b) ((b)>0 ? ((a)<<(b))|((a)>>(32-(b))) : ((a)<<(b))|((a)>>(32-(b))))
K* rot2_(K *a, K *b) {
  K *r=0;
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length");
  if(at==0 || bt==0) r=eache(rot2_,a,b);
  else {
  switch(at) {
  case  1:
    switch(bt) {
    case  1: r=k1(ROT(a1,b1)); break;
    case -1: r=kv1(bc); DO(bc, v1(r)[i]=ROT(a1,v1(b)[i])) break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: r=kv1(ac); DO(ac, v1(r)[i]=ROT(v1(a)[i],b1);) break;
    case -1: r=kv1(ac); DO(ac, v1(r)[i]=ROT(v1(a)[i],v1(b)[i]);) break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  }
  return r->t ? r : knorm(r);
}
MC2A(rot2_)

K* shift2_(K *a, K *b) {
  K *r=0;
  if(at<=0 && bt<=0 && ac!=bc) return kerror("length");
  if(at==0 || bt==0 ) r=eache(shift2_,a,b);
  else {
  switch(at) {
  case  1:
    switch(bt) {
    case  1: r=b1>0 ? k1(a1<<b1) : k1(a1>>abs(b1)); break;
    case -1: r=kv1(bc); DO(bc, v1(r)[i] = v1(b)[i]>0 ? a1<<v1(b)[i] : a1>>abs(v1(b)[i])) break;
    default: return kerror("type");
    } break;
  case -1:
    switch(bt) {
    case  1: r=kv1(ac); DO(ac, v1(r)[i] = b1>0 ? v1(a)[i]<<b1 : v1(a)[i]>>abs(b1)) break;
    case -1: r=kv1(ac); DO(ac, v1(r)[i] = v1(b)[i]>0 ? v1(a)[i]<<v1(b)[i] : v1(a)[i]>>abs(v1(b)[i])) break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  }
  return r->t ? r : knorm(r);
}
MC2A(shift2_)

/* svd stuff based on numerical recipes */
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ABS(x) ((x)<0?-(x):(x))
#define SIGN(u,v) ((v)>=0.0?ABS(u):-ABS(u))
static double pythag(double u, double v) {
  double au,av,aw;
  au=ABS(u);
  av=ABS(v);
  if(au>av) { aw=av/au; return au*sqrt(1.0+aw*aw); }
  if(av!=0.0) { aw=au/av; return av*sqrt(1.0+aw*aw); }
  return 0.0;
}

static K* svdcmp(double **a, int m, int n, double *w, double **v, double *t) {
  int flag,i,its,j,jj,k,l,nm,nm1=n-1,mm1=m-1;
  double c,f,h,s,x,y,z;
  double anorm=0.0,g=0.0,scale=0.0;
  double *rv1=t;

  /* Householder reduction to bidigonal form */
  for(i = 0; i < n; i++) {
    l = i + 1;
    rv1[i] = scale * g;
    g = s = scale = 0.0;
    if(i < m) {
      for(k = i; k < m; k++) scale += ABS(a[k][i]);
      if(scale) {
        for(k = i; k < m; k++) {
          a[k][i] /= scale;
          s += a[k][i] * a[k][i];
        }
        f = a[i][i];
        g = -SIGN(sqrt(s), f);
        h = f * g - s;
        a[i][i] = f - g;
        if(i != nm1) {
          for(j = l; j < n; j++) {
            for(s = 0.0, k = i; k < m; k++) s += a[k][i] * a[k][j];
            f = s / h;
            for(k = i; k < m; k++) a[k][j] += f * a[k][i];
          }
        }
        for(k = i; k < m; k++) a[k][i] *= scale;
      }
    }
    w[i] = scale * g;
    g = s = scale = 0.0;
    if(i < m && i != nm1) {
      for(k = l; k < n; k++) scale += ABS(a[i][k]);
      if(scale) {
        for(k = l; k < n; k++) {
          a[i][k] /= scale;
          s += a[i][k] * a[i][k];
        }
        f = a[i][l];
        g = -SIGN(sqrt(s), f);
        h = f * g - s;
        a[i][l] = f - g;
        for(k = l; k < n; k++) rv1[k] = a[i][k] / h;
        if(i != mm1) {
          for(j = l; j < m; j++) {
            for(s = 0.0, k = l; k < n; k++) s += a[j][k] * a[i][k];
            for(k = l; k < n; k++) a[j][k] += s * rv1[k];
          }
        }
        for(k = l; k < n; k++) a[i][k] *= scale;
      }
    }
    anorm = MAX(anorm, (ABS(w[i]) + ABS(rv1[i])));
  }

  /* Accumulation of right-hand transformations */
  for(i = n - 1; i >= 0; i--) {
    if(i < nm1) {
      if(g) {
        /* F division to avoid possible underflow */
        for(j = l; j < n; j++) v[j][i] = (a[i][j] / a[i][l]) / g;
        for(j = l; j < n; j++) {
          for(s = 0.0, k = l; k < n; k++) s += a[i][k] * v[k][j];
          for(k = l; k < n; k++) v[k][j] += s * v[k][i];
        }
      }
      for(j = l; j < n; j++) v[i][j] = v[j][i] = 0.0;
    }
    v[i][i] = 1.0;
    g = rv1[i];
    l = i;
  }
  /* Accumulation of left-hand transformations */
  for(i = n - 1; i >= 0; i--) {
    l = i + 1;
    g = w[i];
    if(i < nm1) for(j = l; j < n; j++) a[i][j] = 0.0;
    if(g) {
      g = 1.0 / g;
      if(i != nm1) {
        for(j = l; j < n; j++) {
          for(s = 0.0, k = l; k < m; k++) s += a[k][i] * a[k][j];
          f = (s / a[i][i]) * g;
          for(k = i; k < m; k++) a[k][j] += f * a[k][i];
        }
      }
      for(j = i; j < m; j++) a[j][i] *= g;
    }
    else for(j = i; j < m; j++) a[j][i] = 0.0;
    ++a[i][i];
  }
  /* diagonalization of the bidigonal form */
  for(k = n - 1; k >= 0; k--) { /* loop over singlar values */
    for(its = 0; its < 30; its++) { /* loop over allowed iterations */
      flag = 1;
      for(l = k; l >= 0; l--) { /* test for splitting */
        nm = l - 1; /* note that rv1[l] is always zero */
        if(ABS(rv1[l]) + anorm == anorm) {
          flag = 0;
          break;
        }
        if(ABS(w[nm]) + anorm == anorm) break;
      }
      if(flag) {
        c = 0.0;  /* cancellation of rv1[l], if l>1 */
        s = 1.0;
        for(i = l; i <= k; i++) {
          f = s * rv1[i];
          if(ABS(f) + anorm != anorm) {
            g = w[i];
            h = pythag(f, g);
            w[i] = h;
            h = 1.0 / h;
            c = g * h;
            s = (-f * h);
            for(j = 0; j < m; j++) {
              y = a[j][nm];
              z = a[j][i];
              a[j][nm] = y * c + z * s;
              a[j][i] = z * c - y * s;
            }
          }
        }
      }
      z = w[k];
      if(l == k) {  /* convergence */
        if(z < 0.0) {
          w[k] = -z;
          for(j = 0; j < n; j++) v[j][k] = (-v[j][k]);
        }
        break;
      }
      if(its == 30) return kerror("limit");
      x = w[l];  /* shift from bottom 2-by-2 minor */
      nm = k - 1;
      y = w[nm];
      g = rv1[nm];
      h = rv1[k];
      f = ((y - z) * (y + z) + (g - h) * (g + h)) / (2.0 * h * y);
      g = pythag(f, 1.0);
      /* next QR transformation */
      f = ((x - z) * (x + z) + h * ((y / (f + SIGN(g, f))) - h)) / x;
      c = s = 1.0;
      for(j = l; j <= nm; j++) {
        i = j + 1;
        g = rv1[i];
        y = w[i];
        h = s * g;
        g = c * g;
        z = pythag(f, h);
        rv1[j] = z;
        c = f / z;
        s = h / z;
        f = x * c + g * s;
        g = g * c - x * s;
        h = y * s;
        y = y * c;
        for(jj = 0; jj < n; jj++) {
          x = v[jj][j];
          z = v[jj][i];
          v[jj][j] = x * c + z * s;
          v[jj][i] = z * c - x * s;
        }
        z = pythag(f, h);
        w[j] = z; /* rotation can be arbitrary id z=0 */
        if(z) {
          z = 1.0 / z;
          c = f * z;
          s = h * z;
        }
        f = (c * g) + (s * y);
        x = (c * y) - (s * g);
        for(jj = 0; jj < m; jj++) {
          y = a[jj][j];
          z = a[jj][i];
          a[jj][j] = y * c + z * s;
          a[jj][i] = z * c - y * s;
        }
      }
      rv1[l] = 0.0;
      rv1[k] = f;
      w[k] = x;
    }
  }
  return null;
}

K* lsq2_(K *a, K *b) {
  K *x,*y,*z;
  double **u,*w,**v,*t,wmax,thresh,TOL=1.0e-6,s;
  unsigned int r,n,m;
  if(at > 0 || at < -2 || bt) return kerror("type");
  if(!ac || !bc) return kerror("length");
  r=v0(b)[0]->c; if(r<=0) return kerror("length");
  DO(bc, y=v0(b)[i]; if(y->t != -1 && y->t != -2) return kerror("type"); if(r != y->c) return kerror("length"))
  if(!at) DO(ac, y=v0(a)[i]; if(y->t != -1 && y->t != -2) return kerror("type"); if(r != y->c) return kerror("length"))
  else if(r != ac) return kerror("length");

  n=bc;m=MAX(r,n);
  u=xmalloc(m*sizeof(double*));
  u[0] =xmalloc(n*m*sizeof(double));
  w=xmalloc(n*sizeof(double));
  v=xmalloc(n*sizeof(double*));
  v[0] =xmalloc(n*n*sizeof(double));
  t=xmalloc(n*sizeof(double));
  DO(m,u[i]=u[0]+n*i)
  DO(n,v[i]=v[0]+n*i)
  DO(n*m,u[0][i]=0)
  DO(r,DO2(n,y=v0(b)[j];u[i][j]=y->t==-2?v2(y)[i]:I2F(v1(y)[i])))
  z=svdcmp(u,m,n,w,v,t); if(z->t==98) return z;
  wmax=0.0;
  DO(n, if (w[i] > wmax) wmax=w[i])
  thresh=TOL*wmax;
  DO(n, if (w[i] < thresh) w[i]=0.0)
  if(!at){z=kv0(ac); DO(ac,v0(z)[i]=kv2(n))}
  else z=kv2(n);
  DO3(at?1:ac,
    y=at?a:v0(a)[k];
    x=at?z:v0(z)[k];
    DO(n,s=0.0;if(w[i]){DO2(m,s+=u[j][i]*(y->t==-2?v2(y)[j]:I2F(v1(y)[j])))s/=w[i];}t[i]=s)
    //DO(n,s=0.0;DO2(n,s+=v[i][j]*t[j];v2(x)[i]=s))
    //DO(n,s=0.0;DO2(n,s+=v[i][j]*t[j];v2(x)[i]=ABS(s)<TOL?0.0:s))
    DO(n,s=0.0;DO2(n,s+=v[i][j]*t[j];v2(x)[i]=CMPFFT(ABS(s),thresh)<0?0.0:s))
  )
  xfree(u[0]);xfree(u);xfree(w);xfree(v[0]);xfree(v);xfree(t);

  return z;
}
MC2A(lsq2_)

K* atn2_(K *a, K *b) {
  K *r=0;
  switch(at) {
  case -1:
    switch(bt) {
    case  1: r=k1((unsigned int)b1>=ac?INT_MIN:v1(a)[b1]); break;
    case -1: r=kv1(bc); DO(rc, v1(r)[i]=(unsigned int)v1(b)[i]>=ac?INT_MIN:v1(a)[v1(b)[i]]); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC2A(atn2_)

static int sm(char *a, char *b, int an, int bn, int i, int j) {
  int m=1;
  for(;;++i,++j) {
    if(i==an&&j==bn) break;
    if(i==an) { m=0; break; }
    if(j==bn) { m=0; break; }
    if(b[j]=='?') continue;
    if(b[j]=='*') {
      if(++j==bn) { ++i; break; }
      m=0;
      while(++i<an) if((m=sm(a,b,an,bn,i,j))) break;
    }
    if(b[j]=='[') {
      if(++j==bn) { m=0; break; }
      if(b[j]=='^') {
        for(++j;j<bn&&b[j]!=']';j++) if(a[i]==b[j]) break;
        if(b[j]!=']') { m=0; break; }
      }
      else {
        for(;j<bn&&b[j]!=']';j++) if(a[i]==b[j]) break;
        if(b[j]==']') { m=0; break; }
      }
      while(j<bn&&b[j]!=']') ++j;
    }
    else if(i<an&&j<bn&&a[i]!=b[j]) { m=0; break; }
  }
  return m;
}

K* sm2_(K *a, K *b) {
  K *r=0;
  K *f=scope_get(gs,sp("sm"));

  if(at==-4 && bt==-4 && ac!=bc) return kerror("length");
  if(at== 0 && bt==-4 && ac!=bc) return kerror("length");
  if(at==-4 && bt== 0 && ac!=bc) return kerror("length");
  if(at== 0 && bt== 0 && ac!=bc) return kerror("length");

  switch(at) {
  case  4:
    switch(bt) {
    case  4: r=k1(sm(v3(a),v3(b),strlen(v3(a)),strlen(v3(b)),0,0)); break;
    case -3: r=k1(sm(v3(a),v3(b),strlen(v3(a)),bc,0,0)); break;
    case -4: r=eache(sm2_,a,b); break;
    case  0: r=eache(sm2_,a,b); break;
    default: return kerror("type");
    } break;
  case -3:
    switch(bt) {
    case  4: r=k1(sm(v3(a),v3(b),ac,strlen(v3(b)),0,0)); break;
    case -3: r=k1(sm(v3(a),v3(b),ac,bc,0,0)); break;
    case -4: r=eachright(f,a,b,0); break;
    case  0: r=eachright(f,a,b,0); break;
    default: return kerror("type");
    } break;
  case -4:
    switch(bt) {
    case  4: r=eache(sm2_,a,b); break;
    case -3: r=eachleft(f,a,b,0); break;
    case -4: r=eache(sm2_,a,b); break;
    case  0: r=eache(sm2_,a,b); break;
    default: return kerror("type");
    } break;
  case  0:
    switch(bt) {
    case  4: r=eache(sm2_,a,b); break;
    case -3: r=eachleft(f,a,b,0); break;
    case -4: r=eache(sm2_,a,b); break;
    case  0: r=eache(sm2_,a,b); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC2A(sm2_)

static int ss(char *a, char *b, int an, int bn, int i) {
  int m=1,j,k=i;
  for(j=0;j<bn;j++) {
    if(k==an) break;
    if(b[j]=='?') { }
    else if(b[j]=='[') {
      m=1;
      if(++j==bn) { m=0; break; }
      if(b[j]=='^') {
        for(++j;j<bn&&b[j]!=']';j++) if(a[k]==b[j]) break;
        if(b[j]!=']') { m=0; break; }
      }
      else {
        for(;j<bn&&b[j]!=']';j++) if(a[k]==b[j]) break;
        if(b[j]==']') { m=0; break; }
      }
      while(j<bn&&b[j]!=']') ++j;
    }
    else if(a[k]!=b[j]) break;
    ++k;
  }
  return m&&j==bn ? k-i : 0;
}

K* ss2_(K *a, K *b) {
  K *r=0;
  unsigned int i,n=0,m,s;
  K *f=scope_get(gs,sp("ss"));

  if(at!= 0 && at!=-3) return kerror("type");
  if(at== 0 && bt==-4 && ac!=bc) return kerror("length");
  if(at== 0 && bt== 0 && ac!=bc) return kerror("length");
  if(bt==-3 && !bc) return kerror("length");
  if(bt== 4 && !strlen(v3(b))) return kerror("length");

  switch(at) {
  case -3:
    switch(bt) {
    case  4:
      r=kv1(2);
      s=strlen(v3(b));
      for(i=0;i<ac;i++) {
        if((m=ss(v3(a),v3(b),ac,s,i))) {
          if((i==0||!isalpha(v3(a)[i-1])) && ((i+m<ac&&!isalpha(v3(a)[i+m]))||i+m==ac)) {
            if(n==rc) { rc<<=1; r->v=(int*)xrealloc(r->v,rc*sizeof(int)); }
            v1(r)[n++]=i;
            i+=m-1;
          }
        }
      }
      rc=n;
      break;
    case -3:
      r=kv1(2);
      for(i=0;i<ac;i++) {
        if((m=ss(v3(a),v3(b),ac,bc,i))) {
          if(n==rc) { rc<<=1; r->v=(int*)xrealloc(r->v,rc*sizeof(int)); }
          v1(r)[n++]=i;
          i+=m-1;
        }
      }
      rc=n;
      break;
    case -4: r=eachright(f,a,b,0); break;
    case  0: r=eachright(f,a,b,0); break;
    default: return kerror("type");
    } break;
  case  0:
    switch(bt) {
    case  4: r=eachleft(f,a,b,0); break;
    case  0: r=eache(ss2_,a,b); break;
    case -3: r=eachleft(f,a,b,0); break;
    case -4: r=eache(ss2_,a,b); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return r->t ? r : knorm(r);
}
MC2A(ss2_)

K* kv1_(K *a) {
  K *r=0;
  switch(at) {
  case -1:
  case -2:
  case -3:
  case -4: r=kmix(a); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(kv1_)

K* vk1_(K *a) {
  if(at) return kerror("type");
  return knorm(kref(a));
}
MC1A(vk1_)

K* timer1_(K *a) {
  (void)a;
  return null;
}
MC1A(timer1_)

K* error1_(K *a) {
  if(at==6) printf("%d\n",E);
  else E=a->i;
  return null;
}
MC1A(error1_)

K* md5_(K *a) {
  K *r=0,*p=0;
  if(at==0) DO(ac,if(v0(a)[i]->t!=-3) return kerror("type"))
  switch(at) {
  case -3: r=kv3(32); md5(r->v,a->v,ac); break;
  case  4: r=kv3(32); md5(r->v,a->v,strlen(a->v)); break;
  case -4: r=kv0(ac); p=kmix(a); DO(ac,EC(v0(r)[i]=md5_(v0(p)[i]))); kfree(p); break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=md5_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(md5_)

K* sha1_(K *a) {
  K *r=0,*p=0;
  if(at==0) DO(ac,if(v0(a)[i]->t!=-3) return kerror("type"))
  switch(at) {
  case -3: r=kv3(40); sha1(r->v,a->v,ac); break;
  case  4: r=kv3(40); sha1(r->v,a->v,strlen(a->v)); break;
  case -4: r=kv0(ac); p=kmix(a); DO(ac,EC(v0(r)[i]=sha1_(v0(p)[i]))); kfree(p); break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=sha1_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(sha1_)

K* sha2_(K *a) {
  K *r=0,*p=0;
  if(at==0) DO(ac,if(v0(a)[i]->t!=-3) return kerror("type"))
  switch(at) {
  case -3: r=kv3(64); sha2(r->v,a->v,ac); break;
  case  4: r=kv3(64); sha2(r->v,a->v,strlen(a->v)); break;
  case -4: r=kv0(ac); p=kmix(a); DO(ac,EC(v0(r)[i]=sha2_(v0(p)[i]))); kfree(p); break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=sha2_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(sha2_)

static K* crypt_(char*(*f)(char *,unsigned char*,unsigned char*), K *a, K *b) {
  K *r=0,*iv=0,*key=0;
  char *s=0,*e=0,*t=0;
  if(at==0) DO(ac,if(v0(a)[i]->t!=-1) return kerror("type"))
  if(v0(a)[0]->c!=16) return kerror("iv");
  if(v0(a)[1]->c!=32) return kerror("key");
  if(bt!=-3&&bt!=4&&bt!=-4&&bt!=0) return kerror("type");
  iv=ci1_(v0(a)[0]);
  key=ci1_(v0(a)[1]);
  switch(bt) {
  case -3: s=xstrndup(b->v,bc); e=f(s,iv->v,key->v); r=knew(-3,strlen(e),e,0,0,0); break;
  case  4: e=f(b->v,iv->v,key->v); r=knew(-3,strlen(e),e,0,0,0); break;
  case -4: r=kv0(bc); DO(bc,t=f(v4(b)[i],iv->v,key->v); EC(v0(r)[i]=knew(-3,strlen(t),t,0,0,0)); xfree(t)); break;
  case  0: r=kv0(bc); DO(bc,EC(v0(r)[i]=crypt_(f,a,v0(b)[i]))); break;
  default: return kerror("type");
  }
  xfree(s); xfree(e);
  kfree(iv); kfree(key); 
  return r;
}

K* encrypt_(K *a, K *b) { return crypt_(aes256e,a,b); }
MC2A(encrypt_)

K* decrypt_(K *a, K *b) { return crypt_(aes256d,a,b); }
MC2A(decrypt_)

K* hb1_(K *a) {
  K *r=0;
  char *p=0;
  switch(at) {
  case -3: r=kv3(1+2*ac); p=v3(r); DO(ac,sprintf(p,"%02x",(unsigned char)v3(a)[i]);p+=2); r->c--; break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=hb1_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(hb1_)

K* bh1_(K *a) {
  K *r=0;
  char *p,*q,b[5];
  switch(at) {
  case -3:
    if(!ac) return kv3(0);
    p=v3(a);
    r=kv3(1+ac/2); q=v3(r);
    if(ac==1) { sprintf(b,"0x0%c",p[0]); *q=strtol(b,0,0); return r; }
    if(ac%2) { sprintf(b,"0x0%c",p[0]); *q++=strtol(b,0,0); p++; }
    DO(ac/2, sprintf(b,"0x%c%c",p[0],p[1]); *q++=strtol(b,0,0); p+=2; )
    r->c--;
    break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=bh1_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(bh1_)

K* getenv1_(K *a) {
  K *r=null,*m;
  char *b,*p;
  switch(at) {
  case -3: b=xstrndup(v3(a),ac); p=getenv(b); if(p)r=knew(-3,strlen(p),p,0,0,0); xfree(b); break;
  case  4: p=getenv(a4); if(p)r=knew(-3,strlen(p),p,0,0,0); break;
  case -4: r=kv0(ac); m=kmix(a); DO(ac,EC(v0(r)[i]=getenv1_(v0(m)[i]))); kfree(m); break;
  case  0: r=kv0(ac); DO(ac,EC(v0(r)[i]=getenv1_(v0(a)[i]))); break;
  default: return kerror("type");
  }
  return r;
}
MC1A(getenv1_)

static void se(char *k, char *v) {
  #ifdef _WIN32
  _putenv_s(k,v);
  #else
  setenv(k,v,1);
  #endif
}
K* setenv2_(K *a, K *b) {
  char *p,*q;
  switch(at) {
  case -3:
    switch(bt) {
    case -3: p=xstrndup(v3(a),ac); q=xstrndup(v3(b),bc); break;
    case  4: p=xstrndup(v3(a),ac); q=strdup(b4); break;
    default: return kerror("type");
    }
    se(p,q);
    xfree(p); xfree(q);
    break;
  case  4:
    switch(bt) {
    case -3: p=strdup(a4); q=xstrndup(v3(b),bc); break;
    case  4: p=strdup(a4); q=strdup(b4); break;
    default: return kerror("type");
    }
    se(p,q);
    xfree(p); xfree(q);
    break;
  case -4:
    switch(bt) {
    case -4: if(ac!=bc) return kerror("length"); DO(ac,se(v4(a)[i],v4(b)[i])); break;
    default: return kerror("type");
    } break;
  case  0:
    switch(bt) {
    case  0: if(ac!=bc) return kerror("length"); DO(ac,EC(setenv2_(v0(a)[i],v0(b)[i]))); break;
    default: return kerror("type");
    } break;
  default: return kerror("type");
  }
  return null;
}
MC1A(setenv1_)

K* zb1_(K* a) {
  K *r;
  char *b,*pb;
  size_t n;
  LZW *z=lzwc(v3(a),ac);
  n=z->i/8+(z->i%8?1:0);
  b=xmalloc(sizeof(LZW)+n);
  pb=b;
  *b++=z->v;
  memcpy(b,&z->i,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,&z->n,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,&z->c,sizeof(size_t)); b+=sizeof(size_t);
  memcpy(b,z->b,n);
  lzwfree(z);
  r=kv3(1);
  xfree(r->v);
  r->v=pb;
  r->c=sizeof(LZW)+n;
  return r;
}
MC1A(zb1_)

K* bz1_(K* a) {
  K *r;
  LZW *p,*q;
  char *pa=v3(a);
  size_t n;
  p=lzwnew();
  p->v=*pa++;
  memcpy(&p->i,pa,sizeof(size_t)); pa+=sizeof(size_t);
  memcpy(&p->n,pa,sizeof(size_t)); pa+=sizeof(size_t);
  memcpy(&p->c,pa,sizeof(size_t)); pa+=sizeof(size_t);
  n=p->i/8+(p->i%8?1:0);
  p->b=xrealloc(p->b,n);
  memcpy(p->b,pa,n);
  q=lzwd(p);
  lzwfree(p);
  r=kv3(1);
  xfree(r->v);
  r->v=q->b;
  r->c=q->n;
  xfree(q);
  return r;
}
MC1A(bz1_)
