#include "fe.h"
#include <stdio.h>
#include <string.h>
#include "k.h"
#include "av.h"
#include "b.h"
#include "fn.h"
#include "io.h"

K fc(K f, K a, K x, char *av) {
  K r=0,e,p,f2,*pf,*pf2;
  static int d=0;
  if(++d>maxr || (!(d&7)&&stack_low())) { --d; _k(a); _k(x); return KERR_STACK; }
  if(av&&*av) r=avdo(k_(f),k_(a),k_(x),av);
  else {
    pf=px(f);
    f2=pf[0];
    pf2=px(f2);
    i32 i=n(f2)-1;
    r=fe(k_(pf2[i]),k_(a),k_(x),av);
    EC(r);
    for(i--;i>=0;--i) {
      p=fe(k_(pf2[i]),0,r,av);
      EC(p);
      r=p;
    }
  }
  _k(a); _k(x);
  --d;
  return r;
cleanup:
  _k(a); _k(x);
  --d;
  return e;
}

K fe(K f, K a, K x, char *av) {
  K r=3,f0,f1,*pf,*pr,t,*pt,xx,*pxx,fav=0;
  char *P=":+-*%&|<>=~.!@?#_^,$'/\\";
  char ff=0;
  int primitive=0;

  if(0x45==s(a)||0x45==s(x)) { _k(f); _k(a); _k(x); return KERR_TYPE; }

  if(0xd0==s(f)&&(!av||!*av)) {
    if(a) { _k(f); _k(a); _k(x); return KERR_VALENCE; }
    pf=px(f);
    a=k_(pf[0]);
    K f2=k_(pf[1]);
    _k(f); f=f2;
    if(0x81==s(x)) {
      if(nx!=1) { _k(f); _k(a); _k(x); return KERR_VALENCE; }
      K *px=px(x);
      K x2=k_(px[0]);
      _k(x); x=x2;
    }
  }

  /* a predefined k.core fn (0xc9/0xca/0xcb: gtime dv ssr ...) applied as a
     VALUE -- fetched from .r or any dict -- rather than as a lexer token
     (those are resolved by p.c reduce()/rpd before they get here).  Swap in
     a copy of its lambda so the 0xc3 cases below apply it. */
  if(0xc9==s(f)||0xca==s(f)||0xcb==s(f)) {
    K f2=fnpd(f);
    _k(f); f=f2;
    if(E(f2)) { _k(a); _k(x); return f2; }
  }

  if(0xc0==s(f)) {
    ff=ck(f)%32;
    primitive=1;
  }
  else if(!s(f) && f<32) {
    ff=(char)f;
    primitive=1;
  }
  else if(!s(f) && f<256 && f>32) {
    /* r19 primitive verbs are literal '+' '-' etc.  Do not derive an opcode
       from the low byte of an arbitrary K: function subtypes share their base
       tags with ordinary data, and an invalid wrapper payload used to make a
       noun or heap pointer look like whichever primitive its low byte named. */
    char *p=strchr(P,(char)f);
    if(p) {
      ff=(char)(p-P);
      primitive=1;
    }
  }

  if(!a) {
    switch(s(f)) {
    case 0xda: { /* (f;av) modified-verb wrapper, replaces 0xc1/0xc2/0xdb */
      K *pw=px(f); K wf=pw[0]; K wav=pw[1];
      char *avp=(T(wav)==-3 && n(wav)>0) ? (char*)px(wav) : "";
      if(0xc0==s(wf)) {
        if(0x81==s(x)) { _k(x); r=KERR_TYPE; }
        else {
          u32 c=ck(wf); int vi=c%32;
          if(vi<=20 && *avp) r=avdo(vi,a,x,avp);
          else { _k(x); r=KERR_PARSE; }
        }
      }
      else if(0xc7==s(wf)) {
        /* draw/x and friends -- builtin-dyad-as-modified-monad,
           replaces the old 0xdb monad path (b.c:67). */
        if(0x81==s(x)) { _k(x); r=KERR_TYPE; }
        else if(*avp=='\'') {
          /* An adverbed dyadic builtin applied to a single arg is
             under-applied -> project, PRESERVING the each adverb:
             0xd9(0xda(builtin,"'"), (x;)).  Mirrors the lambda path in
             fne_ (proj_av).  Routing through avdo would eachfe the bare
             builtin and drop the adverb, so `in'[1]` came back as `in[1]`
             and `("."in')` type-errored instead of projecting.  over/scan
             (`/`,`\`) stay on avdo below: `draw/x` etc. are real do/fold
             applications, not projections. */
          K t=tn(0,1); ((K*)px(t))[0]=x; /* x transferred into the plist */
          r=wrap_proj(k_(f),st(0x81,t)); /* retain f (the 0xda): fe _k's it at return */
        }
        else if(*avp) r=avdo(k_(wf),0,x,avp);
        else { _k(x); r=KERR_PARSE; }
      }
      else if(0xc6==s(wf)) {
        /* adverbed builtin MONAD held as a value -- g:sqrt/; g 16.0,
           (sin')1 2 3, sqrt\[16.0].  Route through avdo exactly like the
           0xc7 arm above; this branch was missing, so the value forms
           type-errored while the infix spelling (sqrt/16.0) worked. */
        if(0x81==s(x)&&n(x)==1) { K x2=k_(((K*)px(x))[0]); _k(x); x=x2; }
        if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
        else if(*avp) r=avdo(k_(wf),0,x,avp);
        else { _k(x); r=KERR_PARSE; }
      }
      else if(0xc3==s(wf) || 0xc5==s(wf) || 0xd9==s(wf)) {
        /* Pass 2a / 3b-5 / Pass 4: 0xda(lambda/proj/composition,
           av).  0xc4 retired -- only 0xc3 / 0xc5 / 0xd9 inners
           can appear here.  Wrap x into a 0x81 plist if not
           already, then dispatch
           through fne with the wrapper's av (fne routes 0xd9 via
           fapply, which peels the projection wrapper). */
        K xx;
        if(0x81==s(x)) xx=x;
        else { xx=tn(0,1); ((K*)px(xx))[0]=x; xx=st(0x81,xx); }
        r=fne(k_(wf),xx,avp);
      }
      else if(0xdc==s(wf)) { /* 0xda(2:-linked fn, av): e.g. (add')[a;b] */
        K xx;
        if(0x81==s(x)) xx=x;
        else { xx=tn(0,1); ((K*)px(xx))[0]=x; xx=st(0x81,xx); }
        r=avdo(k_(wf),0,xx,avp);
      }
      else { _k(x); r=KERR_TYPE; }
      break;
    }
    case 0xc3: /* 0xc4 retired in Pass 4 */
      if(0x81==s(x)) r=fne(k_(f),x,av);
      else {
        xx=tn(0,1); pxx=px(xx);
        pxx[0]=x;
        r=fne(k_(f),xx,av);
      }
      break;
    case 0xdc: { /* 2:-linked C function.  Pack into a plist, then project on
                    short-fill / elided (inull) slots -- producing a 0xd9
                    wrapper that fapply completes later -- or call when the
                    plist holds exactly `valence` bound args. */
      if(av&&*av) { /* adverbs route through avdo, like the 0xc7 arm below --
                       fapply forwards a preserved ' onto a completed 0xdc
                       projection through here (add[3;]'4).  The old arm
                       KERR_VALENCE'd AND _k(f)'d before breaking into the
                       epilogue's unconditional _k(f): a double release that
                       segfaulted after a few repetitions. */
        if(0x81==s(x)) xx=x;
        else { xx=tn(0,1); pxx=px(xx); pxx[0]=x; xx=st(0x81,xx); }
        r=avdo(k_(f),0,xx,av);
        break;
      }
      if(0x81==s(x)) xx=x;
      else { xx=tn(0,1); pxx=px(xx); pxx[0]=x; xx=st(0x81,xx); }
      pf=px(f); i32 lval=ik(pf[1]);
      u64 ln=n(xx),inc=0; { K *pe=px(xx); i(ln,if(pe[i]==inull)++inc) }
      if((i32)ln<lval || (i32)(ln-inc)<lval) r=wrap_proj(k_(f),xx);
      else r=linkcall(k_(f),xx);
      break;
    }
    case 0xd9: { /* Issue #2 Pass 3a: (f;args) projection wrapper.
                    Route through fapply, which peels the wrapper,
                    merges new x into args (filling inulls or
                    appending), and recurses. The 0x81 plist
                    wrapping convention here matches case 0xc3:
                    fapply expects the new args as a plist. */
      K xx2;
      if(0x81==s(x)) xx2=x;
      else { xx2=tn(0,1); ((K*)px(xx2))[0]=x; xx2=st(0x81,xx2); }
      r=fapply(k_(f),xx2,av);
      break;
    }
    case 0xc6:
      if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(f,0,k_(px[0])); _k(x); }
      else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
      else r=builtin(f,0,x);
      break;
    case 0xc7: /* 0xc8 retired in Pass 4 -- replaced by 0xd9. */
      if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(f,0,k_(px[0])); _k(x); }
      else if(0x81==s(x)&&n(x)==2)  { K *px=px(x); r=builtin(f,k_(px[0]),k_(px[1])); _k(x); }
      else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
      else r=builtin(f,0,x);
      break;
    case 0xcc: /* file verb (0:,1:,...) applied as a value: x is a 0x81
                  arg-plist from bracket/./trap apply -- unpack like 0xc7
                  so both the monadic (1-arg) and dyadic (2-arg) forms
                  dispatch.  Bare x (juxtaposition) stays monadic. */
      if(0x81==s(x)&&n(x)==1) { K *px=px(x); r=builtin(f,0,k_(px[0])); _k(x); }
      else if(0x81==s(x)&&n(x)==2) { K *px=px(x); r=builtin(f,k_(px[0]),k_(px[1])); _k(x); }
      else if(0x81==s(x)) { _k(x); r=KERR_VALENCE; }
      else r=builtin(f,0,x);
      break;
    case 0xc5:
      r=fc(f,0,x,av);
      break;
    /* 0xd4/0xd5/0xd6 retired in Pass 4 -- replaced by 0xd9. */
    case 0xd0:
      r=avdo(k_(f),0,x,av);
      break;
    case 0xd7:
      pf=px(f);
      f0=k_(pf[0]);
      f1=k_(pf[1]);
      /* Pass 2b-step-4: peel 0xda(c3/c4, av) inner. Replace f1 with
         the bare inner lambda (taking ownership), set fav from the
         wrapper. */
      if(0xda==s(f1)) {
        K *pw=px(f1);
        fav=pw[1];
        K wf_owned=k_(pw[0]);
        _k(f1); f1=wf_owned;
      }
      else if(0xc3==s(f1)) ; /* Pass 6 cleanup: no inner av slot anymore */
      else if(0xd9==s(f1)) ; /* Pass 3b-5: fav already set from 0xda peel above */
      else { _k(x); r=KERR_PARSE; break; }
      /* 0xc4 retired in Pass 4 */
      char *favp=-3==T(fav)?(char*)px(fav):"";
      if((!strcmp(favp,"/") || !strcmp(favp,"\\"))) {
        K f1_=kcp(f1); _k(f1); f1=f1_;
        if(E(f1)) { _k(f0); _k(x); r=f1; break; }
        if(s(f0)==0 && (T(f0)==1||T(f0)==8) && !strcmp(favp,"/")) { r=overmonadn(f1,f0,x,""); quiet=0; }
        else if(ISF(f0) && ik(val(f0))==1 && !strcmp(favp,"/")) { r=overmonadb(f1,f0,x,""); quiet=0; }
        else if(s(f0)==0 && (T(f0)==1||T(f0)==8) && !strcmp(favp,"\\")) { r=scanmonadn(f1,f0,x,""); quiet=0; }
        else if(ISF(f0) && ik(val(f0))==1 && !strcmp(favp,"\\")) { r=scanmonadb(f1,f0,x,""); quiet=0; }
        else { _k(f0); _k(f1); _k(x); r=KERR_TYPE; break; }
      }
      else { _k(f0); _k(f1); _k(x); r=KERR_TYPE; }
      break;
    /* 0xd5/0xd6 retired in Pass 4 -- replaced by 0xd9. */
    default:
      if(!primitive) {
        _k(x);
        r=KERR_TYPE;
      }
      else if(!ff) {
        if(0x81==s(x)) { _k(f); _k(x); return KERR_TYPE; }
        else r=x;
      }
      else if(ff>0&&ff<32) {
        if(0x81==s(x)) {
          K *px=px(x);
          if(nx==1) r=k(ff,0,k_(px[0]));
          else if(nx==2) r=k(ff,k_(px[0]),k_(px[1]));
          else if(nx==3&&ff==13) r=kamendi3(k_(px[0]),k_(px[1]),k_(px[2]));
          else if(nx==4&&ff==13) r=kamendi4(k_(px[0]),k_(px[1]),k_(px[2]),k_(px[3]));
          else if(nx==3&&ff==11) r=kamend3(k_(px[0]),k_(px[1]),k_(px[2]));
          else if(nx==4&&ff==11) r=kamend4(k_(px[0]),k_(px[1]),k_(px[2]),k_(px[3]));
          else if(nx==3&&ff==16) r=kslide(k_(px[1]),k_(px[0]),k_(px[2]),"");
          else r=KERR_VALENCE;
          _k(x);
        }
        else if(av && *av) {
          int n=strlen(av);
          if(n>32) { _k(a); _k(x); return KERR_LENGTH; }
          if(n==1) {
            char *p=strchr(P,*av);
            if(!p) { _k(x); r=KERR_TYPE; }
            else r=k((int)(p-P),ff,x);
          }
          else {
            char av2[256];
            snprintf(av2,sizeof(av2),"%s",av);
            av2[n-1]=0;
            r=avdo(ff,0,x,av2);
          }
        }
        else r=k(ff,0,x);
      }
      else _k(x);
    }
  }
  else {
    switch(s(f)) {
    case 0xda: { /* (f;av) modified-verb wrapper, replaces 0xc1/0xc2/0xdb */
      if(a==inull || x==inull) {
        /* An elided slot in a dyadic adverb application -- e.g. f/[5;]
           or f/[;0] -- projects: repack (a;x) (one slot is the inull
           hole) and wrap the adverbed verb as a 0xd9 projection.  A
           later apply fills the hole via merge_args and runs the adverb,
           so f/[5;] behaves as {x f/y}[5;] (a do/while waiting on y). */
        K t=tn(0,2); K *pt=px(t);
        pt[0]=a; pt[1]=x;             /* ownership of a,x transfers into plist */
        r=wrap_proj(k_(f),st(0x81,t)); /* retain f: fe _k's it below */
      }
      else {
        K *pw=px(f); K wf=pw[0]; K wav=pw[1];
        char *avp=(T(wav)==-3 && n(wav)>0) ? (char*)px(wav) : "";
        if(0xc0==s(wf)) {
          u32 c=ck(wf); int vi=c%32;
          if(vi<=20 && *avp) r=avdo(vi,a,x,avp);
          else { _k(a); _k(x); r=KERR_PARSE; }
        }
        else if(0xc7==s(wf)) {
          /* dyadic juxtaposition through a draw/-style wrapper
             (a (draw/) x). Pass 1b doesn't normally produce this
             shape since bc only wraps when no left is present, but
             plumbing exists for completeness so juxtaposition
             routes don't trip the type-error fallback. */
          if(*avp) r=avdo(k_(wf),a,x,avp);
          else { _k(a); _k(x); r=KERR_PARSE; }
        }
        else if(0xc6==s(wf)) {
          /* Issue #2 Pass 6: dyadic juxt through 0xda(0xc6,"/"|"\\")
             -- e.g. `5 sin/ 2` or peeled f=0xd0(5,0xda(sin,"/"))
             monadic apply.  do-n-times (atom int seed) or do-while
             (monadic-function seed), with sin as the inner monad. */
          if(!strcmp(avp,"/") || !strcmp(avp,"\\")) {
            if(s(a)==0 && (T(a)==1||T(a)==8) && !strcmp(avp,"/")) r=overmonadn(k_(wf),a,x,"");
            else if(s(a)==0xc3 && !strcmp(avp,"/")) r=overmonadb(k_(wf),a,x,"");
            else if(s(a)==0 && (T(a)==1||T(a)==8) && !strcmp(avp,"\\")) r=scanmonadn(k_(wf),a,x,"");
            else if(s(a)==0xc3 && !strcmp(avp,"\\")) r=scanmonadb(k_(wf),a,x,"");
            else { _k(a); _k(x); r=KERR_TYPE; }
          }
          else { _k(a); _k(x); r=KERR_PARSE; }
        }
        else if(0xc3==s(wf) || 0xc5==s(wf) || 0xd9==s(wf)) {
          /* Pass 2a / 3b-5 / Pass 4: dyadic apply of
             (lambda/proj/composition/projection, av).
             Pack a,x into a 2-elt plist and dispatch through fne
             (fne routes 0xd9 through fapply for projection peel).
             Wrap as 0x81 so fne's `0x81==s(x) && n(x)==2` dyadic
             dispatch fires; without the subtype tag fne falls to
             monadic avdo with the whole pair, losing the
             each-pair / each-param semantics for a 2-arg lambda
             or projection (e.g. `f[;2;]'[3 4;5 6]`). */
          K xx=tn(0,2); K *pxx=px(xx);
          pxx[0]=a; pxx[1]=x;
          xx=st(0x81,xx);
          r=fne(k_(wf),xx,avp);
        }
        else if(0xdc==s(wf)) { /* dyadic adverb apply of a 2:-linked fn (a add' x) */
          r=avdo(k_(wf),a,x,avp);
        }
        else { _k(a); _k(x); r=KERR_TYPE; }
      }
      break;
    }
    case 0xc3: /* 0xc4 retired in Pass 4 */
      xx=tn(0,2); pxx=px(xx);
      pxx[0]=a; pxx[1]=x;
      r=fne(k_(f),xx,av);
      break;
    case 0xdc: /* 2:-linked C function, dyadic apply (valence 2), or project
                  to a 0xd9 wrapper when a slot is elided (e.g. add[3;]). */
      if(av&&*av) { r=avdo(k_(f),a,x,av); break; } /* like the 0xc7 arm; the
                       old _k(f)+break double-released f (epilogue frees it) */
      xx=tn(0,2); pxx=px(xx);
      pxx[0]=a; pxx[1]=x;
      if(a==inull || x==inull) r=wrap_proj(k_(f),st(0x81,xx)); /* k_: fe _k's f below */
      else r=linkcall(k_(f),xx);
      break;
    case 0xd9: { /* Issue #2 Pass 3a dyadic apply: (f;args) wrapper.
                    Pack {a,x} into 0x81 plist and route through
                    fapply for inull-fill / append. */
      K xx2=tn(0,2); K *pxx2=px(xx2);
      pxx2[0]=a; pxx2[1]=x;
      xx2=st(0x81,xx2);
      r=fapply(k_(f),xx2,av);
      break;
    }
    case 0xc7: case 0xcd: case 0xcc:
      if(a==inull || x==inull) {
        /* Issue #2 Pass 3b-1: produce 0xd9(verb, args_w_inulls).
           Replaces legacy 0xd4 with the canonical (f;args)
           projection wrapper. fapply()'s 0xd9 peel handles
           inull-fill on subsequent application. */
        t=tn(0,2); pt=px(t);
        pt[0]=a;
        pt[1]=x;
        K e2=proj_own_args(t);   /* projected args are VALUES -- see fn.c */
        if(e2) { _k(t); r=e2; break; }
        r=tn(0,2); pr=px(r);
        pr[0]=f;
        pr[1]=st(0x81,t);
        r=st(0xd9,r);
      }
      else if(av && *av) r=avdo(k_(f),a,x,av);
      else r=builtin(f,a,x);
      break;
    case 0xc5:
      if(a==inull || x==inull) {
        /* Issue #2 Pass 3b-1: produce 0xd9 (was 0xd4). */
        t=tn(0,2); pt=px(t);
        pt[0]=a;
        pt[1]=x;
        K e2=proj_own_args(t);   /* projected args are VALUES -- see fn.c */
        if(e2) { _k(t); r=e2; break; }
        r=tn(0,2); pr=px(r);
        pr[0]=k_(f);
        pr[1]=st(0x81,t);
        r=st(0xd9,r);
      }
      else r=fc(f,a,x,av);
      break;
    /* Issue #2 Pass 6: 0xc6 (5 sin\2) retired -- post-pass-6 the
       lexer never produces 0xc6 names with embedded adverbs, so the
       (sin,"/") pair lands as 0xda(0xc6,"/") and is handled by the
       case 0xda block above instead of falling through here. */
    default:
      if(!primitive) { _k(a); _k(x); r=KERR_TYPE; }
      else if(!ff) { _k(a); r=x; }
      else if(ff>0&&ff<32) {
        if(a==inull || x==inull) {
          /* Issue #2 Pass 3b-1: produce 0xd9 (was 0xd4). The verb
             is a 1-byte primitive; encode as t(1,...) of a 0xc0
             tagged scalar so the projection wrapper's f slot is
             a real K (and val/print cases work). */
          t=tn(0,2); pt=px(t);
          pt[0]=a;
          pt[1]=x;
          K e2=proj_own_args(t);   /* projected args are VALUES -- see fn.c */
          if(e2) { _k(t); r=e2; break; }
          r=tn(0,2); pr=px(r);
          pr[0]=t(1,st(0xc0,(u32)ff+32));
          pr[1]=st(0x81,t);
          r=st(0xd9,r);
        }
        else r=k(ff,a,x);
      }
      else { _k(a); _k(x); }
    }
  }
  _k(f);
  return r;
}
