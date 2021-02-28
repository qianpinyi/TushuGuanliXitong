#ifndef PAL_BASICFUNCTIONS_0_CPP
#define PAL_BASICFUNCTIONS_0_CPP 1

#define null NULL

#include <string>
#include <ctime>

void * const CONST_THIS=new int(0);
void * const CONST_TRUE=new int(0);
void * const CONST_FALSE=new int(0);
void * const CONST_Ptr_0=new int(0);
void * const CONST_Ptr_1=new int(1);
void * const CONST_Ptr_2=new int(2);
void * const CONST_Ptr_3=new int(3);

template <typename T1,typename T2,typename T3> T1 EnsureInRange(const T1 &x,const T2 &L,const T3 &R) 
{
	if (x<L) return L;
	else if (x>R) return R;
	else return x;
}

template <typename T1,typename T2,typename T3>  bool InRange(const T1 &x,const T2 &L,const T3 &R)
{return L<=x&&x<=R;}

char *GetTime1()
{
	time_t rawtime;
	time(&rawtime);
	return ctime(&rawtime);
}

#endif
