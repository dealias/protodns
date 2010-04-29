#include "options.h"
#include "kernel.h"
#include "Array.h"
#include "fftw++.h"
#include "Forcing.h"
#include "InitialCondition.h"
#include "convolution.h"
#include "Conservative.h"

#include <sys/stat.h> // On Sun computers this must come after xstream.h

using namespace Array;

const double ProblemVersion=1.0;

const char *method="DNS";
const char *integrator="RK5";
const char *forcing="WhiteNoiseBanded";
const char *ic="Equilibrium";

ForcingBase *Forcing;
InitialConditionBase *InitialCondition;

// Vocabulary
//Real nu=1.0;
//Real rho=1.0;
//Real ICvx=1.0;
//Real ICvy=1.0;
unsigned Nx=1;
unsigned Ny=1;
//Real xmin=0.0;
//Real xmax=1.0;
//Real ymin=0.0;
//Real ymax=1.0;
//Real force=1.0;
//Real kforce=1.0;
//Real deltaf=2.0;
int movie=0;
int rezero=0;

enum Field {OMEGA,EK};

class DNSVocabulary : public VocabularyBase {
public:
  const char *Name() {return "Direct Numerical Simulation of Turbulence";}
  const char *Abbrev() {return "DNS";}
  DNSVocabulary();
  
  Table<ForcingBase> *ForcingTable;
  Table<InitialConditionBase> *InitialConditionTable;
  
  ForcingBase *NewForcing(const char *& key) {
    return ForcingTable->Locate(key);
  }
  InitialConditionBase *NewInitialCondition(const char *& key) {
    return InitialConditionTable->Locate(key);
  }
};
   
class DNS : public ProblemBase {
  unsigned mx, my; // size of data arrays
  unsigned origin; // linear index of Fourier origin.
  unsigned xorigin; // horizontal index of Fourier origin.

  Real kx0, ky0; // grid spacing factor
  array2<Complex> w; // Vorticity field
  array2<Real> wr; // Inverse Fourier transform of vorticity field;
  
  int tcount;
  array1<unsigned>::opt count;
  
  unsigned nmode;
	
//  array3<Complex> S;
  
  unsigned nshells;  // Number of spectral shells
  
  rvector spectrum;
  
  array3<Real> ForceMask;
  array3<Complex> FMk;
  
  array2<Complex> f0,f1,g0,g1;
  Complex *F[2];
  Complex *G[2];
  
  ImplicitHConvolution2 *Convolution;
  
  crfft2d *cr;
  
public:
  DNS();
  virtual ~DNS() {}
  
  void IndexLimits(unsigned& start, unsigned& stop,
		   unsigned& startT, unsigned& stopT,
		   unsigned& startM, unsigned& stopM) {
    start=Start(OMEGA);
    stop=Stop(OMEGA);
    startT=0;//Start(TRANSFER);
    stopT=0;//Stop(TRANSFER);
    startM=Start(EK);
    stopM=Stop(EK);
  }
  unsigned getNx() {return Nx;}
  unsigned getmy() {return my;}
  Real getkx0() {return kx0;}
  Real getky0() {return ky0;}
  unsigned getxorigin() {return xorigin;}


  void InitialConditions();
  void Initialize();
  void Output(int it);
  void FinalOutput();
  void OutFrame(int it);
  void NonLinearSource(const vector2& Src, const vector2& Y, double t);
  void LinearSource(const vector2& Src, const vector2& Y, double t);
  
  void ConservativeSource(const vector2& Src, const vector2& Y, double t) {
    NonLinearSource(Src,Y,t);
    LinearSource(Src,Y,t);
  }
  void NonConservativeSource(const vector2& Src, const vector2& Y, double t) {}
  void ExponentialSource(const vector2& Src, const vector2& Y, double t) {
    NonLinearSource(Src,Y,t);
    NonConservativeSource(Src,Y,t);
  }
  void Source(const vector2& Src, const vector2& Y, double t) {
    ConservativeSource(Src,Y,t);
    NonConservativeSource(Src,Y,t);
  }
  void ComputeInvariants(Real& E, Real& Z, Real& P);
  void Stochastic(const vector2& Y, double, double);
  
  void Spectrum(vector& S, const vector& y);
};

DNS *DNSProblem;

class Zero : public InitialConditionBase {
public:
  const char *Name() {return "Zero";}
  void Set(Var *w, unsigned n) {
    for(unsigned i=0; i < n; i++) 
      w[i]=0.0;
  }
};

class Equilibrium : public InitialConditionBase {
public:
  const char *Name() {return "Equilibrium";}
  void Set(Var *w0, unsigned n) {
    unsigned Nx=DNSProblem->getNx();
    unsigned my=DNSProblem->getmy();
    array2<Var> w(Nx,my,w0);
    unsigned xorigin=DNSProblem->getxorigin();
    Real kx0=DNSProblem->getkx0();
    Real ky0=DNSProblem->getky0();
    for(unsigned i=0; i < Nx; i++) {
      Real kx=kx0*((int) i-(int) xorigin);
      vector wi=w[i];
      for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
        wi[j]=pow(1.0/Complex(hypot(kx,ky0*j)+1),2.0); // FIXME
      }
    }
      
// For movie testing:
// w(xorigin,my-1)=2.0;
//  w(mx-1,0)=2.0;
  }
};


