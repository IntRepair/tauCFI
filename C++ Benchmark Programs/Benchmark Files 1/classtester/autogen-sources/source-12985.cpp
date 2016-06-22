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
struct c1 : c0
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
struct c2;
void __attribute__ ((noinline)) tester2(c2* p);
struct c2 : c0
{
bool active2;
c2() : active2(true) {}
virtual ~c2()
{
tester2(this);
c0 *p0_0 = (c0*)(c2*)(this);
tester0(p0_0);
active2 = false;
}
virtual void f2(){}
};
void __attribute__ ((noinline)) tester2(c2* p)
{
p->f2();
if (p->active0)
p->f0();
}
struct c3;
void __attribute__ ((noinline)) tester3(c3* p);
struct c3 : c0
{
bool active3;
c3() : active3(true) {}
virtual ~c3()
{
tester3(this);
c0 *p0_0 = (c0*)(c3*)(this);
tester0(p0_0);
active3 = false;
}
virtual void f3(){}
};
void __attribute__ ((noinline)) tester3(c3* p)
{
p->f3();
if (p->active0)
p->f0();
}
struct c4;
void __attribute__ ((noinline)) tester4(c4* p);
struct c4 : c2, virtual c1, virtual c3
{
bool active4;
c4() : active4(true) {}
virtual ~c4()
{
tester4(this);
c0 *p0_0 = (c0*)(c2*)(c4*)(this);
tester0(p0_0);
c0 *p0_1 = (c0*)(c1*)(c4*)(this);
tester0(p0_1);
c0 *p0_2 = (c0*)(c3*)(c4*)(this);
tester0(p0_2);
c1 *p1_0 = (c1*)(c4*)(this);
tester1(p1_0);
c2 *p2_0 = (c2*)(c4*)(this);
tester2(p2_0);
c3 *p3_0 = (c3*)(c4*)(this);
tester3(p3_0);
active4 = false;
}
virtual void f4(){}
};
void __attribute__ ((noinline)) tester4(c4* p)
{
p->f4();
if (p->active2)
p->f2();
if (p->active1)
p->f1();
if (p->active3)
p->f3();
}
int __attribute__ ((noinline)) inc(int v) {return ++v;}
int main()
{
c0* ptrs0[25];
ptrs0[0] = (c0*)(new c0());
ptrs0[1] = (c0*)(c1*)(new c1());
ptrs0[2] = (c0*)(c2*)(new c2());
ptrs0[3] = (c0*)(c3*)(new c3());
ptrs0[4] = (c0*)(c2*)(c4*)(new c4());
ptrs0[5] = (c0*)(c1*)(c4*)(new c4());
ptrs0[6] = (c0*)(c3*)(c4*)(new c4());
for (int i=0;i<7;i=inc(i))
{
tester0(ptrs0[i]);
delete ptrs0[i];
}
c1* ptrs1[25];
ptrs1[0] = (c1*)(new c1());
ptrs1[1] = (c1*)(c4*)(new c4());
for (int i=0;i<2;i=inc(i))
{
tester1(ptrs1[i]);
delete ptrs1[i];
}
c2* ptrs2[25];
ptrs2[0] = (c2*)(new c2());
ptrs2[1] = (c2*)(c4*)(new c4());
for (int i=0;i<2;i=inc(i))
{
tester2(ptrs2[i]);
delete ptrs2[i];
}
c3* ptrs3[25];
ptrs3[0] = (c3*)(new c3());
ptrs3[1] = (c3*)(c4*)(new c4());
for (int i=0;i<2;i=inc(i))
{
tester3(ptrs3[i]);
delete ptrs3[i];
}
c4* ptrs4[25];
ptrs4[0] = (c4*)(new c4());
for (int i=0;i<1;i=inc(i))
{
tester4(ptrs4[i]);
delete ptrs4[i];
}
return 0;
}
