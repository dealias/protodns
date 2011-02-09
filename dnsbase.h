#ifndef __dnsbase_h__
#define __dnsbase_h__ 1

#include "options.h"
#include "kernel.h"
#include "Array.h"
#include "ArrayL.h"
#include "fftw++.h"
#include "convolution.h"
#include "Forcing.h"
#include "InitialCondition.h"
#include "Conservative.h"
#include "Exponential.h"
#include <sys/stat.h> // On Sun computers this must come after xstream.h

static const double sqrt2=sqrt(2.0);


using namespace Array;
using namespace fftwpp;
using std::ostringstream;

extern unsigned spectrum;
extern unsigned casimir;

extern int pH;
extern int pL;

extern int circular;

class DNSBase {
protected:
  // Vocabulary:
  unsigned Nx;
  unsigned Ny;
  Real nuH, nuL;
  static const int xpad,ypad;
  
  enum Field {OMEGA,TRANSFER,TRANSFERN,EK};
  enum SPEC {NOSPECTRUM, BINNED, INTERPOLATED, RAW}; 

  // derived variables:
  unsigned mx, my; // size of data arrays
  unsigned origin; // linear index of Fourier origin.
  unsigned xorigin; // horizontal index of Fourier origin.

  Real k0; // grid spacing factor
  Real k02; // k0^2
  array2<Complex> w; // Vorticity field
  array2<Real> wr; // Inverse Fourier transform of vorticity field;

  int tcount;
  Real etanorm;

  unsigned (DNSBase::*Sindex)(unsigned, unsigned, Real); 
  unsigned SkBIN(unsigned I, unsigned j, Real k) {return (unsigned)(k-0.5);}
  unsigned SkRAW(unsigned I, unsigned j, Real k) {return  kval[I][j];}
  
  unsigned nmode;
  unsigned nshells;  // Number of spectral shells
  array1<unsigned> R2; //radii achieved for discrete spectrum
  array2L<unsigned> kval; // for discrete spectrum
  array2L<unsigned> area, areadown; // for interpolated spectrum


  array2<Complex> f0,f1,g0,g1;
  array2<Complex> buffer;
  Complex *F[2];
  Complex *G[2];
  Complex *block;
  ImplicitHConvolution2 *Convolution;
  ExplicitHConvolution2 *Padded;

  ifstream ftin;
  oxstream fwk,fw,fekvk,ftransfer;
  ofstream ft,fevt;
  
  void CasimirTransfer(const vector2& Src, const vector2& Y);
  ImplicitHTConvolution2 *TConvolution;
  oxstream ftransferN;
  Array2<Complex> f,g,h;
  vector Tn;

  void setSindex() {
    switch(spectrum) {
    case NOSPECTRUM:
      Sindex=NULL;
      break;
    case BINNED:
      Sindex=&DNSBase::SkBIN;
      break;
    case INTERPOLATED:
      msg(ERROR,"Interpolated spectrum not done yet.");
      break;
    case RAW:
      Sindex=&DNSBase::SkRAW;
      break;
    default:
      msg(ERROR,"Invalid choice of spectrum.");
    }
  }
  
  void check_rvn(DynVector<unsigned> & R2, const unsigned r2, 
		 const unsigned first)  {
    bool found=false;
    
    unsigned last=R2.Size();
    for(unsigned j=first; j < last; ++j) {
      if(r2 == R2[j]) {
	found=true;
	break;
      }
    }
    if(!found)
      R2.Push(r2);
  }
  

public:
  DNSBase() {setSindex();}
  DNSBase(unsigned Nx, unsigned my, Real k0): Nx(Nx), my(my), k0(k0) {
    block=ComplexAlign(3*Nx*my);
    mx=(Nx+1)/2;
    xorigin=mx-1;
    origin=xorigin*my;
    w.Dimension(Nx,my);
    f0.Dimension(Nx,my);
    f1.Dimension(Nx,my,block);
    g0.Dimension(Nx,my,block+Nx*my);
    g1.Dimension(Nx,my,block+2*Nx*my);
    F[0]=f0;
    F[1]=f1;
    G[0]=g0;
    G[1]=g1;
    Convolution=new ImplicitHConvolution2(mx,my,2);
  
    setSindex();
  }
  virtual ~DNSBase() {}

