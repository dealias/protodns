#include "fftw++.h"

#ifndef __convolution_h__
#define __convolution_h__ 1

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

class convolution {
protected:
  unsigned int n;
  unsigned int m;
  unsigned int c;
  rcfft1d *rc;
  crfft1d *cr;
  double *F,*G;
  Complex zeta;
public:  
  // Pass a temporary work array f to save memory.
  convolution(unsigned int n, unsigned int m, Complex *f=NULL) :
    n(n), m(m) {
    if(f) {
      F=NULL;
      G=NULL;
      rc=new rcfft1d(n,f);
      cr=new crfft1d(n,f);
    } else {
      F=FFTWdouble(n);
      G=FFTWdouble(n+2);
      rc=new rcfft1d(n,F,(Complex *) G);
      cr=new crfft1d(n,(Complex *) G,F);
    }
  }
  
  convolution(unsigned int m) : m(m) {}
  
  convolution(unsigned int m, Complex *f) : m(m) {
    n=3*m;
    c=m/2;
    
    double arg=2.0*M_PI/n;
    zeta=Complex(cos(arg),sin(arg));

    rc=new rcfft1d(m,f);
    cr=new crfft1d(m,f);
  }
  
// Need destructor  
  
// Compute H = F (*) G, where F and G are the non-negative Fourier
// components of real functions f and g, respectively. Dealiasing via
// zero-padding is implemented automatically.
//
// Arrays F[n/2+1], G[n/2+1] must be distinct.
// Input F[i], G[i] (0 <= i < m), where 3*m <= n.
// Output H[i] = F (*) G  (0 <= i < m), F[i]=f[i], G[i]=g[i] (0 <= i < n/2).
//
// Array H[n/2+1] can coincide with either F or G, in which case the output H
// subsumes f or g, respectively.

  void fft(Complex *h, Complex *f, Complex *g) {
    unsigned int n2=n/2;
    double ninv=1.0/n;
    if(F) {
      for(unsigned int i=m; i <= n2; i++) f[i]=0.0;
      cr->fft(f,F);
  
      for(unsigned int i=m; i <= n2; i++) g[i]=0.0;
      cr->fft(g,G);
      
      for(unsigned int i=0; i < n; ++i)
        F[i] *= G[i]*ninv;
	
      rc->fft(F,h);
    } else {
      for(unsigned int i=m; i <= n2; i++) f[i]=0.0;
      cr->fft(f);
  
      for(unsigned int i=m; i <= n2; i++) g[i]=0.0;
      cr->fft(g);
	
      double *F=(double *) f;
      double *G=(double *) g;
      double *H=(double *) h;
      for(unsigned int i=0; i < n; ++i)
        H[i]=F[i]*G[i]*ninv;
	
      rc->fft(h);
    }
  }
  
