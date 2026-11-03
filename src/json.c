/* json: JSON text <-> gk data (RFC 8259).

   jk x   serialize x as JSON text, a char vector (json from k)
   kj t   t a char vector (or char atom): parse the JSON text (k from json)
   -- the bd/db naming: one verb each way, so jk of a string is a JSON string

   read   object -> dict (every key must be a name: domain error otherwise,
                    the same rule as every other way into a dict, dict.c)
          array  -> list; an all-numeric array becomes a typed vector of its
                    widest member (int < long < float), with null as that
                    type's null; anything else stays a general list
          string -> char vector (\uXXXX and surrogate pairs decode to UTF-8;
                    a lone surrogate becomes U+FFFD)
          number -> int if it fits i32, long if it fits i64, else float; a
                    fraction or exponent is always a float.  This is NOT the
                    source-literal rule (a bare 3000000000 in source saturates
                    to 0I): JSON carries 64-bit ids everywhere and has no j.
          true/false -> 1/0; null -> nul
   write  dict -> object; list or vector -> array; char vector -> string;
          symbol -> string; char atom -> one-char string; int/long -> digits
          (a null -> null); float/f32 -> as 5: prints at \p, always with a
          point or exponent so it re-reads as a float (null, inf -> null,
          which has no JSON spelling); nul -> null; anything else (functions,
          handles, projections) -> type error
   Nesting deeper than JDEPTH, or a low C stack, is a stack error either way. */
#include "json.h"
#include "dict.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define JDEPTH 2048

/* ---------------------------------------------------------------- read */
typedef struct { char *p,*e; i32 d; } jp;

static K jval_(jp *j);

static void jws_(jp *j) {
  while(j->p<j->e && (*j->p==' '||*j->p=='\t'||*j->p=='\n'||*j->p=='\r')) ++j->p;
}
static int jhex_(int c) {
  if(c>='0'&&c<='9') return c-'0';
  if(c>='a'&&c<='f') return c-'a'+10;
  if(c>='A'&&c<='F') return c-'A'+10;
  return -1;
}
static int jhex4_(char *p, char *e, u32 *u) {
  u32 v=0; int h,i;
  if(e-p<4) return 0;
  for(i=0;i<4;i++) { h=jhex_((unsigned char)p[i]); if(h<0) return 0; v=v<<4|(u32)h; }
  *u=v; return 1;
}
static char* jutf8_(char *w, u32 u) {
  if(u<0x80) *w++=(char)u;
  else if(u<0x800) { *w++=(char)(0xC0|u>>6); *w++=(char)(0x80|(u&0x3F)); }
  else if(u<0x10000) { *w++=(char)(0xE0|u>>12); *w++=(char)(0x80|(u>>6&0x3F)); *w++=(char)(0x80|(u&0x3F)); }
  else { *w++=(char)(0xF0|u>>18); *w++=(char)(0x80|(u>>12&0x3F)); *w++=(char)(0x80|(u>>6&0x3F)); *w++=(char)(0x80|(u&0x3F)); }
  return w;
}

/* a string: j->p at the opening quote.  Decoded into a fresh buffer sized by
   the raw text (every escape shrinks).  A raw control character is a parse
   error, as the spec says. */
static K jstr_(jp *j, char **ps, u64 *pn) {
  char *p=j->p+1,*e=j->e,*q,*b,*w;
  for(q=p;;++q) {           /* find the closing quote */
    if(q>=e) return KERR_PARSE;
    if(*q=='"') break;
    if(*q=='\\') { if(++q>=e) return KERR_PARSE; }
  }
  b=w=xmalloc((size_t)(q-p)+1);
  while(p<q) {
    unsigned char c=(unsigned char)*p;
    if(c<0x20) { xfree(b); return KERR_PARSE; }
    if(c!='\\') { *w++=(char)c; ++p; continue; }
    ++p;                    /* p<q: the scan above never leaves a backslash last */
    switch(*p++) {
    case '"': *w++='"'; break;
    case '\\': *w++='\\'; break;
    case '/': *w++='/'; break;
    case 'b': *w++='\b'; break;
    case 'f': *w++='\f'; break;
    case 'n': *w++='\n'; break;
    case 'r': *w++='\r'; break;
    case 't': *w++='\t'; break;
    case 'u': {
      u32 u,lo;
      if(!jhex4_(p,q,&u)) { xfree(b); return KERR_PARSE; }
      p+=4;
      if(u>=0xD800&&u<0xDC00) {       /* high surrogate: a low one must follow */
        if(q-p>=6 && p[0]=='\\' && p[1]=='u' && jhex4_(p+2,q,&lo) && lo>=0xDC00 && lo<0xE000) {
          u=0x10000+((u-0xD800)<<10)+(lo-0xDC00); p+=6;
        }
        else u=0xFFFD;
      }
      else if(u>=0xDC00&&u<0xE000) u=0xFFFD;
      w=jutf8_(w,u);
      break;
    }
    default: xfree(b); return KERR_PARSE;
    }
  }
  *w=0;
  *pn=(u64)(w-b);
  *ps=xrealloc(b,(size_t)(w-b)+1);
  j->p=q+1;
  return null;
}

