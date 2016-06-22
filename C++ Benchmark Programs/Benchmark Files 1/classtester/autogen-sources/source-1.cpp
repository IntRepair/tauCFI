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
int __attribute__ ((noinline)) inc(int v) {return ++v;}
int main()
{
c0* ptrs0[25];
ptrs0[0] = (c0*)(new c0());
for (int i=0;i<1;i=inc(i))
{
tester0(ptrs0[i]);
delete ptrs0[i];
}
return 0;
}
