#ifndef PAL_BASICFUNCTIONS_STRINGEX_CPP
#define PAL_BASICFUNCTIONS_STRINGEX_CPP 1

#include <string>
#include <stack>
#include <vector>

int strTOint(const std::string &str)
{
	int x=0;
	for (int i=0;i<str.length();++i)
		x*=10,x+=str[i]-'0';
	return x;
}

long long strTOll(const std::string &str)
{
	long long x=0;
	for (int i=0;i<str.length();++i)
		x*=10,x+=str[i]-'0';
	return x;
}

double strTOdb(const std::string &str)
{
	double x=0;
	for (int i=0,flag=0;i<str.length();++i)
		if (str[i]=='.') flag=1;
		else if (flag==0) x*=10,x+=str[i]-'0';
		else x+=(double(str[i]-'0'))/(flag*=10);
	return x;
}

std::string llTOstr(long long x)
{
	if (x==0) return "0";
	std::string re;
	if (x<0) re+="-",x=-x;
	std::stack <char> sta;
	while (x)
		sta.push(x%10+'0'),x/=10;
	while (!sta.empty())
		re+=sta.top(),sta.pop();
	return re;
}

std::string ReplaceCharInStr(std::string str,const int &L,const int &R,const char &ch1,const char &ch2)
{
	for (int i=L;i<=R;++i)
		if (str[i]==ch1)
			str[i]=ch2;
	return str;
}

std::string ReplaceCharInStr(const std::string &str,const char &ch1,const char &ch2)
{return ReplaceCharInStr(str,0,str.length()-1,ch1,ch2);}

std::string DeletePreBlank(const std::string &str)
{
	for (int i=0;i<str.length();++i)
		switch (str[i])
		{
			case '\n':break;//??
			case '\r':break;//??
			case '\t':break;
			case ' ' :break;
			case '\0':break;
			default:
				return str.substr(i,str.length()-i);
		}
	return "";
}

std::string DeleteEndBlank(const std::string &str)
{
	for (int i=str.length()-1;i>=0;--i)
		switch (str[i])
		{
			case '\n':break;//??
			case '\r':break;//??
			case '\t':break;
			case ' ' :break;
			case '\0':break;
			default:
				return str.substr(0,i+1);
		}
	return "";
}

std::wstring DeleteEndBlank(const std::wstring &wstr)
{
	for (int i=wstr.length()-1;i>=0;--i)
		switch (wstr[i])
		{
			case L'\n':break;//??
			case L'\r':break;//??
			case L'\t':break;
			case L' ' :break;
			case L'\0':break;
			default:
				return wstr.substr(0,i+1);
		}
	return L"";
}

std::string CutFirstCharInvolveSubStr(const std::string &str,const char &ch)
{
	int pos1=0,pos2=0;
	pos1=str.find(ch);
	if (pos1<str.length()-1)
		pos2=str.find(pos1+1,ch);
	if (pos1<pos2) return str.substr(pos1+1,pos2-pos1-1);
	else return "";
}

std::string GetAftername(const std::string &str)
{
	int p=str.rfind(".",str.length()-1);
	if (p==-1) return "";
	return str.substr(p,1e9);
}

std::string GetWithOutAftername(const std::string &str)
{
	int p=str.rfind(".",str.length()-1);
	if (p==-1) return str;
	return str.substr(0,p);
}

std::string GetLastAfterBackSlash(const std::string &str)
{
	return str.substr(str.rfind("\\",str.length()-1)+1,1e9);
}

std::string GetPreviousBeforeBackSlash(const std::string &str)
{
	int p=str.rfind("\\",str.length()-1);
	if (p==-1) return "";
	return str.substr(0,p);
}

std::string Atoa(std::string str)
{
	for (int i=0;i<str.length();++i)
		if ('A'<=str[i]&&str[i]<='Z')
			str[i]+='a'-'A';
	return str;
}

char Atoa(char ch)
{
	if ('A'<=ch&&ch<='Z')
		return ch-'A'+'a';
	else return ch;
}

std::string atoA(std::string str)
{
	for (int i=0;i<str.length();++i)	
		if ('a'<=str[i]&&str[i]<='z')
			str[i]+='A'-'a';
	return str;
}