static K jnum_(jp *j) {
  char *p=j->p,*e=j->e,*s=p,buf[64],*t;
  int neg=0,flt=0,nd=0; u64 v=0; size_t L; double d;
  if(p<e&&*p=='-') { neg=1; ++p; }
  if(p>=e||!isdigit((unsigned char)*p)) return KERR_PARSE;
  if(*p=='0') ++p;
  else while(p<e&&isdigit((unsigned char)*p)) { if(nd<19) v=v*10+(u64)(*p-'0'); else flt=1; ++nd; ++p; }
  if(p<e&&*p=='.') {
    ++p; if(p>=e||!isdigit((unsigned char)*p)) return KERR_PARSE;
    while(p<e&&isdigit((unsigned char)*p)) ++p;
    flt=1;
  }
  if(p<e&&(*p=='e'||*p=='E')) {
    ++p; if(p<e&&(*p=='+'||*p=='-')) ++p;
    if(p>=e||!isdigit((unsigned char)*p)) return KERR_PARSE;
    while(p<e&&isdigit((unsigned char)*p)) ++p;
    flt=1;
  }
  j->p=p;
  /* an integer that stays clear of the sentinels: int, else long, else float */
  if(!flt && v<=(u64)INT64_MAX-1) {
    i64 w=neg?-(i64)v:(i64)v;
    if(w>=INT32_MIN+2 && w<=INT32_MAX-1) return t(1,(u32)(i32)w);
    return tj(w);
  }
  L=(size_t)(p-s);
  t = L<sizeof buf ? buf : xmalloc(L+1);
  memcpy(t,s,L); t[L]=0;
  d=xstrtod(t);
  if(t!=buf) xfree(t);
  return t2(d);
}

/* [ ... ]: items collect in a growable array; an all-numeric array becomes a
   typed vector of its widest type, null as that type's null */
static K jarr_(jp *j) {
  K *v,e,r; u64 n=0,c=16,i; int hi=0,hj=0,hf=0,ho=0;
  ++j->p;
  jws_(j);
  if(j->p<j->e && *j->p==']') { ++j->p; return tn(0,0); }
  v=xmalloc(c*sizeof(K));
  for(;;) {
    e=jval_(j);
    if(E(e)) goto fail;
    if(n==c) { c<<=1; v=xrealloc(v,c*sizeof(K)); }
    v[n++]=e;
    switch(s(e)?-1:T(e)) { case 1: hi=1; break; case 8: hj=1; break; case 2: hf=1; break; case 6: break; default: ho=1; }
    jws_(j);
    if(j->p>=j->e) { e=KERR_PARSE; goto fail; }
    if(*j->p==',') { ++j->p; continue; }
    if(*j->p==']') { ++j->p; break; }
    e=KERR_PARSE; goto fail;
  }
  if(!ho && (hi||hj||hf)) {
    if(hf) { double *pr; r=tn(2,(i64)n); pr=px(r);
      for(i=0;i<n;i++) { K x=v[i]; pr[i]= T(x)==2?fk(x) : T(x)==1?(double)ik(x) : T(x)==8?(double)jk(x) : NAN; _k(x); } }
    else if(hj) { i64 *pr; r=tn(8,(i64)n); pr=px(r);
      for(i=0;i<n;i++) { K x=v[i]; pr[i]= T(x)==8?jk(x) : T(x)==1?(i64)ik(x) : J_NULL; _k(x); } }
    else { i32 *pr; r=tn(1,(i64)n); pr=px(r);
      for(i=0;i<n;i++) { K x=v[i]; pr[i]= T(x)==1?ik(x) : INT32_MIN; } }
    xfree(v);
    return r;
  }
  r=tn(0,(i64)n);
  memcpy(px(r),v,n*sizeof(K));
  xfree(v);
  return r;
fail:
  for(i=0;i<n;i++) _k(v[i]);
  xfree(v);
  return e;
}