  void CountAxesDiag(array1<unsigned>::opt &C, unsigned I)  {
    C[(this->*Sindex)(I,I,sqrt2*I)] += 2;
    C[(this->*Sindex)(I,0,I)] += 2;
  }
  void CountMain(array1<unsigned>::opt &C, unsigned I, unsigned j) {
    C[(this->*Sindex)(I,j,sqrt(I*I+j*j))] += 4;
  }

  void SpectrumAxesDiag(vector& S,Complex wd0,Complex wd1,
			Complex wa0,Complex wa1,unsigned I)  {
    // diagonals
    unsigned I2=I*I;
    Real Wall=abs2(wd0)+abs2(wd1);
    Real k=sqrt2*I;
    S[(this->*Sindex)(I,I,k)] += Complex(Wall/(k0*k),nuk(I2+I2)*Wall);
    //S[kval[I][I]] += Complex(Wall/(k0*k),nuk(2*I2)*Wall);
    // axes
    Wall=abs2(wa0)+abs2(wa1);
    S[(this->*Sindex)(I,0,I)] += Complex(Wall/(k0*I),nuk(I2)*Wall);
    //S[kval[I][0]] += Complex(Wall/(k0*I),nuk(I2)*Wall);
  }    
  void SpectrumMain(vector& S,Complex w0,Complex w1,Complex w2,Complex w3,
		    unsigned I,unsigned j) {
    Real Wall=abs2(w1)+abs2(w2)+abs2(w2)+abs2(w3);
    unsigned k2=I*I+j*j;
    Real k=sqrt(k2);
    S[(this->*Sindex)(I,j,k)] += Complex(Wall/(k0*k),nuk(k2)*Wall);
    //S[kval[I][j]] += Complex(Wall/(k0*k),nuk(k2)*Wall);
  }

  void TransferAxesDiag(vector& T,
			Complex wd0,Complex wd1,Complex wa0,Complex wa1,
			Complex fd0,Complex fd1,Complex fa0,Complex fa1,
			unsigned I)  {
    //T[kval[I][I]] += realproduct(wd0,fd0) + realproduct(wd1,fd1);
    //T[kval[I][0]] += realproduct(wa0,fa0) + realproduct(wa1,fa1);
    
    T[(this->*Sindex)(I,I,sqrt2*I)]+=realproduct(wd0,fd0)+ realproduct(wd1,fd1);
    T[(this->*Sindex)(I,0,I)] += realproduct(wa0,fa0) + realproduct(wa1,fa1);
  }
  void TransferMain(vector& T,
		    Complex w0,Complex w1,Complex w2,Complex w3,
		    Complex f0,Complex f1,Complex f2,Complex f3,
		    unsigned I,unsigned j) {
    T[(this->*Sindex)(I,j,sqrt(I*I+j*j))] 
      //T[kval[I][j]]
      += realproduct(w0,f0) + realproduct(w1,f1) 
      +  realproduct(w2,f2) + realproduct(w3,f3);
  }

  unsigned getNx() {return Nx;}
  unsigned getmx() {return mx;}
  unsigned getmy() {return my;}
  Real getk0() {return k0;}
  Real getk02() {return k02;}
  unsigned getxorigin() {return xorigin;}
  Real getetanorm() {return etanorm;}
  unsigned getkval(const unsigned i, const unsigned j) {return kval[i][j];}
  unsigned getR2(const unsigned i) {return R2[i];}

  void InitialConditions();
  void Initialize();
  virtual void setcount();
  void setcountBINNED();
  void setcountRAW(unsigned lambda2=1);
  //  virtual void Output(int it)=0;
  void FinalOutput();
  void OutFrame(int it);

  virtual void Spectrum(vector& S, const vector& y);
  void Transfer(const vector2& Src, const vector2& Y);
  void NonLinearSource(const vector& Src, const vector& Y, double t);
  void LinearSource(const vector& Src, const vector& Y, double t);

  void ConservativeSource(const vector2& Src, const vector2& Y, double t) {
    NonLinearSource(Src[OMEGA],Y[OMEGA],t);
    if(spectrum != NOSPECTRUM) Transfer(Src,Y);
    LinearSource(Src[OMEGA],Y[OMEGA],t);
  }

