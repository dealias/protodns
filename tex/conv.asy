int p=2,q=3;
write((string) p +"/" +(string) q +" padding");

// Return the inverse Fourier transform of size n for a Hermitian vector f
// of length (n/2+1). The flag even indicates whether n is even.
real[] crfft(pair[] f, bool even=true, int sign=1)
{
  int m=f.length;
  int L=even ? 2m-2 : 2m-1;
  pair[] h=new pair[L];
  h[0]=f[0];
  if(m > 1) {
    for(int i=1; i < m-1; ++i) {
      h[i]=f[i];
      h[L-i]=conj(f[i]);
    }
    h[m-1]=f[m-1];
    if(!even) h[m]=conj(f[m-1]);
  }
  return map(xpart,fft(h,sign));
}

// Return the non-negative Fourier spectrum of a real vector f.
pair[] rcfft(real[] f, int sign=-1)
{
  return fft(f,sign)[0:quotient(f.length,2)+1];
}

// Unrolled scrambled cr version for p=2, q=3
// with n=3m; m even for now
// f has length m, u is a work array of length m/2+1.
real[] crfft0pad(pair[] f, pair[] u, bool unscramble=true)
{
  int m=f.length;
  int c=quotient(m,2);
  assert(2c == m);
  
  int n=3*m;

  pair zeta=exp(2*pi*I/n);
  pair zetac=conj(zeta);
  pair zeta3=(-0.5,0.5*sqrt(3.0));

  real f0=f[0].x;
  u[0]=f0;
  pair fc=f[c];
  int m1=m-1;
  pair fmk=conj(f[m1]);
  f[m1]=f0;
  pair Zetak=zetac;
  for(int k=1; k < c; ++k) {
    pair fk=f[k];
    f[k]=fk+fmk;
    pair A=Zetak*(fk.x+zeta3*fmk.x);
    pair B=-I*Zetak*(fk.y+zeta3*fmk.y);
    Zetak *= zetac;
    u[k]=A-B;
    int mk=m1-k;
    fmk=conj(f[mk]);
    f[mk]=A+B;
  }
  
  real A=fc.x;
  real B=sqrt(3)*fc.y;
  fc=f[c];
  f[c]=2.0*A;
  u[c]=A+B;
  A -= B;

  real[] f0=crfft(f[0:c+1],true);
  //  real fcm1=f[c-1];

  f[c-1]=A;
  f[c]=fc;
  real[] f1=crfft(f[c-1:m],true);
  //  f[m-1]=fcm1; // This is where we will store f0[c-1] in scrambled format.

  // Not necessary for convolution.
  for(int i=1; i < m; i += 2)
    f1[i]=-f1[i];

  real[] f2=crfft(u,true);
  
  real[] h;
  
  if(unscramble) {
    h=new real[3c];
  
    h[0]=f0[0];
    h[1]=f1[0];
    h[3*m-1]=f2[0];
    for(int i=1; i < m; ++i) {
      h[3*i-1]=f2[i];
      h[3*i]=f0[i];
      h[3*i+1]=f1[i];
    }
  } else h=concat(f0,f1,f2);
  
  return h;
}

// Unrolled scrambled rc version for p=2, q=3
// with n=3m
// f has length 3m.
pair[] rcfft0padinv(real[] f, bool unscramble=true)
{
  assert(!unscramble); // Not yet implemented
  
  int n=f.length;
  int m=quotient(n,3);
  assert(n == 3m);
  int c=quotient(m,2);
  
  pair zeta=exp(2*pi*I/n);
  
  pair[] f0=rcfft(f[0:m]);
  pair[] f1=rcfft(f[m:2m]);
  pair[] f2=rcfft(f[2m:n]);

  pair[] F=new pair[m];

  pair zeta3=(-0.5,0.5*sqrt(3.0));

  int stop=m-c-1;
  real ninv=1/n;
  F[0]=(f0[0].x+f1[0].x+f2[0].x)*ninv;
  pair Zetak=zeta*ninv;
  for(int k=1; k <= stop; ++k) {
    pair f0k=f0[k]*ninv;
    pair f1k=conj(Zetak)*f1[k];
    pair f2k=Zetak*f2[k];
    Zetak *= zeta;
    F[k]=f0k+f1k+f2k;
    F[m-k]=conj(f0k)+conj(zeta3*f1k)+zeta3*conj(f2k);
  }
  
  if(2c == m) F[c]=(f0[c].x-f1[c].x*zeta3-f2[c].x*conj(zeta3))*ninv;

  return F;
}