static K jobj_(jp *j) {
  K d=dnew(),e,v; char *ks; u64 kn;
  ++j->p;
  jws_(j);
  if(j->p<j->e && *j->p=='}') { ++j->p; return d; }
  for(;;) {
    jws_(j);
    if(j->p>=j->e || *j->p!='"') { e=KERR_PARSE; goto fail; }
    e=jstr_(j,&ks,&kn); if(E(e)) goto fail;
    if(kn!=strlen(ks) || !dkeyok(ks)) { xfree(ks); e=KERR_DOMAIN; goto fail; }  /* a key must be a name (dict.c) */
    jws_(j);
    if(j->p>=j->e || *j->p!=':') { xfree(ks); e=KERR_PARSE; goto fail; }
    ++j->p;
    v=jval_(j);
    if(E(v)) { xfree(ks); e=v; goto fail; }
    e=dset(d,sp(ks),v); _k(v); xfree(ks);   /* a repeated key: last wins */
    if(E(e)) goto fail;
    jws_(j);
    if(j->p>=j->e) { e=KERR_PARSE; goto fail; }
    if(*j->p==',') { ++j->p; continue; }
    if(*j->p=='}') { ++j->p; return d; }
    e=KERR_PARSE; goto fail;
  }
fail:
  _k(d);
  return e;
}

static K jval_(jp *j) {
  K r;
  jws_(j);
  if(j->p>=j->e) return KERR_PARSE;
  switch(*j->p) {
  case '{': case '[':
    if(++j->d>JDEPTH || (!(j->d&7)&&stack_low())) return KERR_STACK;
    r = *j->p=='{' ? jobj_(j) : jarr_(j);
    --j->d;
    return r;
  case '"': { char *s; u64 n; K e=jstr_(j,&s,&n); if(E(e)) return e; return tnv(3,(i64)n,s); }
  case 't': if(j->e-j->p>=4 && !memcmp(j->p,"true",4)) { j->p+=4; return t(1,1); } return KERR_PARSE;
  case 'f': if(j->e-j->p>=5 && !memcmp(j->p,"false",5)) { j->p+=5; return t(1,0); } return KERR_PARSE;
  case 'n': if(j->e-j->p>=4 && !memcmp(j->p,"null",4)) { j->p+=4; return null; } return KERR_PARSE;
  default: return jnum_(j);
  }
}

static K jparse_(char *p, u64 n) {
  jp j; K r;
  j.p=p; j.e=p+n; j.d=0;
  r=jval_(&j);
  if(E(r)) return r;
  jws_(&j);
  if(j.p<j.e) { _k(r); return KERR_PARSE; }   /* trailing text */
  return r;
}

/* --------------------------------------------------------------- write */
typedef struct { char *b; size_t n,c; i32 ind; } jb;   /* ind: \j spaces per level, 0 = compact */