  // Note: input arrays f and g are destroyed.
  void unpadded( Complex *f, Complex *g, Complex *u, Complex *v) {
    double f0=f[0].re;
    double g0=g[0].re;

    bool even=m % 2 == 0;
    if(!even) _exit(1);
    
    u[0]=f0;
    v[0]=g0;
    Complex fc=f[c];
    unsigned int m1=m-1;
    Complex fmk=conj(f[m1]);
    f[m1]=f0;
    Complex gc=g[c];
    Complex gmk=conj(g[m1]);
    g[m1]=g0;
    Complex zetac=conj(zeta);
    Complex Zetak=zetac;
    
    static const Complex zeta3(-0.5,0.5*sqrt(3.0));
    static const Complex zeta3c=conj(zeta3);
    static const Complex I(0,1);

    for(unsigned int k=1; k < c; ++k) {
      Complex fk=f[k];
      f[k]=fk+fmk;
      Complex A=Zetak*(fk.re+zeta3*fmk.re);
      Complex B=-I*Zetak*(fk.im+zeta3*fmk.im);
      u[k]=A-B;
      int mk=m1-k;
      fmk=conj(f[mk]);
      f[mk]=A+B;

      Complex gk=g[k];
      g[k]=gk+gmk;
      A=Zetak*(gk.re+zeta3*gmk.re);
      B=-I*Zetak*(gk.im+zeta3*gmk.im);
      Zetak *= zeta;
      v[k]=A-B;
      gmk=conj(g[mk]);
      g[mk]=A+B;
    }
  
    double A=fc.re;
    double B=sqrt(3)*fc.im;
    fc=f[c];
    f[c]=2.0*A;
    u[c]=A+B;
    A -= B;

    cr->fft(f);
    double C=gc.re;
    B=sqrt(3)*gc.im;
    gc=g[c];
    g[c]=2.0*C;
    v[c]=C+B;
    C -= B;
    cr->fft(g);
    for(int i=0; i < c+1; ++i)
      f[i] *= g[i];
    rc->fft(f);
    Complex overlap0=f[c-1];
    double overlap1=f[c].re;

    f[c-1]=A;
    f[c]=fc;
    cr->fft(f+c-1);
    g[c-1]=C;
    g[c]=gc;
    cr->fft(g+c-1);
    for(int i=c-1; i < m; ++i)
      f[i] *= g[i];
    rc->fft(f+c-1);
    // Data is shifted down by 1 complex.

    cr->fft(u);
    cr->fft(v);
    for(int i=0; i < c+1; ++i)
      u[i] *= v[i];
    rc->fft(u);

    unsigned int stop=m-c-1;
    double ninv=1.0/n;
    f[0]=(f[0].re+f[c-1].re+u[0].re)*ninv;
    Zetak=zeta*ninv;

    for(unsigned k=1; k < stop; ++k) {
      Complex f0k=f[k]*ninv;
      Complex f1k=conj(Zetak)*f[c-2+k];
      Complex f2k=Zetak*u[k];
      Zetak *= zeta;
      f[k]=f0k+f1k+f2k;
      f[m-k]=conj(f0k)+zeta3c*conj(f1k)+zeta3*conj(f2k);
    }

    Complex f0k=overlap0*ninv;
    Complex f1k=conj(Zetak)*f[m-2];
    Complex f2k=Zetak*u[c-1];
    f[c-1]=f0k+f1k+f2k;
    f[c+1]=conj(f0k)+zeta3c*conj(f1k)+zeta3*conj(f2k);
  
    if(even) f[c]=(overlap1-f[m-1].re*zeta3-u[c].re*conj(zeta3))*ninv;
  }
  
// Compute H = F (*) G, where F and G contain the non-negative Fourier
// components of real functions f and g, respectively, via direct convolution
// instead of a Fast Fourier Transform technique.
//
// Input F[i], G[i] (0 <= i < m).
// Output H[i] = F (*) G  (0 <= i < m), F and G unchanged.
//
// Array H[m] must be distinct from F[m] and G[m].

  void direct(Complex *H, Complex *F, Complex *G) {
    for(unsigned int i=0; i < m; i++) {
      Complex sum=0.0;
      for(unsigned int j=0; j <= i; j++) sum += F[j]*G[i-j];
      for(unsigned int j=i+1; j < m; j++) sum += F[j]*conj(G[j-i]);
      for(unsigned int j=1; j < m-i; j++) sum += conj(F[j])*G[i+j];
      H[i]=sum;
    }
  }	

};

class cconvolution {
protected:
  unsigned int n;
  unsigned int m;
  fft1d *Backwards;
  fft1d *Forwards;
  double c,s;
public:  
  cconvolution(unsigned int n, unsigned int m, Complex *f) :
    n(n), m(m) {
    Backwards=new fft1d(n,1,f);
    Forwards=new fft1d(n,-1,f);
  }
  
  cconvolution(unsigned int m, Complex *f) : m(m) {
    n=2*m;
    
    double arg=2.0*M_PI/n;
    c=cos(arg);
    s=sin(arg);

    Backwards=new fft1d(m,-1,f);
    Forwards=new fft1d(m,1,f);
  }
  
// Need destructor  
  
// Compute H = F (*) G, where F and G are the non-negative Fourier
// components of real functions f and g, respectively. Dealiasing via
// zero-padding is implemented automatically.
//
// Arrays F[n/2+1], G[n/2+1] must be distinct.
// Input F[i], G[i] (0 <= i < m), where 3*m <= n.
// Output H[i] = F (*) G  (0 <= i < m), F[i]=f[i], G[i]=g[i] (0 <= i < n/2).
//
// Array H[n/2+1] can coincide with either F or G, in which case the output H
// subsumes f or g, respectively.

