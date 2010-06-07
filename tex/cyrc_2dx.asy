size(14cm,0);


pair center(path g) {return 0.5*(min(g)+max(g));}


void drawboxes(pair z=(0,0), pair p=(1,1))
{
  path g=box((0,0),p);
  draw(shift(z)*g);
}
path g, p;
pair b=(1,1), a=(0,0), z=(0,0);
real wx=0.01;
real dd=1.2*wx;
real dh=1.1*wx;

g=box(a,(wx,wx));
//draw(shift(z)*g);

z=(0,dd);
p=shift(z)*g;
filldraw(p,yellow);
label("$f$",center(p));
pair f1=(center(p).x,min(p).y);

z=(2.5dh,dd);
p=shift(z)*g;
filldraw(p,yellow);
label("$g$",center(p));
pair g1=(center(p).x,min(p).y);

z=(0,0);
p=shift(z)*g;
filldraw(p,yellow);
label("${\rm FFT_x}\{f\}$",center(p),N);
label("$k_x$ \rm{even}",center(p),S);
pair ff1=(center(p).x,max(p).y);
draw(f1..ff1,EndArrow);

z=(dh,0);
p=shift(z)*g;
filldraw(p,yellow);
label("${\rm FFT_x}\{f\}$",center(p),N);
label("$k_x$ \rm{odd}",center(p),S);
pair ff2=(center(p).x,max(p).y);
draw(f1..ff2,EndArrow);

z=(2.5dh,0);
p=shift(z)*g;
filldraw(p,yellow);
label("${\rm FFT_x}\{g\}$",center(p),N);
label("$k_x$ \rm{even}",center(p),S);
pair ff1=(center(p).x,max(p).y);
draw(g1..ff1,EndArrow);

z=(3.5dh,0);
p=shift(z)*g;
filldraw(p,yellow);
label("${\rm FFT_x}\{g\}$",center(p),N);
label("$k_x$ \rm{odd}",center(p),S);
pair ff2=(center(p).x,max(p).y);
draw(g1..ff2,EndArrow);