  void NonConservativeSource(const vector2& Src, const vector2& Y, double t) {
    if(spectrum != NOSPECTRUM) Spectrum(Src[EK],Y[OMEGA]);
//    HermitianSymmetrizeX(mx,my,xorigin,Src[OMEGA]);
  }

  void ExponentialSource(const vector2& Src, const vector2& Y, double t) {
    NonLinearSource(Src[OMEGA],Y[OMEGA],t);
    if(spectrum != NOSPECTRUM) Transfer(Src,Y);
    NonConservativeSource(Src,Y,t);
  }
  void Source(const vector2& Src, const vector2& Y, double t) {
    ConservativeSource(Src,Y,t);
    NonConservativeSource(Src,Y,t);
  }
  Nu LinearCoeff(unsigned k) {
    unsigned i=k/my;
    unsigned j=k-my*i;
    return nuk(k02*(i*i+j*j));
  }

  // TODO: consider using a lookup table on i2.
  Real nuk(unsigned i2) {
    double k2=i2*k02;
    return nuL*pow(k2,pL)+nuH*pow(k2,pH);
  }

  virtual void ComputeInvariants(const array2<Complex>&, Real&, Real&, Real&);
  void Stochastic(const vector2& Y, double, double);

  array1<unsigned>::opt count;
  vector T; // Transfer
  virtual Real getSpectrum(unsigned i) {
    double c=count[i];
    return c > 0 ? T[i].re*twopi/c : 0.0;
  }
  Real Dissipation(unsigned i) {return T[i].im;}
  Real Pi(unsigned i) {return T[i].re;}
  Real Eta(unsigned i) {return T[i].im;}
  Real kb(unsigned i) {
    if(spectrum == RAW)
      return i == 0 ? 0.5*k0 : k0*sqrt((Real) R2[i-1]);
    return k0*(i+0.5);
  }
  Real kc(unsigned i) {
    if(spectrum == RAW) 
      return k0*sqrt((Real) R2[i]);
    return k0*(i+1);
  }

  void findrads(DynVector<unsigned> &R2, array1<unsigned> nr, 
		unsigned m=0, Real lambda=0, unsigned Invisible=0)
  {
    for(unsigned i=1; i < my; ++i) {
      //unsigned start=nr[(unsigned) floor(sqrt((i-1)/2))];
      double nrstart=floor(sqrt((i-1)/2));
      unsigned start=nr[nrstart > 1 ? (unsigned) nrstart -1 : 0];
      start=0; // FIXME: restore and optimize.
      for(unsigned x=i-1; x <= i; ++x) {
	unsigned x2=x*x;
	unsigned ystopnow= i < x ? i : x;
	for(unsigned y= x == 0 ? 1 : 0; y <= ystopnow; ++y) {
	  if(isvisible(x,y,m,lambda,Invisible)) {
	    check_rvn(R2,x2+y*y,start);
	  }
	}
      }
      nr[i]=R2.Size();
    }
  }

  void killmodes(array2<Complex> &A) {
    if(circular) {
      unsigned m2=my*my;
      for(unsigned i=0; i < Nx; ++i) {
	vector Ai=A[i];
	unsigned I= i > xorigin ? i-xorigin : xorigin-i; 
	unsigned start=(unsigned) ceil(sqrt(m2-I*I));
	for(unsigned j=start; j < my; ++j) {
	  Ai[j]=0.0;
	}
      }
    }
  }

  virtual bool isvisible(unsigned I, unsigned j, 
			 unsigned m=0, Real lambda=0, unsigned Invsible=0) {
    if(circular) 
      return I*I + j*j <= my*my;
    return true;
  }

  virtual unsigned diagstart() {return 1;}
  virtual unsigned diagstop() {
    if(circular) 
      return (unsigned) ceil((my-1)/sqrt(2.0));
    return mx;
  }
  virtual unsigned mainjstart(unsigned I) {return 1;}
  virtual unsigned mainjstop(unsigned I) {
    if(circular) 
      return min(I,(unsigned) ceil(sqrt((my-1)*(my-1)-I*I)));
    return I;
  }
  virtual unsigned xoriginstart() {return 1;}
  virtual unsigned xoriginstop() {return my;}
  virtual unsigned bottomstart() {return 1;}
  virtual unsigned bottomstop() {return mx;}
};