// f and g have length m.
// u and v are work arrays each of length m/2+1.
pair[] convolve0(pair[] f, pair[] g, pair[] u, pair[] v)
{
  int m=f.length;
  if(m == 1) return new pair[] {f[0]*g[0]};
  int c=quotient(m,2);
  
  bool even=2c == m;

  int n=3*m;
  
  pair zeta=exp(2*pi*I/n);
  pair zetac=conj(zeta);
  pair zeta3=(-0.5,0.5*sqrt(3.0));

  real f0=f[0].x;
  real g0=g[0].x;
  u[0]=f0;
  v[0]=g0;
  pair fc=f[c];
  int m1=m-1;
  pair fmk=conj(f[m1]);
  f[m1]=f0;
  pair gc=g[c];
  pair gmk=conj(g[m1]);
  g[m1]=g0;
  pair Zetak=zetac;
  for(int k=1; k < c; ++k) {
    pair fk=f[k];
    f[k]=fk+fmk;
    pair A=Zetak*(fk.x+zeta3*fmk.x);
    pair B=-I*Zetak*(fk.y+zeta3*fmk.y);
    u[k]=A-B;
    int mk=m1-k;
    fmk=conj(f[mk]);
    f[mk]=A+B; // Store conjugate of desired quantity in reverse order.

    pair gk=g[k];
    g[k]=gk+gmk;
    A=Zetak*(gk.x+zeta3*gmk.x);
    B=-I*Zetak*(gk.y+zeta3*gmk.y);
    Zetak *= zetac;
    v[k]=A-B;
    gmk=conj(g[mk]);
    g[mk]=A+B;
  }

  pair A,C;
  if(even) {
    A=fc.x;
    real B=sqrt(3)*fc.y;
    fc=f[c];
    f[c]=2.0*A;
    u[c]=A+B;
    A -= B;

    C=gc.x;
    B=sqrt(3)*gc.y;
    gc=g[c];
    g[c]=2.0*C;
    v[c]=C+B;
    C -= B;
  } else {
    f[c]=fc+fmk;
    A=Zetak*(fc.x+zeta3*fmk.x);
    pair B=I*Zetak*(fc.y+zeta3*fmk.y);
    u[c]=A+B;
    A -= B;

    g[c]=gc+gmk;
    C=Zetak*(gc.x+zeta3*gmk.x);
    B=I*Zetak*(gc.y+zeta3*gmk.y);
    Zetak *= zetac;
    v[c]=C+B;
    C -= B;
  }
      
  real[] f0=crfft(f[0:c+1],even);
  real[] g0=crfft(g[0:c+1],even);
  f0 *= g0;
  pair[] f0=rcfft(f0);
  int start=m-c-1;
  pair overlap0=f0[start];
  real overlap1=f0[c].x;

  f[start]=A;
  if(even)
    f[c]=fc;
  else f[start:m]=reverse(conj(f[start:m]));
  real[] f1=crfft(f[start:m],even);
  g[start]=C;
  if(even) g[c]=gc;
  else g[start:m]=reverse(conj(g[start:m]));
  real[] g1=crfft(g[start:m],even);
  f1 *= g1;
  pair[] f1=rcfft(f1);
  
  real[] f2=crfft(u,even);
  real[] g2=crfft(v,even);
  f2 *= g2;
  pair[] f2=rcfft(f2);

  pair[] F=new pair[m];

  real ninv=1/n;
  F[0]=(f0[0].x+f1[0].x+f2[0].x)*ninv;
  pair Zetak=zeta*ninv;

  for(int k=1; k < start; ++k) {
    pair f0k=f0[k]*ninv;
    pair f1k=conj(Zetak)*f1[k];
    pair f2k=Zetak*f2[k];
    Zetak *= zeta;
    F[k]=f0k+f1k+f2k;
    F[m-k]=conj(f0k)+conj(zeta3*f1k)+zeta3*conj(f2k);
  }

  pair f0k=overlap0*ninv;
  pair f1k=conj(Zetak)*f1[start];
  pair f2k=Zetak*f2[start];

  if(c > 1 || !even) {
    F[start]=f0k+f1k+f2k;
    F[c+1]=conj(f0k+zeta3*f1k)+zeta3*conj(f2k);
  }
  
  if(even)
    F[c]=(overlap1-f1[c].x*zeta3-f2[c].x*conj(zeta3))*ninv;

  return F;
}

pair[] convolve(pair[] F, pair[] G)
{
  int m=F.length;
  int n=3*m-2;
  int n2=quotient(n,2);
  
  F=copy(F);
  G=copy(G);
  
  for(int i=m; i <= n2; ++i) {
    G[i]=0.0;
    F[i]=0.0;
  }
  bool even=n % 2 == 0;
  
  return rcfft((crfft(F,even)*crfft(G,even))/n)[0:m];
}	

pair[] direct(pair[] F, pair[] G)
{
  int m=F.length;
  pair[] H=new pair[m];
  for(int i=0; i < m; ++i) {
    pair sum;
    for(int j=0; j <= i; ++j) sum += F[j]*G[i-j];
    for(int j=i+1; j < m; ++j) sum += F[j]*conj(G[j-i]);
    for(int j=1; j < m-i; ++j) sum += conj(F[j])*G[i+j];
    H[i]=sum;
  }
  return H;
}	

pair[] d={-5,(3,1),(4,-2),(-3,1),(0,-2),(0,1),(4,0),(-3,-1),(1,2),(2,1),(3,1)};
//pair[] d={-5,(3,1),(4,-2),(-3,1),(0,-2),(0,1),(4,1),(-3,-1),(1,2),(2,1),(3,1),3};
//pair[] d={-5,(3,1),(4,-2),(-3,1),(0,-2),(0,1),(4,0),(-3,-1),(1,2),(2,1)};
//pair[] d={-5};

pair[] f=copy(d);
pair[] g=copy(d);
//pair[] g=copy(d+1);

write();

int c=quotient(f.length,2);

pair[] u=new pair[c+1];
pair[] v=new pair[c+1];

write(convolve(f,g));
write();
write(direct(f,g));
write();
write(convolve0(f,g,u,v));
write();

write();
f=copy(d);
//write(f);
//write();
//write(crfft0pad(f,u));
//write(rcfft0padinv(crfft0pad(f,u,false),false));
