size(6cm,0);

string[] outnames={"lambda1","lambdar2","lambdar2rot","lambda2"};
real[] lambda={1,sqrt(2),sqrt(2),2};
pair[] R={(1,0),(1,0),exp(-pi*I/4),(1,0)};

int n0=7;
int ngrids=3; // NB: the following three arrays only work up to ngrids=3
pen[] dotpen={black,blue,deepgreen};
filltype[] dotfill={Fill,NoFill,NoFill};
pen[] dotfillpen={black,invisible,invisible};
int n;
path g;
picture pic;
pen p, fillpen;
real L, s;
pair r;
filltype F;


void drawdots() {
  for(int i=-n; i <=n; ++i) {
    for(int j= i >= 0? 0 : 1; j <=n; ++j) {
      pair a=L*r*(i,j);
      g=scale(s)*unitcircle;
      filldraw(pic,shift(a)*g,fillpen,p);
    }
  }
}

for(int i=0; i < lambda.length; ++i) {
  pic = new picture;
  size(pic,10cm,0);
  
  for(int G=0; G < ngrids; ++G) {
    n=n0;
    if(i==0) n=2^G*n0-1;
    p=dotpen[G];
    fillpen=dotfillpen[G];
    F=dotfill[G];
    r=R[i]^G;
    L=lambda[i]^G;
    s=G*0.1;
    drawdots();
  }

  /*
  // FIXME: this stuff really only applies to radix-4 grids
  real kmax=sqrt(2)*n0*2^(ngrids-1);
  int glast=ngrids-1;
  for(int G=0; G < ngrids; ++G) {
    int w=2^G;
    int start=G==0? 0 : n0;
    real stop=G==glast ? kmax : w*n0;
    for(int j=start; j < stop; j += w)
      draw(pic,scale(j+w/2)*unitcircle);
  }
  //  draw(pic,scale(kmax)*unitcircle);
  */
  //for(int j=0; j < n; ++j) draw(pic,scale(j+0.5)*unitcircle);
  //for(int j=n; j < sqrt(8)*n; j+=1) draw(pic,scale(j+0.5)*unitcircle);
    
  shipout(outnames[i],pic);

  
}