//***** initial conditions *****//

extern InitialConditionBase *InitialCondition;
class Zero : public InitialConditionBase {
public:
  const char *Name() {return "Zero";}
  void Set(Complex *w, unsigned n) {
    for(unsigned i=0; i < n; i++)
      w[i]=0.0;
  }
};

//***** loop class *****//
typedef void (DNSBase::*Sad)(vector&,Complex,Complex,Complex,Complex,unsigned);
typedef void (DNSBase::*Sm)(vector&,Complex,Complex,Complex,Complex,
			    unsigned,unsigned);
typedef void (DNSBase::*Tad)(vector&,
			     Complex,Complex,Complex,Complex,
			     Complex,Complex,Complex,Complex,unsigned);
typedef void (DNSBase::*Tm)(vector&,
			     Complex,Complex,Complex,Complex,
			     Complex,Complex,Complex,Complex,
			     unsigned,unsigned);
typedef void (DNSBase::*Cad)(array1<unsigned>::opt&,unsigned);
typedef void (DNSBase::*Cm)(array1<unsigned>::opt&,unsigned,unsigned);

class Hloop{
 private:
  unsigned Nx, my, mx, xorigin;
  DNSBase *parent;
 public:
  Hloop() {};
  Hloop(DNSBase *parent0) {
    parent=parent0;
    Nx=parent->getNx();
    my=parent->getmy();
    mx=parent->getmx();
    xorigin=parent->getxorigin();
  };
  ~Hloop() {};
  
  virtual bool isvisible(unsigned i, unsigned j) {return true;}
  
  // loop over the Hermitian-symmetric array2 w, calculate
  // something, put it into the appropriate index of S, a spectrum-like array
  void Sloop(vector& S, const array2<Complex> w,Sad  adfp, Sm mfp) {
    vector wx=w[xorigin];
    for(unsigned I=1; I < mx; I++) {
      unsigned i=xorigin+I;
      unsigned im=xorigin-I;
      vector wi=w[i];
      vector wim=w[im];
      vector wxi=w[xorigin+I];
      for(unsigned j=1; j < I; ++j)
	(parent->*mfp)(S,wi[j],wim[j],w[xorigin-j][I],w[xorigin+j][I],I,j);
      (parent->*adfp)(S,wi[I],wim[I],wx[I],wxi[0],I);
    }
  }

  void Tloop(vector& T, const array2<Complex> w, const array2<Complex> f,
	     Tad adfp, Tm mfp) {
    vector wx=w[xorigin];
    vector fx=f[xorigin];
    for(unsigned I=1; I < mx; I++) {
      unsigned i=xorigin+I;
      unsigned im=xorigin-I;
      vector wi=w[i];
      vector wim=w[im];
      vector fi=f[i];
      vector fim=f[im];
      for(unsigned j=1; j < I; ++j)
	(parent->*mfp)(T,
		       wi[j],wim[j],w[xorigin-j][I],w[xorigin+j][I],
		       fi[j],fim[j],f[xorigin-j][I],f[xorigin+j][I],I,j);
      (parent->*adfp)(T,
		      wi[I],wim[I],wx[I],w[xorigin+I][0],
		      fi[I],fim[I],fx[I],f[xorigin+I][0],I);
    }
  }

  void Cloop(array1<unsigned>::opt &C, Cad adfp, Cm mfp) {
    for(unsigned I=1; I < mx; I++) {
      (parent->*adfp)(C,I);
      for(unsigned j=1; j < I; ++j) {
	(parent->*mfp)(C,I,j);
      }
    }
  }

  void Rloop(DynVector<unsigned> &R2) {
    DynVector<unsigned> temp;
    for(unsigned I=1; I < mx; I++) {
      unsigned I2=I*I;
      temp.Push(I2);
      temp.Push(2*I2);
      for(unsigned j=1; j < I; ++j) {
	temp.Push(I2+j*j);
      }
    }
    temp.sort();
    R2.Push(temp[0]);
    unsigned last=0;
    for(unsigned i=1; i < temp.Size(); ++i) {
      if(temp[i] != R2[last])
	R2[++last]=temp[i];
    }
  }
  
};


#endif