  void fft(Complex *h, Complex *f, Complex *g) {
    for(unsigned int i=m; i < n; i++) f[i]=0.0;
    Backwards->fft(f);
  
    for(unsigned int i=m; i < n; i++) g[i]=0.0;
    Backwards->fft(g);
      
    double ninv=1.0/n;
    for(unsigned int i=0; i < n; ++i)
      f[i] *= g[i]*ninv;
	
    Forwards->fft(f);
  }
  
  // Note: input arrays f and g are destroyed.
  // u and v are temporary work arrays each of size m.
  void unpadded(Complex *f, Complex *g, Complex *u, Complex *v) {
    double re=1.0;
    double im=0.0;
    for(unsigned int k=0; k < m; ++k) {
      Complex *P=u+k;
      Complex *Q=v+k;
      Complex fk=*(f+k);
      Complex gk=*(g+k);
      P->re=re*fk.re-im*fk.im;
      P->im=im*fk.re+re*fk.im;
      Q->re=re*gk.re-im*gk.im;
      Q->im=im*gk.re+re*gk.im;
      double temp=re*c+im*s; 
      im=-re*s+im*c;
      re=temp;
    }  
    
    Backwards->fft(f);
    Backwards->fft(u);
    Backwards->fft(g);
    Backwards->fft(v);
    
    for(unsigned int k=0; k < m; ++k) {
      Complex *p=f+k;
      Complex fk=*p;
      Complex gk=*(g+k);
      p->re=fk.re*gk.re-fk.im*gk.im;
      p->im=fk.re*gk.im+fk.im*gk.re;
    }
    
    Forwards->fft(f);
    
    for(unsigned int k=0; k < m; ++k) {
      Complex *p=u+k;
      Complex fk=*p;
      Complex gk=*(v+k);
      p->re=fk.re*gk.re-fk.im*gk.im;
      p->im=fk.re*gk.im+fk.im*gk.re;
    }
    
    Forwards->fft(u);
    
    double ninv=1.0/n;
    re=ninv;
    im=0.0;
    for(unsigned int k=0; k < m; ++k) {
      Complex *p=f+k;
      Complex fk=*p;
      Complex fkm=*(u+k);
      p->re=ninv*fk.re+re*fkm.re-im*fkm.im;
      p->im=ninv*fk.im+im*fkm.re+re*fkm.im;
      double temp=re*c-im*s;
      im=re*s+im*c;
      re=temp;
    }
  }
  
// Compute H = F (*) G, where F and G contain the non-negative Fourier
// components of real functions f and g, respectively, via direct convolution
// instead of a Fast Fourier Transform technique.
//
// Input F[i], G[i] (0 <= i < m).
// Output H[i] = F (*) G  (0 <= i < m), F and G unchanged.
//
// Array H[m] must be distinct from F[m] and G[m].

  void direct(Complex *H, Complex *F, Complex *G) {
    for(unsigned int i=0; i < m; i++) {
      Complex sum=0.0;
      for(unsigned int j=0; j <= i; j++) sum += F[j]*G[i-j];
      H[i]=sum;
    }
  }	
};

class mcconvolution {
protected:
  unsigned int n;
  unsigned int m;
  unsigned int M;
  unsigned int stride;
  unsigned int dist;
  mfft1d *Backwards;
  mfft1d *Forwards;
  double c,s;
public:  
  mcconvolution(unsigned int m, unsigned int M, unsigned int stride,
                unsigned int dist, Complex *f) : m(m), M(M), stride(stride),
                                                 dist(dist) {
    n=2*m;
    
    double arg=2.0*M_PI/n;
    c=cos(arg);
    s=sin(arg);

    Backwards=new mfft1d(m,-1,M,stride,dist,f);
    Forwards=new mfft1d(m,1,M,stride,dist,f);
  }
  
