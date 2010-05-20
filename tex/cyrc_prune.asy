size(10cm,0);


//pair center(path g) {return 0.5*(min(g)+max(g));}


void drawboxes(pair z=(0,0), pair p=(1,1))
{
  path g=box((0,0),p);
  draw(shift(z)*g);
}
path g;
pair b=(1,1), a=(0,0), z=(0,0);

g=box(a,b);
draw(shift(z)*g);
g=box(a,0.5*b);
filldraw(shift(z)*g,gray);

z=(2,0);
g=box(a,b);
draw(shift(z)*g);
g=box(a,(1,0.5));
filldraw(shift(z)*g,gray);

g=box(a,b);
draw(shift(z)*g);
int n=10;
for(int i=0; i < n-1; ++i) {
  z=(0,0.5*i/n);
  g=box(a,(1,0.1));
  draw(shift(z)*g,red);
}

z=(4,0);
g=box(a,b);
filldraw(shift(z)*g,gray);
int n=10;
for(int i=0; i < n-1; ++i) {
  z=(2+i/n,0);
  g=box(a,(0.1,1));
  draw(shift(z)*g,blue);
}


real h=0.5;
//Arrows between boxes
draw((1,h)..(2,h),EndArrow);
label("xFFT",(1.5,h),N,red);
draw((3,h)..(4,h),EndArrow);
label("yFFT",(3.5,h),N,blue);