void ReplaceXMLescapecharWithReal(std::string &str)
{
	while (1)
	{
		int c=str.find("&amp;");
		if (c==-1) break;
		str.replace(c,5,"&");
	}
	while (1)
	{
		int c=str.find("&lt;");
		if (c==-1) break;
		str.replace(c,4,"<");
	}
	while (1)
	{
		int c=str.find("&gt;");
		if (c==-1) break;
		str.replace(c,4,">");
	}
	while (1)
	{
		int c=str.find("&apos;");
		if (c==-1) break;
		str.replace(c,6,"'");
	}
	while (1)
	{
		int c=str.find("&quot;");
		if (c==-1) break;
		str.replace(c,6,"\"");
	}
}

void GetRidOfEndChar0(std::string &str)
{
	while (!str.empty()&&(*str.rbegin())==0)
		str.erase(str.end()-1);
}

void GetRidOfEndChar0(std::wstring &wstr)
{
	while (!wstr.empty()&&(*wstr.rbegin())==0)
		wstr.erase(wstr.end()-1);
}

std::string GetRidOfEndChar0_Re(std::string str)
{
	while (!str.empty()&&(*str.rbegin())==0)
		str.erase(str.end()-1);
	return str;
}

template <class T> class stringUTF8_WithData;

class stringUTF8
{
	template <class T> friend class stringUTF8_WithData;
	protected:
		std::vector <std::string> strUTF8;
		
	public:
		inline bool empty() const
		{return strUTF8.size()==0;}
		
		void clear()
		{strUTF8.clear();}
		
		inline int length() const
		{return strUTF8.size();}
		
		void append(const stringUTF8 &str,int srcPos,int srcLen)
		{
			for (int i=srcPos;i<srcPos+srcLen;++i)
				strUTF8.push_back(str[i]);
		}
		
		void append(const stringUTF8 &str)
		{
			for (auto vp:str.strUTF8)
				strUTF8.push_back(vp);
		}
		
		void append(const std::string &str)
		{
			int p=0,q=0,stat=0;
			while (p<str.length())
			{
				if ((str[p]&0x80)==0)//0XXXXXXX
				{
					stat=0;
					strUTF8.push_back(str.substr(p,1));
					q=p;
				}
				else if ((str[p]&0xC0)==0x80)//10XXXXXX
				{
					if (p-q+1==stat)
					{
						strUTF8.push_back(str.substr(q,p-q+1));
						stat=0;
					}
				}
				else 
				{
					if ((str[p]&0xE0)==0xC0)	stat=2;//110XXXXX
					else if ((str[p]&0xF0)==0xE0)	stat=3;//1110XXXX
					else if ((str[p]&0xF8)==0xF0)	stat=4;//11110XXX
					else if ((str[p]&0xFC)==0xF8)	stat=5;//111110XX
					else if ((str[p]&0xFE)==0xFC)	stat=6;//1111110X
					else ;//Error
					q=p;
				}
				++p;
			}
		}
		
		void append(const char *str)
		{append(std::string(str));}
		
		std::string operator () (int l,int r,char mode[3]) const
		{
			if (mode[0]=='(') ++l;
			if (mode[1]==')') --r;
			std::string re;
			for (int i=l;i<=r;++i)
				re.append(strUTF8[i]);
			return re;	
		}
		
		std::string operator () (int l,int r) const
		{
			std::string re;
			for (int i=l;i<=r;++i)
				re.append(strUTF8[i]);
			return re;
		}
		
		stringUTF8 substrUTF8(int pos,int len) const
		{
			stringUTF8 re;
			for (int i=pos;i<pos+len;++i)
				re.strUTF8.push_back(strUTF8[i]);
			return re;
		}
		
		std::string substr(int pos,int len)
		{
			std::string re;
			for (int i=pos;i<pos+len;++i)
				re.append(strUTF8[i]);
			return re;
		}
		
		std::string cppString() const
		{
			std::string re;
			for (auto vp:strUTF8)
				re.append(vp);
			return re;
		}
		
		int find(const stringUTF8 &str)
		{
			for (int i=0;i<strUTF8.size();++i)
			{
				bool flag=1;
				for (int j=0;j<str.strUTF8.size();++j)
					if (strUTF8[i]!=str.strUTF8[j])
					{
						flag=0;
						break;
					}
				if (flag)
					return i;
			}
		}
		
