using namespace std;

#include "Complex.h"
#include "convolution.h"
#include "utils.h"
#include "Array.h"

using namespace Array;

// g++ -g -O3 -DNDEBUG -fomit-frame-pointer -fstrict-aliasing -ffast-math -msse2 -mfpmath=sse conv3.cc fftw++.cc -lfftw3 -march=native

// icpc -O3 -ansi-alias -malign-double -fp-model fast=2 conv3.cc fftw++.cc -lfftw3

// FFTW: 
// configure --enable-sse2 CC=icpc CFLAGS="-O3 -ansi-alias -malign-double -fp-model fast=2"

// Number of iterations.
unsigned int N0=10000000;
unsigned int N=0;
unsigned int nx=0;
unsigned int ny=0;
unsigned int nz=0;
unsigned int mx=4;
unsigned int my=4;
unsigned int mz=4;
unsigned int nxp;
unsigned int nyp;
unsigned int nzp;
unsigned int M=1;

bool Direct=false, Implicit=true;

unsigned int outlimit=300;

inline void init(array3<Complex>& f, array3<Complex>& g, unsigned int M=1) 
{
  unsigned int xorigin=mx-1;
  unsigned int yorigin=my-1;
  unsigned int xstop=nxp*M;
  double factor=1.0/sqrt(M);
  
  
  for(unsigned int i=0; i < xstop; ++i) {
    for(unsigned int j=0; j < nyp; ++j) {
      for(unsigned int k=0; k < mz; ++k) {
        f[i][j][k]=factor*Complex(3.0,2.0);
        g[i][j][k]=factor*Complex(5.0,3.0);
      }
    }
  }

  for(unsigned int i=0; i < M; ++i) {
    f[xorigin+i*nxp][yorigin][0]=f[xorigin+i*nxp][yorigin][0].re;
    g[xorigin+i*nxp][yorigin][0]=g[xorigin+i*nxp][yorigin][0].re;
  }
}

unsigned int padding(unsigned int m)
{
  unsigned int n=3*m-2;
  cout << "min padded buffer=" << n << endl;
  unsigned int log2n;
  // Choose next power of 2 for maximal efficiency.
  for(log2n=0; n > ((unsigned int) 1 << log2n); log2n++);
  return 1 << log2n;
}

int main(int argc, char* argv[])
{
#ifndef __SSE2__
  fftw::effort |= FFTW_NO_SIMD;
#endif  
  
#ifdef __GNUC__	
  optind=0;
#endif	
  for (;;) {
    int c = getopt(argc,argv,"deiptM:N:m:x:y:n:");
    if (c == -1) break;
		
    switch (c) {
      case 0:
        break;
      case 'd':
        Direct=true;
        break;
      case 'e':
        Implicit=false;
        break;
      case 'i':
        Implicit=true;
        break;
      case 'p':
        Implicit=false;
        break;
      case 'M':
        M=atoi(optarg);
        break;
      case 'N':
        N=atoi(optarg);
        break;
      case 'm':
        mx=my=mz=atoi(optarg);
        break;
      case 'x':
        mx=atoi(optarg);
        break;
      case 'y':
        my=atoi(optarg);
        break;
      case 'z':
        mz=atoi(optarg);
        break;
      case 'n':
        N0=atoi(optarg);
        break;
    }
  }

  nx=padding(mx);
  ny=padding(my);
  nz=padding(mz);
  
  cout << "nx=" << nx << ", ny=" << ny << ", nz=" << ny << endl;
  cout << "mx=" << mx << ", my=" << my << ", mz=" << mz << endl;
  
  if(N == 0) {
    N=N0/(nx*ny*nz);
    if(N < 10) N=10;
  }
  cout << "N=" << N << endl;
    
  size_t align=sizeof(Complex);
  nxp=2*mx-1;
  nyp=2*my-1;
  nzp=mz;
  unsigned int nxp0=Implicit ? nxp*M : nxp;
  array3<Complex> f(nxp0,nyp,nzp,align);
  array3<Complex> g(nxp0,nyp,nzp,align);

  double offset=0.0, mean=0.0, sigma=0.0;
  double *T=new double[N];
  offset=emptytime(T,N);

  if(Implicit) {
    ImplicitHConvolution3 C(mx,my,mz,M);
    for(unsigned int i=0; i < N; ++i) {
      init(f,g,M);
      seconds();
      C.convolve(f,g);
      T[i]=seconds();
    }
    
    timings(T,N,offset,mean,sigma);
    cout << "\nImplicit:\n" << mean << "\t" << sigma << "\n" << endl;

    if(nxp*nyp*mz < outlimit) {
      for(unsigned int i=0; i < nxp; ++i) {
        for(unsigned int j=0; j < nyp; ++j) {
          for(unsigned int k=0; k < mz; ++k)
            cout << f[i][j][k] << "\t";
          cout << endl;
        }
        cout << endl;
      }
    } else cout << f[0][0][0] << endl;
  }
  
  if(Direct) {
    array3<Complex> h(nxp,nyp,mz,align);
    array3<Complex> f(nxp,nyp,mz,align);
    array3<Complex> g(nxp,nyp,mz,align);
    DirectHConvolution3 C(mx,my,mz);
    init(f,g);
    seconds();
    C.convolve(h,f,g);
    mean=seconds();
  
    cout << "\nDirect:\n" << mean << "\t" << "0" << "\n" << endl;

    if(nxp*nyp*mz < outlimit)
      for(unsigned int i=0; i < nxp; ++i) {
        for(unsigned int j=0; j < nyp; ++j) {
          for(unsigned int k=0; k < mz; ++k)
            cout << h[i][j][k] << "\t";
          cout << endl;
        }
        cout << endl;
      }
    else cout << h[0][0][0] << endl;
    
  }
}
