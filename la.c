#include "la.h"
#include <float.h>
#include "av.h"

/* svd stuff based on numerical recipes */
static K svdcmp(double **a, int m, int n, double *w, double **v, double *t) {
  int flag,i,its,j,jj,k,l,nm,nm1=n-1,mm1=m-1;
  double c=0.0,f,h,s,x,y,z;
  double anorm=0.0,g=0.0,scale=0.0;
  double *rv1=t;

  /* householder reduction to bidigonal form */
  for(i = 0; i < n; i++) {
    l = i + 1;
    rv1[i] = scale * g;
    g = s = scale = 0.0;
    if(i < m) {
      for(k = i; k < m; k++) scale += fabs(a[k][i]);
      if(scale) {
        for(k = i; k < m; k++) {
          a[k][i] /= scale;
          s += a[k][i] * a[k][i];
        }
        f = a[i][i];
        g = -copysign(sqrt(s), f);
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
      for(k = l; k < n; k++) scale += fabs(a[i][k]);
      if(scale) {
        for(k = l; k < n; k++) {
          a[i][k] /= scale;
          s += a[i][k] * a[i][k];
        }
        f = a[i][l];
        g = -copysign(sqrt(s), f);
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
    anorm = fmax(anorm, (fabs(w[i]) + fabs(rv1[i])));
  }

  /* accumulation of right-hand transformations */
  for(i = n - 1; i >= 0; i--) {
    if(i < nm1) {
      if(g) {
        /* f division to avoid possible underflow */
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
  /* accumulation of left-hand transformations */
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
      for (l = k; l >= 0; l--) {
        nm = l - 1;
        if (fabs(rv1[l]) + anorm == anorm) { flag = 0; break; }
        if (l > 0 && fabs(w[nm]) + anorm == anorm) break;
      }
      if(flag) {
        //c = 0.0;  /* cancellation of rv1[l], if l>1 */
        s = 1.0;
        for(i = l; i <= k; i++) {
          f = s * rv1[i];
          if(fabs(f) + anorm != anorm) {
            g = w[i];
            h = hypot(f, g);
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
      g = hypot(f, 1.0);
      /* next QR transformation */
      f = ((x - z) * (x + z) + h * ((y / (f + copysign(g, f))) - h)) / x;
      c = s = 1.0;
      for(j = l; j <= nm; j++) {
        i = j + 1;
        g = rv1[i];
        y = w[i];
        h = s * g;
        g = c * g;
        z = hypot(f, h);
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
        z = hypot(f, h);
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

K lsq_(K a, K x) {
  K e=0, svdres=0, *psv, U=0, S=0, V=0, Sinv=0, Ut=0, Uy=0, Su=0, Vy=0, r=0, t=0;
  u64 i,j,n;

  if(s(a)||s(x)) return kerror("type");
  if(tx) return kerror("type");
  if(ta&&ta!=-1&&ta!=-2) return kerror("type");

  K x0=k_(x);
  x=k(1,0,k_(x)); /* transpose */
  if(E(x)) { _k(x0); return x; }
  if(tx) { _k(x0); _k(x); return kerror("length"); } // x must be a matrix

  if(!na||!nx) { _k(x); _k(x0); return kerror("length"); }

  if(ta==0) {
    _k(x);
    K *pak=px(a);
    r=tn(0,na);
    K *prk=px(r);
    for(i=0;i<na;++i) {
      t=lsq_(pak[i],x0);
      if(E(t)) { _k(r); _k(x0); return t; }
      prk[i]=t;
    }
    _k(x0);
    return r;
  }
  _k(x0);

  if(ta!=-1&&ta!=-2) { _k(x); return kerror("type"); }

  /* design matrix: x (mxn) */
  K *pxk = px(x);
  u64 m = n(x);
  if(T(pxk[0])>0) { _k(x); return kerror("length"); }
  u64 ncols = n(pxk[0]);
  for(i=0;i<m;i++){
    K row = pxk[i];
    if(s(row) || (T(row)!=-1 && T(row)!=-2)) { _k(x); return kerror("type"); }
    if((u64)n(row)!=ncols) { _k(x); return kerror("length"); }
  }

  /* rhs: a (mxk) */
  if(T(a)>0 || T(a)<-2) { _k(x); return kerror("type"); }
  if((u64)n(a)!=m) { _k(x); return kerror("length"); }

  /* svd of design matrix */
  svdres = svd_(x);
  if(E(svdres)) { _k(x); return svdres; }
  psv = px(svdres);
  U = k_(psv[0]); S = k_(psv[1]);
  V = k(1,0,k_(psv[2])); /* V' -> V */
  _k(svdres);

  n = n(S);

  /* build Sinv */
  Sinv = tn(0,n);
  int any_zero = 0;
  double TOL = 1e-12;
  for(i=0;i<n;i++) {
    K row = tn(2,n);
    double *p = px(row);
    for(j=0;j<n;j++) p[j]=0.0;
    double s = ((double*)px(((K*)px(S))[i]))[i];
    if(fabs(s) <= TOL) {
      p[i] = 0.0;
      any_zero = 1;
    }
    else p[i] = 1.0 / s;
    ((K*)px(Sinv))[i] = row;
  }
  _k(S);

  /* if any singular value is approx 0, return all 0n */
  if(any_zero) {
    r=tn(2,n);
    double *pr=px(r);
    i(n,pr[i]=NAN)
    _k(Sinv); _k(U); _k(V); _k(x);
    return r;
  }

  K mul=t(4,st(0xc7,sp("dot")));
  Ut=k(1,0,U); EC(Ut);
  Uy=avdo(mul,Ut,k_(a),"\\"); EC(Uy);
  Su=avdo(mul,Sinv,Uy,"\\"); EC(Su);
  Vy=avdo(mul,V,Su,"\\"); EC(Vy);
  r=knorm(Vy);

  /* zero-clamp tiny noise in final result */
  double EPS = DBL_EPSILON * 1e6;
  if(r && !s(r) && T(r)==-2) {
    double *pr = px(r);
    for(u64 i=0; i<n(r); i++)
      if(fabs(pr[i]) < EPS) pr[i] = 0.0;
  }

cleanup:
  _k(x);
  if(e) return e;
  return r;
}

/* canonicalize_svd(): normalize svd output for deterministic results across platforms */
static void canonicalize_svd(K U, K S, K V) {
  u64 i,j,k,m,n;
  double tol=1e-12;

  n=n(S);
  m=n(U);

  /* 1. sort singular values descending, reorder columns of U and V */
  for(i=0;i<n-1;i++){
    u64 max_i=i;
    double max_s=((double*)px(((K*)px(S))[i]))[i];
    for(j=i+1;j<n;j++){
      double sj=((double*)px(((K*)px(S))[j]))[j];
      if(sj>max_s){ max_s=sj; max_i=j; }
    }
    if(max_i!=i){
      /* swap singular values */
      double *si=px(((K*)px(S))[i]);
      double *sj=px(((K*)px(S))[max_i]);
      double tmp=si[i]; si[i]=sj[i]; sj[i]=tmp;

      /* swap columns in U */
      for(k=0;k<m;k++){
        double *pu=px(((K*)px(U))[k]);
        double tmpu=pu[i]; pu[i]=pu[max_i]; pu[max_i]=tmpu;
      }

      /* swap columns in V */
      for(k=0;k<n;k++){
        double *pv=px(((K*)px(V))[k]);
        double tmpv=pv[i]; pv[i]=pv[max_i]; pv[max_i]=tmpv;
      }
    }
  }

  /* 2. canonicalize signs: make first nonzero element of each U column positive */
  for(j=0;j<n;j++){
    double sgn=1.0;
    for(i=0;i<m;i++){
      double val=((double*)px(((K*)px(U))[i]))[j];
      if(fabs(val)>tol){ sgn=(val<0.0)?-1.0:1.0; break; }
    }
    if(sgn<0.0){
      for(i=0;i<m;i++) ((double*)px(((K*)px(U))[i]))[j]*=-1.0;
      for(i=0;i<n;i++) ((double*)px(((K*)px(V))[i]))[j]*=-1.0;
    }
  }

  /* 3. canonicalize nullspace (degenerate zero singular values) */
  for(j=0;j<n;j++){
    double s=((double*)px(((K*)px(S))[j]))[j];
    if(fabs(s)<tol){
      for(i=0;i<m;i++) ((double*)px(((K*)px(U))[i]))[j]=(i==j)?1.0:0.0;
      for(i=0;i<n;i++) ((double*)px(((K*)px(V))[i]))[j]=(i==j)?1.0:0.0;
    }
  }
}

/* (U;S;+V) = svd A */
K svd_(K x) {
  K U=0,S=0,V=0,e=0,z=0;
  double **a=0, **v=0, *w=0, *rv1=0, *A0=0;
  u32 m,n;                 /* original rows, cols of A */
  u64 i,j;

  /* 1) validate rectangular numeric matrix (list of rows) */
  if(s(x) || tx) return kerror("type");
  if(!nx) return kerror("length");
  K *pxk = px(x);
  m = n(x);
  if(s(pxk[0]) || (T(pxk[0])!=0 && T(pxk[0])!=-1 && T(pxk[0])!=-2)) return kerror("type");
  n = n(pxk[0]);
  for(i=0;i<m;i++) {
    K row = pxk[i];
    if(s(row) || (T(row)!=0 && T(row)!=-1 && T(row)!=-2)) return kerror("type");
    if(n(row)!=n) return kerror("length");
  }

  /* keep a copy of original A (row-major) for alignment checks */
  A0 = xmalloc((size_t)m * n * sizeof(double));
  for(i=0;i<m;i++) {
    K row = k_(pxk[i]);
    if(T(row)==0) row=k(3,t2(1.0),row);
    if(E(row)) { xfree(A0); return kerror("type"); }
    int isf = (T(row)==-2);
    for(j=0;j<n;j++)
      A0[(size_t)i*n + j] = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
    _k(row);
  }

  /* 2) prepare matrix for svdcmp: ensure rows >= cols by transposing if wide */
  int transposed = (m < n);
  u32 mm = transposed ? n : m;   /* rows for svdcmp input */
  u32 nn = transposed ? m : n;   /* cols for svdcmp input */

  a   = xmalloc(mm*sizeof(*a));
  a[0]= xmalloc((size_t)mm*nn*sizeof(**a));
  for(i=0;i<mm;i++) a[i] = a[0] + (size_t)i*nn;

  v   = xmalloc(nn*sizeof(*v));
  v[0]= xmalloc((size_t)nn*nn*sizeof(**v));
  for(i=0;i<nn;i++) v[i] = v[0] + (size_t)i*nn;

  w   = xmalloc(nn*sizeof(*w));
  rv1 = xmalloc(nn*sizeof(*rv1));

  if(!transposed) {
    /* a = A (mxn) */
    for(i=0;i<m;i++) {
      K row = k_(pxk[i]);
      if(T(row)==0) row=k(3,t2(1.0),row);
      if(E(row)) return kerror("type");
      int isf = (T(row)==-2);
      for(j=0;j<n;j++)
        a[i][j] = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
      _k(row);
    }
  }
  else {
    /* a = A' (nxm) */
    for(i=0;i<n;i++) {
      for(j=0;j<m;j++) {
        K row = k_(pxk[j]);
        if(T(row)==0) row=k(3,t2(1.0),row);
        if(E(row)) return kerror("type");
        int isf = (T(row)==-2);
        a[i][j] = isf ? ((double*)px(row))[i] : fi(((int*)px(row))[i]);
        _k(row);
      }
    }
  }

  /* 3) svd on mmxnn with mm >= nn */
  z = svdcmp(a,(int)mm,(int)nn,w,v,rv1);
  if(E(z)) { e=z; goto cleanup; }

  /* 4) nonnegative singular values + same sign in U/V columns */
  for(i=0;i<nn;i++) {
    if(w[i] < 0.0) {
      w[i] = -w[i];
      for(j=0;j<mm;j++) a[j][i] = -a[j][i];
      for(j=0;j<nn;j++) v[j][i] = -v[j][i];
    }
  }

  /* 5) sort singular values (sigma) descending; permute U (=a) and V columns */
  for(i=0;i+1<nn;i++) {
    u32 p=i;
    for(j=i+1;j<nn;j++) if (w[j] > w[p]) p=j;
    if(p!=i) {
      double tmp=w[i]; w[i]=w[p]; w[p]=tmp;
      for(u64 r=0;r<mm;r++){ double t=a[r][i]; a[r][i]=a[r][p]; a[r][p]=t; }
      for(u64 r=0;r<nn;r++){ double t=v[r][i]; v[r][i]=v[r][p]; v[r][p]=t; }
    }
  }

  /* 6) deterministic sign per column: dominant entry in each U_col positive */
  for(i=0;i<nn;i++) {
    double maxabs=0.0, maxval=0.0;
    for(j=0;j<mm;j++) {
      double val=a[j][i], av=fabs(val);
      if(av>maxabs){ maxabs=av; maxval=val; }
    }
    if(maxval < 0.0) {
      for(j=0;j<mm;j++) a[j][i] = -a[j][i];
      for(j=0;j<nn;j++) v[j][i] = -v[j][i];
    }
  }

  /* 6.5) zero-clamp tiny noise in U (=a), S (=w), and V */
  //double EPS = pow(10.0, -(double)precision);  /* same as QR/LU */
  double EPS = DBL_EPSILON * 1e6;

  /* clamp U (mxn) */
  for(i=0; i<mm; i++)
    for(j=0; j<nn; j++)
      if(fabs(a[i][j]) < EPS) a[i][j] = 0.0;

  /* clamp singular values */
  for(i=0; i<nn; i++)
    if(fabs(w[i]) < EPS) w[i] = 0.0;

  /* clamp V (nxn) */
  for(i=0; i<nn; i++)
    for(j=0; j<nn; j++)
      if(fabs(v[i][j]) < EPS) v[i][j] = 0.0;


  /* 7) build outputs (U mxn, S nxn diag, V nxn) so USV' == A */
  U = tn(0, m);
  S = tn(0, n);
  V = tn(0, n);

  if(!transposed) {
    /* tall/square: U_out = a (mxn), S_out diag=w (nxn), V_out = v (nxn) (pad when needed) */
    for(i=0;i<m;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) p[j] = (j<nn) ? a[i][j] : 0.0;
      ((K*)px(U))[i]=row;
    }
    for(i=0;i<n;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) p[j] = (i==j && i<nn) ? w[i] : 0.0;
      ((K*)px(S))[i]=row;
    }
    for(i=0;i<n;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) p[j] = (i<nn && j<nn) ? v[i][j] : (i==j ? 1.0 : 0.0);
      ((K*)px(V))[i]=row;
    }
  }
  else {
    /* wide: svd(A') gave A' = U_t (n x m) * diag(w) (m x m) * V_t' (m x m)
       => A = V_t (m x m) * diag(w) (m x m) * U_t' (m x n)
       We export:
         U_out (m x n): first m columns = V_t, rest zeros
         S_out (n x n): diag[0..m-1] = w
         V_out (n x n): first m columns = U_t (n x m); remaining columns = identity basis
    */
    /* U_out */
    for(i=0;i<m;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) p[j] = (j<nn) ? v[i][j] : 0.0;   /* V_t into first m columns */
      ((K*)px(U))[i]=row;
    }
    /* S_out */
    for(i=0;i<n;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) p[j]=0.0;
      if(i < nn) p[i] = w[i];
      ((K*)px(S))[i]=row;
    }
    /* V_out */
    for(i=0;i<n;i++) {
      K row = tn(2,n); double *p=(double*)px(row);
      for(j=0;j<n;j++) {
        if(j < nn) p[j] = a[i][j];       /* U_t (which is 'a' after svdcmp) */
        else p[j] = (j==i ? 1.0 : 0.0);  /* simple completion (irrelevant where sigma = 0) */
      }
      ((K*)px(V))[i]=row;
    }
  }

  /* 8) optional: per-column alignment (usually not needed once U_col sign fixed) */
  for(u32 k=0;k<n;k++) {
    double corr=0.0;
    for(u32 row=0; row<m; row++) {
      double accum=0.0;
      double *Urow=(double*)px(((K*)px(U))[row]);
      for(u32 i2=0;i2<n;i2++){
        double sii = ((i2<n)?((double*)px(((K*)px(S))[i2]))[i2]:0.0);
        double Vki = ((double*)px(((K*)px(V))[k]))[i2];
        accum += Urow[i2]*(sii*Vki);
      }
      corr += A0[(size_t)row*n + k]*accum;
    }
    if(corr < 0.0) {
      double *Vk=(double*)px(((K*)px(V))[k]);
      for(u32 i2=0;i2<n;i2++) Vk[i2] = -Vk[i2];
    }
  }

  /* 9) final per-row alignment: fix cases like (-1 -2 -3; 4 5 6) */
  for(u32 row=0; row<m; row++) {
    double corr=0.0;
    for(u32 col=0; col<n; col++) {
      double accum=0.0;
      double *Urow=(double*)px(((K*)px(U))[row]);
      for(u32 i2=0;i2<n;i2++){
        double sii = ((i2<n)?((double*)px(((K*)px(S))[i2]))[i2]:0.0);
        double Vci = ((double*)px(((K*)px(V))[col]))[i2];
        accum += Urow[i2]*(sii*Vci);
      }
      corr += A0[(size_t)row*n + col]*accum;
    }
    if(corr < 0.0) {
      double *Urow=(double*)px(((K*)px(U))[row]);
      for(u32 i2=0;i2<n;i2++) Urow[i2] = -Urow[i2];
    }
  }

  /* 10) free scratch & return (U;S;V') */
  xfree(a[0]); xfree(a);
  xfree(v[0]); xfree(v);
  xfree(w); xfree(rv1);
  xfree(A0);

  K r=tn(0,3); K *pr=px(r);
  pr[0]=U; pr[1]=S; pr[2]=V;
  canonicalize_svd(U,S,V);
  /* transpose V for composibility, ie a~mul/svd a */
  pr[2]=k(1,0,pr[2]);
  return r;

cleanup:
  if(a){ xfree(a[0]); xfree(a); }
  if(v){ xfree(v[0]); xfree(v); }
  xfree(w); xfree(rv1);
  xfree(A0);
  return e;
}

/* lu decomposition */
K lu_(K x) {
  K r=0, L=0, U=0, P=0, e=0;
  double **a = 0;
  u32 m, n, rnk, i, j, kk;
  int bad = 0;

  /* 1) validate rectangular numeric matrix (list of rows) */
  if(s(x) || tx) return kerror("type");
  if(!nx) return kerror("length");
  K *pxk = px(x);
  m = n(x);
  if(s(pxk[0]) || (T(pxk[0])!=0 && T(pxk[0])!=-1 && T(pxk[0])!=-2)) return kerror("type");
  n = n(pxk[0]);
  for(i=0;i<m;i++) {
    K row = pxk[i];
    if(s(row) || (T(row)!=0 && T(row)!=-1 && T(row)!=-2)) return kerror("type");
    if(n(row)!=n) return kerror("length");
  }

  /* 2) copy A into a[m][n] (double) */
  double *a0 = xmalloc((size_t)m * n * sizeof(double));  // real backing store
  a = xmalloc(m * sizeof(*a));
  for(i = 0; i < m; i++)
    a[i] = a0 + (size_t)i * n;

  for(i = 0; i < m; i++) {
    K row = k_(pxk[i]);
    if(T(row)==0) row=k(3,t2(1.0),row);
    if(E(row)) return kerror("type");
    int isf = (T(row) == -2);
    for(j = 0; j < n; j++) {
      double v = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
      if(!isfinite(v)) bad = 1;
      a[i][j] = v;
    }
    _k(row);
  }

  /* prepare return tuple now so we can fill with NaN on bad input */
  r = tn(0,3);
  K *pr = px(r);

  if(bad) {
    /* non-finite -> return (0n matrices) with expected shapes */
    L = tn(0, m);
    for(i=0;i<m;i++) {
      K row=tn(2, (u32)(m<n?m:n)); double *p=px(row);
      for(j=0;j<(u32)(m<n?m:n); j++) p[j]=NAN;
      ((K*)px(L))[i]=row;
    }
    U = tn(0, (u32)(m<n?m:n));
   for(i=0;i<(u32)(m<n?m:n); i++) {
     K row=tn(2,n); double *p=px(row);
     for(j=0;j<n;j++) p[j]=NAN;
     ((K*)px(U))[i]=row;
    }
    P = tn(0, m);
    for(i=0;i<m;i++) {
      K row=tn(2,m); double *p=px(row);
      for(j=0;j<m;j++) p[j]=NAN;
      ((K*)px(P))[i]=row;
    }
    pr[0]=L; pr[1]=U; pr[2]=P;
    goto cleanup;
  }

  /* 3) lu with partial pivoting for rectangular A
        We do r = min(m, n) steps, storing multipliers in a[i][k] (i>k) */
  rnk = (m < n) ? m : n;
  u32 *perm = xmalloc(m * sizeof(*perm));
  for(i=0; i<m; i++) perm[i] = i;

  const double eps = 1e-12;

  for(kk=0; kk<rnk; kk++) {
    /* pivot row in [k..m-1] for column k */
    u32 piv = kk;
    double maxv = fabs(a[kk][kk]);
    for(i=kk+1; i<m; i++) {
      double v = fabs(a[i][kk]);
      if(v > maxv) { maxv = v; piv = i; }
    }
    if(piv != kk) {
      /* swap rows in a */
      double *tmp = a[piv]; a[piv] = a[kk]; a[kk] = tmp;
      /* record permutation */
      u32 tperm = perm[piv]; perm[piv] = perm[kk]; perm[kk] = tperm;
    }

    /* if pivot small, treat as zero and skip multipliers for this column */
    if(fabs(a[kk][kk]) < eps) {
      /* zero out below-pivot entries in column k (cleaner L) */
      for(i=kk+1; i<m; i++) a[i][kk] = 0.0;
      continue;
    }

    /* eliminate below pivot */
    for(i=kk+1; i<m; i++) {
      a[i][kk] /= a[kk][kk];
      double lik = a[i][kk];
      /* update trailing block */
      for(j=kk+1; j<n; j++) {
        a[i][j] -= lik * a[kk][j];
      }
    }
  }

  /* 3.5) zero-clamp tiny noise before output */
  double EPS = DBL_EPSILON * 1e6;
  for(u32 i = 0; i < m; ++i)
    for(u32 j = 0; j < n; ++j)
      if(fabs(a[i][j]) < EPS)
        a[i][j] = 0.0;

  /* 4) build L (mxrnk), U (rnkxn), P (mxm) */
  L = tn(0, m);
  for(i=0; i<m; i++) {
    K row = tn(2, rnk);
    double *p = px(row);
    for(j=0; j<rnk; j++) {
      if(i == j) p[j] = 1.0;
      else if(i > j)  p[j] = a[i][j];
      else p[j] = 0.0;
    }
    ((K*)px(L))[i] = row;
  }

  U = tn(0, rnk);
  for(i=0; i<rnk; i++) {
    K row = tn(2, n);
    double *p = px(row);
    for(j=0; j<n; j++) {
      p[j] = (i <= j) ? a[i][j] : 0.0;
    }
    ((K*)px(U))[i] = row;
  }

  P = tn(0, m);
  for(i=0; i<m; i++) {
    K row = tn(2, m);
    double *p = px(row);
    for(j=0; j<m; j++) p[j] = 0.0;
    p[perm[i]] = 1.0;     /* P * A applied these row swaps */
    ((K*)px(P))[i] = row;
  }

  pr[0] = k(1,0,P); /* P -> P' */
  pr[1] = L; pr[2] = U;
  xfree(perm);

cleanup:
  if(a0) xfree(a0);
  if(a)  xfree(a);
  return r ? r : e;
}

/* qr decomposition */
K qr_(K x) {
  K r=0, Qk=0, Rk=0;
  double **A=0, *A0=0;      /* A[m][n] */
  double **Q=0, *Q0=0;      /* Q[m][m] */
  u32 m,n; u64 i,j,kk;

  /* 1) validate rectangular numeric matrix (list of rows) */
  if(s(x) || tx) return kerror("type");
  if(!nx) return kerror("length");
  K *pxk = px(x);
  m = n(x);
  if(s(pxk[0]) || (T(pxk[0])!=0 && T(pxk[0])!=-1 && T(pxk[0])!=-2)) return kerror("type");
  n = n(pxk[0]);
  for(i=0;i<m;i++) {
    K row = pxk[i];
    if(s(row) || (T(row)!=0 && T(row)!=-1 && T(row)!=-2)) return kerror("type");
    if(n(row)!=n) return kerror("length");
  }

  /* 2) copy A into contiguous A0 (row-major) + row ptrs A[i] */
  A0 = xmalloc((size_t)m * n * sizeof(double));
  A  = xmalloc((size_t)m * sizeof(*A));
  for(i=0;i<m;i++) A[i] = A0 + (size_t)i * n;
  for(i=0;i<m;i++) {
    K row = k_(pxk[i]);
    if(T(row)==0) row=k(3,t2(1.0),row);
    if(E(row)) { xfree(A); xfree(A0); return kerror("type"); }
    int isf = (T(row)==-2);
    for(j=0;j<n;j++) {
      double v = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
      A[i][j] = v;
    }
    _k(row);
  }

  /* 3) initialize Q = I (mxm) */
  Q0 = xmalloc((size_t)m * m * sizeof(double));
  Q  = xmalloc((size_t)m * sizeof(*Q));
  for(i=0;i<m;i++) Q[i] = Q0 + (size_t)i * m;
  for(i=0;i<m;i++) for (j=0;j<m;j++) Q[i][j] = (i==j)?1.0:0.0;

  /* 4) householder QR */
  u32 rnk = (m<n) ? m : n;
  for(kk=0; kk<rnk; kk++) {
    /* build reflector for column kk (rows kk..m-1) */
    double sigma = 0.0;
    for(i=kk;i<m;i++) sigma += A[i][kk]*A[i][kk];
    if(sigma == 0.0) continue;

    double x0    = A[kk][kk];
    double normx = sqrt(sigma);
    double alpha = (x0 >= 0.0 ? -normx : normx);
    double v0    = x0 - alpha;

    /* snapshot v = [0..0, v0, A[kk+1][kk], ..., A[m-1][kk]] */
    u32 len = m - kk;
    double *v = xmalloc((size_t)len * sizeof(double));
    v[0] = v0;
    for(i=1;i<len;i++) v[i] = A[kk + i][kk];

    /* beta = 2 / (v^T v) */
    double vnorm2 = 0.0;
    for(i=0;i<len;i++) vnorm2 += v[i]*v[i];
    if(vnorm2 == 0.0) { xfree(v); continue; }
    double beta = 2.0 / vnorm2;

    /* apply H = I - beta v v^T to A(:, j>=kk) */
    for(j=kk; j<n; j++) {
      double dot = 0.0;
      for(i=0;i<len;i++) dot += v[i] * A[kk + i][j];
      double coeff = beta * dot;
      for(i=0;i<len;i++) A[kk + i][j] -= coeff * v[i];
    }
    /* set exact R column kk */
    A[kk][kk] = alpha;
    for(i=kk+1;i<m;i++) A[i][kk] = 0.0;

    /* accumulate Q := Q * H  (post-multiply with same Householder) */
    for(i = 0; i < m; i++) {
      double dot = 0.0;
      for(u32 t = 0; t < len; t++)
        dot += Q[i][kk + t] * v[t];         /* dot = row i, segment kk.., dot v */
      double coeff = beta * dot;
      for(u32 t = 0; t < len; t++)
        Q[i][kk + t] -= coeff * v[t];        /* row-segment update */
    }

    xfree(v);
  }

  /* 5) make diag(R) nonnegative by flipping column kk of Q and row kk of R */
  for(u32 kk = 0; kk < rnk; ++kk) {
    if(A[kk][kk] < 0.0) {
      /* flip entire kk-th row of R from column kk..n-1 (include the diag) */
      for(u32 j = kk; j < n; ++j) A[kk][j] = -A[kk][j];
      /* flip entire kk-th column of Q for all m rows */
      for(u32 i = 0; i < m; ++i) Q[i][kk] = -Q[i][kk];
    }
  }
  
  /* 6) zero-clamp tiny noise based on global print precision */
  double EPS = DBL_EPSILON * 1e6;
  
  /* clamp Q (mxrnk) */
  for(u32 i = 0; i < m; ++i)
    for(u32 j = 0; j < rnk; ++j)
      if(fabs(Q[i][j]) < EPS) Q[i][j] = 0.0;
  
  /* clamp R (rnkxn) */
  for(u32 i = 0; i < rnk; ++i)
    for(u32 j = 0; j < n; ++j)
      if(fabs(A[i][j]) < EPS) A[i][j] = 0.0;

  /* 6.5) build K results */
  if(m >= n) {
    /* economy Q: mxn from first n columns of full Q */
    Qk = tn(0, m);
    for(i=0;i<m;i++) {
      K row = tn(2, n);
      double *p = (double*)px(row);
      for(j=0;j<n;j++) p[j] = Q[i][j];
      ((K*)px(Qk))[i] = row;
    }
    /* R: nxn upper triangular from A */
    Rk = tn(0, n);
    for(i=0;i<n;i++) {
      K row = tn(2, n);
      double *p = (double*)px(row);
      for(j=0;j<n;j++) p[j] = (i<=j) ? A[i][j] : 0.0;
      ((K*)px(Rk))[i] = row;
    }
  }
  else {
    /* wide: Q mxm, R mxn (upper-trapezoidal) */
    Qk = tn(0, m);
    for(i=0;i<m;i++) {
      K row = tn(2, m);
      double *p = (double*)px(row);
      for(j=0;j<m;j++) p[j] = Q[i][j];
      ((K*)px(Qk))[i] = row;
    }
    Rk = tn(0, m);
    for(i=0;i<m;i++) {
      K row = tn(2, n);
      double *p = (double*)px(row);
      for(j=0;j<n;j++) p[j] = (i<=j) ? A[i][j] : 0.0;
      ((K*)px(Rk))[i] = row;
    }
  }

  /* 7) pack and free */
  r = tn(0,2);
  ((K*)px(r))[0] = Qk;
  ((K*)px(r))[1] = Rk;

  xfree(A0); xfree(A);
  xfree(Q0); xfree(Q);
  return r;
}

/* ldu decomposition: A = L*D*U where L,U have 1s on diagonal */
K ldu_(K x) {
  K lu = lu_(x);
  if(E(lu)) return lu;

  K *pr = px(lu);
  K P = pr[0];  /* P' */
  K L = pr[1];  /* L (already has 1s on diagonal) */
  K U = pr[2];  /* U */

  u32 m = n(U);
  if(m == 0) {
    /* empty: return (P';L;D;U) with empty D */
    K r = tn(0,4);
    K *rp = px(r);
    rp[0] = k_(P);
    rp[1] = k_(L);
    rp[2] = tn(0,0);  /* empty D */
    rp[3] = k_(U);
    _k(lu);
    return r;
  }

  K *pU = px(U);
  u32 ncols = n(pU[0]);

  /* extract diagonal into D (as diagonal matrix), normalize U rows */
  K D = tn(0, m);
  K *pDrows = px(D);

  for(u32 i = 0; i < m; i++) {
    double *Urow = px(pU[i]);
    double d = Urow[i];

    /* build diagonal matrix row */
    K Drow = tn(2, m);
    double *pDrow = px(Drow);
    for(u32 j = 0; j < m; j++)
      pDrow[j] = (i == j) ? d : 0.0;
    pDrows[i] = Drow;

    if(fabs(d) > 1e-12) {
      /* normalize U row i */
      for(u32 j = i; j < ncols; j++)
        Urow[j] /= d;
    }
  }

  /* return (P';L;D;U) */
  K r = tn(0,4);
  K *rp = px(r);
  rp[0] = k_(P);
  rp[1] = k_(L);
  rp[2] = D;
  rp[3] = k_(U);
  _k(lu);
  return r;
}

/* rref: reduced row echelon form */
K rref_(K x) {
  K r = 0;
  double *a0 = 0, **a = 0;
  u32 m, n, i, j, kk;
  int bad = 0;

  /* 1) validate rectangular numeric matrix */
  if(s(x) || tx) return kerror("type");
  if(!nx) return kerror("length");
  K *pxk = px(x);
  m = n(x);
  if(s(pxk[0]) || (T(pxk[0])!=0 && T(pxk[0])!=-1 && T(pxk[0])!=-2))
    return kerror("type");
  n = n(pxk[0]);
  for(i = 0; i < m; i++) {
    K row = pxk[i];
    if(s(row) || (T(row)!=0 && T(row)!=-1 && T(row)!=-2))
      return kerror("type");
    if(n(row) != n) return kerror("length");
  }

  /* 2) copy into a[m][n] */
  a0 = xmalloc((size_t)m * n * sizeof(double));
  a = xmalloc(m * sizeof(*a));
  for(i = 0; i < m; i++)
    a[i] = a0 + (size_t)i * n;

  for(i = 0; i < m; i++) {
    K row = k_(pxk[i]);
    if(T(row) == 0) row = k(3, t2(1.0), row);
    if(E(row)) { xfree(a); xfree(a0); return kerror("type"); }
    int isf = (T(row) == -2);
    for(j = 0; j < n; j++) {
      double v = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
      if(!isfinite(v)) bad = 1;
      a[i][j] = v;
    }
    _k(row);
  }

  /* 2.5) if non-finite input, return NaN matrix */
  if(bad) {
    r = tn(0, m);
    for(i = 0; i < m; i++) {
      K row = tn(2, n);
      double *p = px(row);
      for(j = 0; j < n; j++) p[j] = NAN;
      ((K*)px(r))[i] = row;
    }
    xfree(a0); xfree(a);
    return r;
  }

  /* 3) gaussian elimination with partial pivoting -> RREF */
  const double eps = 1e-12;
  u32 piv_row = 0;

  for(kk = 0; kk < n && piv_row < m; kk++) {
    /* find pivot in column kk */
    u32 piv = piv_row;
    double maxv = fabs(a[piv_row][kk]);
    for(i = piv_row + 1; i < m; i++) {
      double v = fabs(a[i][kk]);
      if(v > maxv) { maxv = v; piv = i; }
    }

    if(maxv < eps) {
      /* no pivot in this column, zero it out */
      for(i = piv_row; i < m; i++) a[i][kk] = 0.0;
      continue;
    }

    /* swap rows */
    if(piv != piv_row) {
      double *tmp = a[piv]; a[piv] = a[piv_row]; a[piv_row] = tmp;
    }

    /* normalize pivot row */
    double pivot = a[piv_row][kk];
    for(j = kk; j < n; j++)
      a[piv_row][j] /= pivot;

    /* eliminate column kk in ALL other rows (above and below) */
    for(i = 0; i < m; i++) {
      if(i == piv_row) continue;
      double factor = a[i][kk];
      if(fabs(factor) > eps) {
        for(j = kk; j < n; j++)
          a[i][j] -= factor * a[piv_row][j];
      }
      a[i][kk] = 0.0;  /* exact zero */
    }

    piv_row++;
  }

  /* 4) zero-clamp tiny noise */
  double EPS = DBL_EPSILON * 1e6;
  for(i = 0; i < m; i++)
    for(j = 0; j < n; j++)
      if(fabs(a[i][j]) < EPS) a[i][j] = 0.0;

  /* 5) build result matrix */
  r = tn(0, m);
  for(i = 0; i < m; i++) {
    K row = tn(2, n);
    double *p = px(row);
    for(j = 0; j < n; j++)
      p[j] = a[i][j];
    ((K*)px(r))[i] = row;
  }

  xfree(a0);
  xfree(a);
  return r;
}

/* det: determinant of square matrix via LU */
K det_(K x) {
  double *a0 = 0, **a = 0;
  u32 m, i, j, kk;
  int swaps = 0;
  double det = 1.0;
  const double eps = 1e-12;

  /* 1) validate square numeric matrix */
  if(s(x) || tx) return kerror("type");
  if(!nx) return t2(1.0);  /* empty matrix: det = 1 */
  K *pxk = px(x);
  m = n(x);
  if(s(pxk[0]) || (T(pxk[0])!=0 && T(pxk[0])!=-1 && T(pxk[0])!=-2))
    return kerror("type");
  if(n(pxk[0]) != m) return kerror("length");  /* must be square */
  for(i = 0; i < m; i++) {
    K row = pxk[i];
    if(s(row) || (T(row)!=0 && T(row)!=-1 && T(row)!=-2))
      return kerror("type");
    if(n(row) != m) return kerror("length");
  }

  /* 2) copy into a[m][m] */
  a0 = xmalloc((size_t)m * m * sizeof(double));
  a = xmalloc(m * sizeof(*a));
  for(i = 0; i < m; i++)
    a[i] = a0 + (size_t)i * m;

  for(i = 0; i < m; i++) {
    K row = k_(pxk[i]);
    if(T(row) == 0) row = k(3, t2(1.0), row);
    if(E(row)) { xfree(a); xfree(a0); return kerror("type"); }
    int isf = (T(row) == -2);
    for(j = 0; j < m; j++) {
      double v = isf ? ((double*)px(row))[j] : fi(((int*)px(row))[j]);
      if(!isfinite(v)) { _k(row); xfree(a); xfree(a0); return t2(NAN); }
      a[i][j] = v;
    }
    _k(row);
  }

  /* 3) LU with partial pivoting, tracking swaps and diagonal product */
  for(kk = 0; kk < m; kk++) {
    /* find pivot */
    u32 piv = kk;
    double maxv = fabs(a[kk][kk]);
    for(i = kk + 1; i < m; i++) {
      double v = fabs(a[i][kk]);
      if(v > maxv) { maxv = v; piv = i; }
    }

    /* singular check */
    if(maxv < eps) {
      xfree(a0); xfree(a);
      return t2(0.0);
    }

    /* swap rows */
    if(piv != kk) {
      double *tmp = a[piv]; a[piv] = a[kk]; a[kk] = tmp;
      swaps++;
    }

    /* accumulate diagonal */
    det *= a[kk][kk];

    /* eliminate below pivot */
    for(i = kk + 1; i < m; i++) {
      double lik = a[i][kk] / a[kk][kk];
      for(j = kk + 1; j < m; j++)
        a[i][j] -= lik * a[kk][j];
    }
  }

  xfree(a0);
  xfree(a);

  /* apply sign from row swaps */
  if(swaps & 1) det = -det;
  return t2(det);
}

/* mag: vector magnitude (L2 norm) */
K mag_(K x) {
  if(s(x)) return kerror("type");

  switch(tx) {
  case 1: {
    /* int scalar: return fabs as float */
    double v = fi(ik(x));
    return t2(fabs(v));
  }
  case 2: {
    /* float scalar: return fabs */
    return t2(fabs(fk(x)));
  }
  case -1: {
    /* int vector: sqrt(sum of squares) */
    u64 len = nx;
    i32 *p = px(x);
    double sum = 0.0;
    for(u64 i = 0; i < len; i++) {
      double v = fi(p[i]);
      sum += v * v;
    }
    return t2(sqrt(sum));
  }
  case -2: {
    /* float vector: sqrt(sum of squares) */
    u64 len = nx;
    double *p = px(x);
    double sum = 0.0;
    for(u64 i = 0; i < len; i++) {
      double v = p[i];
      sum += v * v;
    }
    return t2(sqrt(sum));
  }
  case 0: {
    /* list (matrix): apply row-wise */
    u64 len = nx;
    if(len == 0) return tn(2, 0);  /* empty -> empty float vector */
    K *pk = px(x);
    K r = tn(2, len);
    double *pr = px(r);
    for(u64 i = 0; i < len; i++) {
      K row = pk[i];
      K m = mag_(row);
      if(E(m)) { _k(r); return m; }
      /* m should be a scalar (float) */
      pr[i] = fk(m);
      _k(m);
    }
    return r;
  }
  default:
    return kerror("type");
  }
}