  // Special case optimized for stride=1.
  void unpadded1(Complex *f, Complex *g, Complex *u) {
    unsigned int istop=M*dist;
    unsigned int mM=m*M;
    for(unsigned int i=0; i < istop; i += dist) {
      double re=1.0;
      double im=0.0;
      Complex *Gi=g+i;
      Complex *Fi=f+i;
      Complex *ui=u+i;
      for(unsigned int k=0; k < m; ++k) {
        Complex *P=ui+k;
        Complex *Q=P+mM;
        Complex fk=*(Fi+k);
        Complex gk=*(Gi+k);
        P->re=re*fk.re-im*fk.im;
        P->im=im*fk.re+re*fk.im;
        Q->re=re*gk.re-im*gk.im;
        Q->im=im*gk.re+re*gk.im;
        double temp=re*c+im*s; 
        im=-re*s+im*c;
        re=temp;
      }  
    }
    
    Forwards->fft(f);
    Forwards->fft(u);
    Forwards->fft(g);
    Forwards->fft(u+mM);
    
    for(unsigned int i=0; i < istop; i += dist) {
      Complex *fi=f+i;
      Complex *Gi=g+i;
      for(unsigned int k=0; k < m; ++k) {
        Complex *p=fi+k;
        Complex fk=*p;
        Complex gk=*(Gi+k);
        p->re=fk.re*gk.re-fk.im*gk.im;
        p->im=fk.re*gk.im+fk.im*gk.re;
      }
    }
    
    Backwards->fft(f);
    
    for(unsigned int i=0; i < istop; i += dist) {
      Complex *ui=u+i;
      for(unsigned int k=0; k < m; ++k) {
        Complex *p=ui+k;
        Complex fk=*p;
        Complex gk=*(p+mM);
        p->re=fk.re*gk.re-fk.im*gk.im;
        p->im=fk.re*gk.im+fk.im*gk.re;
      }
    }
    
    Backwards->fft(u);
    
    double ninv=1.0/n;
    for(unsigned int i=0; i < istop; i += dist) {
      Complex *ui=u+i;
      Complex *fi=f+i;
      double re=ninv;
      double im=0.0;
      for(unsigned int k=0; k < m; ++k) {
        Complex *p=fi+k;
        Complex fk=*p;
        Complex fkm=*(ui+k);
        p->re=ninv*fk.re+re*fkm.re-im*fkm.im;
        p->im=ninv*fk.im+im*fkm.re+re*fkm.im;
        double temp=re*c-im*s;
        im=re*s+im*c;
        re=temp;
      }
    }
  }

  // Note: input arrays f and g are destroyed.
  // u is a temporary work array of size n*M.
  void unpadded(Complex *f, Complex *g, Complex *u) {
    if(stride == 1) {
      unpadded1(f,g,u);
      return;
    }
    double re=1.0;
    double im=0.0;
    unsigned int kstop=m*stride;
    unsigned int istop=M*dist;
    unsigned int mM=m*M;
    for(unsigned int k=0; k < kstop; k += stride) {
      Complex *Gk=g+k;
      Complex *Fk=f+k;
      Complex *uk=u+k;
      for(unsigned int i=0; i < istop; i += dist) {
        Complex *P=uk+i;
        Complex *Q=P+mM;
        Complex fk=*(Fk+i);
        Complex gk=*(Gk+i);
        P->re=re*fk.re-im*fk.im;
        P->im=im*fk.re+re*fk.im;
        Q->re=re*gk.re-im*gk.im;
        Q->im=im*gk.re+re*gk.im;
      }  
      double temp=re*c+im*s; 
      im=-re*s+im*c;
      re=temp;
    }
    
    Forwards->fft(f);
    Forwards->fft(u);
    Forwards->fft(g);
    Forwards->fft(u+mM);
    
    for(unsigned int k=0; k < kstop; k += stride) {
      Complex *fk=f+k;
      Complex *Gk=g+k;
      for(unsigned int i=0; i < istop; i += dist) {
        Complex *p=fk+i;
        Complex fk=*p;
        Complex gk=*(Gk+i);
        p->re=fk.re*gk.re-fk.im*gk.im;
        p->im=fk.re*gk.im+fk.im*gk.re;
      }
    }
    
    Backwards->fft(f);
    
    for(unsigned int k=0; k < kstop; k += stride) {
      Complex *uk=u+k;
      for(unsigned int i=0; i < istop; i += dist) {
        Complex *p=uk+i;
        Complex fk=*p;
        Complex gk=*(p+mM);
        p->re=fk.re*gk.re-fk.im*gk.im;
        p->im=fk.re*gk.im+fk.im*gk.re;
      }
    }
    
    Backwards->fft(u);
    
    double ninv=1.0/n;
    re=ninv;
    im=0.0;
    for(unsigned int k=0; k < kstop; k += stride) {
      Complex *uk=u+k;
      Complex *fk=f+k;
      for(unsigned int i=0; i < istop; i += dist) {
        Complex *p=fk+i;
        Complex fk=*p;
        Complex fkm=*(uk+i);
        p->re=ninv*fk.re+re*fkm.re-im*fkm.im;
        p->im=ninv*fk.im+im*fkm.re+re*fkm.im;
      }
      double temp=re*c-im*s;
      im=re*s+im*c;
      re=temp;
    }
  }
};

