#ifndef PAL_BASICFUNCTIONS_DEBUG_CPP
#define PAL_BASICFUNCTIONS_DEBUG_CPP 1

#include <iostream>
#include <fstream>
#include <string>

enum DebugOut_Channel
{
	DebugOut_OFF=0,
	DebugOut_COUT,
	DebugOut_CERR,
	DebugOut_LOG,
	DebugOut_CERR_LOG,
	DebugOut_PAL_DEBUG
};

class Debug_Out
{
		#define DefaultDebugOut_Channel DebugOut_CERR
		#define DefaultDebugOutLog "DebugOut_Log.txt"
	private:
		bool DebugOn=1;
		DebugOut_Channel CurrentOutMode=DefaultDebugOut_Channel;
		std::ofstream fout;
		
	public:
		void SwitchToContrary()
		{DebugOn=!DebugOn;}
		
		void Switch(bool on)
		{DebugOn=on;}
		
		bool SetLOGFile(const std::string &path,const std::ios_base::openmode &_mode)
		{
			fout.open(path,_mode);
			return fout.is_open();
		}
		
		bool SetLOGFile(const std::string &path)
		{
			fout.open(path);
			return fout.is_open();
		}
		
		Debug_Out& operator % (DebugOut_Channel X)
		{
			if (X==DebugOut_LOG||X==DebugOut_CERR_LOG)
			{
				if (!fout.is_open())
					fout.open(DefaultDebugOutLog);
				if (!fout.is_open())
					return *this;
			}
			CurrentOutMode=X;
			return *this;
		}
		
		Debug_Out& operator << (Debug_Out& (*func)(Debug_Out&))
		{
			return (*func)(*this);
		}
		
		template <typename T> Debug_Out& operator << (const T &X)//const??
		{
			if (DebugOn)
				switch (CurrentOutMode)
				{
					case DebugOut_OFF: 			break;
					case DebugOut_COUT: 		std::cout<<X; break;
					case DebugOut_CERR: 		std::cerr<<X; break;
					case DebugOut_LOG: 			fout<<X; break;
					case DebugOut_CERR_LOG: 	std::cerr<<X; fout<<X; break;
				}
			return *this;
		}
		
	#ifdef PAL_BASICFUNCTIONS_POSIZE_CPP	
		Debug_Out& operator << (const Point &pt)
		{return *this<<pt.x<<" "<<pt.y;}
	
		Debug_Out& operator << (const Posize &ps)
		{return *this<<ps.x<<" "<<ps.y<<" "<<ps.w<<" "<<ps.h;}
	#endif
	
		Debug_Out(DebugOut_Channel X,const std::string &logFile="")
		{
			if (logFile!="")
				SetLOGFile(logFile);
			operator %(X);
		}
		
		Debug_Out(DebugOut_Channel X,const std::string &logFile,const std::ios_base::openmode &_mode)
		{
			SetLOGFile(logFile,_mode);
			operator %(X);
		}
		
		Debug_Out() {}
		
		#undef DefaultDebugOut_Channel
		#undef DefaultDebugOutLog
}DD;

Debug_Out& endl(Debug_Out &dd)
{
	dd<<"\n";
	return dd;
}

class Test_ConDeStructorClass
{
	public:
		static int IDcnt;
		int ID;
		std::string msg;

		Test_ConDeStructorClass& operator = (const Test_ConDeStructorClass &t)
		{
			DD<<"Test_ConDeStructorClass Assign "<<t.msg<<" to "<<msg<<endl;
			msg=t.msg;
			return *this;
		}

		Test_ConDeStructorClass(const Test_ConDeStructorClass &t)
		{
			ID=++IDcnt;
			msg=t.msg;
			DD<<"Test_ConDeStructorClass CopyCon "<<ID<<" "<<msg<<endl;
		}
		
		Test_ConDeStructorClass(const std::string &_msg=""):msg(_msg)
		{
			ID=++IDcnt;
			DD<<"Test_ConDeStructorClass Con "<<ID<<" "<<msg<<endl;
		}
		
		~Test_ConDeStructorClass()
		{
			DD<<"Test_ConDeStructorClass De "<<ID<<" "<<msg<<endl;
		}
};
int Test_ConDeStructorClass::IDcnt=0;

class Test_ConDeStructorClass_WithMove
{
	public:
		static int IDcnt;
		int ID;
		std::string msg;

		Test_ConDeStructorClass_WithMove& operator = (const Test_ConDeStructorClass_WithMove &t)
		{
			DD<<"Test_ConDeStructorClass_WithMove Assign "<<t.msg<<" to "<<msg<<endl;
			msg=t.msg;
			return *this;
		}
		
		Test_ConDeStructorClass_WithMove& operator = (const Test_ConDeStructorClass_WithMove &&t)
		{
			msg=std::move(t.msg);
			DD<<"Test_ConDeStructorClass_WithMove MoveAssign "<<ID<<" "<<msg<<endl;
			return *this;
		}
		
		Test_ConDeStructorClass_WithMove(const Test_ConDeStructorClass_WithMove &&t)
		{
			ID=++IDcnt;
			msg=move(t.msg);
			DD<<"Test_ConDeStructorClass_WithMove MoveCon "<<ID<<" "<<msg<<endl;
		}

		Test_ConDeStructorClass_WithMove(const Test_ConDeStructorClass_WithMove &t)
		{
			ID=++IDcnt;
			msg=t.msg;
			DD<<"Test_ConDeStructorClass_WithMove CopyCon "<<ID<<" "<<msg<<endl;
		}
		
		Test_ConDeStructorClass_WithMove(const std::string &_msg=""):msg(_msg)
		{
			ID=++IDcnt;
			DD<<"Test_ConDeStructorClass_WithMove Con "<<ID<<" "<<msg<<endl;
		}
		
		~Test_ConDeStructorClass_WithMove()
		{
			DD<<"Test_ConDeStructorClass_WithMove De "<<ID<<" "<<msg<<endl;
		}
};
int Test_ConDeStructorClass_WithMove::IDcnt=0;

void Debug_PrintStr(const std::string &str)
{
	DD<<"Debug_PrintStr: "<<str<<"\n";
	for (int i=0;i<str.length();++i)
		DD<<str[i]<<"|";
	DD<<"\n";
	for (int i=0;i<str.length();++i)
		DD<<(int)str[i]<<"|";
	DD<<"\n";
}

#endif
