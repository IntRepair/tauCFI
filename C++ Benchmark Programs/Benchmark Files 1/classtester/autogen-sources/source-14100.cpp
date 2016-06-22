struct c0;
void __attribute__ ((noinline)) tester0(c0* p);
struct c0
{
bool active0;
c0() : active0(true) {}
virtual ~c0()
{
tester0(this);
active0 = false;
}
virtual void f0(){}
};
void __attribute__ ((noinline)) tester0(c0* p)
{
p->f0();
}
struct c1;
void __attribute__ ((noinline)) tester1(c1* p);
struct c1 : virtual c0
{
bool active1;
c1() : active1(true) {}
virtual ~c1()
{
tester1(this);
c0 *p0_0 = (c0*)(c1*)(this);
tester0(p0_0);
active1 = false;
}
virtual void f1(){}
};
void __attribute__ ((noinline)) tester1(c1* p)
{
p->f1();
if (p->active0)
p->f0();
}
int __attribute__ ((noinline)) inc(int v) {return ++v;}
int main()
{
c0* ptrs0[25];
ptrs0[0] = (c0*)(new c0());
ptrs0[1] = (c0*)(c1*)(new c1());
for (int i=0;i<2;i=inc(i))
{
tester0(ptrs0[i]);
delete ptrs0[i];
}
c1* ptrs1[25];
ptrs1[0] = (c1*)(new c1());
for (int i=0;i<1;i=inc(i))
{
tester1(ptrs1[i]);
delete ptrs1[i];
}
return 0;
}