DNSVocabulary::DNSVocabulary()
{
  Vocabulary=this;

  //  VOCAB(rho,0.0,REAL_MAX,"density");
  //  VOCAB(nu,0.0,REAL_MAX,"Kinematic viscosity");
  //  VOCAB(force,0.0,REAL_MAX,"force coefficient");
  //  VOCAB(kforce,0.0,REAL_MAX,"forcing wavenumber");
  //  VOCAB(deltaf,0.0,REAL_MAX,"forcing band width");
  VOCAB_NOLIMIT(ic,"Initial Condition");
  
  //  VOCAB(ICvx,0.0,0.0,"Initial condition multiplier for vx");
  //  VOCAB(ICvy,0.0,0.0,"Initial condition multiplier for vy");

  VOCAB(Nx,1,INT_MAX,"Number of dealiased modes in x direction");
  VOCAB(Ny,1,INT_MAX,"Number of dealiased modes in y direction");
  
  //  VOCAB(xmin,0.0,0.0,"Minimum x value");
  //  VOCAB(xmax,0.0,0.0,"Maximum x value");
	
  //  VOCAB(ymin,0.0,0.0,"Minimum y value");
  //  VOCAB(ymax,0.0,0.0,"Maximum y value");

  VOCAB(movie,0,1,"Movie flag (0=off, 1=on)");
  
  VOCAB(rezero,0,INT_MAX,"Rezero moments every rezero output steps for high accuracy");
  
//  ForcingTable=new Table<ForcingBase>("forcing");
  InitialConditionTable=new Table<InitialConditionBase>("initial condition");
  
  METHOD(DNS);

  INITIALCONDITION(Zero);
  INITIALCONDITION(Equilibrium);
}


DNSVocabulary DNS_Vocabulary;


DNS::DNS()
{
  DNSProblem=this;
  check_compatibility(DEBUG);
  ConservativeIntegrators(DNS_Vocabulary.IntegratorTable,this);
}

ifstream ftin;
ofstream ft,fevt,fu;
oxstream fvx,fvy,fw,fekvk;

void DNS::InitialConditions()
{
  if(Nx % 2 == 0 || Ny % 2 == 0) msg(ERROR,"Nx and Ny must be odd");
  
  kx0=1.0;
  ky0=1.0;
  mx=(Nx+1)/2;
  my=(Ny+1)/2;

  xorigin=mx-1;
  origin=xorigin*my;
  nshells=(unsigned) (hypot(mx-1,my-1)+0.5);
  
  NY[OMEGA]=Nx*my;
  NY[EK]=nshells;
  
  size_t align=sizeof(Complex);  
  
  Allocator(align);
  
  w.Dimension(Nx,my);
  f0.Dimension(Nx,my);
  f1.Allocate(Nx,my,align);
  g0.Allocate(Nx,my,align);
  g1.Allocate(Nx,my,align);
  
  F[1]=f1;
  G[0]=g0;
  G[1]=g1;
    
  //u.Dimension(Nx,Ny);
//  ForceMask.Allocate(Nx,my,align);
  //FMk.Dimension(Nx,my,(Complex *) ForceMask());
  
  cout << "\nGEOMETRY: (" << Nx << " X " << Ny << ")" << endl; 

  cout << "\nALLOCATING FFT BUFFERS (" << mx << " x " << my << ")" << endl;
  
  Convolution=new ImplicitHConvolution2(mx,my,2);

  Allocate(count,nshells);
  
  InitialCondition=DNS_Vocabulary.NewInitialCondition(ic);
  // Initialize the vorticity field.
  w.Set(Y[OMEGA]);
  
  if(movie) {
    wr.Dimension(Nx,2*(Ny/2+1),(double *) g0());
    cr=new crfft2d(Nx,Ny,g0);
  }
  
  InitialCondition->Set(w,NY[OMEGA]);

  HermitianSymmetrizeX(mx,my,xorigin,w);
  
  for(unsigned i=0; i < NY[EK]; i++)
    Y[EK][i]=0.0;
  
  tcount=0;
  if(restart) {
    Real t0;
    ftin.open(Vocabulary->FileName(dirsep,"t"));
    while(ftin >> t0, ftin.good()) tcount++;
    ftin.close();
  }
  
  open_output(ft,dirsep,"t");
  open_output(fevt,dirsep,"evt");
  
  if(!restart) 
    remove_dir(Vocabulary->FileName(dirsep,"ekvk"));
  mkdir(Vocabulary->FileName(dirsep,"ekvk"),0xFFFF);
  
  errno=0;
    
  if(output) 
    open_output(fu,dirsep,"u");
  
  if(movie)
    open_output(fw,dirsep,"w");
}