		int find(const std::string &str)
		{return find(stringUTF8(str));}
		
		int find(const char *str)
		{return find(stringUTF8(std::string(str)));}
		
		void erase(int pos,int len)
		{strUTF8.erase(strUTF8.begin()+pos,strUTF8.begin()+pos+len);}
		
		stringUTF8& insert(int pos,const stringUTF8 &str,int srcPos,int srcLen)
		{
			if (pos>length()||pos<0)
			{
//				std::cerr<<"stringUTF8 Error: insert out of range!"<<std::endl;
				return *this;
			}
			if (pos==length())
				append(str,srcPos,srcLen);
			else strUTF8.insert(strUTF8.begin()+pos,str.strUTF8.begin()+srcPos,str.strUTF8.begin()+srcPos+srcLen);
			return *this;
		}
		
		stringUTF8& insert(int pos,const stringUTF8 &str)
		{
			if (pos>length()||pos<0)
			{
//				std::cerr<<"stringUTF8 Error: insert out of range!"<<std::endl;
				return *this;
			}
			if (pos==length())
				append(str);
			else strUTF8.insert(strUTF8.begin()+pos,str.strUTF8.begin(),str.strUTF8.end());
			return *this;
		}
		
		stringUTF8& insert(int pos,const std::string &str)
		{return insert(pos,stringUTF8(str));}
		
		stringUTF8& insert(int pos,const char *str)
		{return insert(pos,stringUTF8(str));}
		