static void jput_(jb *o, const char *s, size_t n) {
  if(o->n+n+1>o->c) { while(o->n+n+1>o->c) o->c<<=1; o->b=xrealloc(o->b,o->c); }
  memcpy(o->b+o->n,s,n); o->n+=n;
}
static void jputc_(jb *o, char c) { jput_(o,&c,1); }
static void jnl_(jb *o, i32 d) {   /* a line break and d levels of indent (nothing when compact) */
  if(!o->ind) return;
  jputc_(o,'\n'); for(i32 i=0;i<d*o->ind;i++) jputc_(o,' ');
}
static void jputs_(jb *o, const char *s, size_t n) {   /* a string literal */
  static const char hx[]="0123456789abcdef";
  size_t i; char u[6]={'\\','u','0','0',0,0};
  jputc_(o,'"');
  for(i=0;i<n;i++) {
    unsigned char c=(unsigned char)s[i];
    switch(c) {
    case '"':  jput_(o,"\\\"",2); break;
    case '\\': jput_(o,"\\\\",2); break;
    case '\n': jput_(o,"\\n",2); break;
    case '\r': jput_(o,"\\r",2); break;
    case '\t': jput_(o,"\\t",2); break;
    case '\b': jput_(o,"\\b",2); break;
    case '\f': jput_(o,"\\f",2); break;
    default:
      if(c<0x20) { u[4]=hx[c>>4]; u[5]=hx[c&15]; jput_(o,u,6); }
      else jputc_(o,(char)c);
    }
  }
  jputc_(o,'"');
}
static void jputi_(jb *o, i32 v) {
  char ds[32];
  if(v==INT32_MIN) { jput_(o,"null",4); return; }
  sprintf(ds,"%d",v); jput_(o,ds,strlen(ds));
}
static void jputj_(jb *o, i64 v) {
  char ds[32];
  if(v==J_NULL) { jput_(o,"null",4); return; }
  sprintf(ds,"%lld",(long long)v); jput_(o,ds,strlen(ds));
}
static void jputf_(jb *o, double f, int e32) {
  char ds[256];
  if(isnan(f)||isinf(f)) { jput_(o,"null",4); return; }
  if(e32) kfmte(ds,(float)f); else kfmtf(ds,f);
  jput_(o,ds,strlen(ds));
}

static K jwrite_(jb *o, K x, i32 d) {
  u64 i,nn; K e;
  if(0x80==s(x)) {
    K *pd=px(x),*pv=px(pd[1]); char **pk=px(pd[0]); nn=n(pd[0]);
    if(++d>JDEPTH || (!(d&7)&&stack_low())) return KERR_STACK;
    jputc_(o,'{');
    for(i=0;i<nn;i++) {
      if(i) jputc_(o,',');
      jnl_(o,d);
      jputs_(o,pk[i],strlen(pk[i])); jputc_(o,':'); if(o->ind) jputc_(o,' ');
      e=jwrite_(o,pv[i],d); if(E(e)) return e;
    }
    if(nn) jnl_(o,d-1);
    jputc_(o,'}');
    return null;
  }
  if(s(x)) return KERR_TYPE;
  switch(T(x)) {
  case 1: jputi_(o,ik(x)); break;
  case 8: jputj_(o,jk(x)); break;
  case 2: jputf_(o,fk(x),0); break;
  case 9: jputf_(o,(double)ek(x),1); break;
  case 3: { char c=(char)ck(x); jputs_(o,&c,1); break; }
  case 4: jputs_(o,sk(x),strlen(sk(x))); break;
  case 6: jput_(o,"null",4); break;
  case -3: jputs_(o,px(x),(size_t)n(x)); break;
  case -1: { i32 *p=px(x); jputc_(o,'['); for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jputi_(o,p[i]); } jputc_(o,']'); break; }
  case -8: { i64 *p=px(x); jputc_(o,'['); for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jputj_(o,p[i]); } jputc_(o,']'); break; }
  case -2: { double *p=px(x); jputc_(o,'['); for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jputf_(o,p[i],0); } jputc_(o,']'); break; }
  case -9: { float *p=px(x); jputc_(o,'['); for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jputf_(o,(double)p[i],1); } jputc_(o,']'); break; }
  case -4: { char **p=px(x); jputc_(o,'['); for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jputs_(o,p[i],strlen(p[i])); } jputc_(o,']'); break; }
  case  0: {
    K *p=px(x);
    if(++d>JDEPTH || (!(d&7)&&stack_low())) return KERR_STACK;
    jputc_(o,'[');
    for(i=0;i<n(x);i++) { if(i) jputc_(o,','); jnl_(o,d); e=jwrite_(o,p[i],d); if(E(e)) return e; }
    if(n(x)) jnl_(o,d-1);
    jputc_(o,']');
    break;
  }
  default: return KERR_TYPE;
  }
  return null;
}

K kj_(K x) {
  if(!s(x) && T(x)==-3) return jparse_(px(x),n(x));
  if(!s(x) && T(x)==3) { char c=(char)ck(x); return jparse_(&c,1); }
  return KERR_TYPE;
}

K jk_(K x) {
  jb o; K e;
  o.c=256; o.n=0; o.b=xmalloc(o.c); o.ind=jindent;
  e=jwrite_(&o,x,0);
  if(E(e)) { xfree(o.b); return e; }
  o.b[o.n]=0;
  return tnv(3,(i64)o.n,o.b);
}