// Compute the scrambled virtual m-padded complex Fourier transform of M complex
// vectors, each of length m.
// Before calling fft(), the arrays in and out (which may coincide) must be
// allocated as Complex[M*m].
//
// In-place usage:
//
//   ffthalf Backward(m,M,stride);
//   Backward.fft(in);
//
// Notes:
//   stride is the spacing between the elements of each Complex vector;
//
class ffthalf {
  unsigned int n;
  unsigned int m;
  unsigned int M;
  unsigned int stride;
  unsigned int dist;
  mfft1d *Backwards;
  mfft1d *Forwards;
  Complex zeta;
  double c,s;

public:  
  ffthalf(unsigned int m, unsigned int M,
          unsigned int stride, Complex *f) : m(m), M(M), stride(stride) {
    n=2*m;
    double arg=2.0*M_PI/n;
    c=cos(arg);
    s=sin(arg);
    
    // TODO: Standardize signs
    Backwards=new mfft1d(m,-1,M,stride,1,f);
    Forwards=new mfft1d(m,1,M,stride,1,f);
  }
  
  void backwards(Complex *f, Complex *u) {
    double re=1.0;
    double im=0.0;
    unsigned int stop=m*stride;
    for(unsigned int k=0; k < stop; k += stride) {
      Complex *fk=f+k;
      Complex *uk=u+k;
      for(unsigned int i=0; i < M; ++i) {
        Complex *P=uk+i;
        Complex *p=fk+i;
        Complex fk=*p;
        P->re=re*fk.re-im*fk.im;
        P->im=im*fk.re+re*fk.im;
      }
      double temp=re*c+im*s; 
      im=-re*s+im*c;
      re=temp;
    }
    
    Backwards->fft(f);
    Backwards->fft(u);
  }
  
  void forwards(Complex *f, Complex *u) {
    Forwards->fft(f);
    Forwards->fft(u);

    double ninv=1.0/n;
    double re=ninv;
    double im=0.0;
    unsigned int stop=m*stride;
    for(unsigned int k=0; k < stop; k += stride) {
      Complex *uk=u+k;
      Complex *fk=f+k;
      for(unsigned int i=0; i < M; ++i) {
        Complex *p=fk+i;
        Complex fk=*p;
        Complex fkm=*(uk+i);
        p->re=ninv*fk.re+re*fkm.re-im*fkm.im;
        p->im=ninv*fk.im+im*fkm.re+re*fkm.im;
      }
      double temp=re*c-im*s;
      im=re*s+im*c;
      re=temp;
    }
  }
};
  