void DNS::Initialize()
{
  fevt << "#   t\t\t E\t\t\t Z" << endl;
  for(unsigned i=0; i < Nx; i++) {
    int I=(int) i-(int) xorigin;
    int I2=I*I;
    vector wi=w[i];
    for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
      unsigned K=(unsigned)(sqrt(I2+j*j)-0.5);
      count[K]++;
    }
  }
}

void DNS::Spectrum(vector& S, const vector& y)
{
  w.Set(y);
  
  for(unsigned K=0; K < nshells; K++) {
    count[K]=0;
    S[K]=0.0;
  }
				
  // Compute instantaneous angular average over circular shell.
		
  for(unsigned i=0; i < Nx; i++) {
    int I=(int) i-(int) xorigin;
    int I2=I*I;
    vector wi=w[i];
    for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
      unsigned K=(unsigned)(sqrt(I2+j*j)-0.5);
      S[K] += abs2(wi[j]);
    }
  }
  
  for(unsigned K=0; K < nshells; K++)
    if(count[K]) S[K] *= twopi*K/count[K];
}

void DNS::Output(int it)
{
  Real E,Z,P;
	
  w.Set(y);
  ComputeInvariants(E,Z,P);
  fevt << t << "\t" << E << "\t" << Z << "\t" << P << endl;

  Var *y=Y[0];
  if(output) out_curve(fu,y,"u",NY[0]);
  
  if(movie) OutFrame(it);
	
  ostringstream buf;
  buf << "ekvk" << dirsep << "t" << tcount;
  open_output(fekvk,dirsep,buf.str().c_str(),0);
  out_curve(fekvk,t,"t");
  Var *y1=Y[EK];
  out_curve(fekvk,y1,"ekvk",nshells);
  fekvk.close();
  if(!fekvk) msg(ERROR,"Cannot write to file ekvk");
    
  tcount++;
  ft << t << endl;
  
  if(rezero && it % rezero == 0) {
    vector2 Y=Integrator->YVector();
    vector T=Y[EK];
    for(unsigned i=0; i < NY[EK]; i++)
      T[i]=0.0;
  }
}

void DNS::OutFrame(int)
{
  g0.Load(Y[OMEGA]);
  cr->fft0(g0);

  fw << 1 << Ny << Nx;

  for(int j=Ny-1; j >= 0; j--) {
    for(unsigned i=0; i < Nx; i++) {
      fw << (float) wr(i,j);
    }
  }
  
  fw.flush();
}	

void DNS::ComputeInvariants(Real& E, Real& Z, Real& P)
{
  E=Z=P=0.0;
  w.Set(Y[OMEGA]);
  for(unsigned i=0; i < Nx; i++) {
    Real kx=kx0*((int) i-(int) xorigin);
    Real kx2=kx*kx;
    vector wi=w[i];
    for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
      Real w2=abs2(wi[j]);
      Z += w2;
      Real ky=ky0*j;
      Real k2=kx2+ky*ky;
      E += w2/k2;
      P += k2*w2;
    }
  }
}

void DNS::FinalOutput()
{
  Real E,Z,P;
  ComputeInvariants(E,Z,P);
  cout << endl;
  cout << "Energy = " << E << newl;
  cout << "Enstrophy = " << Z << newl;
  cout << "Palenstrophy = " << P << newl;
}

void DNS::LinearSource(const vector2& Src, const vector2& Y, double)
{
}

void DNS::NonLinearSource(const vector2& Src, const vector2& Y, double)
{
  w.Set(Y[OMEGA]);
  f0.Set(Src[OMEGA]);
 
  f0(origin)=0.0;
  f1(origin)=0.0;
  g0(origin)=0.0;
  g1(origin)=0.0;
  
  for(unsigned i=0; i < Nx; ++i) {
    Real kx=kx0*((int) i-(int) xorigin);
    Real kx2=kx*kx;
    vector wi=w[i];
    vector f0i=f0[i];
    vector f1i=f1[i];
    vector g0i=g0[i];
    vector g1i=g1[i];
    for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
      Real ky=ky0*j;
      Complex wij=wi[j];
      Complex kxw=Complex(-kx*wij.im,kx*wij.re);
      Complex kyw=Complex(-ky*wij.im,ky*wij.re);
      f0i[j]=kxw;
      f1i[j]=kyw;
      Real k2inv=1.0/(kx2+ky*ky);
      g0i[j]=k2inv*kyw;
      g1i[j]=-k2inv*kxw;
    }
  }
  
  F[0]=f0;
  Convolution->convolve(F,G);
  
#if 0
  double sum=0.0;
  for(unsigned i=0; i < Nx; ++i) {
    vector wi=w[i];
    for(unsigned j=i <= xorigin ? 1 : 0; j < my; ++j) {
      Complex wij=wi[j];
      sum += (f0[i][j]*conj(wij)).re;
    }
  }
  
  cout << sum << endl;
#endif  
  
  Spectrum(Src[EK],Y[OMEGA]);
}

void DNS::Stochastic(const vector2&Y, double, double)
{
//  u.Set(Y[OMEGA]);
//  Real factor=sqrt(2.0*dt)*rand_gauss();
}
