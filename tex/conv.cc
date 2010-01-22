using namespace std;
#include "Complex.h"
#include "convolution.h"

// Compile with:
// g++ -g -O3 -msse2 -march=native -mfpmath=sse conv.cc fftw++.cc -lfftw3
//
// g++ -g -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -march=native -msse2 -mfpmath=sse conv.cc fftw++.cc -lfftw3
//
// g++ -g -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -mpentiumpro -msse2 -mfpmath=sse conv.cc fftw++.cc -lfftw3 -I$HOME/include -L$HOME/lib
//
//
// usage: aout [int m]
// optionally specifies the size of m.

// Number of iterations.
unsigned int N=1000;
unsigned int m=4096;
  
using namespace std;

#include <sys/time.h>

inline double seconds()
{
  static timeval lasttime;
  timeval tv;
  gettimeofday(&tv,NULL);
  double seconds=tv.tv_sec-lasttime.tv_sec+
    ((double) tv.tv_usec-lasttime.tv_usec)/1000000.0;
  lasttime=tv;
  return seconds;
}

inline void init(Complex *f, Complex *g) 
{
  f[0]=1.0;
  for(unsigned int i=1; i < m-1; i++) f[i]=Complex(3.0,2.0);
  f[m-1]=3.0;
  g[0]=2.0;
  for(unsigned int i=1; i < m-1; i++) g[i]=Complex(5.0,3.0);
  g[m-1]=2.0;
}


int main(int argc, char* argv[])
{
  int pad=0;
  
  if (argc >= 2) {
    m=atoi(argv[1]);
  }
 
  if (argc >= 3) {
    pad=atoi(argv[2]);
  }
  
//  Complex d[]={-5,Complex(3,1),Complex(4,-2),Complex(-3,1),Complex(0,-2),Complex(0,1),Complex(4,0),Complex(-3,-1),Complex(1,2),Complex(2,1),Complex(3,1)};
	
//  unsigned int m=sizeof(d)/sizeof(Complex);
  unsigned int n=(2*m-1)*3;
  if(n % 2 == 1) ++n;
  n /= 2;
  cout << "min padded buffer=" << n << endl;
  unsigned int log2n;
  // Choose next power of 2 for maximal efficiency.
  for(log2n=0; n > (1 << log2n); log2n++);
  n=1 << log2n;
  cout << "n=" << n << endl;
  cout << "m=" << m << endl;
  
  unsigned int np=pad ? n/2+1 : m;
    
  Complex *f=FFTWComplex(np);
  Complex *g=FFTWComplex(np);
  Complex *h;
  if(pad) h=f;
  else h=FFTWComplex(np);
#ifdef TEST  
  Complex pseudoh[m];
#endif

  /*
  Complex *d=FFTWComplex(m);
  d[0]=1.0;
  for(unsigned int i=1; i < m; i++) d[i]=Complex(3.0,2.0);
  */
  
  double offset=0.0;
  seconds();
  for(int i=0; i < N; ++i) {
    seconds();
    offset += seconds();
  }

  double sum=0.0;
  if(!pad) {
    convolution convolve(m);
    for(int i=0; i < N; ++i) {
      init(f,g);
      seconds();
      convolve.unpadded(h,f,g);
      sum += seconds();
    }
    
    cout << endl;
    cout << "Unpadded:" << endl;
    cout << (sum-offset)/N << endl;
    cout << endl;
    if(m < 100) 
      for(unsigned int i=0; i < m; i++) cout << h[i] << endl;
    else cout << h[0] << endl;
#ifdef TEST    
    for(unsigned int i=0; i < m; i++) pseudoh[i]=h[i];
#endif    
  }
  
  if(pad) {
    sum=0.0;
    convolution Convolve(n,m,f);
    for(int i=0; i < N; ++i) {
      // FFTW out-of-place cr routines destroy the input arrays.
      init(f,g);
      seconds();
      Convolve.fft(h,f,g);
      sum += seconds();
    }
    cout << endl;
    cout << "Padded:" << endl;
    cout << (sum-offset)/N << endl;
    cout << endl;
    if(m < 100) 
      for(unsigned int i=0; i < m; i++) cout << h[i] << endl;
    else cout << h[0] << endl;
    cout << endl;
#ifdef TEST    
    for(unsigned int i=0; i < m; i++) pseudoh[i]=h[i];
#endif
  }
  
  if(!pad) {
    convolution convolve(m);
    init(f,g);
    seconds();
    convolve.direct(h,f,g);
    sum=seconds();
  
    cout << endl;
    cout << "Direct:" << endl;
    cout << sum-offset/N << endl;
    cout << endl;

    if(m < 100) 
      for(unsigned int i=0; i < m; i++) cout << h[i] << endl;
    else cout << h[0] << endl;

    // test accuracy of convolution methods:
    double error=0.0;
#ifdef TEST    
    for(unsigned int i=0; i < m; i++) 
      error += abs2(h[i]-pseudoh[i]);
    cout << "error="<<error<<endl;
    if (error > 1e-12)
      cerr << "Caution! error="<<error<<endl;
#endif
  }
  
  FFTWdelete(f);
  FFTWdelete(g);
  if(!pad) FFTWdelete(h);
}