class cconvolution2 {
protected:
  unsigned int n;
  unsigned int m;
  bool prune;
  mfft1d *xBackwards;
  mfft1d *yBackwards;
  mfft1d *xForwards;
  mfft1d *yForwards;
  fft2d *Backwards;
  fft2d *Forwards;
  cconvolution *C;
  ffthalf *fftpad;
  Complex *u,*v;
  Complex *work;
public:  
  // Set prune=true to skip Fourier transforming zero rows.
  cconvolution2(unsigned int n, unsigned int m, Complex *f, bool prune=false) :
    n(n), m(m), prune(prune) {
    if(prune) {
      xBackwards=new mfft1d(n,1,m,n,1,f);
      yBackwards=new mfft1d(n,1,n,1,n,f);
      xForwards=new mfft1d(n,-1,m,n,1,f);
      yForwards=new mfft1d(n,-1,n,1,n,f);
    } else {
      Backwards=new fft2d(n,n,1,f);
      Forwards=new fft2d(n,n,-1,f);
    }
  }
  
  cconvolution2(unsigned int m, Complex *f) : m(m) {
    n=2*m;
    u=FFTWComplex(m*m);
    v=FFTWComplex(m*m);
    work=FFTWComplex(n);
    fftpad=new ffthalf(m,m,m,u);
    C=new cconvolution(m,work);
  }
  
// Need destructor  
  
// Compute H = F (*) G, where F and G are the non-negative Fourier
// components of real functions f and g, respectively. Dealiasing via
// zero-padding is implemented automatically.
//
// Arrays F[n/2+1], G[n/2+1] must be distinct.
// Input F[i], G[i] (0 <= i < m), where 3*m <= n.
// Output H[i] = F (*) G  (0 <= i < m), F[i]=f[i], G[i]=g[i] (0 <= i < n/2).
//
// Array H[n/2+1] can coincide with either F or G, in which case the output H
// subsumes f or g, respectively.

  void pad(Complex *f) {
    for(unsigned int i=0; i < m;) {
      unsigned int j=n*i+m;
      ++i;
      unsigned int nip=n*i;
      for(; j < nip; ++j)
        f[j]=0.0;
    }
    
    for(unsigned int i=m; i < n;) {
      unsigned int j=n*i;
      ++i;
      unsigned int nip=n*i;
      for(; j < nip; ++j)
        f[j]=0.0;
    }
  }
  
  void fft(Complex *f, Complex *g) {
    pad(f);
    if(prune) {
      xBackwards->fft(f);
      yBackwards->fft(f);
    } else
      Backwards->fft(f);
  
    pad(g);
    if(prune) {
      xBackwards->fft(g);
      yBackwards->fft(g);
    } else
      Backwards->fft(g);
    
    unsigned int n2=n*n;
    double ninv=1.0/n2;
    for(unsigned int i=0; i < n2; ++i)
      f[i] *= g[i]*ninv;
	
    if(prune) {
      yForwards->fft(f);
      xForwards->fft(f);
    } else {
      Forwards->fft(f);
    }
  }
  
  // Note: input arrays f and g are destroyed.
  void unpadded(Complex *f, Complex *g) {
    fftpad->backwards(f,u);
    fftpad->backwards(g,v);

    unsigned int m2=m*m;
    Complex *work2=work+m;
    for(unsigned int i=0; i < m2; i += m)
      C->unpadded(f+i,g+i,work,work2);
    for(unsigned int i=0; i < m2; i += m)
      C->unpadded(u+i,v+i,work,work2);
    
    fftpad->forwards(f,u);
  }
  
// Compute H = F (*) G, where F and G contain the non-negative Fourier
// components of real functions f and g, respectively, via direct convolution
// instead of a Fast Fourier Transform technique.
//
// Input F[i], G[i] (0 <= i < m).
// Output H[i] = F (*) G  (0 <= i < m), F and G unchanged.
//
// Array H[m] must be distinct from F[m] and G[m].

  void direct(Complex *H, Complex *F, Complex *G) {
    for(unsigned int i=0; i < m; ++i) {
      for(unsigned int j=0; j < m; ++j) {
        Complex sum=0.0;
        for(unsigned int k=0; k <= i; ++k)
          for(unsigned int p=0; p <= j; ++p)
            sum += F[k*m+p]*G[(i-k)*m+j-p];
        H[i*m+j]=sum;
      }
    }
  }	
};

#endif