		stringUTF8& operator += (const stringUTF8 &str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8& operator += (const std::string &str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8& operator += (const char *str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8 operator + (const stringUTF8 &str) const
		{
			stringUTF8 re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8 operator + (const std::string &str) const
		{
			stringUTF8 re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8 operator + (const char *str) const
		{
			stringUTF8 re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8& operator = (const stringUTF8 &str)
		{
			if (&str!=this)
				strUTF8=str.strUTF8;
			return *this;
		}
		
		stringUTF8& operator = (const std::string &str)
		{
			strUTF8.clear();
			append(str);
			return *this;
		}
		
		stringUTF8& operator = (const char *str)
		{
			strUTF8.clear();
			append(str);
			return *this;
		}
		
		std::string& operator [] (int x)
		{return strUTF8[x];}
		
		const std::string& operator [] (int x) const
		{return strUTF8[x];}
		
		stringUTF8(const stringUTF8 &str):strUTF8(str.strUTF8) {}
		
		stringUTF8(const std::string &str)
		{append(str);}
		
		stringUTF8(const char *str)
		{append(str);}
		
		stringUTF8() {}
};

template <class T> class stringUTF8_WithData
{
	protected:
		struct EachCharData
		{
			std::string ch;
			T Data;
			
			EachCharData(const std::string str,const T &da)
			:ch(str),Data(da) {}
			
			EachCharData(const std::string str)
			:ch(str) {}
		};
		
		std::vector <EachCharData> strUTF8;
		
	public:
		inline bool empty() const
		{return strUTF8.size()==0;}
		
		void clear()
		{strUTF8.clear();}
		
		inline int length() const
		{return strUTF8.size();}
		
		void append(const stringUTF8_WithData <T> &str,int srcPos,int srcLen)
		{
			for (int i=srcPos;i<srcPos+srcLen;++i)
				strUTF8.push_back(str[i]);
		}
		
		void append(const stringUTF8 &str,const T &defaultData,int srcPos,int srcLen)
		{
			for (int i=srcPos;i<srcPos+srcLen;++i)
				strUTF8.push_back(EachCharData(str[i],defaultData));
		}
		
		void append(const stringUTF8 &str,int srcPos,int srcLen)
		{
			for (int i=srcPos;i<srcPos+srcLen;++i)
				strUTF8.push_back(EachCharData(str[i]));
		}
		
		void append(const stringUTF8_WithData <T> &str)
		{
			for (auto vp:str.strUTF8)
				strUTF8.push_back(vp);
		}
		
		void append(const stringUTF8 &str,const T &defaultData)
		{
			for (auto vp:str.strUTF8)
				strUTF8.push_back(EachCharData(str,defaultData));
		}
		
		void append(const stringUTF8 &str)
		{
			for (auto vp:str.strUTF8)
				strUTF8.push_back(EachCharData(vp));
		}
		
		void append(const std::string &str,const T &defaultData)
		{
			int p=0,q=0,stat=0;
			while (p<str.length())
			{
				if ((str[p]&0x80)==0)//0XXXXXXX
				{
					stat=0;
					strUTF8.push_back(EachLineData(str.substr(p,1),defaultData));
					q=p;
				}
				else if ((str[p]&0xC0)==0x80)//10XXXXXX
				{
					if (p-q+1==stat)
					{
						strUTF8.push_back(EachCharData(str.substr(q,p-q+1),defaultData));
						stat=0;
					}
				}
				else 
				{
					if ((str[p]&0xE0)==0xC0)	stat=2;//110XXXXX
					else if ((str[p]&0xF0)==0xE0)	stat=3;//1110XXXX
					else if ((str[p]&0xF8)==0xF0)	stat=4;//11110XXX
					else if ((str[p]&0xFC)==0xF8)	stat=5;//111110XX
					else if ((str[p]&0xFE)==0xFC)	stat=6;//1111110X
					else ;//Error
					q=p;
				}
				++p;
			}
		}
		
		void append(const std::string &str)
		{
			int p=0,q=0,stat=0;
			while (p<str.length())
			{
				if ((str[p]&0x80)==0)//0XXXXXXX
				{
					stat=0;
					strUTF8.push_back(EachCharData(str.substr(p,1)));
					q=p;
				}
				else if ((str[p]&0xC0)==0x80)//10XXXXXX
				{
					if (p-q+1==stat)
					{
						strUTF8.push_back(EachCharData(str.substr(q,p-q+1)));
						stat=0;
					}
				}
				else 
				{
					if ((str[p]&0xE0)==0xC0)	stat=2;//110XXXXX
					else if ((str[p]&0xF0)==0xE0)	stat=3;//1110XXXX
					else if ((str[p]&0xF8)==0xF0)	stat=4;//11110XXX
					else if ((str[p]&0xFC)==0xF8)	stat=5;//111110XX
					else if ((str[p]&0xFE)==0xFC)	stat=6;//1111110X
					else ;//Error
					q=p;
				}
				++p;
			}
		}
		
		void append(const char *str,const T &defaultData)
		{append(std::string(str),defaultData);}
		
		void append(const char *str)
		{append(std::string(str));}
		
		std::string operator () (int l,int r,char mode[3]) const
		{
			if (mode[0]=='(') ++l;
			if (mode[1]==')') --r;
			std::string re;
			for (int i=l;i<=r;++i)
				re.append(strUTF8[i].ch);
			return re;
		}
		
		std::string operator () (int l,int r) const
		{
			std::string re;
			for (int i=l;i<=r;++i)
				re.append(strUTF8[i].ch);
			return re;
		}
		
		stringUTF8_WithData <T> substrUTF8_WithData(int pos,int len) const
		{
			stringUTF8_WithData <T> re;
			for (int i=pos;i<pos+len;++i)
				re.strUTF8.push_back(strUTF8[i]);
			return re;
		}
		
		stringUTF8 substrUTF8(int pos,int len) const
		{
			stringUTF8 re;
			for (int i=pos;i<pos+len;++i)
				re.strUTF8.push_back(strUTF8[i].ch);
			return re;
		}
		
		std::string substr(int pos,int len)
		{
			std::string re;
			for (int i=pos;i<pos+len;++i)
				re.append(strUTF8[i].ch);
			return re;
		}
		
		stringUTF8 StringUTF8() const
		{
			stringUTF8 re;
			for (auto vp:strUTF8)
				re.strUTF8.push_back(vp.ch);
			return re;
		}
		
		std::string cppString() const
		{
			std::string re;
			for (auto vp:strUTF8)
				re.append(vp.ch);
			return re;
		}
		
		int find(const stringUTF8 &str)
		{
			for (int i=0;i<strUTF8.size();++i)
			{
				bool flag=1;
				for (int j=0;j<str.strUTF8.size();++j)
					if (strUTF8[i]!=str.strUTF8[j])
					{
						flag=0;
						break;
					}
				if (flag)
					return i;
			}
		}
		
		int find(const std::string &str)
		{return find(stringUTF8(str));}
		
		int find(const char *str)
		{return find(stringUTF8(std::string(str)));}
		
		void erase(int pos,int len)
		{strUTF8.erase(strUTF8.begin()+pos,strUTF8.begin()+pos+len);}
		
		stringUTF8_WithData <T>& insert(int pos,const stringUTF8_WithData <T> &str,int srcPos,int srcLen)
		{
			if (pos>length()||pos<0)
				return *this;
			if (pos==length())
				append(str,srcPos,srcLen);
			else strUTF8.insert(strUTF8.begin()+pos,str.strUTF8.begin()+srcPos,str.strUTF8.begin()+srcPos+srcLen);
			return *this;
		}
		
		stringUTF8_WithData <T>& insert(int pos,const stringUTF8_WithData <T> &str)
		{
			if (pos>length()||pos<0)
				return *this;
			if (pos==length())
				append(str);
			else strUTF8.insert(strUTF8.begin()+pos,str.strUTF8.begin(),str.strUTF8.end());
			return *this;
		}
		
		stringUTF8_WithData <T>& insert(int pos,const stringUTF8 &str,const T &defaultData)
		{return insert(stringUTF8_WithData <T> (str,defaultData));}
		
		stringUTF8_WithData <T>& insert(int pos,const stringUTF8 &str)
		{return insert(stringUTF8_WithData <T> (str));}
		
		stringUTF8_WithData <T>& insert(int pos,const std::string &str,const T &defaultData)
		{return insert(pos,stringUTF8_WithData <T> (str,defaultData));}
		
		stringUTF8_WithData <T>& insert(int pos,const std::string &str)
		{return insert(pos,stringUTF8_WithData <T> (str));}
		
		stringUTF8_WithData <T>& insert(int pos,const char *str,const T &defaultData)
		{return insert(pos,stringUTF8_WithData <T> (str,defaultData));}
		
		stringUTF8_WithData <T>& insert(int pos,const char *str)
		{return insert(pos,stringUTF8_WithData <T> (str));}
		
		stringUTF8_WithData <T>& operator += (const stringUTF8_WithData <T> &str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T>& operator += (const stringUTF8 &str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T>& operator += (const std::string &str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T>& operator += (const char *str)
		{
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T> operator + (const stringUTF8_WithData <T> &str) const
		{
			stringUTF8_WithData <T> re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8_WithData <T> operator + (const stringUTF8 &str) const
		{
			stringUTF8_WithData <T> re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8_WithData <T> operator + (const std::string &str) const
		{
			stringUTF8_WithData <T> re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8_WithData <T> operator + (const char *str) const
		{
			stringUTF8_WithData <T> re(*this);
			re.append(str);
			return re;
		}
		
		stringUTF8_WithData <T>& operator = (const stringUTF8_WithData <T> &str)
		{
			if (&str!=this)
				strUTF8=str.strUTF8;
			return *this;
		}
		
		stringUTF8_WithData <T>& operator = (const stringUTF8 &str)
		{
			strUTF8.clear();
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T>& operator = (const std::string &str)
		{
			strUTF8.clear();
			append(str);
			return *this;
		}
		
		stringUTF8_WithData <T>& operator = (const char *str)
		{
			strUTF8.clear();
			append(str);
			return *this;
		}
		
		std::string& operator [] (int x)
		{return strUTF8[x].ch;}
		
		const std::string& operator [] (int x) const
		{return strUTF8[x].ch;}
		
		T& operator () (int x)
		{return strUTF8[x].Data;}
		
		const T& operator () (int x) const
		{return strUTF8[x].Data;}
		
		stringUTF8_WithData(const stringUTF8_WithData <T> &str)
		:strUTF8(str.strUTF8) {}
		
		stringUTF8_WithData(const stringUTF8 &str,const T &defaultData)
		{append(str,defaultData);}
		
		stringUTF8_WithData(const stringUTF8 &str)
		{append(str);}
		
		stringUTF8_WithData(const std::string &str,const T &defaultData)
		{append(str,defaultData);}
		
		stringUTF8_WithData(const std::string &str)
		{append(str);}
		
		stringUTF8_WithData(const char *str,const T &defaultData)
		{append(str,defaultData);}
		
		stringUTF8_WithData(const char *str)
		{append(str);}
		
		stringUTF8_WithData() {}
};
#endif
