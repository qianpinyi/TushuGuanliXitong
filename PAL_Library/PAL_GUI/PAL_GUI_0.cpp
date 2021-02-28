 #ifndef PAL_GUI_0_CPP
#define PAL_GUI_0_CPP 1
/*
	Refactoring of PAL_GUI_Alpha
	By:qianpinyi
	
*/
#include <string>
#include <set>
#include <map>
#include <cstring>
#include <atomic>
#include <queue>
#include <cmath>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define USE_SDL

#include "../PAL_BasicFunctions/PAL_BasicFunctions_0.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_Posize.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_StringEX.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_Color.cpp"
#include "../PAL_BasicFunctions/PAL_BasicFunctions_Debug.cpp"
#include "../PAL_DataStructure/PAL_SplayTree.cpp"

namespace PAL_GUI
{
	using namespace std;
//	PAL_Config_Alpha PUI_Cfg("PUI_Cfg.txt");
	
	//Global option:
	//...
	int WheelSensibility=40;
	bool SkipEventFlag=1;
	bool SkipFrameFlag=0; 
	
	//Debug option:
	//...
	bool DEBUG_DisplayBorderFlag=0,
		 DEBUG_EnableWidgetsShowInTurn=0,
		 DEBUG_DisplayPresentLimitFlag=0;
	
	int DEBUG_WidgetsShowInTurnDelayTime=100;
	
	//Surface/Texture/Drawing/... Functions:
	Posize GetTexturePosize(SDL_Texture *tex)
	{
		if (tex==NULL) return ZERO_POSIZE;
		Posize re;
		re.x=re.y=re.w=re.h=0;
		SDL_QueryTexture(tex,NULL,NULL,&re.w,&re.h);
		return re;
	}
	
	//Data
	//...
	struct SharedTexturePtr
	{
		protected:
			int *count=NULL;
			SDL_Texture *tex=NULL;
			
		public:
			inline bool operator ! ()
			{return tex==NULL;}
			
			inline SDL_Texture* operator () ()
			{return tex;}
			
			inline SDL_Texture* GetPic()
			{return tex;}
			
			SharedTexturePtr& operator = (const SharedTexturePtr &tar)
			{
				if (&tar==this)
					return *this;
				if (tex!=NULL)
				{
					--*count;
					if (*count==0)
					{
						SDL_DestroyTexture(tex);
						delete count;
					}
					tex=NULL;
					count=NULL;
				}
				tex=tar.tex;
				count=tar.count;
				if (tex!=NULL)
					++*count;
				return *this;
			}
			
			~SharedTexturePtr()
			{
				if (tex!=NULL)
				{
					--*count;
					if (*count==0)
					{
						SDL_DestroyTexture(tex);
						delete count;
					}
				}
			}
			
			SharedTexturePtr(const SharedTexturePtr &tar)
			{
				tex=tar.tex;
				count=tar.count;
				if (tex!=NULL)
					++*count;
			}
			
			explicit SharedTexturePtr(SDL_Texture *tarTex)
			{
				tex=tarTex;
				if (tex!=NULL)
					count=new int(1);
			}
			
			explicit SharedTexturePtr() {}
	};
	
	class Widgets;
	class Layer;
	class PUI_Window//A window(to support multi windows)
	{
		friend class Widgets;
//		protected:
		public:
			static set <PUI_Window*> AllWindow;
			static map <unsigned int,PUI_Window*> WinOfSDLWinID;
			static int WindowCnt;
			static bool NeedFreshScreenAll;
			
			string WindowTitle;
			SDL_Window *win=NULL;
			SDL_Renderer *ren=NULL;
			unsigned int SDLWinID;
			
			SDL_Event *NowSolvingEvent=NULL;
		
			bool NeedFreshScreen=1,
				 NeedUpdatePosize=1,
				 NeedSolveEvent=0,
				 NeedSolvePosEvent=0,
				 
				 PosFocused=1;
			int NowSolvingPosEventMode=0;//0:common 1:from LoseFocus 2:from OccupyPos 3:virtual PosEvent
			
			Posize WinPS,
				   PresentLimit;
		
			Point NowPos;
			
			Layer *BackGroundLayer=NULL,
				  *MenuLayer=NULL;
			
			struct LoseFocusLinkTable
			{
				LoseFocusLinkTable *nxt=NULL;
				Widgets *wg=NULL;
				
				LoseFocusLinkTable(Widgets *tar):wg(tar) {}
			};
			LoseFocusLinkTable *LoseFocusWgHead=NULL;//The original OccupyPos
			int LoseFocusState=0;
			
//			union ToRenderQueueData
//			{
//				int type=0;
//				struct
//				{
//					
//				};
//			};
//			queue <ToRenderQueueData> ToRenderQueue;
//			
//			void RenderQueue()
//			{
//				
//			}

		public:
			Widgets *OccupyPosWg=NULL;
			Widgets *KeyboardInputWg=NULL;
			
//			inline const Point& NowPos()
//			{return NowPos;}
//			
//			inline Widgets* BackgroundLayer()
//			{return BackGroundLayer;}
//			
//			inline Widgets* MenuLayer()
//			{return MenuLayer;}
			
			void SetBackgroundColor(const RGBA &co);
			
			void SetRenderColor(const RGBA &co)
			{SDL_SetRenderDrawColor(ren,co.r,co.g,co.b,co.a);}
	
			void RenderDrawLine(const Point &pt1,const Point &pt2)
			{SDL_RenderDrawLine(ren,pt1.x,pt1.y,pt2.x,pt2.y);}
			
			void RenderFillRect(const Posize &ps,const RGBA &co)
			{
				if (!co.HaveColor()||ps.Size()==0) return;
				SDL_SetRenderDrawColor(ren,co.r,co.g,co.b,co.a);
				SDL_Rect rct=PosizeToSDLRect(ps);
				SDL_RenderFillRect(ren,&rct);
			}
			
			void RenderDrawRectWithLimit(const Posize &ps,const RGBA &co,Posize lmt)
			{
				SDL_SetRenderDrawColor(ren,co.r,co.g,co.b,co.a);
				lmt=lmt&ps;
				
				if (lmt.Size()==0)
					return;
				
				if (lmt.x==ps.x)
					RenderDrawLine(lmt.GetLU(),lmt.GetLD());
				if (lmt.x2()==ps.x2())
					RenderDrawLine(lmt.GetRU(),lmt.GetRD());
				if (lmt.y==ps.y)
					RenderDrawLine(lmt.GetLU(),lmt.GetRU());
				if (lmt.y2()==ps.y2())
					RenderDrawLine(lmt.GetLD(),lmt.GetRD());
			}
			
			void RenderCopy(SDL_Texture *tex,const Point &targetPt)
			{
				if (tex==NULL) return;
				Posize tex_PS=GetTexturePosize(tex);
				SDL_Rect srct=PosizeToSDLRect(tex_PS),
					 	 drct=PosizeToSDLRect(tex_PS+targetPt);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopy(SDL_Texture *tex,const Posize &targetPS)
			{
				if (tex==NULL||targetPS.Size()==0) return;
				Posize tex_PS=GetTexturePosize(tex);
				SDL_Rect srct=PosizeToSDLRect(tex_PS),
					 	 drct=PosizeToSDLRect(targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopy(SDL_Texture *tex,Posize tex_PS,const Posize &targetPS)
			{
				if (targetPS.Size()==0||tex_PS.Size()==0||tex==NULL) return;
				if (tex_PS==ZERO_POSIZE) tex_PS=GetTexturePosize(tex);
				SDL_Rect srct=PosizeToSDLRect(tex_PS),
					 	 drct=PosizeToSDLRect(targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopy(SDL_Texture *tex,Posize tex_PS,const Point &pt)
			{
				if (tex_PS.Size()==0||tex==NULL) return;
				if (tex_PS==ZERO_POSIZE) tex_PS=GetTexturePosize(tex);
				SDL_Rect srct=PosizeToSDLRect(tex_PS),
					 	 drct={pt.x,pt.y,tex_PS.w,tex_PS.h};
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt(SDL_Texture *tex,Posize tex_PS,const Posize &targetPS,const Posize &lmt)
			{
				if (lmt.Size()==0||targetPS.Size()==0||tex_PS.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				if (tex_PS==ZERO_POSIZE) tex_PS=texPs;
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS).Flexible(texPs.w*1.0/targetPS.w,texPs.h*1.0/targetPS.h)&tex_PS),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt(SDL_Texture *tex,const Posize &targetPS,const Posize &lmt)
			{
				if (lmt.Size()==0||targetPS.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS).Flexible(texPs.w*1.0/targetPS.w,texPs.h*1.0/targetPS.h)&texPs),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt(SDL_Texture *tex,Posize tex_PS,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()==0||tex_PS.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				if (tex_PS==ZERO_POSIZE) tex_PS=texPs;
				Posize targetPS={pt.x,pt.y,tex_PS.w,tex_PS.h};
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS)&tex_PS),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt(SDL_Texture *tex,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				Posize targetPS={pt.x,pt.y,texPs.w,texPs.h};
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS)&texPs),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt_Centre(SDL_Texture *tex,Posize targetPS,const Posize &lmt)
			{
				if (lmt.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				targetPS.x+=targetPS.w-texPs.w>>1;
				targetPS.y+=targetPS.h-texPs.h>>1;
				targetPS.w=texPs.w;
				targetPS.h=texPs.h;
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS)&texPs),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderCopyWithLmt_Centre(SDL_Texture *tex,const Point &pt,const Posize &lmt)
			{
				if (lmt.Size()==0||tex==NULL) return;
				Posize texPs=GetTexturePosize(tex);
				Posize targetPS={pt.x-texPs.w/2,pt.y-texPs.h/2,texPs.w,texPs.h};
				SDL_Rect srct=PosizeToSDLRect(((lmt&targetPS)-targetPS)&texPs),
						 drct=PosizeToSDLRect(lmt&targetPS);
				SDL_RenderCopy(ren,tex,&srct,&drct);
			}
			
			void RenderDrawText(const string &str,const Posize &tarPS,const Posize &lmt,const int mode,const RGBA &co,const int fontSize);
			
			void Debug_DisplayBorder(const Posize &ps,const RGBA &co={255,0,0,200})
			{
				if (DEBUG_DisplayBorderFlag)
				{
					SDL_SetRenderDrawColor(ren,co.r,co.g,co.b,co.a);
					SDL_RenderDrawLine(ren,ps.x+5,ps.y,ps.x2()-5,ps.y);
					SDL_RenderDrawLine(ren,ps.x+5,ps.y2(),ps.x2()-5,ps.y2());
					SDL_RenderDrawLine(ren,ps.x,ps.y+5,ps.x,ps.y2()-5);
					SDL_RenderDrawLine(ren,ps.x2(),ps.y+5,ps.x2(),ps.y2()-5);
					SDL_RenderDrawLine(ren,ps.x+5,ps.y,ps.x,ps.y+5);
					SDL_RenderDrawLine(ren,ps.x2()-5,ps.y,ps.x2(),ps.y+5);
					SDL_RenderDrawLine(ren,ps.x+5,ps.y2(),ps.x,ps.y2()-5);
					SDL_RenderDrawLine(ren,ps.x2()-5,ps.y2(),ps.x2(),ps.y2()-5);
					
					SDL_SetRenderDrawColor(ren,0,0,255,200);
					SDL_RenderDrawLine(ren,ps.x,ps.y,ps.x+4,ps.y);
					SDL_RenderDrawLine(ren,ps.x,ps.y,ps.x,ps.y+4);
					SDL_RenderDrawLine(ren,ps.x2(),ps.y,ps.x2()-4,ps.y);
					SDL_RenderDrawLine(ren,ps.x2(),ps.y,ps.x2(),ps.y+4);
					SDL_RenderDrawLine(ren,ps.x,ps.y2(),ps.x+4,ps.y2());
					SDL_RenderDrawLine(ren,ps.x,ps.y2(),ps.x,ps.y2()-4);
					SDL_RenderDrawLine(ren,ps.x2(),ps.y2(),ps.x2()-4,ps.y2());
					SDL_RenderDrawLine(ren,ps.x2(),ps.y2(),ps.x2(),ps.y2()-4);
				}
			}
			
			void DEBUG_DisplayPresentLimit()
			{
//				Debug_DisplayBorder(PresentLimit,{0,255,0,127});
//				Debug_DisplayBorder(PresentLimit.Shrink(1),{0,255,0,127});
//				Debug_DisplayBorder(PresentLimit.Shrink(2),{0,255,0,127});
				SetRenderColor({0,255,0,127});
				for (int i=0;i<=2;++i)
				{
					SDL_Rect rct=PosizeToSDLRect(PresentLimit.Shrink(i));
					SDL_RenderDrawRect(ren,&rct);
				}
			}
			
			inline void SetPresentArea(const Posize &ps)//Need improve...
			{
				NeedFreshScreen=1;
				PresentLimit|=ps;
			}
			
			PUI_Window(const Posize &winps,const string &title,unsigned int flagWin,unsigned int flagRen);
			
			~PUI_Window();
	};
	set <PUI_Window*> PUI_Window::AllWindow;
	map <unsigned int,PUI_Window*> PUI_Window::WinOfSDLWinID;
	int PUI_Window::WindowCnt=0;
	bool PUI_Window::NeedFreshScreenAll=1;
	
	PUI_Window *MainWindow=NULL,
			   *CurrentWindow=NULL;
	
	struct PUI_ThemeColor
	{
		RGBA //MainColor[8]={{224,255,224,255},{192,255,192,255},{160,255,160,255},{128,255,128,255},{96,255,96,255},{64,255,64,255},{32,255,32,255},{0,255,0,255}},
			 MainColor[8]={{224,236,255,255},{192,217,255,255},{160,197,255,255},{128,178,255,255},{96,158,255,255},{64,139,255,255},{32,119,255,255},{0,100,255,255}},
			 //SecondColor[8]={{224,236,255,255},{192,217,255,255},{160,197,255,255},{128,178,255,255},{96,158,255,255},{64,139,255,255},{32,119,255,255},{0,100,255,255}},
			 SecondColor[8]={{224,255,224,255},{192,255,192,255},{160,255,160,255},{128,255,128,255},{96,255,96,255},{64,255,64,255},{32,255,32,255},{0,255,0,255}},
			 BackgroundColor[8]={{250,250,250,255},{224,224,224,255},{192,192,192,255},{160,160,160,255},{128,128,128,255},{96,96,96,255},{64,64,64,255},{32,32,32,255}},//
			 MainTextColor[2]={RGBA_BLACK,RGBA_WHITE}
			 //...
			 ;
		inline RGBA& operator [] (int x)
		{
			if (InRange(x,0,7))
				return MainColor[x];
			else return DD<<"[Error] ThemeColor[x],x is not in Range[0,7]"<<endl,MainColor[7];
		}
	}ThemeColor,BackupOfThemeColor;
	
	//Font manage
	//...
//	namespace FontManager
//	{
	
	struct PUI_Font_Struct
	{
		private:
			TTF_Font *font[128];
			string fontFile;
			int defaultSize;

		public:
			inline TTF_Font* operator () ()
			{return font[defaultSize];}
			
			inline TTF_Font* operator [] (int x)
			{
				if (x==0)
					return font[defaultSize];
				if (InRange(x,1,127))
					if (font[x]==NULL)
						return font[x]=TTF_OpenFont(fontFile.c_str(),x);
					else return font[x];
				else DD<<"[Error] PUI_Font[x] x is not in range[1,127]"<<endl;
			}
			
			~PUI_Font_Struct()
			{
				for (int i=1;i<=127;++i)
					if (font[i]!=NULL)
						TTF_CloseFont(font[i]),
						font[i]=NULL;
			}
			
			PUI_Font_Struct(const string &_fontfile,const int _defaultsize):fontFile(_fontfile),defaultSize(_defaultsize)
			{
				memset(font,0,sizeof font);
				static bool Inited=0;
				if (!Inited)
				{
					if (TTF_Init()!=0)
						DD<<"[Error] Failed to TTF_Init()"<<endl;
					Inited=1;
				}	
				font[defaultSize]=TTF_OpenFont(fontFile.c_str(),defaultSize);
			}
	}PUI_Font("msyhl.ttc",14);//???
	
//	}
	
	//Surface/Texture/Drawing/... Functions:
	
//	void RenderDrawPointWithLmt(const Point &pt,const RGBA &co,const Posize &lmt,const PUI_Window *win=CurrentWindow)
//	{
//		if (lmt.In(pt)&&co.a!=0)//Is it slow?
//		{
//			SDL_SetRenderDrawColor(win->ren,co.r,co.g,co.b,co.a);
//			SDL_RenderDrawPoint(win->ren,pt.x,pt.y);
//		}
//	}
	
//	void RenderDrawShadow(/*...*/)
//	{
//		//...
//	}
	
	SDL_Texture *CreateTextureFromSurface(SDL_Surface *sur,const PUI_Window *win=CurrentWindow)
	{return SDL_CreateTextureFromSurface(win->ren,sur);}
	
	SDL_Texture *CreateTextureFromSurfaceAndDelete(SDL_Surface *sur,const PUI_Window *win=CurrentWindow)
	{
		SDL_Texture *tex=SDL_CreateTextureFromSurface(win->ren,sur);
		SDL_FreeSurface(sur);
		return tex;
	}
	
	SDL_Surface *CreateRGBATextSurface(const char *text,const RGBA &co,const int &fontSize=0)
	{
		SDL_Surface *sur=TTF_RenderUTF8_Blended(PUI_Font[fontSize],text,RGBAToSDLColor(co));
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		SDL_SetSurfaceAlphaMod(sur,co.a);
		return sur;
	}
	
	SDL_Texture *CreateRGBATextTexture(const char *text,const RGBA &co,const int &fontSize=0,const PUI_Window *win=CurrentWindow)
	{return CreateTextureFromSurfaceAndDelete(CreateRGBATextSurface(text,co,fontSize),win);}
	
	void PUI_Window::RenderDrawText(const string &str,const Posize &tarPS,const Posize &lmt,const int mode=0,const RGBA &co=RGBA_NONE,const int fontSize=0)
	{
		if (lmt.Size()==0||tarPS.Size()==0) return;
		SDL_Texture *tex=CreateRGBATextTexture(str.c_str(),!co?ThemeColor.MainTextColor[0]:co,fontSize,this);
		Posize texPs=GetTexturePosize(tex),
			   DBGborderPS;
		
		switch (mode)//0:mid -1:Left 1:Right
		{
			case 0:
				RenderCopy(tex,((lmt&tarPS)-tarPS-Point(tarPS.w-texPs.w>>1,tarPS.h-texPs.h>>1))&texPs,
						lmt&tarPS&(DBGborderPS=Posize((tarPS.w-texPs.w>>1)+tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h)));
				break;
			case 1:
				RenderCopy(tex,((lmt&tarPS)-tarPS-Point(tarPS.w-texPs.w,tarPS.h-texPs.h>>1))&texPs,
						lmt&tarPS&(DBGborderPS=Posize(tarPS.w-texPs.w+tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h)));
				break;
			case -1:
				RenderCopy(tex,((lmt&tarPS)-tarPS)&texPs,lmt&tarPS&(DBGborderPS=Posize(tarPS.x,(tarPS.h-texPs.h>>1)+tarPS.y,texPs.w,texPs.h)));
				break;
			default:
				DD<<"[Error] RenderDrawText wrong mode :"<<mode<<endl;
		}
		SDL_DestroyTexture(tex);

		if (DEBUG_DisplayBorderFlag)
			Debug_DisplayBorder(DBGborderPS,RGBA(255,178,178,255));
	}
	
	SDL_Surface *CreateRingSurface(int d1,int d2,const RGBA &co)//return a ring whose radius is d1/2 and d2/2 
	{
		if (d2<=0) return NULL;
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,d2,d2,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		Uint32 col=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a),col0=SDL_MapRGBA(sur->format,0,0,0,0);
		
		int r1=d1/2,r2=d2/2;
		for (int i=0;i<d2;++i)
			for (int j=0;j<d2;++j)
				*((Uint32*)sur->pixels+i*sur->pitch/4+j)=InRange((i-r2)*(i-r2)+(j-r2)*(j-r2),r1*r1,r2*r2)?col:col0;
		return sur;
	}
	
	SDL_Surface* CreateTriangleSurface(int w,int h,const Point &pt1,const Point &pt2,const Point &pt3,const RGBA &co)
	{
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		SDL_Rect rct;
		Uint32 col=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a),col0=SDL_MapRGBA(sur->format,0,0,0,0);
		for (int i=0;i<h;++i)
		{
			int j,k;
			for (j=0;j<w;++j)
			{
				Point v0(j,i),
					  v1=pt1-v0,
					  v2=pt2-v0,
					  v3=pt3-v0;
				if (abs(v1%v2)+abs(v2%v3)+abs(v3%v1)==abs((pt2-pt1)%(pt3-pt1)))
					break;
			}
			for (k=w-1;k>=j;--k)
			{
				Point v0(k,i),
					  v1=pt1-v0,
					  v2=pt2-v0,
					  v3=pt3-v0;
				if (abs(v1%v2)+abs(v2%v3)+abs(v3%v1)==abs((pt2-pt1)%(pt3-pt1)))
					break;
			}
			if (j>0)
			{
				rct={0,i,j,1};
				SDL_FillRect(sur,&rct,col0);
			}
			if (k>j)
			{
				rct={j,i,k-j+1,1};
				SDL_FillRect(sur,&rct,col);
			}
			if (k<w-1)
			{
				rct={k+1,i,w-k-1,1};
				SDL_FillRect(sur,&rct,col0);
			}
		}
		return sur;
	}
	
	SDL_Surface* CreateSpotLightSurface(int r,const RGBA &co={255,255,255,100})//return a round whose radius is r,alpha color decrease from center to edge 
	{
		if (r<=0) return NULL;
		DD<<"[Debug] CreateSpotLightSurface"<<endl;
		SDL_Surface *sur=SDL_CreateRGBSurfaceWithFormat(0,r<<1,r<<1,32,SDL_PIXELFORMAT_RGBA32);
		SDL_SetSurfaceBlendMode(sur,SDL_BLENDMODE_BLEND);
		
		for (int i=0;i<=r<<1;++i)
			for (int j=0;j<=r<<1;++j)
				*((Uint32*)sur->pixels+i*sur->pitch/4+j)=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a*max(0.0,1-((i-r)*(i-r)+(j-r)*(j-r))*1.0/(r*r)));
		return sur;
	}
	
	RGBA GetSDLSurfacePixel(SDL_Surface *sur,const Point &pt)//maybe format has some problem
	{
		if (sur==NULL||!Posize(0,0,sur->w-1,sur->h-1).In(pt))
			return RGBA_NONE;
		RGBA re=RGBA_NONE;
		SDL_LockSurface(sur);
		Uint32 col=((Uint32*)sur->pixels)[(pt.y*sur->w+pt.x)];
		SDL_UnlockSurface(sur);
		SDL_GetRGBA(col,sur->format,&re.r,&re.g,&re.b,&re.a);
		return re;
	}
	
	void SetSDLSurfacePixel(SDL_Surface *sur,const Point &pt,const RGBA &co)
	{
		if (sur==NULL||!Posize(0,0,sur->w-1,sur->h-1).In(pt))
			return;
		SDL_Rect rct={pt.x,pt.y,1,1};
		Uint32 col=SDL_MapRGBA(sur->format,co.r,co.g,co.b,co.a);
		SDL_FillRect(sur,&rct,col);
	}
	
	void SetVirturalPosEvent(const Point &pt,int type)//1:motion 2: down 3:up
	{
		
	}
	
	Point GetGlobalMousePoint()
	{
		Point re;
		SDL_GetGlobalMouseState(&re.x,&re.y);
		return re;
	}
	
	//Special functions:
	//...
	Uint32 PUI_EVENT_UpdateTimer=-1;
	struct PUI_UpdateTimerData
	{
		Widgets *tar=NULL;//the target to uptate
		int cnt=0;//0:Stop >0:use same interval cnt times(at this time stack is empty) -1:use same interval infinite times  -2:use intervals in stack;
		stack <Uint32> sta;
		atomic_char *enableFlag=NULL;//0:stop(set by timerFunc) 1:running 2:stop(set by outside func) 3:stop and delete atomic(set by ouside func)
									 //when set 2,timer will stop,and it will be set 0 when stoped; It won't be deleted after Data decontructed
		void *data=NULL;
	
		PUI_UpdateTimerData(Widgets *_tar,int _cnt=0,atomic_char *_enableflag=NULL,void *_data=NULL)
		:tar(_tar),cnt(_cnt),enableFlag(_enableflag),data(_data) {}
		
		PUI_UpdateTimerData() {}
	};
	
	Uint32 PUI_UpdateTimer(Uint32 interval,void *param)//mainly used to support animation effect
	{
		PUI_UpdateTimerData *p=(PUI_UpdateTimerData*)param;
//		DD<<"[Debug] PUI_UpdateTimer:  cnt "<<(p->cnt)<<endl;
	    
	    if (p->enableFlag!=NULL)
	    	if (*(p->enableFlag)!=1)
	    	{
	    		if (*(p->enableFlag)==3)
	    			delete p->enableFlag;
				else *(p->enableFlag)=0;
	    		delete p;
	    		return 0;
			}
			
	    SDL_Event event;
	    SDL_UserEvent userevent;
	    userevent.type=PUI_EVENT_UpdateTimer;
	    userevent.code=p->cnt==-2?p->sta.size():p->cnt;//how many times to run it needs
	    userevent.data1=p->tar;//target widgets
	    userevent.data2=p->data;
	    event.type=SDL_USEREVENT;
	    event.user=userevent;
	    SDL_PushEvent(&event);
	    
	    if (p->cnt==-2)//use interval in stack
		    if (p->sta.empty())
			{
		    	if (p->enableFlag!=NULL)
		    		if (*(p->enableFlag)==3)
	    				delete p->enableFlag;
					else *(p->enableFlag)=0;
				delete p;
		    	return 0;
			}
			else
			{
				Uint32 t=p->sta.top();
				p->sta.pop();
				if (t==0)
					t=1;
				return t;
			}
	    else if (p->cnt==-1)//infinite times
	    	return interval;
	    else 
			if (p->cnt==0)//run to end
		    {
		    	if (p->enableFlag!=NULL) 
		    		if (*(p->enableFlag)==3)
	    				delete p->enableFlag;
					else *(p->enableFlag)=0;
		    	delete p;
		    	return 0;
		    }
		    else//this time end
			{
				--p->cnt;
				return interval;	
			}
	}
	
	//Widgets:
	//...
	
	class PosizeEX
	{
		friend class Widgets;
		protected: 
			Widgets *wg=NULL;
			PosizeEX *nxt=NULL;
//			int UsedCnt=0;
			
			PosizeEX()//Ban construct PosizeEX
			{
				
			}
			
		public:
			virtual void GetrPS(Posize &ps)=0;
			
			virtual ~PosizeEX()
			{
				if (nxt!=NULL)
					delete nxt;
			}
	};
	 
	class Widgets
	{
		friend bool PresentWidgets();
		friend void PresentWidgets(PUI_Window *win,Posize lmt);
		friend void PresentWidgets(Widgets *tar);
		friend void UpdateWidgetsPosize(Widgets *wg);
		friend void UpdateWidgetsPosize(PUI_Window *win);
		friend int SolveEvent(SDL_Event &event);
		friend class PUI_Window;
		
		public:
			static map <int,Widgets*> WidgetsID;
			
			enum WidgetType
			{
				WidgetType_Widgets=0,
				WidgetType_Layer,
				WidgetType_Button,
				WidgetType_LargeLayerWithScrollBar,
				WidgetType_TinyText,
				WidgetType_CheckBox,
				WidgetType_SingleChoiceButton,
				WidgetType_Slider,
				WidgetType_TwinLayerWithDivideLine,
				WidgetType_ShapedPictureButton,
				WidgetType_PictureBox,
				WidgetType_SimpleListView,
				WidgetType_SimpleListView_MultiColor,
				WidgetType_ProgressBar,
				WidgetType_Menu1,
				WidgetType_SwitchButton,
				WidgetType_TextEditLine,
				WidgetType_SimpleBlockView,
				WidgetType_TextEditBox,
				WidgetType_TabLayer,
				WidgetType_AddressSection,
				WidgetType_FileAddressSection,
				WidgetType_TinyText2,
				WidgetType_SimpleTreeView1,
				WidgetType_SimpleTreeView,
//				WidgetType_WallpaperLayer_Dot,
				WidgetType_DetailedListView
			};
			
		protected:
			static queue <Widgets*> WidgetsToDeleteAfterEvent;
			
			int ID=0;
			WidgetType Type=WidgetType_Widgets;
			int Depth=0;
			bool MultiWidgets=0;
			Posize rPS=ZERO_POSIZE,gPS=ZERO_POSIZE,//Relative Posize,Global Posize
				   CoverLmt=ZERO_POSIZE;//Last limit posize,used for partial update
			PUI_Window *Win=NULL;
			PosizeEX *PsEx=NULL;
			bool Enabled=1,
				 Selected=0; 
			Widgets *fa=NULL,
					*preBrother=NULL,
					*nxtBrother=NULL,
					*childWg=NULL;
			bool PresentThisLater=0,
				 SolvePosEventFirst=0, 
				 LimitPosIngPS=1,
				 NeedLoseFocus=0;
				 
			void SetDelayDeleteThis()
			{WidgetsToDeleteAfterEvent.push(this);}
			
			void SetNeedLoseFocus()
			{
				if (NeedLoseFocus)
				{
					DD<<"[Waring] NeedLoseFocus "<<ID<<"  have been set!"<<endl;
					return;
				}
				NeedLoseFocus=1;
				PUI_Window::LoseFocusLinkTable *p=new PUI_Window::LoseFocusLinkTable(this);
				p->nxt=Win->LoseFocusWgHead;
				Win->LoseFocusWgHead=p;
				++Win->LoseFocusState;
			}
			
			void RemoveNeedLoseFocus()
			{
				if (!NeedLoseFocus)
				{
					DD<<"[Waring] NeedLoseFocus of Widgets "<<ID<<" have been removed!"<<endl;
					return;
				}
				NeedLoseFocus=0;
				PUI_Window::LoseFocusLinkTable *p=Win->LoseFocusWgHead;
				if (Win->LoseFocusWgHead->wg==this)
				{
					Win->LoseFocusWgHead=Win->LoseFocusWgHead->nxt;
					delete p;
					--Win->LoseFocusState;
					return;
				}
				else
				{
					for (PUI_Window::LoseFocusLinkTable *q=Win->LoseFocusWgHead->nxt;q;p=q,q=q->nxt)
						if (q->wg==this)
						{
							p->nxt=q->nxt;
							delete q;
							q=p;
							--Win->LoseFocusState;
							return;
						}
					DD<<"[Error] Failed to find this LoseFocusWidget "<<ID<<endl;
				}
			}
			
			int AutoGetID()
			{
				static int re=1e4;
				++re;
				while (WidgetsID.find(re)!=WidgetsID.end())
					++re;
				return re;
			}

			void SplitFromFa()//Split this Node from linktable
			{
				if (fa==NULL) return;
				if (fa->childWg==this)
					fa->childWg=nxtBrother;
				else preBrother->nxtBrother=nxtBrother;
				if (nxtBrother!=NULL)
					nxtBrother->preBrother=preBrother;
				fa=preBrother=nxtBrother=NULL;
			}
			
			const Posize& GetFaCoverLmt()
			{
				if (fa==NULL)
					return gPS;
				else return fa->CoverLmt;
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
//				Win->PresentLimit|=gPS;
				if (fa!=NULL)
					gPS=rPS+fa->gPS;
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
//				Win->PresentLimit|=gPS;
			}
			
			virtual void ReceiveKeyboardInput() {}
			
			virtual void CheckEvent() {}
			
			virtual void CheckPos() {}
			
			virtual void Show(Posize &lmt) {}
			
			void _SolveEvent()
			{
				if (!Win->NeedSolveEvent) 
					return;
				if (nxtBrother!=NULL)
					nxtBrother->_SolveEvent();
				if (!Enabled) return;
				if (childWg!=NULL)
					childWg->_SolveEvent();
				if (Win->NeedSolveEvent)
					CheckEvent();
			}
			
			void _SolvePosEvent()
			{
				if (!Win->NeedSolvePosEvent)
					return;
				if (Enabled)
					if (!LimitPosIngPS||gPS.In(Win->NowPos))
						if (SolvePosEventFirst)
						{
							CheckPos();
							if (childWg!=NULL)
								childWg->_SolvePosEvent();
						}
						else
						{
							if (childWg!=NULL)
								childWg->_SolvePosEvent();
							if (Win->NeedSolvePosEvent)
								CheckPos();
						}
				if (nxtBrother!=NULL)
					nxtBrother->_SolvePosEvent();
			}
			
			void _PresentWidgets(Posize lmt)
			{
				if (nxtBrother!=NULL)
					nxtBrother->_PresentWidgets(lmt);
				if (Enabled)
				{
//					LastLmt=lmt=lmt&gPS;
					lmt=lmt&gPS;
					if (PresentThisLater)
					{
						if (childWg!=NULL)
							childWg->_PresentWidgets(lmt);
//						if (lmt.Size()!=0)//??
							Show(lmt);
						if (DEBUG_EnableWidgetsShowInTurn)
							SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
							SDL_RenderPresent(Win->ren);
					}
					else
					{
//						if (lmt.Size()!=0)//??
							Show(lmt);
						if (DEBUG_EnableWidgetsShowInTurn)
							SDL_Delay(DEBUG_WidgetsShowInTurnDelayTime),
							SDL_RenderPresent(Win->ren);
						if (childWg!=NULL)
							childWg->_PresentWidgets(lmt);
					}
				}
			}
			
			Widgets()//Ban construct widgets.
			{
				
			}
			
		public:
			static void SetDelayDeleteWidget(Widgets *tar)
			{WidgetsToDeleteAfterEvent.push(tar);}
			
			void DelayDelete()
			{WidgetsToDeleteAfterEvent.push(this);}
			
			inline int GetID()
			{return ID;}
		
			void SetID(int _ID)//if _ID==0 auto set ID
			{
				if (_ID==0)
					_ID=AutoGetID();
				if (ID==_ID) return;
				if (ID!=0)
					WidgetsID.erase(ID);
				if (WidgetsID.find(_ID)==WidgetsID.end())
					WidgetsID[ID=_ID]=this;
				else DD<<"[ERROR] ID Occupied :"<<_ID<<"\n";
			}
				
			inline WidgetType GetType()
			{return Type;}
			
//			Posize& GetrPS_Ref()
//			{return rPS;}
			
//			Posize& GetgPS_Ref()
//			{return gPS;}
			
			inline Posize GetrPS()
			{return rPS;}
			
			virtual void SetrPS(const Posize &ps)//virtual??
			{
				CoverLmt=gPS=rPS=ps;
				//...
				if (Win!=NULL)
				{
					Win->NeedUpdatePosize=1;
					Win->NeedFreshScreen=1;
				}
			}
			
			inline Posize GetgPS()
			{return gPS;}
			
			inline void SetEnabled(bool bo)
			{
				Enabled=bo;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline bool GetEnabled()
			{return Enabled;}
			
			inline PUI_Window* GetWin()
			{return Win;}

			inline Widgets* GetFa()
			{return fa;}
			
			inline int GetDepth()
			{return Depth;}
			
			void SetFa(Widgets *_fa)//Add in the linktable head 
			{
				SplitFromFa();
				if (_fa==NULL) return;
				fa=_fa;
				Win=fa->Win;
				Depth=fa->Depth+1;
				nxtBrother=fa->childWg;
				if (nxtBrother!=NULL)
					nxtBrother->preBrother=this;
				fa->childWg=this;
				
			}
			
			void AddPsEx(PosizeEX *psex)
			{
				if (PsEx!=NULL)
					psex->nxt=PsEx;
				PsEx=psex;
				PsEx->wg=this;
				CalcPsEx();//??
				Win->NeedUpdatePosize=1;
			}

			
			virtual ~Widgets()
			{
				DD<<"[Info] Delete Widgets "<<ID<<endl;
				delete PsEx;
				while (childWg!=NULL)
					delete childWg;
				SplitFromFa();
				if (NeedLoseFocus)
					RemoveNeedLoseFocus();
				if (ID!=0)
					WidgetsID.erase(ID),ID=0;
				if (Enabled)
				{
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				DD<<"[Info] Delete Widgets OK"<<endl;
			}
	};
	
	map <int,Widgets*> Widgets::WidgetsID;
	queue <Widgets*> Widgets::WidgetsToDeleteAfterEvent;
	
	class PosizeEX_Fa6:public PosizeEX
	{
		protected:
			Uint8 xNotConsider=0,yNotConsider=0;//faDep=1;//faDep:relative to which father(Cannot use yet)
			double ra,rb,rc,rd;//Positive:pixels Negative:proportion
						
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);//Linktable:Run GetrPS in the order it created 
				Posize psfa=wg->GetFa()->GetrPS();
					
				double a=ra<0?(-ra)*psfa.w:ra,
					   b=rb<0?(-rb)*psfa.w:rb,
					   c=rc<0?(-rc)*psfa.h:rc,
					   d=rd<0?(-rd)*psfa.h:rd;
				switch (xNotConsider)
				{
					case 1: ps.w=a;ps.SetX2_ChangeX(psfa.w-b-1);break;
					case 2: ps.x=a;ps.SetX2(psfa.w-b-1);break;
					case 3: ps.x=a;ps.w=b;break;
				}
				
				switch (yNotConsider)
				{
					case 1: ps.h=c;ps.SetY2_ChangeY(psfa.h-d-1);break;
					case 2: ps.y=c;ps.SetY2(psfa.h-d-1);break;
					case 3: ps.y=c;ps.h=d;break;
				}
			}
			
			PosizeEX_Fa6(Uint8 _xn,Uint8 _yn,double a,double b,double c,double d)
			:xNotConsider(_xn),yNotConsider(_yn),ra(a),rb(b),rc(c),rd(d)
			{
				if (!InRange(xNotConsider,1,3)) DD<<"PosizeEX Error: xNotConsiderValue set "<<xNotConsider<<"\n";
				if (!InRange(yNotConsider,1,3)) DD<<"PosizeEX Error: yNotConsiderValue set "<<yNotConsider<<"\n";
			}
	};
	
	class PosizeEX_MidFa:public PosizeEX
	{
		protected:
			Posize CenterShift;
		
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);
				Posize psfa=wg->GetFa()->GetrPS();
				
				if (CenterShift.Size()!=0)
					ps.w=CenterShift.w,
					ps.h=CenterShift.h;
				ps.x=(psfa.w-ps.w>>1)+CenterShift.x;
				ps.y=(psfa.h-ps.h>>1)+CenterShift.y;
			}
			
			PosizeEX_MidFa(const Point &centreshift=ZERO_POINT)
			:CenterShift(centreshift.x,centreshift.y,0,0) {}
			
			PosizeEX_MidFa(const Posize &centreshift=ZERO_POSIZE)
			:CenterShift(centreshift) {}
	};
	
	class PosizeEX_MidFa_Single:public PosizeEX
	{
		protected:
			int delta;
			bool IsY;
					
		public:
			virtual void GetrPS(Posize &ps)
			{
				if (nxt!=NULL) nxt->GetrPS(ps);
				Posize psfa=wg->GetFa()->GetrPS();
				
				if (IsY) ps.y=(psfa.h-ps.h>>1)+delta;
				else ps.x=(psfa.w-ps.w>>1)+delta;
			}
			
			PosizeEX_MidFa_Single(bool isY,int _delta=0):IsY(isY),delta(_delta) {}
	};
	
	class PosizeEX_Nearby:public PosizeEX
	{
		protected:
			Widgets *tar=NULL;
			
			
		public:
			virtual void GetrPS(Posize &ps)
			{
				
			}

	};
	
	class Layer:public Widgets
	{
		protected:
			RGBA LayerColor=RGBA_NONE;
		
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,LayerColor);
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SetLayerColor(const RGBA &co)
			{
				LayerColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}

			Layer(int _ID,Layer *_fa,Posize _rPS)
			{
				SetID(_ID);
				DD<<"[Info] Create Layer "<<ID<<endl;
				Type=WidgetType_Layer;
				SetFa(_fa);
				SetrPS(_rPS);
			}
			
			Layer(int _ID,Widgets *_fa,PosizeEX *psex)
			{
				SetID(_ID);
				DD<<"[Info] Create Layer "<<ID<<endl;
				Type=WidgetType_Layer;
				SetFa(_fa);
				AddPsEx(psex);
			}
	};
	
	void PUI_Window::SetBackgroundColor(const RGBA &co=RGBA_WHITE)
	{
		if (BackGroundLayer!=NULL)
			BackGroundLayer->SetLayerColor(co);
		else DD<<"[Error] BackGroundLayer of Window "<<WindowTitle<<" is not created"<<endl;
	}
	
	PUI_Window::PUI_Window(const Posize &winps,const string &title="",unsigned int flagWin=SDL_WINDOW_RESIZABLE,unsigned int flagRen=SDL_RENDERER_ACCELERATED)
	:WinPS(winps),PresentLimit(winps.ToOrigin()),WindowTitle(title)
	{
		DD<<"[Info] Create window "<<WindowTitle<<endl;
		++WindowCnt;
		AllWindow.insert(this);
		win=SDL_CreateWindow(WindowTitle.c_str(),WinPS.x,WinPS.y,WinPS.w,WinPS.h,flagWin);
		ren=SDL_CreateRenderer(win,-1,flagRen);
		SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
		WinOfSDLWinID[SDLWinID=SDL_GetWindowID(win)]=this;
		
//		CurrentWindow=this;
		
		BackGroundLayer=new Layer(0,NULL,WinPS.ToOrigin());
		MenuLayer=new Layer(0,NULL,WinPS.ToOrigin());
		BackGroundLayer->Win=MenuLayer->Win=this;
		BackGroundLayer->SetLayerColor(RGBA_WHITE);
		PresentLimit|=WinPS.ToOrigin();
	}

	PUI_Window::~PUI_Window()
	{
		DD<<"[Info] Delete window "<<WindowTitle<<endl;
		
//		CurrentWindow=this;
		
		delete BackGroundLayer;
		delete MenuLayer;
		
		WinOfSDLWinID.erase(SDLWinID); 
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);
		AllWindow.erase(this);
		--WindowCnt;
		DD<<"[Debug] Delete window OK"<<endl;
	}
	
	class Button:public Widgets
	{
		protected:
//			bool EnableAnimation=1;
			int stat=0;//0:NoFocus 1:Focus 2:Down
			string Text;
			void (*func)(void*)=NULL;
			void *funcData=NULL;//All widgets won't delete pointer data set by user actively
			RGBA ButtonColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},//stat0,1,2
				 TextColor=RGBA_NONE;
				//if RGBA_NONE (or !HaveColor) ,use default ThemeColor 
			
			virtual void CheckEvent()
			{
				
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							stat=0;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							DD<<"[Info] Button "<<ID<<" click"<<endl;
							stat=2;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							DD<<"[Info] Button "<<ID<<" function"<<endl;
							if (func!=NULL)
								func(funcData);
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(lmt,ButtonColor[stat]?ButtonColor[stat]:ThemeColor[stat?(stat==1?3:5):1]);
				
//				static SDL_Texture *tex=CreateTextureFromSurfaceAndDelete(CreateSpotLightSurface(80,{255,255,255,160}));
//				if (stat!=0)//Test of spotLight effect
//					RenderCopyWithLmt(tex,Win->NowPos-Point(80,80),lmt&gPS);
				
				Win->RenderDrawText(Text,gPS,lmt,0,TextColor?TextColor:ThemeColor.MainTextColor[0]);

				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetButtonColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ButtonColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SetButtonColor error : p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetButtonText(const string &_text)
			{
				Text=_text;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void* GetFuncData()
			{return funcData;}
			
			void SetFunc(void (*_func)(void*),void *_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			Button(int _ID,Widgets *_fa,Posize _rPS,const string &_text="",void (*_func)(void*)=NULL,void *_funcdata=NULL)
			:Text(_text),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create Button "<<ID<<endl;
				Type=WidgetType_Button;
				SetFa(_fa);
				SetrPS(_rPS);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
			
			Button(int _ID,Widgets *_fa,PosizeEX *psex,const string &_text="",void (*_func)(void*)=NULL,void *_funcdata=NULL)
			:Text(_text),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create Button "<<ID<<endl;
				Type=WidgetType_Button;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
	};
	
	class TinyText:public Widgets
	{
		protected:
			int Mode=0;//0:mid -1:Left 1:Right
			string text;
			int FontSize=0;//0:default
//			bool AutoW=0;
			RGBA TextColor=RGBA_NONE;
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderDrawText(text,gPS,lmt,Mode,TextColor,FontSize);
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetText(const string &str)
			{
				text=str;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetFontSize(int size)
			{FontSize=size;}
			
			TinyText(int _ID,Widgets *_fa,const Posize &_rPS,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:text(str),Mode(_mode),TextColor(textColor)
			{
				SetID(_ID);
				DD<<"[Info] Create TinyText "<<ID<<endl;
				Type=WidgetType_TinyText;
				SetFa(_fa);
				SetrPS(_rPS);
			}
			
			TinyText(int _ID,Widgets *_fa,PosizeEX *psex,const string &str,int _mode=0,const RGBA &textColor=RGBA_NONE)
			:text(str),Mode(_mode),TextColor(textColor)
			{
				SetID(_ID);
				DD<<"[Info] Create TinyText "<<ID<<endl;
				Type=WidgetType_TinyText;
				SetFa(_fa);
				AddPsEx(psex);
			}
	};
	
	class TinyText2:public TinyText
	{
		protected:
			void (*func)(void*,string&)=NULL;
			void *funcData=NULL;
			
			virtual void Show(Posize &lmt)
			{
				if (func!=NULL)
					func(funcData,text);
				Win->RenderDrawText(text,gPS,lmt,Mode,TextColor,FontSize);
				
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetUpdateFunc(void (*_func)(void*,string&),void *_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			TinyText2(int _ID,Widgets *_fa,const Posize &_rps,const string &str,int _mode=0,void (*_func)(void*,string&)=NULL,void *_funcdata=NULL)
			:TinyText(_ID,_fa,_rps,str,_mode),func(_func)
			{
				DD<<"[Info] Create TinyText2 "<<ID<<endl;
				Type=WidgetType_TinyText2;
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
		
			TinyText2(int _ID,Widgets *_fa,PosizeEX *psex,const string &str,int _mode=0,void (*_func)(void*,string&)=NULL,void *_funcdata=NULL)
			:TinyText(_ID,_fa,psex,str,_mode),func(_func)
			{
				DD<<"[Info] Create TinyText2 "<<ID<<endl;
				Type=WidgetType_TinyText2;
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
	};

	class CheckBox:public Widgets
	{
		protected:
			bool on=0;
			int stat=0;//0:Up_NoFocus 1:Up_Focus 2:Down (same with Button)
			void (*func)(void*,bool)=NULL;//bool: on
			void *funcData=NULL;
			RGBA BorderColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 ChooseColor=RGBA_NONE;
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							stat=0;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							DD<<"[Info] CheckBox "<<ID<<" click"<<endl;
							stat=2;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							on=!on;
							DD<<"[Info] CheckBox "<<ID<<" switch to "<<on<<endl;
							if (func!=NULL)
								func(funcData,on);
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}

			virtual void Show(Posize &lmt)
			{
				Win->RenderDrawRectWithLimit(gPS,BorderColor[stat]?BorderColor[stat]:ThemeColor[stat?(stat==1?4:6):2],lmt);
				if (on)
					Win->RenderFillRect(lmt&gPS.Shrink(3),ChooseColor?ChooseColor:ThemeColor[5]);

				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] CheckBox: SetBorderColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
		 	inline void SetChooseColor(const RGBA &co)
			{
				ChooseColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void* GetFuncData()
			{return funcData;}
			
			void SetFunc(void (*_func)(void*,bool),void *_funcdata=NULL)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			void SetOnOff(bool _on)
			{
				if (on==_on) return;
				on=_on;
				func(funcData,on);
				Win->NeedFreshScreen;
				Win->PresentLimit|=gPS;
			}
			
			inline bool GetOnOff()
			{return on;}
			
			CheckBox(int _ID,Widgets *_fa,Posize _rPS,bool defaultOnOff,void(*_func)(void*,bool)=NULL,void *_funcData=NULL)
			:on(defaultOnOff),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create CheckBox "<<ID<<endl;
				Type=WidgetType_CheckBox;
				SetFa(_fa);
				SetrPS(_rPS);
				if (_funcData==CONST_THIS)
					funcData=this;
				else funcData=_funcData;
			}
			
			CheckBox(int _ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff,void(*_func)(void*,bool)=NULL,void *_funcData=NULL)
			:on(defaultOnOff),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create CheckBox "<<ID<<endl;
				Type=WidgetType_CheckBox;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcData==CONST_THIS)
					funcData=this;
				else funcData=_funcData;
			}
	};
	
	class SwitchButton:public CheckBox//This is also a widget to test animation
	{
		protected:
			RGBA ChunkColor=RGBA_NONE;
			double chunkPercent=0;
			int ChunkWidth=12,
				UpdateInterval=15,
				UpdataCnt=5;
			SDL_TimerID IntervalTimerID=0;
			
			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				if (event.type==SDL_USEREVENT)
					if (event.user.type==PUI_EVENT_UpdateTimer)
						if (event.user.data1==this)
						{
							if (event.user.code==0)
								IntervalTimerID=0;
							chunkPercent=on?1-event.user.code*1.0/UpdataCnt:event.user.code*1.0/UpdataCnt;
							Win->NeedSolveEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							stat=0;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							DD<<"[Info] SwitchButton: "<<ID<<" click"<<endl;
							stat=2;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							on=!on;
							DD<<"[Info] SwitchButton: "<<ID<<" switch to "<<on<<endl;
							if (func!=NULL)
								func(funcData,on);
							if (IntervalTimerID!=0)
								SDL_RemoveTimer(IntervalTimerID);
							IntervalTimerID=SDL_AddTimer(UpdateInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,UpdataCnt));
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderDrawRectWithLimit(gPS,BorderColor[stat]?BorderColor[stat]:ThemeColor[stat?(stat==1?3:5):1],lmt);
				Win->RenderDrawRectWithLimit(gPS.Shrink(1),BorderColor[stat]?BorderColor[stat]:ThemeColor[stat?(stat==1?3:5):1],lmt);
				
				Win->RenderFillRect(gPS.Shrink(4)&lmt,BorderColor[stat]?BorderColor[stat]:ThemeColor[stat?(stat==1?3:5):1]); 
				Win->RenderFillRect(Posize(gPS.x+4,gPS.y+4,(gPS.w-ChunkWidth)*chunkPercent-4,gPS.h-8)&lmt,ChooseColor?ChooseColor:ThemeColor.SecondColor[3]);
				Win->RenderFillRect(Posize(gPS.x+(gPS.w-ChunkWidth)*chunkPercent,gPS.y,ChunkWidth,gPS.h)&lmt,ChunkColor?ChunkColor:ThemeColor.MainTextColor[0]);//ThisColor OK?
				
				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			inline void SetChunkColor(const RGBA &co)
			{
				ChunkColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetChunkWidth(int w)
			{
				ChunkWidth=w;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			SwitchButton(int _ID,Widgets *_fa,Posize _rPS,bool defaultOnOff,void(*_func)(void*,bool)=NULL,void *_funcData=NULL)
			:CheckBox(_ID,_fa,_rPS,defaultOnOff,_func,_funcData),chunkPercent(defaultOnOff)
			{
				DD<<"[Info] Create SwitchButton "<<ID<<endl;
				Type=WidgetType_SwitchButton;
			}
			
			SwitchButton(int _ID,Widgets *_fa,PosizeEX* psex,bool defaultOnOff,void(*_func)(void*,bool)=NULL,void *_funcData=NULL)
			:CheckBox(_ID,_fa,psex,defaultOnOff,_func,_funcData),chunkPercent(defaultOnOff)
			{
				DD<<"[Info] Create SwitchButton "<<ID<<endl;
				Type=WidgetType_SwitchButton;
			}
	};
	
	class SingleChoiceButton:public Widgets
	{
		protected :
			int ButtonCnt=0,
				CurrentChoose=-1,//count from 0;
				ChosenChoice=-1,
				eachChoiceHeight=20,
				ringD1=10,ringD2=12,
				stat=0;//0:Up_NoFocus 1:Up_Focus 2:Down
			SDL_Texture *ringTex=NULL,
						*roundTex=NULL;
			vector <string> Text;
			void (*func)(void *,const string&,int)=NULL;//int:choose
			void *funcData=NULL;
			RGBA TextColor=RGBA_NONE,
				 ButtonColor[2]={RGBA_NONE,RGBA_NONE},
				 RingColor[2]={RGBA_NONE,RGBA_NONE};//ring,round
			
			int GetCurrentChoose(int y)
			{
				int re=(Win->NowPos.y-gPS.y)/eachChoiceHeight;
				if (InRange(re,0,ButtonCnt-1))
					return re;
				else return -1;
			}
			
			virtual void CheckEvent()
			{
				
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							stat=0;
							Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
							CurrentChoose=-1;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							if (InRange(CurrentChoose,0,ButtonCnt-1))
								Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
							CurrentChoose=GetCurrentChoose(Win->NowPos.y);
							if (CurrentChoose!=-1)
							{
								DD<<"[Info] SingleChoiceButton "<<ID<<" click "<<CurrentChoose<<endl;
								stat=2;
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
							}
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							if (InRange(CurrentChoose,0,ButtonCnt-1))
							Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
							if (InRange(ChosenChoice,0,ButtonCnt-1))
								Win->PresentLimit|=Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight);
							CurrentChoose=GetCurrentChoose(Win->NowPos.y);
							if (CurrentChoose!=-1)
							{
								ChosenChoice=CurrentChoose;
								DD<<"[Info] SingleChoiceButton "<<ID<<" choose "<<CurrentChoose<<endl;
								if (func!=NULL)
									func(funcData,Text[CurrentChoose],CurrentChoose);
								stat=1;
							}
							if (InRange(ChosenChoice,0,ButtonCnt-1))
								Win->PresentLimit|=Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight);
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							CurrentChoose=GetCurrentChoose(Win->NowPos.y);
							stat=1;
							SetNeedLoseFocus();
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							if (CurrentChoose!=-1)
								Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
						}
						else
							if (CurrentChoose!=GetCurrentChoose(Win->NowPos.y))
							{
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
								CurrentChoose=GetCurrentChoose(Win->NowPos.y);
								if (!InRange(CurrentChoose,0,ButtonCnt-1))
									CurrentChoose=-1;
								else Win->PresentLimit|=Posize(gPS.x,gPS.y+CurrentChoose*eachChoiceHeight,gPS.w,eachChoiceHeight);
							}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			void Show(Posize &lmt)
			{
				for (int i=0;i<ButtonCnt;++i)
				{
					if (i==CurrentChoose)
						Win->RenderFillRect(lmt&Posize(gPS.x,gPS.y+i*eachChoiceHeight,gPS.w,eachChoiceHeight),ButtonColor[stat-1]?ButtonColor[stat-1]:ThemeColor[stat-1]);
					if (ringTex==NULL)
						ringTex=CreateTextureFromSurfaceAndDelete(CreateRingSurface(ringD1,ringD2,RingColor[0]?RingColor[0]:ThemeColor[5]));
					Win->RenderCopyWithLmt(ringTex,Posize(gPS.x,gPS.y+i*eachChoiceHeight,eachChoiceHeight,eachChoiceHeight).Shrink(eachChoiceHeight-ringD2>>1),lmt);
					if (i==ChosenChoice)
					{
						if (roundTex==NULL)
							roundTex=CreateTextureFromSurfaceAndDelete(CreateRingSurface(0,ringD1-4,RingColor[1]?RingColor[1]:ThemeColor[7]));
						Win->RenderCopyWithLmt(roundTex,Posize(gPS.x,gPS.y+i*eachChoiceHeight,eachChoiceHeight,eachChoiceHeight).Shrink(eachChoiceHeight-ringD1+4>>1),lmt);
					}
					Win->RenderDrawText(Text[i],{gPS.x+eachChoiceHeight,gPS.y+i*eachChoiceHeight,gPS.w-eachChoiceHeight,eachChoiceHeight},lmt,-1,TextColor);
					
					Win->Debug_DisplayBorder({gPS.x,gPS.y+i*eachChoiceHeight,gPS.w,eachChoiceHeight});
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetButtonColor(int p,const RGBA &co)
			{
				if (p==0||p==1)
				{
					ButtonColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SingleChoiceButton: SetButtonColor: p "<<p<<" is not 0 or 1"<<endl;
			}
			
			inline void SetRingColor(int p,const RGBA &co)
			{
				if (p==0||p==1)
				{
					RingColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SingleChoiceButton: SetRingColor: p "<<p<<" is not 0 or 1"<<endl;
			}
			
			virtual void SetrPS(const Posize &ps)//ignore h;
			{
				CoverLmt=gPS=rPS=ps;
				rPS.h=ButtonCnt*eachChoiceHeight;
				Win->NeedFreshScreen=1;
				Win->NeedUpdatePosize=1;
			}
			
			inline void* GetFuncData()
			{return funcData;}
			
			inline void SetFunc(void (*_func)(void*,const string&,int),void* _funcdata)
			{func=_func;funcData=_funcdata;}
			
			void SetAccentData(int h,int d1,int d2)
			{
				eachChoiceHeight=h;
				rPS.h=ButtonCnt*eachChoiceHeight;
				ringD1=d1;ringD2=d2;
				if (ringTex!=NULL)
					SDL_DestroyTexture(ringTex),
					ringTex=NULL;
				if (roundTex!=NULL)
					SDL_DestroyTexture(roundTex),
					roundTex=NULL;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
			SingleChoiceButton* AddChoice(const string &str)
			{
				Text.push_back(str);
				++ButtonCnt;
				rPS.h=ButtonCnt*eachChoiceHeight;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				return this;
			}
			
			void ClearAllChoice()
			{
				Text.clear();
				ButtonCnt=0;
				rPS.h=0;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
//			void EraseChoice(int p)
			
			void SetChoice(int p)
			{
				if (InRange(p,0,ButtonCnt-1))
					if (ChosenChoice!=p)
					{
						if (InRange(ChosenChoice,0,ButtonCnt-1))
							Win->PresentLimit|=Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight);
						ChosenChoice=p;
						if (InRange(ChosenChoice,0,ButtonCnt-1))
							Win->PresentLimit|=Posize(gPS.x,gPS.y+ChosenChoice*eachChoiceHeight,gPS.w,eachChoiceHeight);
						if (func!=NULL)
							func(funcData,Text[ChosenChoice],p);
						Win->NeedFreshScreen=1;
					}
					else DD<<"[Waring] SingleChoiceButton: SetChoice: p==ChosenChoice "<<p<<endl;
				else DD<<"[Error] SingleChoiceButton: SetChoice: p "<<p<<" is not in Range[0,ButtonCnt "<<ButtonCnt<<")"<<endl;
			}
			
			inline int GetChoice()
			{return ChosenChoice;}
			
			inline string GetChoiceText()
			{return Text[ChosenChoice];}

			virtual ~SingleChoiceButton()
			{
				DD<<"[Info] Delete SingleChoiceButton "<<ID<<endl;
				ClearAllChoice();
				if (ringTex!=NULL)
					SDL_DestroyTexture(ringTex);
				if (roundTex!=NULL)
					SDL_DestroyTexture(roundTex);
			}
			
			SingleChoiceButton(int _ID,Widgets *_fa,const Posize &_rps,void(*_func)(void*,const string&,int)=NULL,void *_funcData=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SingleChoiceButton "<<ID<<endl;
				Type=WidgetType_SingleChoiceButton;
				SetFa(_fa);
				SetrPS(_rps);
				if (_funcData==CONST_THIS)
					funcData=this;
				else funcData=_funcData;
			}
			
			SingleChoiceButton(int _ID,Widgets *_fa,PosizeEX* psex,void(*_func)(void*,const string&,int)=NULL,void *_funcData=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SingleChoiceButton "<<ID<<endl;
				Type=WidgetType_SingleChoiceButton;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcData==CONST_THIS)
					funcData=this;
				else funcData=_funcData;
			}
	};
	
	class DropDownBox:public Widgets
	{
		protected:
		
		public:
			
	};
	
	class Button2:public Button
	{
		
	};
	
	class ShapedPictureButton:public Widgets
	{
		protected:
			string Text;
			SDL_Texture *pic[3]={NULL,NULL,NULL};//this pic should have the same size with rPS
			SDL_Surface *sur0=NULL;//mask
			bool AutoDeletePic[3]={0,0,0},
				 AutoDeleteSur0=0,
//				 OnlyOnePic=1,//only use sur0,pic0,pic1 and 2 auto create;(not usable yet)
				 ThroughBlankPixel=1;
			int stat=0,//0:Up_NoFocus 1:Up_Focus 2:Down
				ThroughLmtValue=0;//rgba.a not greater than this will through
			void (*func)(void*)=NULL;
			void *funcData=NULL;
			RGBA TextColor=RGBA_NONE;
		
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos)||(ThroughBlankPixel&&GetSDLSurfacePixel(sur0,Win->NowPos-gPS.GetLU()).a<=ThroughLmtValue))
						{
							stat=0;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
							if (!ThroughBlankPixel||GetSDLSurfacePixel(sur0,Win->NowPos-gPS.GetLU()).a>ThroughLmtValue)
							{
								DD<<"[Info] ShapedPictureButton "<<ID<<" click"<<endl;
								stat=2;
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							if (!ThroughBlankPixel||GetSDLSurfacePixel(sur0,Win->NowPos-gPS.GetLU()).a>ThroughLmtValue)
							{
								DD<<"[Info] ShapedPictureButton "<<ID<<" function"<<endl;
								if (func!=NULL)
									func(funcData);
								stat=1;
								Win->NeedSolvePosEvent=0;
							}
							else
							{
								stat=0;
								RemoveNeedLoseFocus();
							}
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (!ThroughBlankPixel||GetSDLSurfacePixel(sur0,Win->NowPos-gPS.GetLU()).a>ThroughLmtValue)
						{
							if (stat==0)
							{
								DD<<"[Info] ShapedPictureButton "<<ID<<" focus"<<endl;
								stat=1;
								SetNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
							Win->NeedSolvePosEvent=0;
						}
						break;
				}
			}		
			
			virtual void Show(Posize &lmt)//So short ^v^
			{
				Win->RenderCopyWithLmt(pic[stat],gPS,lmt);
				Win->RenderDrawText(Text,gPS,lmt);

				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetMaskPic(SDL_Surface *sur,bool AutoDelete,bool UseThisSurAsPic=0)
			{
				if (sur==NULL) return;
				if (AutoDeleteSur0)
					SDL_FreeSurface(sur0);
				sur0=sur;
				AutoDeleteSur0=AutoDelete;
				if (UseThisSurAsPic)
					DD<<"[Error] ShapedPictureButton: UseThisSurAsPic is not usable yet"<<endl;
			}
			
			void SetButtonPic(int p,SDL_Texture *tex,bool AutoDelete)
			{
				if (InRange(p,0,2))
				{
					if (AutoDeletePic[p])
						SDL_DestroyTexture(pic[p]);
					pic[p]=tex;
					AutoDeletePic[p]=AutoDelete;
					Win->NeedFreshScreen;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] ShapedPictureButton: SetButtonPic: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void* GetFuncData()
			{return funcData;}
			
			void SetFunc(void (*_func)(void*),void *_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			inline void SetText(const string &_text)
			{
				Text=_text;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
		 	inline void SetThroughBlankPixel(int x=-1)//x==-1 means it won't through any pixel
			{
				if (x==-1)
					ThroughBlankPixel=0;
				else ThroughBlankPixel=1,ThroughLmtValue=x;
			}
			
			virtual ~ShapedPictureButton()
			{
				DD<<"[Info] Delete ShapedPictureButton "<<ID<<"\n";
				for (int i=0;i<=2;++i)
					if (AutoDeletePic[i])
						SDL_DestroyTexture(pic[i]);
				if (AutoDeleteSur0)
					SDL_FreeSurface(sur0);
			}
			
			ShapedPictureButton(int _ID,Widgets *_fa,const Posize &_rPS,void (*_func)(void*)=NULL,void *_funcdata=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create ShapedPictureButton "<<ID<<endl;
				Type=WidgetType_ShapedPictureButton;
				SetFa(_fa);
				SetrPS(_rPS);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
			
			ShapedPictureButton(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(void*)=NULL,void *_funcdata=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create ShapedPictureButton "<<ID<<endl;
				Type=WidgetType_ShapedPictureButton;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
	};
	 
	class LargeLayerWithScrollBar:public Widgets
	{
		protected:
			Posize ckPs_Ri,ckPs_Bu,ckBGps_Ri,ckBGps_Bu;//Right,Down
			bool EnableRightBar,EnableButtomBar;//Remember to adjust after changing the LargeAreaPosize!
			Layer* LargeAreaLayer=NULL;
			Posize largeAreaPS;
			Point ckCentreDelta={0,0};//Current click point to Chunk centre
			int ScrollBarWidth=16,
				MinBarLength=5,
				CustomWheelSensibility=0;//0 means use global
			bool AutoHideScrollBar=0,
				 EnableBarGap=1,
				 EnableBarSideButton=0,//I don't want to realize it immediatly...
				 EnableSmoothWheelScroll=0,
				 SmoothScrollData_Direction_y=1;
			int SmoothScrollData_Delta=0,
				SmoothScrollData_LeftCnt=0;
			SDL_TimerID SmoothScrollTimerID=0;
			int stat=0;//0:Up_NoFocus 1:Up_Focus_Bar_Right 2:Up_Focus_Bar_Buttom 3:Up_Focus_Chunk_Right 4:Up_Focus_Chunk_Buttom
					   //5:Down_Scroll_Chunk_Right 6:Down_Scroll_Chunk_Buttom 7:Up_Focus_LargeLayer__NotFocus_Bar
					   //9:Up_Focus_Button_Right_Minus 10:Down_Click_Button_Right_Minus 11:Up_Focus_Button_Right_Plus 12:Down_Click_Button_Right_Plus
					   //13:Up_Focus_Button_Buttom_Minus 14:Down_Click_Button_Buttom_Minus 15:Up_Focus_Button_Buttom_Plus 16:Down_Click_Button_Buttom_Plus
			RGBA BackgroundBarColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 BorderColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 ChunkColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 SideButtonColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE};
				 //Too many colors?
			
			class PosizeEX_LargeLayer:public PosizeEX
			{
				protected: 
					LargeLayerWithScrollBar *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=tar->largeAreaPS;
					}
					
					PosizeEX_LargeLayer(LargeLayerWithScrollBar *_tar)
					:tar(_tar) {}
			};
			
			void UpdateChunkPs()
			{
				if (EnableRightBar)
				{
					ckBGps_Ri={gPS.x2()-ScrollBarWidth+1,gPS.y,ScrollBarWidth,gPS.h};
					if (EnableBarGap)
						ckPs_Ri=ckBGps_Ri.Shrink(2);
					else ckPs_Ri=ckBGps_Ri;
					ckPs_Ri.h=EnsureInRange((ckBGps_Ri.h-EnableBarSideButton*2*ScrollBarWidth)*gPS.h*1.0/largeAreaPS.h,MinBarLength,ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth);
					ckPs_Ri.y=(long long)(ckBGps_Ri.h-EnableBarSideButton*2*ScrollBarWidth-EnableBarGap*4-ckPs_Ri.h)*(-largeAreaPS.y)*1.0/(largeAreaPS.h-gPS.h+EnableButtomBar*ScrollBarWidth)+ckBGps_Ri.y+EnableBarGap*2+EnableBarSideButton*ScrollBarWidth;
				}
				else ckPs_Ri=ckBGps_Ri=ZERO_POSIZE;

				if (EnableButtomBar)
				{
					ckBGps_Bu={gPS.x,gPS.y2()-ScrollBarWidth+1,EnableRightBar?gPS.w-ScrollBarWidth:gPS.w,ScrollBarWidth};
					if (EnableBarGap)
						ckPs_Bu=ckBGps_Bu.Shrink(2);
					else ckPs_Bu=ckBGps_Bu;
					ckPs_Bu.w=EnsureInRange((ckBGps_Bu.w-EnableBarSideButton*2*ScrollBarWidth)*gPS.w*1.0/largeAreaPS.w,MinBarLength,ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth);
					ckPs_Bu.x=(long long)(ckBGps_Bu.w-EnableBarSideButton*2*ScrollBarWidth-EnableBarGap*4-ckPs_Bu.w)*(-largeAreaPS.x)*1.0/(largeAreaPS.w-gPS.w+EnableRightBar*ScrollBarWidth)+ckBGps_Bu.x+EnableBarGap*2+EnableBarSideButton*ScrollBarWidth;			
				}
				else ckPs_Bu=ckBGps_Bu=ZERO_POSIZE;
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
				CoverLmt=gPS&GetFaCoverLmt();
				ResizeLL(0,0);//LargeArea may not change,but rPS may change,this function will also update large state
			}
			
		public:
			void SetViewPort(int ope,double val)//ope:: 1:SetPosX 2:..Y 3:SetPosPercent 4:..Y 5:MoveX 6:..Y
			{
				switch (ope)
				{
					case 1: largeAreaPS.x=-val;	break;
					case 2: largeAreaPS.y=-val;	break;
					case 3:	largeAreaPS.x=(rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w)*val;	break;
					case 4: largeAreaPS.y=(rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h)*val;	break;
					case 5:	largeAreaPS.x-=val;	break;
					case 6:	largeAreaPS.y-=val;	break;
					default:
						DD<<"[Error] SetViewPort ope "<<ope<<" is not in Range[1,6]"<<endl;
						return;
				}
				if (rPS.w==largeAreaPS.w) largeAreaPS.x=0;
				else largeAreaPS.x=EnsureInRange(largeAreaPS.x,rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w,0);
				if (rPS.h==largeAreaPS.h) largeAreaPS.y=0;
				else largeAreaPS.y=EnsureInRange(largeAreaPS.y,rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h,0);
				UpdateChunkPs();
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void ResizeLL(int _ww,int _hh)//_ww or _hh is 0 means not change this; 
			{
//				DD<<"[Info] ResizeLL "<<_ww<<" "<<_hh<<endl;
				if (_ww!=0) largeAreaPS.w=EnsureInRange(_ww,rPS.w,1e9);//?? +1 -1??
				else largeAreaPS.w=EnsureInRange(largeAreaPS.w,rPS.w,1e9);
				if (_hh!=0) largeAreaPS.h=EnsureInRange(_hh,rPS.h,1e9);
				else largeAreaPS.h=EnsureInRange(largeAreaPS.h,rPS.h,1e9);
				EnableButtomBar=largeAreaPS.w>rPS.w;
				EnableRightBar=largeAreaPS.h>rPS.h;
				if (rPS.w==largeAreaPS.w) largeAreaPS.x=0;
				else largeAreaPS.x=EnsureInRange(largeAreaPS.x,rPS.w-EnableRightBar*ScrollBarWidth-largeAreaPS.w,0);
				if (rPS.h==largeAreaPS.h) largeAreaPS.y=0;
				else largeAreaPS.y=EnsureInRange(largeAreaPS.y,rPS.h-EnableButtomBar*ScrollBarWidth-largeAreaPS.h,0);
				UpdateChunkPs();
				Win->NeedUpdatePosize=1;
			}
		
		protected:
			void SetSmoothScrollBar(bool IsY,int delta)//On testing...
			{
				SmoothScrollData_Direction_y=IsY;
				SmoothScrollData_Delta+=delta;
				SmoothScrollData_LeftCnt=10;
				if (SmoothScrollTimerID==0)
			 		SmoothScrollTimerID=SDL_AddTimer(10,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));//Careful! There exist a bug that the widget is deconstructed but the timer is still running(Use a class to manage the widgets' pointer may be a good way)
			}
			
			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				if (event.type==SDL_MOUSEWHEEL)
					switch (stat)
					{
						case 0: break;
						case 1:	case 3:
//							DD<<"Wheel H \n";
							if (EnableRightBar)
							{
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(1,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								else SetViewPort(6,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								Win->NeedSolveEvent=0;
							}
							break;
						case 2:	case 4:
//							DD<<"Wheel W \n";
							if (EnableButtomBar)
							{
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(0,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								else SetViewPort(5,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								Win->NeedSolveEvent=0;
							}
							break;
						case 7:
							if (EnableRightBar)
							{
//								DD<<"Wheel H \n";
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(1,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								else SetViewPort(6,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								Win->NeedSolveEvent=0;
							}
							else if (EnableButtomBar)
							{
//								DD<<"Wheel W \n";
								if (EnableSmoothWheelScroll) SetSmoothScrollBar(0,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								else SetViewPort(5,-event.wheel.y*(WheelSensibility==0?CustomWheelSensibility:WheelSensibility));
								Win->NeedSolveEvent=0;
							}
							break;
						default :
							DD<<"[Error] LargeLayerWithScrollBar "<<ID<<":Wheel Such state "<<stat<<" is not considered yet!"<<endl;
					}
				else if (event.type==SDL_USEREVENT)
					if (event.user.type==PUI_EVENT_UpdateTimer)
						if (event.user.data1==this)
							if (SmoothScrollData_LeftCnt>0)
							{
								int d=SmoothScrollData_Delta/SmoothScrollData_LeftCnt;
								if (d==0)
									d=SmoothScrollData_Delta;
								SetViewPort(SmoothScrollData_Direction_y?6:5,d);
								SmoothScrollData_Delta-=d;
								SmoothScrollData_LeftCnt--;
								if (SmoothScrollData_Delta==0||SmoothScrollData_LeftCnt<=0)
									if (SmoothScrollTimerID!=0)
									{
										SDL_RemoveTimer(SmoothScrollTimerID);
										SmoothScrollTimerID=0;
									}
								Win->NeedSolveEvent=0;
							}
			}
			
			/*
			//0:Up_NoFocus 1:Up_Focus_Bar_Right 2:Up_Focus_Bar_Buttom 3:Up_Focus_Chunk_Right 4:Up_Focus_Chunk_Buttom
			//5:Down_Scroll_Chunk_Right 6:Down_Scroll_Chunk_Buttom 7:Up_Focus_LargeLayer__NotFocus_Bar
			//9:Up_Focus_Button_Right_Minus 10:Down_Click_Button_Right_Minus 11:Up_Focus_Button_Right_Plus 12:Down_Click_Button_Right_Plus
			//13:Up_Focus_Button_Buttom_Minus 14:Down_Click_Button_Buttom_Minus 15:Up_Focus_Button_Buttom_Plus 16:Down_Click_Button_Buttom_Plus
			*/
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
							if (ckBGps_Ri.In(Win->NowPos))
							{
								DD<<"[Info] Sc.Down H"<<ID<<endl;
								stat=5;
								Win->OccupyPosWg=this;
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=ckBGps_Ri;
								if (ckPs_Ri.In(Win->NowPos))
									ckCentreDelta.y=Win->NowPos.y-ckPs_Ri.midY();
								else
								{
									ckCentreDelta.y=0;
									SetViewPort(4,(Win->NowPos.y-ckPs_Ri.h/2-ckBGps_Ri.y-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Ri.h));
								}
							}
							else if (ckBGps_Bu.In(Win->NowPos))
							{
								DD<<"[Info] Sc.Down W"<<ID<<endl;
								stat=6;
								Win->OccupyPosWg=this;
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=ckBGps_Bu;
								if (ckPs_Bu.In(Win->NowPos))
									ckCentreDelta.x=Win->NowPos.x-ckPs_Bu.midX();
								else
								{
									ckCentreDelta.x=0;
									SetViewPort(3,(Win->NowPos.x-ckPs_Bu.w/2-ckBGps_Bu.x-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Bu.w));
								}
							}
						break;
					case SDL_MOUSEBUTTONUP:
						if (stat==5||stat==6)
						{
							DD<<"[Info] Sc.Up "<<ID<<"\n";
							Win->OccupyPosWg=NULL;
							if (ckBGps_Ri.In(Win->NowPos)) stat=3;
							else if (ckPs_Bu.In(Win->NowPos)) stat=4;
							else if (ckBGps_Ri.In(Win->NowPos)) stat=1;
							else if (ckBGps_Bu.In(Win->NowPos)) stat=2;
							else if (gPS.In(Win->NowPos)) stat=7;
							else stat=0;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEMOTION://The code order here is very important
						if (stat==5)
							SetViewPort(4,(Win->NowPos.y-ckCentreDelta.y-ckPs_Ri.h/2-ckBGps_Ri.y-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Ri.h-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Ri.h)),
							Win->NeedSolvePosEvent=0;
						else if (stat==6)
							SetViewPort(3,(Win->NowPos.x-ckCentreDelta.x-ckPs_Bu.w/2-ckBGps_Bu.x-EnableBarGap*2-EnableBarSideButton*ScrollBarWidth)*1.0/(ckBGps_Bu.w-EnableBarGap*4-EnableBarSideButton*2*ScrollBarWidth-ckPs_Bu.w)), 
							Win->NeedSolvePosEvent=0;
						else
						{
							if (stat==0)
								SetNeedLoseFocus();
							bool MouseEventFlagChangeFlag=1;
							int temp_stat=stat;
							if (ckPs_Ri.In(Win->NowPos)) stat=3;
							else if (ckPs_Bu.In(Win->NowPos)) stat=4;
							else if (ckBGps_Ri.In(Win->NowPos)) stat=1;
							else if (ckBGps_Bu.In(Win->NowPos)) stat=2;
							else stat=7,MouseEventFlagChangeFlag=0;
							
							if (MouseEventFlagChangeFlag)
								Win->NeedSolvePosEvent=0;
							if (temp_stat!=stat)
							{
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=ckBGps_Bu|ckBGps_Ri;
								DD<<"[Info] Sc.Focus "<<ID<<endl;
							}
						}
						break;
					
					case SDL_FINGERMOTION://Testing
						if (stat==7)
						{
//							SetViewPort(5,-event.tfinger.dx*Win->WinPS.w);
							SetViewPort(6,-event.tfinger.dy*Win->WinPS.h);
							Win->PresentLimit|=gPS;
							Win->NeedSolvePosEvent=0;
						}
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (EnableRightBar)
				{
					RGBA colBG,colBorder,colCk,colButton;
					switch (stat)
					{
						case 0:	case 2:	case 4:	case 6:
							colBG=BackgroundBarColor[0]?BackgroundBarColor[0]:RGBA(255,255,255,63);
							colBorder=BorderColor[0]?BorderColor[0]:ThemeColor[1];
							colCk=ChunkColor[0]?ChunkColor[0]:ThemeColor[1];
							colButton=SideButtonColor[0]?SideButtonColor[0]:ThemeColor[0];
							break;
						case 1:	case 7:	
							colBG=BackgroundBarColor[1]?BackgroundBarColor[1]:RGBA(255,255,255,127);
							colBorder=BorderColor[1]?BorderColor[1]:ThemeColor[3];
							colCk=ChunkColor[1]?ChunkColor[1]:ThemeColor[3];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 3:	
							colBG=BackgroundBarColor[2]?BackgroundBarColor[2]:RGBA(255,255,255,191);
							colBorder=BorderColor[2]?BorderColor[2]:ThemeColor[5];
							colCk=ChunkColor[2]?ChunkColor[2]:ThemeColor[5];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 5:	
							colBG=BackgroundBarColor[3]?BackgroundBarColor[2]:RGBA(255,255,255,255);
							colBorder=BorderColor[3]?BorderColor[3]:ThemeColor[7];
							colCk=ChunkColor[3]?ChunkColor[3]:ThemeColor[7];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 9:	
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
						case 16:
						default:
							DD<<"[Error] LargeLayerWithScrollBar "<<ID<<":Such state "<<stat<<" is not considered yet!"<<endl;
					}
					Win->RenderFillRect(lmt&ckBGps_Ri,colBG);
					Win->RenderDrawRectWithLimit(ckBGps_Ri,colBorder,lmt);
					Win->RenderFillRect(lmt&ckPs_Ri,colCk);
					if (EnableBarSideButton)
						;
					
					Win->Debug_DisplayBorder(ckBGps_Ri);
				}
				
				if (EnableButtomBar)
				{
					RGBA colBG,colBorder,colCk,colButton;
					switch (stat)
					{
						case 0:	case 1:	case 3:	case 5:
							LargeLayerWithScrollBar_Show_case0:
							colBG=BackgroundBarColor[0]?BackgroundBarColor[0]:RGBA(255,255,255,63);
							colBorder=BorderColor[0]?BorderColor[0]:ThemeColor[1];
							colCk=ChunkColor[0]?ChunkColor[0]:ThemeColor[1];
							colButton=SideButtonColor[0]?SideButtonColor[0]:ThemeColor[0];
							break;
						case 2:
							LargeLayerWithScrollBar_Show_case2:
							colBG=BackgroundBarColor[1]?BackgroundBarColor[1]:RGBA(255,255,255,127);
							colBorder=BorderColor[1]?BorderColor[1]:ThemeColor[3];
							colCk=ChunkColor[1]?ChunkColor[1]:ThemeColor[3];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 4:	
							colBG=BackgroundBarColor[2]?BackgroundBarColor[2]:RGBA(255,255,255,191);
							colBorder=BorderColor[2]?BorderColor[2]:ThemeColor[5];
							colCk=ChunkColor[2]?ChunkColor[2]:ThemeColor[5];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 6:	
							colBG=BackgroundBarColor[3]?BackgroundBarColor[2]:RGBA(255,255,255,255);
							colBorder=BorderColor[3]?BorderColor[3]:ThemeColor[7];
							colCk=ChunkColor[3]?ChunkColor[3]:ThemeColor[7];
							colButton=SideButtonColor[1]?SideButtonColor[1]:ThemeColor[2];
							break;
						case 7:
							if (EnableRightBar) goto LargeLayerWithScrollBar_Show_case0;
							else goto LargeLayerWithScrollBar_Show_case2;
						case 9:	
						case 10:
						case 11:
						case 12:
						case 13:
						case 14:
						case 15:
						case 16:
						default:
							DD<<"[Error] LargeLayerWithScrollBar "<<ID<<":Such state "<<stat<<" is not considered yet!"<<endl;
					}
					Win->RenderFillRect(lmt&ckBGps_Bu,colBG);
					Win->RenderDrawRectWithLimit(ckBGps_Bu,colBorder,lmt);
					Win->RenderFillRect(lmt&ckPs_Bu,colCk);
					if (EnableBarSideButton)
						;
					
					Win->Debug_DisplayBorder(ckBGps_Bu);
				}
				
				Win->Debug_DisplayBorder(gPS);
				Win->Debug_DisplayBorder(largeAreaPS+gPS);
			}
			
		public:
			inline LargeLayerWithScrollBar* SetLargeAreaColor(const RGBA &co)
			{
				LargeAreaLayer->SetLayerColor(co);
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetBackgroundBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundBarColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
				}
				else DD<<"[Error] SetBackgroundBarColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BorderColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
				}
				else DD<<"[Error] SetBorderColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetChunkColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					ChunkColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
				}
				else DD<<"[Error] ChunkColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline LargeLayerWithScrollBar* SetSideButtonColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					SideButtonColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
				}
				else DD<<"[Error] SideButtonColor: p "<<p<<" is not in Range[0,3]"<<endl;
				return this;
			}
			
			inline void SetEnableBarGap(bool enable)
			{
				EnableBarGap=enable;
				Win->NeedUpdatePosize;
				Win->NeedFreshScreen;
				Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
			}
			
			inline void SetEnableBarSideButton(bool enable)
			{
				EnableBarSideButton=enable;
				Win->NeedUpdatePosize;
				Win->NeedFreshScreen;
				Win->PresentLimit=Win->PresentLimit|ckBGps_Bu|ckBGps_Ri;
			}
			
			inline void SetCustomWheelSensibility(int s)
			{CustomWheelSensibility=s;}
			
			inline bool GetButtomBarEnableState()
			{return EnableButtomBar;}
			
			inline bool GetRightBarEnableState()
			{return EnableRightBar;}
			
			inline int GetScrollBarWidth()
			{return ScrollBarWidth;}
			
//			inline void SetEnableSmoothWheelScroll(bool enable)
//			{EnableSmoothWheelScroll=enable;}
			
			inline Layer* LargeArea()
			{return LargeAreaLayer;}
			
			LargeLayerWithScrollBar(int _ID,Widgets *_fa,const Posize &_rps,const Posize &_largeareaPS)
			{
				SetID(_ID);
				DD<<"[Info] Create LargeLayerWithScrollBar "<<ID<<endl;
				Type=WidgetType_LargeLayerWithScrollBar;
				SetFa(_fa);
				SetrPS(_rps);
				largeAreaPS=_largeareaPS;
				ResizeLL(0,0);
				LargeAreaLayer=new Layer(0,this,new PosizeEX_LargeLayer(this));
				PresentThisLater=1;
				SolvePosEventFirst=1;
			}
			
			LargeLayerWithScrollBar(int _ID,Widgets *_fa,PosizeEX *psex,const Posize &_largeareaPS)
			{
				SetID(_ID);
				DD<<"[Info] Create LargeLayerWithScrollBar "<<ID<<endl;
				Type=WidgetType_LargeLayerWithScrollBar;
				SetFa(_fa);
				AddPsEx(psex);
				largeAreaPS=_largeareaPS;
				ResizeLL(0,0);
				LargeAreaLayer=new Layer(0,this,new PosizeEX_LargeLayer(this));
				PresentThisLater=1;
				SolvePosEventFirst=1;
				MultiWidgets=1;
			}
	};
	
	class TwinLayerWithDivideLine:public Widgets
	{
		protected:
			bool VerticalDivide=1;
			Layer *LayerAreaA=NULL,//if Vertical ,AreaA is up ,else AreaA is Left
				  *LayerAreaB=NULL;
			int DivideLineShowWidth=2,
				DivideLineEventWidth=6,
				ResizeMode=0;//0:keep percent not changing 1:keep A not changing 2:keep B (It will satisfy LimitA and LimitB first) 
			double DivideLinePos=0.5,//Similar with Slider
				   LimitA=-0.1,LimitB=-0.1;//positive or 0:pixels negative:percent
			Posize DLEventPs,DLShowPs;
			int stat=0;//same with Slider
			RGBA DivideLineColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE};
			
			void GetAreaPosize(Posize &ps,bool IsB)
			{
				if (IsB)
					if (VerticalDivide)	ps={rPS.w*DivideLinePos+DivideLineShowWidth/2,0,rPS.w*(1-DivideLinePos)-DivideLineShowWidth/2,rPS.h};
					else ps={0,rPS.h*DivideLinePos+DivideLineShowWidth/2,rPS.w,rPS.h*(1-DivideLinePos)-DivideLineShowWidth/2};
				else
					if (VerticalDivide) ps={0,0,rPS.w*DivideLinePos-DivideLineShowWidth/2,rPS.h};
					else ps={0,0,rPS.w,rPS.h*DivideLinePos-DivideLineShowWidth/2};
			}
			
			class PosizeEX_TwinLayer:public PosizeEX
			{
				protected: 
					TwinLayerWithDivideLine *tar;
					bool IsB;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						tar->GetAreaPosize(ps,IsB);
					}
					
					PosizeEX_TwinLayer(TwinLayerWithDivideLine *_tar,bool isB)
					:tar(_tar),IsB(isB) {}
			};
			
			void SetDivideLinePos(double per)
			{
				Win->PresentLimit|=DLShowPs;
				DivideLinePos=EnsureInRange(per,LimitA>=0?LimitA/(VerticalDivide?rPS.w:rPS.h):-LimitA,LimitB>=0?1-LimitB/(VerticalDivide?rPS.w:rPS.h):1+LimitB);
				if (VerticalDivide)
					DLEventPs={gPS.x+gPS.w*DivideLinePos-DivideLineEventWidth/2,gPS.y,DivideLineEventWidth,gPS.h},
					DLShowPs={gPS.x+gPS.w*DivideLinePos-DivideLineShowWidth/2,gPS.y,DivideLineShowWidth,gPS.h};
				else
					DLEventPs={gPS.x,gPS.y+gPS.h*DivideLinePos-DivideLineEventWidth/2,gPS.w,DivideLineEventWidth},
					DLShowPs={gPS.x,gPS.y+gPS.h*DivideLinePos-DivideLineShowWidth/2,gPS.w,DivideLineShowWidth};
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=DLShowPs;
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!(DLEventPs&CoverLmt).In(Win->NowPos))
							{
								DD<<"[Info] TwinLayerWithDivideLine "<<ID<<" losefocus"<<endl;
								stat=0;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=DLShowPs;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
							if (DLEventPs.In(Win->NowPos))
							{
								DD<<"[Info] TwinLayerWithDivideLine "<<ID<<" click"<<endl;
								stat=2;
								Win->OccupyPosWg=this; 
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=DLShowPs;
							}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							DD<<"[Info] TwinLayerWithDivideLine "<<ID<<" loose"<<endl;
							stat=1;
							Win->OccupyPosWg=NULL;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=DLShowPs;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==2)
						{
//							DD<<"[Info] TwinLayerWithDivideLine "<<ID<<" shift"<<endl;
							SetDivideLinePos(VerticalDivide?(Win->NowPos.x-gPS.x)*1.0/gPS.w:(Win->NowPos.y-gPS.y)*1.0/gPS.h);
							Win->NeedSolvePosEvent=0; 
						}
						else if (stat==0)
						{
							if ((DLEventPs&CoverLmt).In(Win->NowPos)) 
							{
								DD<<"[Info] TwinLayerWithDivideLine "<<ID<<" focus"<<endl;
								stat=1;
								SetNeedLoseFocus();
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=DLShowPs;
							}
						}
						else
							if ((DLEventPs&CoverLmt).In(Win->NowPos))
								Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(DLShowPs&lmt,DivideLineColor[stat]?DivideLineColor[stat]:ThemeColor[stat*2+1]);
				
				Win->Debug_DisplayBorder(gPS); 
				Win->Debug_DisplayBorder(DLEventPs);
				Win->Debug_DisplayBorder(DLShowPs);
			}
			
			virtual void CalcPsEx()
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
				
				if (!(lastPs==gPS))
					switch (ResizeMode)
					{
						case 0:
							SetDivideLinePos(DivideLinePos);
							break;
						case 1:
							if (VerticalDivide) SetDivideLinePos((LayerAreaA->GetrPS().w+DivideLineShowWidth/2.0)/rPS.w);
							else SetDivideLinePos((LayerAreaA->GetrPS().h+DivideLineShowWidth/2.0)/rPS.h);
							break;
						case 2:
							if (VerticalDivide) SetDivideLinePos(1-(LayerAreaB->GetrPS().w+DivideLineShowWidth/2.0)/rPS.w);
							else SetDivideLinePos(1-(LayerAreaB->GetrPS().h+DivideLineShowWidth/2.0)/rPS.h);
							break;
						default:
							DD<<"[Error] TwinLayerWithDivideLin: ResizeMode "<<ResizeMode<<" is not in Range[0,2]"<<endl;
					}
			}
			
		public:
			inline void SetDivideLineColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					DivideLineColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=DLShowPs;
				}
				else DD<<"[Error] TwinLayerWithDivideLine: SetDivideLineColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetAreaAColor(const RGBA &co)
			{LayerAreaA->SetLayerColor(co);}
			
			inline void SetAreaBColor(const RGBA &co)
			{LayerAreaB->SetLayerColor(co);}
			
			inline void SetDivideLineWidth(int showWidth,int eventWidth)
			{
				DivideLineShowWidth=showWidth;
				DivideLineEventWidth=eventWidth;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
			inline void SetDivideLineMode(int mode,double _limitA,double _limitB)
			{
				ResizeMode=mode;
				LimitA=_limitA;
				LimitB=_limitB;
				SetDivideLinePos(DivideLinePos);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
			void SetDivideLinePosition(double per)
			{SetDivideLinePos(per);}
			
			inline Layer* AreaA()
			{return LayerAreaA;}
			
			inline Layer* AreaB()
			{return LayerAreaB;}
			
			TwinLayerWithDivideLine(int _ID,Widgets *_fa,const Posize &_rps,bool _verticalDivide,double initPercent=0.5)
			:VerticalDivide(_verticalDivide)
			{
				SetID(_ID);
				DD<<"[Info] Create TwinLayerWithDivideLine "<<ID<<endl;
				Type=WidgetType_TwinLayerWithDivideLine;
				SetFa(_fa);
				SetrPS(_rps);
				LayerAreaA=new Layer(0,this,new PosizeEX_TwinLayer(this,0));
				LayerAreaB=new Layer(0,this,new PosizeEX_TwinLayer(this,1));
				SetDivideLinePos(initPercent);
				PresentThisLater=1;
				SolvePosEventFirst=1;
				MultiWidgets=1;
			}
			
			TwinLayerWithDivideLine(int _ID,Widgets *_fa,PosizeEX *psex,bool _verticalDivide,double initPercent=0.5)
			:VerticalDivide(_verticalDivide)
			{
				SetID(_ID);
				DD<<"[Info] Create TwinLayerWithDivideLine "<<ID<<endl;
				Type=WidgetType_TwinLayerWithDivideLine;
				SetFa(_fa);
				AddPsEx(psex);
				LayerAreaA=new Layer(0,this,new PosizeEX_TwinLayer(this,0));
				LayerAreaB=new Layer(0,this,new PosizeEX_TwinLayer(this,1));
				SetDivideLinePos(initPercent);
				PresentThisLater=1;
				SolvePosEventFirst=1;
				MultiWidgets=1;
			}
	};
	
	class TabLayer:public Widgets
	{
		protected:
			struct TabLayerData
			{
				Layer *lay=NULL;
				string title;
				RGBA TabColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},//none,focus/current,click
					 TextColor=RGBA_NONE;
				int TabWidth=50;
				SharedTexturePtr pic;
				void *funcData=NULL;
				
				TabLayerData() {}
				
				TabLayerData(Layer *_lay,const string &_title,const SharedTexturePtr &_pic,void *_funcdata)
				:lay(_lay),title(_title),pic(_pic),funcData(_funcdata) {}
			};
			
			vector <TabLayerData> EachTabLayer;
			int TabCnt=0;
			int stat=0,//0:none 1:focus 2:leftclick 3:rightclick
				TabHeight=24,
				LeftMostShowLayer=0,
				FocusingPos=-1,
				CurrentShowLayerPos=-1;//-1:means no layer
			Range TabLayerWidthRange={100,400};
			bool ShowCloseX=0,
				 EnableDrag=0,
				 EnableTabScrollAnimation=0,
				 EnableSwitchGradientAnimation=0;
			RGBA TabBarBackgroundColor=RGBA_NONE;
			void (*func)(void*,int,int)=NULL;//int1:which int2:stat (1:switch to 2:rightClick)
			
			class PosizeEX_TabLayer:public PosizeEX
			{
				protected: 
					TabLayer *tar=NULL;
					
				public:
					virtual void GetrPS(Posize &ps)
					{
						if (nxt!=NULL) nxt->GetrPS(ps);
						ps=Posize(0,tar->TabHeight,tar->rPS.w,tar->rPS.h-tar->TabHeight);
					}
					
					PosizeEX_TabLayer(TabLayer *_tar)
					:tar(_tar) {}
			};
			
			int GetTabFromPos(const Point &pt) 
			{
				if (TabCnt==0) return -1;
				if ((CoverLmt&Posize(gPS.x,gPS.y,gPS.w,TabHeight)).In(pt))
				{
					for (int i=LeftMostShowLayer,w=0;i<TabCnt;w+=EachTabLayer[i++].TabWidth)
						if (pt.x-gPS.x<w+EachTabLayer[i].TabWidth)
							return i;
					return -1;
				}
				else return -1;
			}

			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_MOUSEWHEEL:
						if (Posize(gPS.x,gPS.y,gPS.w,TabHeight).In(Win->NowPos))
						{
							if (event.wheel.y>0)
							{
								if (LeftMostShowLayer>0)
								{
									LeftMostShowLayer=max(0,LeftMostShowLayer-event.wheel.y);
									Win->NeedFreshScreen=1;
									Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
								}
							}
							else
							{
								int w=0;
								for (int i=LeftMostShowLayer;i<TabCnt;++i)
									w+=EachTabLayer[i].TabWidth;
								if (w>gPS.w)
								{
									for (int i=-event.wheel.y;i>=1&&LeftMostShowLayer<TabCnt&&w>gPS.w;--i)
										w-=EachTabLayer[LeftMostShowLayer++].TabWidth;
									Win->NeedFreshScreen=1;
									Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
								}
							}
							Win->NeedSolveEvent=0;
						}
						break;
						
					case SDL_USEREVENT:
						if (event.user.type==PUI_EVENT_UpdateTimer)
							if (event.user.data1==this)
							{
								
							}
						break;
				}
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								FocusingPos=-1;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						FocusingPos=GetTabFromPos(Win->NowPos);
						if (FocusingPos!=-1)
						{
							stat=2;
							if (CurrentShowLayerPos!=FocusingPos)
							{
								DD<<"[Info] TabLayer "<<ID<<" Switch layer to "<<FocusingPos<<endl;
								if (CurrentShowLayerPos!=-1)
									EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(0);
								CurrentShowLayerPos=FocusingPos;
								EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(1);
								if (func!=NULL)
									func(EachTabLayer[CurrentShowLayerPos].funcData,CurrentShowLayerPos,1);
								Win->NeedUpdatePosize=1;
							}
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						if (event.button.button==SDL_BUTTON_RIGHT)
							stat=3;
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							if (stat==3)
								if (func!=NULL)
									func(EachTabLayer[FocusingPos].funcData,FocusingPos,2); 
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							FocusingPos=GetTabFromPos(Win->NowPos);
							if (FocusingPos!=-1)
							{
								stat=1;
								SetNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->NeedSolvePosEvent=0;
								Win->PresentLimit|=gPS;
							}
						}
						else
						{
							int p=GetTabFromPos(Win->NowPos);
							if (p==FocusingPos)
								Win->NeedSolvePosEvent=0;
							else if (p==-1)
							{
								stat=0;
								FocusingPos=-1;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->NeedSolvePosEvent=0;
								Win->PresentLimit|=gPS;
							}
							else
							{
								FocusingPos=p;
								Win->NeedFreshScreen=1;
								Win->NeedSolvePosEvent=0;
								Win->PresentLimit|=gPS;
							}
						}
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(Posize(gPS.x,gPS.y,gPS.w-1,TabHeight)&lmt,TabBarBackgroundColor?TabBarBackgroundColor:ThemeColor.BackgroundColor[1]);
				if (TabCnt>0)
				{
					Posize TabPs(gPS.x,gPS.y,EachTabLayer[LeftMostShowLayer].TabWidth,TabHeight);
					for (int i=LeftMostShowLayer;i<TabCnt&&TabPs.x<gPS.x2();++i)
					{
						int colorPos=FocusingPos==i?(stat==2||stat==3?2:1):CurrentShowLayerPos==i;
						Win->RenderFillRect(TabPs&lmt,EachTabLayer[i].TabColor[colorPos]?EachTabLayer[i].TabColor[colorPos]:ThemeColor[colorPos*2+1]);
						if (EachTabLayer[i].pic()!=NULL)
							Win->RenderCopyWithLmt(EachTabLayer[i].pic(),Posize(TabPs.x,TabPs.y,TabHeight,TabHeight),lmt);
						Win->RenderDrawText(EachTabLayer[i].title,TabPs,lmt,0,EachTabLayer[i].TextColor?EachTabLayer[i].TextColor:ThemeColor.MainTextColor[0]);
						
						Win->Debug_DisplayBorder(TabPs);
						
						TabPs.x+=TabPs.w+1;
						TabPs.w=EachTabLayer[i].TabWidth;
					}
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			void SwitchLayer(int p)
			{
				if (TabCnt==0) return;
				p=EnsureInRange(p,0,TabCnt-1);
				if (CurrentShowLayerPos!=p)
				{
					DD<<"[Info] TabLayer "<<ID<<" Switch layer to "<<p<<endl;
					if (CurrentShowLayerPos!=-1)
						EachTabLayer[CurrentShowLayerPos].lay->SetEnabled(0);
					CurrentShowLayerPos=p;
					EachTabLayer[p].lay->SetEnabled(1);
					if (func!=NULL)
						func(EachTabLayer[p].funcData,p,1);
					Win->NeedUpdatePosize=1;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
			}
			
			Layer* PushbackLayer(const string &title,void *_funcdata=NULL,const SharedTexturePtr &pic=SharedTexturePtr(NULL))
			{
				EachTabLayer.push_back(TabLayerData(new Layer(0,this,new PosizeEX_TabLayer(this)),title,pic,_funcdata));
				int w=0;
				TTF_SizeUTF8(PUI_Font(),title.c_str(),&w,NULL);
				EachTabLayer[TabCnt].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				++TabCnt;
				SwitchLayer(TabCnt-1);
			}
			
			Layer* AddLayer(int pos,const string &title,void *_funcdata=NULL,const SharedTexturePtr &pic=SharedTexturePtr(NULL))
			{
				pos=EnsureInRange(pos,0,TabCnt);
				EachTabLayer.insert(EachTabLayer.begin()+pos,TabLayerData(new Layer(0,this,new PosizeEX_TabLayer(this)),title,pic,_funcdata));
				if (pos<=CurrentShowLayerPos)
					++CurrentShowLayerPos;
				int w=0;
				TTF_SizeUTF8(PUI_Font(),title.c_str(),&w,NULL);
				EachTabLayer[pos].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				++TabCnt;
				SwitchLayer(pos);
			}
			
			void DeleteLayer(int pos)
			{
				if (TabCnt==0) return;
				pos=EnsureInRange(pos,0,TabCnt-1);
				if (pos==CurrentShowLayerPos)
					if (pos==0)
						if (TabCnt>=2)
						{
							SwitchLayer(1);
							CurrentShowLayerPos=0;
						}	
						else CurrentShowLayerPos=-1;
					else SwitchLayer(pos-1);
				else
					if (pos<CurrentShowLayerPos)
						--CurrentShowLayerPos;
				delete EachTabLayer[pos].lay;
				EachTabLayer.erase(EachTabLayer.begin()+pos);
				--TabCnt;
			}
			
			inline void SetTabBarBackgroundColor(const RGBA &co)
			{
				TabBarBackgroundColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
			}
			
			inline void SetTabColor(int pos,int p,const RGBA &co)
			{
				pos=EnsureInRange(pos,0,TabCnt-1);
				if (InRange(p,0,2))
				{
					EachTabLayer[pos].TabColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
				}
				else DD<<"[Error] TabLayer: SetTabColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetTabTextColor(int pos,const RGBA &co)
			{
				EachTabLayer[EnsureInRange(pos,0,TabCnt-1)].TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
			}
			
			void SetTabTitle(int pos,const string &str)
			{
				pos=EnsureInRange(pos,0,TabCnt-1);
				EachTabLayer[pos].title=str;
				int w=0;
				TTF_SizeUTF8(PUI_Font(),str.c_str(),&w,NULL);
				EachTabLayer[pos].TabWidth=TabLayerWidthRange.EnsureInRange(w+10);
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
			}
			
			void SetTabPic(int pos,const SharedTexturePtr &pic)
			{
				EachTabLayer[EnsureInRange(pos,0,TabCnt-1)].pic=pic;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
			}
			
			void SetTabHeight(int h)
			{
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,max(h,TabHeight));
				TabHeight=h;
				Win->NeedFreshScreen=1;
				Win->NeedUpdatePosize=1;
			}
			
			void SetTabLayerWidthRange(const Range &ran)
			{
				TabLayerWidthRange=ran;
				for (int i=0,w;i<TabCnt;++i)
				{
					TTF_SizeUTF8(PUI_Font(),EachTabLayer[i].title.c_str(),&w,NULL);
					EachTabLayer[i].TabWidth=TabLayerWidthRange.EnsureInRange(EachTabLayer[i].TabWidth);
				}
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TabHeight);
			}
			
			inline void SetFunc(void (*_func)(void*,int,int))
			{func=_func;}
			
			inline void SetTabFuncdata(int p,void *_funcdata)
			{EachTabLayer[p].funcData=_funcdata;}
			
			inline int GetTabcnt()
			{return TabCnt;}
			
			inline int GetCurrenShowLayerPos()
			{return CurrentShowLayerPos;}
			
			Layer* operator [] (int p)
			{return EachTabLayer[EnsureInRange(p,0,TabCnt-1)].lay;}
			
			Layer* Tab(int p)
			{return EachTabLayer[EnsureInRange(p,0,TabCnt-1)].lay;}
			
			TabLayer(int _ID,Widgets *_fa,PosizeEX *psex)
			{
				SetID(_ID);
				DD<<"[Info] Create TabLayer "<<ID<<endl;
				Type=WidgetType_TabLayer;
				SetFa(_fa);
				AddPsEx(psex);
			}
			
			TabLayer(int _ID,Widgets *_fa,const Posize &_rps)
			{
				SetID(_ID);
				DD<<"[Info] Create TabLayer "<<ID<<endl;
				Type=WidgetType_TabLayer;
				SetFa(_fa);
				SetrPS(_rps);
			}
	};
	
//	class WallpaperLayer_Dot:public Widgets//Just for fun.//Slow.... 
//	{
//		protected:
//			RGBA DotColor=RGBA_NONE;
//			Point DotDistance={10,10};
//			
//			virtual void Show(Posize &lmt)
//			{
//				if (lmt.Size()==0) return;
//				RGBA &co=!DotColor?ThemeColor[1]:DotColor;
//				Point pt(((lmt.x-1)/DotDistance.x+1)*DotDistance.x,((lmt.y-1)/DotDistance.y+1)*DotDistance.y);//??
//				while (1)
//					if (lmt.In(pt))
//					{
//						RenderDrawPointWithLmt(pt,co,lmt);
//						RenderDrawPointWithLmt(pt+Point(1,0),co,lmt);
//						RenderDrawPointWithLmt(pt+Point(-1,0),co,lmt);
//						RenderDrawPointWithLmt(pt+Point(0,1),co,lmt);
//						RenderDrawPointWithLmt(pt+Point(0,-1),co,lmt);
//						pt.x+=DotDistance.x;
//					}
//					else
//					{
//						pt.y+=DotDistance.y;
//						pt.x=((lmt.x-1)/DotDistance.x+1)*DotDistance.x;
//						if (!lmt.In(pt))
//							break;
//					}
//								
//				if (DEBUG_DisplayBorderFlag)
//					Debug_DisplayBorder(gPS);
//			}
//			
//		public:
//			void SetDotColor(const RGBA &co)
//			{
//				DotColor=co;
//				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=gPS;
//			}
//			
//			void SetDotDistance(const Point &pt)
//			{
//				DotDistance=pt;
//				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=gPS;
//			}
//
//			WallpaperLayer_Dot(int _ID,Layer *_fa,Posize _rPS)
//			{
//				SetID(_ID);
//				DD<<"[Info] Create WallpaperLayer_Dot "<<ID<<endl;
//				Type=WidgetType_WallpaperLayer_Dot;
//				SetFa(_fa);
//				SetrPS(_rPS);
//			}
//			
//			WallpaperLayer_Dot(int _ID,Widgets *_fa,PosizeEX *psex)
//			{
//				SetID(_ID);
//				DD<<"[Info] Create WallpaperLayer_Dot "<<ID<<endl;
//				Type=WidgetType_WallpaperLayer_Dot;
//				SetFa(_fa);
//				AddPsEx(psex);
//			}
//	};
	
	class BlurEffectLayer:public Widgets
	{
		protected:
			
			
		public:
			
			
	}; 
	
	class Slider:public Widgets
	{
		protected:
			bool Vertical;
			Posize ckPs={0,0,8,8};
			int barWidth=3;
			double Percent=0;
			int stat=0;//0:Up_NoFocus 1:Up_Focus_Chunk 2:Down_Slide_Chunk
			void (*func)(void*,double,bool)=NULL;//double: percent bool:IsLooseMouse(set 0 when continuous changing)
			void *funcData=NULL;
			RGBA BarColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 ChunkColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE};
			
			void SetChunkPs(const Point &pt,bool bo)
			{
				if (Vertical)
					ckPs.y=EnsureInRange(pt.y,gPS.y,gPS.y2()-ckPs.h),
					Percent=(ckPs.y-gPS.y)*1.0/(gPS.h-ckPs.h);
				else 
					ckPs.x=EnsureInRange(pt.x,gPS.x,gPS.x2()-ckPs.w),
					Percent=(ckPs.x-gPS.x)*1.0/(gPS.w-ckPs.w);
				if (func!=NULL)
					func(funcData,Percent,bo);
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								DD<<"[Info] Slider "<<ID<<" losefocus"<<endl;
								stat=0;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							DD<<"[Info] Slider "<<ID<<" click"<<endl;
							stat=2;
							SetChunkPs(Win->NowPos,0);
							Win->OccupyPosWg=this;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							DD<<"[Info] Slider "<<ID<<" loose"<<endl;
							stat=1;
							SetChunkPs(Win->NowPos,1);
							Win->OccupyPosWg=NULL;
//							RemoveNeedLoseFocus();
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat==2)
						{
							DD<<"[Info] Slider "<<ID<<" slider"<<endl;
							Win->PresentLimit|=ckPs;
							SetChunkPs(Win->NowPos,0);
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=ckPs;
							Win->NeedSolvePosEvent=0; 
						}
						else if (stat==0)
						{
							DD<<"[Info] Slider "<<ID<<" focus"<<endl;
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (Vertical)
					Win->RenderFillRect(Posize(gPS.x+(gPS.w-barWidth>>1),gPS.y,barWidth,gPS.h)&lmt,BarColor[stat]?BarColor[stat]:ThemeColor[stat*2]);
				else Win->RenderFillRect(Posize(gPS.x,gPS.y+(gPS.h-barWidth>>1),gPS.w,barWidth)&lmt,BarColor[stat]?BarColor[stat]:ThemeColor[stat*2]);
				Win->RenderFillRect(ckPs&lmt,ChunkColor[stat]?ChunkColor[stat]:ThemeColor[stat*2+1]); 
				
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
				
				if (Vertical) 
					ckPs.x=gPS.x,ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent,ckPs.w=gPS.w;
				else ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent,ckPs.y=gPS.y,ckPs.h=gPS.h;
			}
			
		public:
			inline double operator () ()//??
			{return Percent;}
			
			inline void SetBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BarColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] Slider: SetBarColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetChunkColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					ChunkColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] Slider: SetChunkColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			void SetPercent(double per,bool triggerFunc=1)
			{
				Percent=EnsureInRange(per,0,1);
				Win->PresentLimit|=ckPs;
				if (Vertical) ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent;
				else ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent;
				if (triggerFunc)
					if (func!=NULL)
						func(funcData,Percent,1);
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=ckPs;
			}
			
			void SetPercent_Delta(double per,bool triggerFunc=1)
			{SetPercent(Percent+per,triggerFunc);}
			
			inline void SetFunc(void (*_func)(void*,double,bool),void *_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}
			
			inline void SetBarWidth(int w)
			{
				barWidth=max(0,w);
				Win->NeedFreshScreen;
				Win->PresentLimit|=gPS;			
			}
			
			void SetChunkWidth(int w)
			{
				Win->PresentLimit|=ckPs;
				if (Vertical)
					ckPs.h=w,ckPs.y=gPS.y+(gPS.h-ckPs.h)*Percent;
				else ckPs.w=w,ckPs.x=gPS.x+(gPS.w-ckPs.w)*Percent;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=ckPs;
			}
			
			Slider(int _ID,Widgets *_fa,const Posize &_rps,bool _vertical,void (*_func)(void*,double,bool)=NULL,void *_funcdata=NULL)
			:Vertical(_vertical),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create Slider "<<ID<<endl;
				Type=WidgetType_Slider;
				SetFa(_fa);
				SetrPS(_rps);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
			
			Slider(int _ID,Widgets *_fa,PosizeEX *psex,bool _vertical,void (*_func)(void*,double,bool)=NULL,void *_funcdata=NULL)
			:Vertical(_vertical),func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create Slider "<<ID<<endl;
				Type=WidgetType_Slider;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
	};
	
	class ProgressBar:public Widgets
	{
		protected:
			double Percent=0;
			bool Vertical=0;
//			double (*UpdateFunc)(void *)=NULL;//return -1 means not change;
//			void *funcData=NULL;
			RGBA BackgroundColor=RGBA_NONE,
				 BarColor=RGBA_NONE,
				 FullColor=RGBA_NONE;
			
			virtual void Show(Posize &lmt)
			{
//				if (UpdateFunc!=NULL)
//				{
//					double per=UpdateFunc(funcData);
//					if (per!=-1)
//						Percent=EnsureInRange(per,0,1);
//				}

				Win->RenderFillRect(gPS&lmt,BackgroundColor?BackgroundColor:ThemeColor[0]);
				Win->RenderFillRect((Vertical?Posize(gPS.x,gPS.y+gPS.h*(1-Percent),gPS.w,gPS.h*Percent):Posize(gPS.x,gPS.y,gPS.w*Percent,gPS.h)).Shrink(0)&lmt,
									Percent==1?(FullColor?FullColor:ThemeColor.SecondColor[4]):(BarColor?BarColor:ThemeColor[4]));
				
				Win->Debug_DisplayBorder(gPS);
			}

		public:
			inline void SetBackgroundColor(const RGBA &co)
			{
				BackgroundColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetBarColor(const RGBA &co)
			{
				BarColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetFullColor(const RGBA &co)
			{
				FullColor=co;
				if (Percent==1)
				{
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
			}
			
			inline void SetPercent(double per)
			{
				per=EnsureInRange(per,0,1);
				if (per==Percent) return;
				if (per==1||Percent==1)
					Win->PresentLimit|=gPS;
				if (Vertical)
					Win->PresentLimit|=Posize(gPS.x,gPS.y+gPS.h*(1-min(Percent,per))-1,gPS.w,gPS.h*fabs(per-Percent)+2);
				else Win->PresentLimit|=Posize(gPS.x+gPS.w*min(Percent,per)-1,gPS.y,gPS.w*fabs(per-Percent)+2,gPS.h);
				Percent=per;  
				Win->NeedFreshScreen=1;
			}
			
//			inline void SetUpdateFunc(double (*func)(void*),void *_funcdata)
//			{
//				UpdateFunc=func;
//				funcData=_funcdata;
//			}
			
			ProgressBar(int _ID,Widgets *_fa,const Posize &_rps,bool _vertical=0/*,double (*func)(void*)=NULL,void *_funcdata=NULL*/)
			:Vertical(_vertical)//,UpdateFunc(func)
			{
				SetID(_ID);
				DD<<"[Info] CreateProgessBar "<<ID<<endl;
				Type=WidgetType_ProgressBar;
				SetFa(_fa);
				SetrPS(_rps);
//				if (_funcdata==CONST_THIS)
//					funcData=this;
//				else funcData=_funcdata;
//				if (UpdateFunc!=NULL)
//				{
//					double per=UpdateFunc(funcData);
//					if (per!=-1)
//						Percent=EnsureInRange(per,0,1);
//				}
			}
			
			ProgressBar(int _ID,Widgets *_fa,PosizeEX *psex,bool _vertical=0/*,double (*func)(void*)=NULL,void *_funcdata=NULL*/)
			:Vertical(_vertical)//,UpdateFunc(func)
			{
				SetID(_ID);
				DD<<"[Info] CreateProgessBar "<<ID<<endl;
				Type=WidgetType_ProgressBar;
				SetFa(_fa);
				AddPsEx(psex);
//				if (_funcdata==CONST_THIS)
//					funcData=this;
//				else funcData=_funcdata;
//				if (UpdateFunc!=NULL)
//				{
//					double per=UpdateFunc(funcData);
//					if (per!=-1)
//						Percent=EnsureInRange(per,0,1);
//				}
			}
	};
	
	class PictureBox:public Widgets
	{
		protected:
			SDL_Texture *src=NULL;
			int stat=0;//0:NotFocus 1:Focus 2:Down_Left 3:Down_Right
			Posize srcPS;
			int Mode=0;//0:Fullfill rPS 1:show part that can see in original size,pin centre(MM)
					   //2:pin LU 3:pin RU 4:pin LD 5:pin RD
					   //6:pin MU 7:pin MD 8:pin ML 9:pin MR (Mid,Left,Right,Up,Down)
			bool AutoDeletePic=0;
			void (*func)(void*,bool)=NULL;//bool:1:Left_Click 2:Right_Click 
			void *funcData=NULL;		
			RGBA BackGroundColor=RGBA_BLACK;

			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								RemoveNeedLoseFocus();
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						DD<<"[Info] PictureBox "<<ID<<" click"<<endl;
						if (event.button.button==SDL_BUTTON_LEFT)
							stat=2;
						else if (event.button.button==SDL_BUTTON_RIGHT)
							stat=3;
						Win->NeedSolvePosEvent=0;						
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2||stat==3)
						{
							if (func!=NULL)
							{
								DD<<"[Info] PictureBox "<<ID<<" function "<<(stat==2?"left":"right")<<endl;
								if (stat==2)
									func(funcData,0);
								else if (stat==3)
									func(funcData,1);
								Win->NeedSolvePosEvent=0;
							}
							stat=1;
						}
						break;
					
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}

			virtual void Show(Posize &lmt)
			{
				Win->RenderFillRect(gPS&lmt,BackGroundColor);
				if (src!=NULL)
					switch (Mode)
					{
						case 0:	Win->RenderCopyWithLmt(src,gPS,lmt);														break;
						case 1:	Win->RenderCopyWithLmt(src,Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y+(gPS.h-srcPS.h>>1)),lmt);	break;
						case 2: Win->RenderCopyWithLmt(src,gPS.GetLU(),lmt);												break;
						case 3: Win->RenderCopyWithLmt(src,Point(gPS.x2()-srcPS.w,gPS.y),lmt);								break;
						case 4: Win->RenderCopyWithLmt(src,Point(gPS.x,gPS.y2()-srcPS.h),lmt);								break;
						case 5: Win->RenderCopyWithLmt(src,Point(gPS.x2()-srcPS.w,gPS.y2()-srcPS.h),lmt);					break;
						case 6: Win->RenderCopyWithLmt(src,Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y),lmt);						break;
						case 7: Win->RenderCopyWithLmt(src,Point(gPS.x+(gPS.w-srcPS.w>>1),gPS.y2()-srcPS.h),lmt);			break;
						case 8: Win->RenderCopyWithLmt(src,Point(gPS.x,gPS.y+(gPS.h-srcPS.h>>1)),lmt);						break;
						case 9: Win->RenderCopyWithLmt(src,Point(gPS.x2()-srcPS.w,gPS.y+(gPS.h-srcPS.h>>1)),lmt);			break;
						default:
							DD<<"[Error] PictureBox: Mode "<<Mode<<" is not in Range[0,9]"<<endl;
							Win->RenderCopyWithLmt(src,gPS,lmt);
					}
					
				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetBackGroundColor(const RGBA &co)
			{
				BackGroundColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetPicture(SDL_Texture *tex,bool AutoDelete,int _mode=0)
			{
				if (AutoDeletePic)
					SDL_DestroyTexture(src);
				src=tex;
				AutoDeletePic=AutoDelete;
				Mode=_mode;
				srcPS=GetTexturePosize(src);
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetFunc(void (*_func)(void*,bool),void *_funcdata)
			{
				func=_func;
				funcData=_funcdata;
			}

			PictureBox(int _ID,Widgets *_fa,const Posize &_rps)
			{
				SetID(_ID);
				DD<<"[Info] Create PictureBox "<<ID<<endl;
				Type=WidgetType_PictureBox;
				SetFa(_fa);
				SetrPS(_rps);
			}
			
			PictureBox(int _ID,Widgets *_fa,PosizeEX *psex)
			{
				SetID(_ID);
				DD<<"[Info] Create PictureBox "<<ID<<endl;
				Type=WidgetType_PictureBox;
				SetFa(_fa);
				AddPsEx(psex);
			}
	};
	
	template <class T> class SimpleListView:public Widgets
	{
		protected:
			int ListCnt=0;
			vector <string> Text;
			vector <T> FuncData;//It means this widget's FuncData would be deconstructed when deleted
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
			LargeLayerWithScrollBar *fa2=NULL;
			int stat=0,//0:Up_NoFocus 1:Up_Focus_Row 2:Down_Left_TwiceClick 3:Down_Right 4:Down_Left_OnceClick
				FocusChoose=-1,
				ClickChoose=-1;
			int EachHeight=24,
				Interval=2,
				TextMode=-1;
			RGBA TextColor=RGBA_NONE,
				 RowColor[3];
				 
			inline Posize GetLinePosize(int pos)
			{
				if (pos==-1) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+pos*(EachHeight+Interval),gPS.w,EachHeight);
			}
			
			inline int GetLineFromPos(int y)
			{
				int re=(Win->NowPos.y-gPS.y)/(EachHeight+Interval);
				if ((y-gPS.y)%(EachHeight+Interval)<EachHeight&&InRange(re,0,ListCnt-1))
					return re;
				else return -1;
			}
			
				 
			virtual void CheckEvent()
			{
				
			}
			
			virtual void CheckPos()
			{
				if (ListCnt==0||!Win->NeedSolvePosEvent) return;
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								Win->PresentLimit|=GetLinePosize(FocusChoose);
								FocusChoose=-1;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (ClickChoose!=-1)
							Win->PresentLimit|=GetLinePosize(ClickChoose);
						if (FocusChoose!=-1)
							Win->PresentLimit|=GetLinePosize(FocusChoose);
						ClickChoose=FocusChoose=GetLineFromPos(Win->NowPos.y);
						if (event.button.button==SDL_BUTTON_LEFT)
						{
							if (event.button.clicks==2)
								stat=2;
							else stat=4;
						}
						else if (event.button.button==SDL_BUTTON_RIGHT)
							stat=3;
						else stat=4,DD<<"[Waring] SimpleListView: Unknown click button,use it as left click once"<<endl;
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						if (ClickChoose!=-1)
							Win->PresentLimit|=GetLinePosize(ClickChoose);
						if (FocusChoose!=-1)
							Win->PresentLimit|=GetLinePosize(FocusChoose);
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							DD<<"[Info] SimpleListView "<<ID<<" func "<<ClickChoose<<" "<<(ClickChoose!=-1?Text[ClickChoose]:"")<<endl;
							if (func!=NULL)
								func(ClickChoose!=-1?FuncData[ClickChoose]:BackgroundFuncData,ClickChoose,stat==4?1:stat);
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
						}
						break;
					
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
						}
						else
						{
							int pos=(Win->NowPos.y-gPS.y)/(EachHeight+Interval);
							if ((Win->NowPos.y-gPS.y)%(EachHeight+Interval)<EachHeight&&InRange(pos,0,ListCnt-1))
							{
								if (pos!=FocusChoose)
								{
									if (FocusChoose!=-1)
										Win->PresentLimit|=Posize(gPS.x,gPS.y+FocusChoose*(EachHeight+Interval),gPS.w,EachHeight);
									Win->PresentLimit|=Posize(gPS.x,gPS.y+pos*(EachHeight+Interval),gPS.w,EachHeight);
									FocusChoose=pos;
									Win->NeedFreshScreen=1;
								}
							}
							else
								if (FocusChoose!=-1)
								{
									Win->PresentLimit|=Posize(gPS.x,gPS.y+FocusChoose*(EachHeight+Interval),gPS.w,EachHeight);
									FocusChoose=-1;
									Win->NeedFreshScreen=1;
								}
						}
						Win->NeedSolvePosEvent=0;
						break;	
				}
			}

			virtual void Show(Posize &lmt)
			{
				if (ListCnt==0) return;
				int ForL=(-fa->GetrPS().y)/(EachHeight+Interval),
					ForR=ForL+fa2->GetgPS().h/(EachHeight+Interval)+1;
				ForL=EnsureInRange(ForL,0,ListCnt-1);
				ForR=EnsureInRange(ForR,0,ListCnt-1);
				if (!InRange(ClickChoose,0,ListCnt-1)) ClickChoose=-1;
				if (!InRange(FocusChoose,0,ListCnt-1)) FocusChoose=-1;
//				DD<<"[Debug] SLV "<<ForL<<" "<<ForR<<" "<<ClickChoose<<" "<<FocusChoose<<endl;
				Posize RowPs=Posize(gPS.x,gPS.y+ForL*(EachHeight+Interval),gPS.w,EachHeight);
				for (int i=ForL;i<=ForR;RowPs.y+=EachHeight+Interval,++i)
				{
					Win->RenderFillRect(RowPs&lmt,RowColor[ClickChoose==i?2:FocusChoose==i]?RowColor[ClickChoose==i?2:FocusChoose==i]:ThemeColor[ClickChoose==i?4:(FocusChoose==i?2:0)]);
					Win->RenderDrawText(Text[i],TextMode==-1?RowPs+Point(8,0):(TextMode==1?RowPs-Point(24,0):RowPs),lmt,TextMode,TextColor?TextColor:ThemeColor.MainTextColor[0]);
					Win->Debug_DisplayBorder(RowPs);
				}	
				Win->Debug_DisplayBorder(gPS);
			}

			virtual void CalcPsEx()//??
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				rPS.w=fa2->GetrPS().w;
				rPS.h=EnsureInRange(ListCnt*(EachHeight+Interval)-Interval,fa2->GetrPS().h,1e9);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				fa2->ResizeLL(rPS.w,rPS.h);
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}

			inline void SetRowColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					RowColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SimpleListView: SetRowColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			void SetBackGroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}

			inline void SetRowHeightAndInterval(int _height,int _interval)
			{
				EachHeight=_height;
				Interval=_interval;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
			inline void SetTextMode(int mode)
			{
				TextMode=mode;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetListFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetListContent(int p,const string &str,const T &_funcdata)//p: 0<=p<ListCnt:SetInP >=ListCnt:SetInLast <0:SetInFirst
			{
				p=EnsureInRange(p,0,ListCnt);
				Text.insert(Text.begin()+p,str);
				FuncData.insert(FuncData.begin()+p,_funcdata);
				++ListCnt;
				if (ClickChoose>=p) ++ClickChoose;
				rPS.h=EnsureInRange(ListCnt*(EachHeight+Interval)-Interval,fa2->GetrPS().h,1e9);
				fa2->ResizeLL(-1,rPS.h);
//				CalcPsEx();//??
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;//?? It don't need fresh such big area
			}
			
			void DeleteListContent(int p)//p: 0<=p<ListCnt:SetInP >=ListCnt:DeleteLast <0:DeleteFirst
			{
				if (!ListCnt) return;
				p=EnsureInRange(p,0,ListCnt-1);
				Text.erase(Text.begin()+p);
				FuncData.erase(FuncData.begin()+p);
				--ListCnt;
				if (FocusChoose==ListCnt)
					FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				rPS.h=EnsureInRange(ListCnt*(EachHeight+Interval)-Interval,fa2->GetrPS().h,1e9);
				fa2->ResizeLL(-1,rPS.h);
//				CalcPsEx();
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void ClearListContent()
			{
				if (!ListCnt) return;
				DD<<"[Info] Clear SimpleListViewContent: ListCnt:"<<ListCnt<<endl;
				FocusChoose=ClickChoose=-1;
				Text.clear();
				FuncData.clear();
				ListCnt=0;
				rPS.h=fa2->GetrPS().h;
				fa2->ResizeLL(-1,rPS.h);
				fa2->SetViewPort(1,0);//??
				fa2->SetViewPort(2,0);
//				CalcPsEx();
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			SimpleListView* PushbackContent(const string &str,const T &_funcdata)
			{
				SetListContent(1e9,str,_funcdata);
				return this;
			}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<ListCnt;++i)
					if (FuncData[i]==_funcdata)
						return i;
				return -1;
			}
			
			void SetText(int p,const string &str)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				Text[p]=str;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS&GetLinePosize(p);
			}
			
			T& GetFuncData(int p)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				return FuncData[p];
			}
			
			T& operator [] (int p)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				return FuncData[p];
			}
			
			void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			void AddPsEx(PosizeEX *psex)//?? Need virtual?
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
//			~SimpleListView()
//			{
//				DD<<"[Info] Delete SimpleListView "<<ID<<endl;
//				ClearListContent();
//			}
			
			SimpleListView(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleListView "<<ID<<endl;
				Type=WidgetType_SimpleListView;
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,0};
				for (int i=0;i<=2;++i)
					RowColor[i]=RGBA_NONE;
			}
			
			SimpleListView(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleListView "<<ID<<endl;
				Type=WidgetType_SimpleListView;
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				for (int i=0;i<=2;++i)
					RowColor[i]=RGBA_NONE;
			}
	};
	
	template <class T> class SimpleListView_MultiColor:public SimpleListView <T> 
	{
		public:
			struct EachRowColor0
			{
				RGBA row[3],
					 text=RGBA_NONE;
					
				EachRowColor0(const RGBA &row0,const RGBA &row1,const RGBA &row2,const RGBA &_text=RGBA_NONE)
				:row{row0,row1,row2},text(_text) {}
				
				EachRowColor0():row{RGBA_NONE,RGBA_NONE,RGBA_NONE} {}
			};
			static EachRowColor0 EachRowColor_NONE;
		
		protected:
			vector <EachRowColor0> EachRowColor;
			
			virtual void Show(Posize &lmt)
			{
				if (this->ListCnt==0) return;//Oh ! Too many this pointers! >_<
				int ForL=(-this->fa->GetrPS().y)/(this->EachHeight+this->Interval),
					ForR=ForL+this->fa2->GetgPS().h/(this->EachHeight+this->Interval)+1;
				ForL=EnsureInRange(ForL,0,this->ListCnt-1);
				ForR=EnsureInRange(ForR,0,this->ListCnt-1);
				if (!InRange(this->ClickChoose,0,this->ListCnt-1)) this->ClickChoose=-1;
				if (!InRange(this->FocusChoose,0,this->ListCnt-1)) this->FocusChoose=-1;
				Posize RowPs=Posize(this->gPS.x,this->gPS.y+ForL*(this->EachHeight+this->Interval),this->gPS.w,this->EachHeight);
				for (int i=ForL;i<=ForR;RowPs.y+=this->EachHeight+this->Interval,++i)
				{
					this->Win->RenderFillRect(RowPs&lmt,this->EachRowColor[i].row[this->ClickChoose==i?2:this->FocusChoose==i]?this->EachRowColor[i].row[this->ClickChoose==i?2:this->FocusChoose==i]:
						(this->RowColor[this->ClickChoose==i?2:this->FocusChoose==i]?this->RowColor[this->ClickChoose==i?2:this->FocusChoose==i]:ThemeColor[this->ClickChoose==i?4:(this->FocusChoose==i?2:0)]));//so long! *_*
					this->Win->RenderDrawText(this->Text[i],this->TextMode==-1?RowPs+Point(8,0):(this->TextMode==1?RowPs-Point(24,0):RowPs),lmt,this->TextMode,this->EachRowColor[i].text?this->EachRowColor[i].text:(this->TextColor?this->TextColor:ThemeColor.MainTextColor[0]));
					this->Win->Debug_DisplayBorder(RowPs);
				}	
				this->Win->Debug_DisplayBorder(this->gPS);
			}
		
		public:
			void SetListContent(int p,const string &str,const T &_funcdata,const EachRowColor0 &co=EachRowColor_NONE)
			{
				p=EnsureInRange(p,0,this->ListCnt);
				EachRowColor.insert(EachRowColor.begin()+p,co);
				SimpleListView <T>::SetListContent(p,str,_funcdata);
			}
			
			SimpleListView_MultiColor* PushbackContent(const string &str,const T &_funcdata,const EachRowColor0 &co=EachRowColor_NONE)
			{
				SetListContent(1e9,str,_funcdata,co);
				return this;
			}
			
			void DeleteListContent(int p)
			{
				if (!this->ListCnt) return;
				p=EnsureInRange(p,0,this->ListCnt-1);
				EachRowColor.erase(EachRowColor.begin()+p);
				SimpleListView <T>::DeleteListContent(p);
			}

			void ClearListContent()
			{
				if (!this->ListCnt) return;
				DD<<"[Info] Clear SimpleListView_MultiColor_Content: ListCnt:"<<this->ListCnt<<endl;
				EachRowColor.clear();
				SimpleListView <T>::ClearListContent();
			}
			
			SimpleListView_MultiColor(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:SimpleListView <T>(_ID,_fa,_rps,_func)
			{
				DD<<"[Info] Create SimpleListView_MultiColor "<<this->ID<<endl;
				this->Type=Widgets::WidgetType_SimpleListView_MultiColor;
			}
			
			SimpleListView_MultiColor(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:SimpleListView <T>(_ID,_fa,psex,_func)
			{
				DD<<"[Info] Create SimpleListView_MultiColor "<<this->ID<<endl;
				this->Type=Widgets::WidgetType_SimpleListView_MultiColor;
			}
	};
	template <class T> typename SimpleListView_MultiColor<T>::EachRowColor0 SimpleListView_MultiColor<T>::EachRowColor_NONE;
	
	template <class T> class SimpleBlockView:public Widgets
	{
		public:
			struct BlockViewData
			{
				string MainText,
					   SubText1,
					   SubText2;
				T FuncData;
				SharedTexturePtr pic;
				RGBA MainTextColor=RGBA_NONE,
				 	 SubTextColor1=RGBA_NONE,
				 	 SubTextColor2=RGBA_NONE,
					 BlockColor[3];//NoFocusRow,FocusBlock,ClickBlock
				
				BlockViewData(const T &funcdata,const string &maintext,const string &subtext1,const string &subtext2,const SharedTexturePtr &_pic)
				:FuncData(funcdata),MainText(maintext),SubText1(subtext1),SubText2(subtext2),pic(_pic)
				{
					for (int i=0;i<=2;++i)
						BlockColor[i]=RGBA_NONE;
				}
				
				BlockViewData()
				{
					for (int i=0;i<=2;++i)
						BlockColor[i]=RGBA_NONE;
				}
			};
			
		protected:
			int BlockCnt=0;
			vector <BlockViewData> BlockData;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0,-1 means background)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
			LargeLayerWithScrollBar *fa2=NULL;
			int stat=0,//0:Up_NoFocus 1:Up_Focus_Row 2:Down_Left_TwiceClick 3:Down_Right 4:Down_Left_OnceClick
				FocusChoose=-1,
				ClickChoose=-1,
				EachLineBlocks=1;//update this after changing rPS or EachPs
			Posize EachPs={5,5,240,80};//min(w,h) is the pic edge length, if w>=h ,the text will be show on right,else on buttom
			RGBA MainTextColor=RGBA_NONE,//These colors are default color
				 SubTextColor1=RGBA_NONE,
				 SubTextColor2=RGBA_NONE,
				 BlockColor[3];//NoFocusRow,FocusBlock,ClickBlock
			bool EnablePic=1,
				 EnableCheckBox=0,
				 EnableDrag=0,
				 DisableKeyboardEvent=0;
			
			Posize GetBlockPosize(int p)//get specific Block Posize
			{
				if (p==-1) return ZERO_POSIZE;
				return Posize(gPS.x+p%EachLineBlocks*(EachPs.x+EachPs.w),gPS.y+p/EachLineBlocks*(EachPs.y+EachPs.h),EachPs.w,EachPs.h);
			}

			int InBlockPosize(const Point &pt)//return In which Posize ,if not exist,return -1
			{
				int re=(pt.y-gPS.y+EachPs.y)/(EachPs.y+EachPs.h)*EachLineBlocks+(pt.x-gPS.x+EachPs.x)/(EachPs.x+EachPs.w);
				if (InRange(re,0,BlockCnt-1)&&(GetBlockPosize(re)&CoverLmt).In(pt))
					return re;
				else return -1;					
			}
			
			virtual void CheckEvent()
			{
				if (BlockCnt==0||DisableKeyboardEvent) return;
				const SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_KEYDOWN:
						int MoveSelectPosDelta=0;
						switch (event.key.keysym.sym)
						{
							case SDLK_RETURN:
								if (InRange(ClickChoose,0,BlockCnt-1))
								{
//									if (...)//Mod??
									DD<<"[Info] SimpleBlockView "<<ID<<": func "<<ClickChoose<<" "<<BlockData[ClickChoose].MainText<<endl;
									if (func!=NULL)
										func(BlockData[ClickChoose].FuncData,ClickChoose,2);
									Win->NeedSolveEvent=0;
								}
								break;
							case SDLK_LEFT:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=-1;
							case SDLK_RIGHT:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=1;
							case SDLK_UP:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=-EachLineBlocks;
							case SDLK_DOWN:
								if (MoveSelectPosDelta==0)
									MoveSelectPosDelta=EachLineBlocks;
								if (ClickChoose!=-1)
								{
									Win->PresentLimit|=GetBlockPosize(ClickChoose);
									ClickChoose=EnsureInRange(ClickChoose+MoveSelectPosDelta,0,BlockCnt-1);
									if (ClickChoose/EachLineBlocks*(EachPs.y+EachPs.h)+fa->GetrPS().y<0)
										fa2->SetViewPort(2,ClickChoose/EachLineBlocks*(EachPs.y+EachPs.h));
									else if (ClickChoose/EachLineBlocks*(EachPs.y+EachPs.h)+fa->GetrPS().y>fa2->GetrPS().h-EachPs.y-EachPs.h)
										fa2->SetViewPort(2,(ClickChoose/EachLineBlocks+1)*(EachPs.y+EachPs.h)-fa2->GetrPS().h);
								}
								else ClickChoose=0;
								Win->PresentLimit|=GetBlockPosize(ClickChoose);
								Win->NeedFreshScreen=1;
								Win->NeedSolveEvent=0;
								break;
						}
				}
			}
			
			virtual void CheckPos()
			{
				if (BlockCnt==0||!Win->NeedSolvePosEvent) return;
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								Win->PresentLimit|=GetBlockPosize(FocusChoose);
								FocusChoose=-1;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
							}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						Win->PresentLimit|=GetBlockPosize(ClickChoose);
						Win->PresentLimit|=GetBlockPosize(FocusChoose);
						ClickChoose=FocusChoose=InBlockPosize(Win->NowPos);
						Win->PresentLimit|=GetBlockPosize(ClickChoose);
						Win->PresentLimit|=GetBlockPosize(FocusChoose);
//						if (ClickChoose!=-1)
//						{
							if (event.button.button==SDL_BUTTON_LEFT)
							{
								if (event.button.clicks==2)
									stat=2;
								else stat=4;
							}
							else if (event.button.button==SDL_BUTTON_RIGHT)
								stat=3;
							else stat=4,DD<<"[Waring] SimpleBlockView "<<ID<<": Unknown click button,use it as left click once"<<endl;
//						}
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							if (ClickChoose>=0)
								DD<<"[Info] SimpleBlockView "<<ID<<": func "<<ClickChoose<<" "<<BlockData[ClickChoose].MainText<<endl;
							if (func!=NULL)
								func(ClickChoose==-1?BackgroundFuncData:BlockData[ClickChoose].FuncData,ClickChoose,stat==4?1:stat);
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
						}
						break;
					
					case SDL_MOUSEMOTION:
						if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							FocusChoose=InBlockPosize(Win->NowPos);
							Win->PresentLimit|=GetBlockPosize(FocusChoose);
							Win->NeedFreshScreen=1;
						}
						else
						{
							int pos=InBlockPosize(Win->NowPos);
							if (pos!=FocusChoose)
							{
								Win->PresentLimit|=GetBlockPosize(FocusChoose);
								Win->PresentLimit|=GetBlockPosize(pos);
								FocusChoose=pos;
								Win->NeedFreshScreen=1;
							}	
						}
						Win->NeedSolvePosEvent=0;
						break;	
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (BlockCnt==0) return;
				int ForL=-fa->GetrPS().y/(EachPs.y+EachPs.h)*EachLineBlocks,
					ForR=((-fa->GetrPS().y+fa2->GetgPS().h)/(EachPs.y+EachPs.h)+1)*EachLineBlocks-1;
				ForL=EnsureInRange(ForL,0,BlockCnt-1);
				ForR=EnsureInRange(ForR,0,BlockCnt-1);
//				if (!InRange(ClickChoose,0,ListCnt-1)) ClickChoose=-1;
//				if (!InRange(FocusChoose,0,ListCnt-1)) FocusChoose=-1;

				Posize BlockPs=GetBlockPosize(ForL);
				for (int i=ForL;i<=ForR;++i)
				{
					Win->RenderFillRect(BlockPs&lmt,BlockData[i].BlockColor[ClickChoose==i?2:FocusChoose==i]?BlockData[i].BlockColor[ClickChoose==i?2:FocusChoose==i]:(BlockColor[ClickChoose==i?2:FocusChoose==i]?BlockColor[ClickChoose==i?2:FocusChoose==i]:ThemeColor[ClickChoose==i?4:(FocusChoose==i?2:0)]));
					
					if (EnablePic&&BlockData[i].pic()!=NULL)
					{
						Win->RenderCopyWithLmt(BlockData[i].pic(),Posize(BlockPs.x,BlockPs.y,min(EachPs.w,EachPs.h),min(BlockPs.w,BlockPs.h)),lmt);
						Win->Debug_DisplayBorder(Posize(BlockPs.x,BlockPs.y,min(EachPs.w,EachPs.h),min(BlockPs.w,BlockPs.h)));
					}
					
					if (EnablePic)
						if (EachPs.w>=EachPs.h)
						{
							Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,BlockData[i].MainTextColor?BlockData[i].MainTextColor:(MainTextColor?MainTextColor:ThemeColor.MainTextColor[0]));
							Win->RenderDrawText(BlockData[i].SubText1,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y+EachPs.h/3,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,BlockData[i].SubTextColor1?BlockData[i].SubTextColor1:(SubTextColor1?SubTextColor1:ThemeColor.BackgroundColor[6]));
							Win->RenderDrawText(BlockData[i].SubText2,Posize(BlockPs.x+BlockPs.h+8,BlockPs.y+EachPs.h/3*2,BlockPs.w-BlockPs.h-8,BlockPs.h/3),lmt,-1,BlockData[i].SubTextColor2?BlockData[i].SubTextColor2:(SubTextColor2?SubTextColor2:ThemeColor.BackgroundColor[6]));
						}
						else Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x,BlockPs.y+EachPs.w,BlockPs.w,BlockPs.h-BlockPs.w),lmt,0,BlockData[i].MainTextColor?BlockData[i].MainTextColor:(MainTextColor?MainTextColor:ThemeColor.MainTextColor[0]));
					else
					{
						Win->RenderDrawText(BlockData[i].MainText,Posize(BlockPs.x+8,BlockPs.y,BlockPs.w-8,BlockPs.h/3),lmt,-1,BlockData[i].MainTextColor?BlockData[i].MainTextColor:(MainTextColor?MainTextColor:ThemeColor.MainTextColor[0]));
						Win->RenderDrawText(BlockData[i].SubText1,Posize(BlockPs.x+8,BlockPs.y+EachPs.h/3,BlockPs.w-8,BlockPs.h/3),lmt,-1,BlockData[i].SubTextColor1?BlockData[i].SubTextColor1:(SubTextColor1?SubTextColor1:ThemeColor.BackgroundColor[6]));
						Win->RenderDrawText(BlockData[i].SubText2,Posize(BlockPs.x+8,BlockPs.y+EachPs.h/3*2,BlockPs.w-8,BlockPs.h/3),lmt,-1,BlockData[i].SubTextColor2?BlockData[i].SubTextColor2:(SubTextColor2?SubTextColor2:ThemeColor.BackgroundColor[6]));
					}
					
 					Win->Debug_DisplayBorder(BlockPs);
					
					if ((i+1)%EachLineBlocks==0)
						BlockPs.y+=EachPs.y+EachPs.h,BlockPs.x=gPS.x;
					else BlockPs.x+=EachPs.x+EachPs.w;
				}
				
				Win->Debug_DisplayBorder(gPS);
			}

			virtual void CalcPsEx()
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				rPS.w=fa2->GetrPS().w;
				EachLineBlocks=max(1,(rPS.w+EachPs.x)/(EachPs.x+EachPs.w));
				rPS.h=EnsureInRange(ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y,fa2->GetrPS().h,1e9);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				fa2->ResizeLL(rPS.w,rPS.h);
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
			}
			
		public:
			inline void SetMainTextColor(const RGBA &co)
			{
				MainTextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetSubTextColor1(const RGBA &co)
			{
				SubTextColor1=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetSubTextColor2(const RGBA &co)
			{
				SubTextColor2=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}

			inline void SetBlockColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BlockColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SimpleBlockView "<<ID<<": SetBlockColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			void SetBackGroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}

			void SetEachBlockPosize(const Posize &ps)
			{
				EachPs=ps;//Other data would be claculated in CalcPsex (?)
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}

			inline void SetBlockFunc(void (*_func)(T&,int,int))
			{func=_func;}
			
			void SetBlockContent(int p,const BlockViewData &data)//p: 0<=p<ListCnt:SetInP >=ListCnt:SetInLast <0:SetInFirst
			{
				p=EnsureInRange(p,0,BlockCnt);
				BlockData.insert(BlockData.begin()+p,data);
				++BlockCnt;
				if (ClickChoose>=p) ++ClickChoose;
				rPS.h=EnsureInRange(ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y,fa2->GetrPS().h,1e9);
				fa2->ResizeLL(-1,rPS.h);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;//?? It don't need fresh such big area
			}
			
			void DeleteBlockContent(int p)//p: 0<=p<ListCnt:SetInP >=ListCnt:DeleteLast <0:DeleteFirst
			{
				if (!BlockCnt) return;
				p=EnsureInRange(p,0,BlockCnt-1);
				BlockData.erase(BlockData.begin()+p);
				--BlockCnt;
				if (FocusChoose==BlockCnt) FocusChoose=-1;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				rPS.h=EnsureInRange(ceil(BlockCnt*1.0/EachLineBlocks)*(EachPs.y+EachPs.h)-EachPs.y,fa2->GetrPS().h,1e9);
				fa2->ResizeLL(-1,rPS.h);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void ClearBlockContent()
			{
				if (!BlockCnt) return;
				DD<<"[Info] Clear SimpleBlockView "<<ID<<" content: BlockCnt:"<<BlockCnt<<endl;
				FocusChoose=ClickChoose=-1;
				BlockData.clear();
				BlockCnt=0;
				rPS.h=fa2->GetrPS().h;
				fa2->ResizeLL(-1,rPS.h);
				fa2->SetViewPort(1,0);
				fa2->SetViewPort(2,0);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetSelectBlock(int p)
			{
				if (!InRange(p,0,BlockCnt-1))
					return;
				ClickChoose=p;
				if (!InRange(p/EachLineBlocks*(EachPs.y+EachPs.h)+fa->GetrPS().y,0,fa2->GetrPS().h-EachPs.y-EachPs.h))
					fa2->SetViewPort(2,p/EachLineBlocks*(EachPs.y+EachPs.h));
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline int GetCurrentSelectBlock()
			{return ClickChoose;}
			
			SimpleBlockView* PushbackContent(const BlockViewData &data)
			{
				SetBlockContent(1e9,data);
				return this;
			}
			
			inline void SetBackgroundFuncData(const T &data)
			{BackgroundFuncData=data;}
			
			inline int GetBlockCnt()
			{return BlockCnt;}
			
			inline bool Empty()
			{return BlockCnt==0;}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<BlockCnt;++i)
					if (BlockData[i].FuncData==_funcdata)
						return i;
				return -1;
			}
			
			BlockViewData& GetBlockData(int p)
			{
				p=EnsureInRange(p,0,BlockCnt);
				return BlockData[p];
			}
			
			T& GetFuncData(int p)
			{
				p=EnsureInRange(p,0,BlockCnt-1);
				return BlockData[p].FuncData;
			}
			
			inline T& GetBackgroundFuncData()
			{return BackgroundFuncData;}
			
			T& operator [] (int p)
			{
				p=EnsureInRange(p,0,BlockCnt-1);
				return BlockData[p].FuncData;
			}
			
			void SetUpdateBlock(int p)
			{
				if (!InRange(p,0,BlockCnt-1))
					return;
				Posize bloPs=GetBlockPosize(p);
				if ((bloPs&CoverLmt).Size()!=0)
				{
					Win->PresentLimit|=bloPs;
					Win->NeedFreshScreen=1;
				}
			}
			
			Range GetCurrentShowRange()
			{
				Range re;
				re.l=-fa->GetrPS().y/(EachPs.y+EachPs.h)*EachLineBlocks;
				re.r=((-fa->GetrPS().y+fa2->GetgPS().h)/(EachPs.y+EachPs.h)+1)*EachLineBlocks-1;
				re.l=EnsureInRange(re.l,0,BlockCnt-1);
				re.r=EnsureInRange(re.r,0,BlockCnt-1);
				return re;
			}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			void SetEnablePic(bool enable)
			{
				EnablePic=enable;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetDisableKeyboardEvent(bool disable)
			{DisableKeyboardEvent=disable;}
			
			SimpleBlockView(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleBlockView "<<ID<<endl;
				Type=WidgetType_SimpleBlockView;
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,0};
				for (int i=0;i<=2;++i)
					BlockColor[i]=RGBA_NONE;
			}
			
			SimpleBlockView(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleBlockView "<<ID<<endl;
				Type=WidgetType_SimpleBlockView;
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				for (int i=0;i<=2;++i)
					BlockColor[i]=RGBA_NONE;
			}
	};
	
	template <class T> class SimpleTreeView1:public Widgets//It is not designed so good...
	{
		public:
			struct TreeViewData
			{
				friend SimpleTreeView1;
				protected:
					int dep=0;
					bool unfolded=0;
					int subTreeSize=1;
				
				public:
					bool unfoldble=0;
					string text;
					T FuncData;
					SharedTexturePtr pic;
					RGBA textColor=RGBA_NONE,
						 NodeColor[3];
						 
					inline int GetDep()
					{return dep;}
					
					inline int GetSubTreeSize()
					{return subTreeSize;}
					
					TreeViewData(const string &_text,const T &_funcdata,bool _unfoldble,const SharedTexturePtr &_pic=SharedTexturePtr(NULL))
					:text(_text),FuncData(_funcdata),pic(_pic),unfoldble(_unfoldble)
					{
						for (int i=0;i<=2;++i)
							NodeColor[i]=RGBA_NONE;
					}
					
					TreeViewData()
					{
						for (int i=0;i<=2;++i)
							NodeColor[i]=RGBA_NONE;
					}
			};
			
		protected:
			vector <TreeViewData> NodesData;
			void (*func)(T&,SimpleTreeView1<T>*,int,int)=NULL;//funcdata,this,click_type(stat),pos
			LargeLayerWithScrollBar *fa2=NULL;
			int stat=0,//0:Up_NoFocus 1:Up_Focus_Node 2:Down_Left_TwiceClick 3:Down_Right 4:Down_Left_OnceClick 5:click unfold
				NodeCnt=0,
				ClickChoose=-1,
				FocusChoose=-1;
			int EachHeight=30,
				TabWidth=10;
			bool ShowAlign=0;
			RGBA textColor=RGBA_NONE,
				 NodeColor[3];
			SharedTexturePtr TriTex[6];

			Posize GetNodePosize(int pos)
			{
				if (!InRange(pos,0,NodeCnt-1)) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+pos*EachHeight,gPS.w,EachHeight);
			}
			
			int GetNodeFromPos(int y)
			{
				int re=(y-gPS.y)/EachHeight;
				if (!InRange(re,0,NodeCnt-1))
					re=-1;
				return re;
			}
			
			void GetTriTex(int p)
			{
				
			}

			virtual void CheckEvent()
			{
				
			}
			
			virtual void CheckPos()
			{
				if (NodeCnt==0) return;
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
							{
								stat=0;
								if (FocusChoose!=-1)
									Win->PresentLimit|=GetNodePosize(FocusChoose);
								FocusChoose=-1;
								RemoveNeedLoseFocus();
								Win->NeedFreshScreen=1;
							}
					return;
				}
				
				switch (event.type)
				{	
					case SDL_MOUSEBUTTONDOWN:
						if (ClickChoose!=-1)
							Win->PresentLimit|=GetNodePosize(ClickChoose);
						if (FocusChoose!=-1)
							Win->PresentLimit|=GetNodePosize(FocusChoose);
						ClickChoose=FocusChoose=GetNodeFromPos(Win->NowPos.y);
						if (ClickChoose!=-1)
							if (event.button.button==SDL_BUTTON_LEFT)
								if (NodesData[ClickChoose].unfoldble&&InRange(Win->NowPos.x,NodesData[ClickChoose].dep*TabWidth,NodesData[ClickChoose].dep*TabWidth+EachHeight))
									stat=5;
								else if (event.button.clicks==2)
									stat=2;
								else stat=4;
							else if (event.button.button==SDL_BUTTON_RIGHT)
								stat=3;
							else stat=4,DD<<"[Waring] SimpleTreeView1: Unknown click button,use it as left click once"<<endl;
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						if (ClickChoose!=-1)
							Win->PresentLimit|=GetNodePosize(ClickChoose);
						if (FocusChoose!=-1)
							Win->PresentLimit|=GetNodePosize(FocusChoose);
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							DD<<"[Info] SimpleTreeView1 "<<ID<<" func "<<stat<<" "<<ClickChoose<<" "<<NodesData[ClickChoose].text<<endl;
							if (func!=NULL)
								func(NodesData[ClickChoose].FuncData,this,stat,ClickChoose);
							stat=1;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=GetNodePosize(ClickChoose);
						}
						break;
					
					case SDL_MOUSEMOTION:
						int pos=GetNodeFromPos(Win->NowPos.y);
						if (pos!=-1)
						{
							if (stat==0)
							{
								stat=1;
								SetNeedLoseFocus();
							}
							else if (pos!=FocusChoose)
								if (FocusChoose!=-1)
									Win->PresentLimit|=GetNodePosize(FocusChoose);
							Win->PresentLimit|=GetNodePosize(pos);
							FocusChoose=pos;
							Win->NeedFreshScreen=1;
						}
						else
							if (FocusChoose!=-1)
							{
								Win->PresentLimit|=GetNodePosize(FocusChoose);
								FocusChoose=-1;
								Win->NeedFreshScreen=1;
							}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (NodeCnt==0) return;
				int ForL=EnsureInRange(-fa->GetrPS().y/EachHeight,0,NodeCnt-1),
					ForR=EnsureInRange(ForL+fa2->GetgPS().h/EachHeight+1,0,NodeCnt-1);
				for (int i=ForL;i<=ForR;++i)
				{
					Posize NodePs=GetNodePosize(i);
					TreeViewData &tvd=NodesData[i];
					int col=ClickChoose==i?2:FocusChoose==i;
					Win->RenderFillRect(NodePs&lmt,!tvd.NodeColor[col]?(!NodeColor[col]?ThemeColor[col*2]:NodeColor[col]):tvd.NodeColor[col]);
					Win->RenderDrawText(tvd.text,Posize(NodePs.x+tvd.dep*TabWidth+EachHeight*2,NodePs.y,NodePs.w-tvd.dep*TabWidth-EachHeight*2,EachHeight),lmt,-1,!tvd.textColor?(!textColor?ThemeColor.MainTextColor[0]:textColor):tvd.textColor);
					if (tvd.unfoldble)
					{
						int p=stat==5?2:FocusChoose==i;
						if (tvd.unfolded)
							p+=3;
						if (!TriTex[p])
							GetTriTex(p);
						Win->RenderCopyWithLmt(TriTex[p].GetPic(),Posize(NodePs.x+tvd.dep*TabWidth,NodePs.y,EachHeight,EachHeight),lmt); 
					}
					if (tvd.pic.GetPic()!=NULL)
						Win->RenderCopyWithLmt(tvd.pic.GetPic(),Posize(NodePs.x+tvd.dep*TabWidth+EachHeight,NodePs.y,EachHeight,EachHeight),lmt);
				}
				Win->Debug_DisplayBorder(gPS);
			}
			
			virtual void CalcPsEx()//??
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				rPS.w=fa2->GetrPS().w;
				rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				fa2->ResizeLL(rPS.w,rPS.h);
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
			}
			
		public:
			inline void SetTextColor(const RGBA &co)
			{
				textColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}

			inline void SetNodeColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					NodeColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SimpleTreeView1: SetNodeColor: p "<<p<<" is not in range[0,2]"<<endl;
			}			
			
			void SetBackGroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}

			inline void SetNodeHeight(int h)
			{
				EachHeight=h;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
			}
			
			inline void SetTreeViewFunc(void (*_func)(T&,SimpleTreeView1<T>*,int,int))
			{func=_func;}
			
			int GetSubNodeCnt(int faNode,bool recursive=0)
			{
				faNode=EnsureInRange(faNode,-1,NodeCnt-1);
				int re=0,subDep=faNode==-1?0:NodesData[faNode].dep+1;
				for (int i=faNode+1;i<NodeCnt&&NodesData[i].dep>=subDep;++i)
					if (NodesData[i].dep==subDep||recursive&&NodesData[i].dep>subDep)
						++re;
				return re;
			}
			
			int SetSubNode(int faNode,int p,TreeViewData &data)//if faNode==-1 insert in top level(dep 0)
			{
				DD<<"[Info] SimpleTreeView1 "<<ID<<" SetSubNode "<<faNode<<" "<<p<<endl;
				faNode=EnsureInRange(faNode,-1,NodeCnt);
				if (faNode!=-1)
					data.dep=NodesData[faNode].dep+1;
				p=max(p,0);
				for (int i=faNode+1;i<=NodeCnt;++i)
				{
					if (p==0||i==NodeCnt||NodesData[i].dep<data.dep)
					{
						NodesData.insert(NodesData.begin()+i,data);
						++NodeCnt;
						if (ClickChoose>=i) ++ClickChoose;
						rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
						fa2->ResizeLL(-1,rPS.h);
						Win->NeedUpdatePosize=1;
						Win->NeedFreshScreen=1;
						Win->PresentLimit|=gPS;
						return i;
					}
					if (NodesData[i].dep==data.dep)
						--p;
				}
			}
			
			int PushbackNode(int faNode,TreeViewData &data)
			{return SetSubNode(faNode,1e9,data);}
			
			int SetSubNodeMulti(int faNode,int p,vector <TreeViewData> &data)
			{
				DD<<"[Info] SimpleTreeView1 "<<ID<<" SetSubNodeMulti "<<faNode<<" "<<p<<endl;
				faNode=EnsureInRange(faNode,-1,NodeCnt);
				int dataDep=faNode==-1?0:NodesData[faNode].dep+1;
				if (faNode!=-1)
					for (int i=0;i<data.size();++i)
						data[i].dep=dataDep;
				p=max(p,0);
				for (int i=faNode+1;i<=NodeCnt;++i)
				{
					if (p==0||i==NodeCnt||NodesData[i].dep<dataDep)
					{
						NodesData.insert(NodesData.begin()+i,data.begin(),data.end());
						NodeCnt+=data.size();
						if (ClickChoose>=i) ClickChoose+=data.size();
						rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
						fa2->ResizeLL(-1,rPS.h);
						Win->NeedUpdatePosize=1;
						Win->NeedFreshScreen=1;
						Win->PresentLimit|=gPS;
						return i;
					}
					if (NodesData[i].dep==dataDep)
						--p;
				}
			}
			
			void DeleteSubNodeMulti(int faNode,int p,int cnt)
			{
				if (NodeCnt==0||cnt<=0) return;
				DD<<"[Info] SimpleTreeView1 "<<ID<<" DeleteSubNodeMulti "<<faNode<<" "<<p<<" "<<cnt<<endl;
				faNode=EnsureInRange(faNode,-1,NodeCnt-1);
				int delL=-1,delR=-1,delDep=faNode==-1?0:NodesData[faNode].dep+1;
				p=max(p,0);
				for (int i=faNode+1;i<NodeCnt;++i)
				{
					if (p==0)
						delL=i;
					if (NodesData[i].dep==delDep)
						if (p==0)
							--cnt;
						else --p;
					else if (NodesData[i].dep<delDep)
						break;
					delR=i;
					if (cnt==0)
						break;
				}
				if (delL!=-1)
				{
					NodesData.erase(NodesData.begin()+delL,NodesData.begin()+delR+1);
					NodeCnt-=delR-delL+1;
					if (FocusChoose>=NodeCnt) FocusChoose=-1;
					if (ClickChoose>=delR+1) ClickChoose-=delR-delL+1;
					else if (ClickChoose>delL) ClickChoose=-1;
					rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
					fa2->ResizeLL(-1,rPS.h);
					Win->NeedUpdatePosize=1;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
			}
			
			void DeleteAllSubNode(int faNode)
			{DeleteSubNodeMulti(faNode,0,GetSubNodeCnt(faNode));}
			
			void DeleteSubNode(int faNode,int p)
			{DeleteSubNodeMulti(faNode,p,1);}
			
			void ClearAllNode()
			{
				if (!NodeCnt) return;
				DD<<"[Info] SimpleTreeView1 "<<ID<<" ClearAllNode "<<NodeCnt<<endl;
				FocusChoose=ClickChoose=-1;
				NodesData.clear();
				NodeCnt=0;
				rPS.h=fa2->GetrPS().h;
				fa2->ResizeLL(-1,rPS.h);
				fa2->SetViewPort(1,0);
				fa2->SetViewPort(2,0);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetSelectNode(int p)
			{
				if (!InRange(p,0,NodeCnt-1))
					return;
				ClickChoose=p;
				if (!InRange(p*EachHeight+fa->GetrPS().y,0,fa2->GetrPS().h-EachHeight))
					fa2->SetViewPort(2,(p-1)*EachHeight);
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<NodeCnt;++i)
					if (NodesData[i].FuncData==_funcdata)
						return i;
				return -1;
			}
			
			TreeViewData& GetNodeData(int p)
			{return NodesData[EnsureInRange(p,0,NodeCnt-1)];}
			
			T& GetFuncData(int p)
			{return NodesData[EnsureInRange(p,0,NodeCnt-1)].FuncData;}
			
			T& operator [] (int p)
			{return NodesData[EnsureInRange(p,0,NodeCnt-1)].FuncData;}
			
			inline int GetNodeCnt()
			{return NodeCnt;}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			SimpleTreeView1(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,SimpleTreeView1<T>*,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleTreeView1 "<<ID<<endl;
				Type=WidgetType_SimpleTreeView1;
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,0};
				for (int i=0;i<=2;++i)
					NodeColor[i]=RGBA_NONE;
			}
			
			SimpleTreeView1(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,SimpleTreeView1<T>*,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create SimpleTreeView1 "<<ID<<endl;
				Type=WidgetType_SimpleTreeView1;
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				for (int i=0;i<=2;++i)
					NodeColor[i]=RGBA_NONE;
			}
	};
	
//	template <class T> class SimpleTreeView:public Widgets
//	{
//		public:
//			struct TreeViewNode
//			{
//				friend SimpleTreeView;
//				protected:
//					TreeViewNode *fa=NULL,
//								 *pre=NULL,
//								 *nxt=NULL,
//								 *firstchild=NULL,
//								 *lastchild=NULL;
//					int dep=0,size=1;
//					bool unfolded=0,
//						 unfoldble=0;
//					Posize onShowPs;
//				
//				public:
//					string text;
//					SharedTexturePtr pic;
//					RGBA textColor=RGBA_NONE,
//						 NodeColor[3];
//					T funcData;
//
//					inline int GetDep()
//					{return dep;}
//					
//					inline int GetSubTreeSize()
//					{return size;}
//					
//					TreeViewNode* GetNxtNode()
//					{
//						if (unfolded&&firstchild!=NULL)
//							return firstchild;
//						else if (nxt==NULL)
//						{
//							TreeViewNode *p=fa;
//							while (fa)
//								if (fa->nxt)
//									return fa->nxt;
//								else fa=fa->fa;
//							return fa;
//						}
//						else return nxt;
//					}
//					
//					TreeViewNode* GetPreNode()
//					{
//						if (pre==NULL)
//							return fa;
//						else
//						{
//							TreeViewNode *p=pre;
//							while (p->unfolded&&p->lastchild!=NULL)
//								p=p->lastchild;
//							return p;
//						}
//					}
//					
////					TreeViewData(const string &_text,const T &_funcdata,bool _unfoldble,const SharedTexturePtr &_pic=SharedTexturePtr(NULL))
////					:text(_text),FuncData(_funcdata),pic(_pic),unfoldble(_unfoldble)
////					{
////						for (int i=0;i<=2;++i)
////							NodeColor[i]=RGBA_NONE;
////					}
////					
////					TreeViewData()
////					{
////						for (int i=0;i<=2;++i)
////							NodeColor[i]=RGBA_NONE;
////					}
//			};
//			
//		protected:
//			TreeViewNode *rootNode=NULL;//It won't be displayed
//			void (*func)(T&,int,int)=NULL;//funcdata,pos,click_type(stat)
//			LargeLayerWithScrollBar *fa2=NULL;
//			int stat=0;//0:Up_NoFocus 1:Up_Focus_Node 2:Down_Left_TwiceClick 3:Down_Right 4:Down_Left_OnceClick 5:click unfold
//			TreeViewNode *ClickChoose=NULL,
//						 *FocusChoose=NULL,
//						 *FirstShowNode=NULL,
//						 *LastShowNode=NULL;
//			int EachHeight=30,
//				IntervalHeight=5,
//				TabWidth=10;
//			bool ShowAlign=0;
//			RGBA textColor=RGBA_NONE,
//				 NodeColor[3];
//			SharedTexturePtr TriTex[6];
//
//			TreeViewNode* GetNodeFromPos(int y)
//			{
//				if (rootNode->child==NULL) return NULL; 
//				for (TreeViewNode *p=FirstShowNode;p;p=p->GetNxtNode())
//					if (y<=p->onShowPs.y2())
//						return p;
//					else if (y<=p->onShowPs.y2()+IntervalHeight||p==LastShowNode)
//						return NULL;
//				return NULL;
//			}
//
//			virtual void CheckEvent()
//			{
//				
//			}
//			
//			virtual void CheckPos()
//			{
//				if (rootNode.empty()) return;
//				const SDL_Event &event=*Win->NowSolvingEvent;
//				if (Win->NowSolvingPosEventMode==1)
//				{
//					if (stat!=0)
//						if (!CoverLmt.In(Win->NowPos))
//							{
//								stat=0;
//								if (FocusChoose)
//									Win->PresentLimit|=FocusChoose->onShowPs;
//								FocusChoose=NULL;
//								RemoveNeedLoseFocus();
//								Win->NeedFreshScreen=1;
//							}
//					return;
//				}
//				
//				switch (event.type)
//				{	
//					case SDL_MOUSEBUTTONDOWN:
//						if (ClickChoose)
//							Win->PresentLimit|=ClickChoose->onShowPs;
//						if (FocusChoose)
//							Win->PresentLimit|=FocusChoose->onShowPs;
//						ClickChoose=FocusChoose=GetNodeFromPos(Win->NowPos.y);
//						if (ClickChoose)
//							if (event.button.button==SDL_BUTTON_LEFT)
//								if (ClickChoose->unfoldble&&InRange(Win->NowPos.x,,))
//									stat=5;
//								else if (event.button.clicks==2)
//									stat=2;
//								else stat=4;
//							else if (event.button.button==SDL_BUTTON_RIGHT)
//								stat=3;
//							else stat=4,DD<<"[Waring] SimpleTreeView: Unknown click button,use it as left click once"<<endl;
//						Win->NeedSolvePosEvent=0;
//						Win->NeedFreshScreen=1;
//						if (ClickChoose!=-1)
//							Win->PresentLimit|=GetNodePosize(ClickChoose);
//						if (FocusChoose!=-1)
//							Win->PresentLimit|=GetNodePosize(FocusChoose);
//						break;
//					
//					case SDL_MOUSEBUTTONUP:
//						if (stat>=2)
//						{
//							DD<<"[Info] SimpleTreeView "<<ID<<" func "<<stat<<" "<<ClickChoose<<" "<<NodesData[ClickChoose].text<<endl;
//							if (func!=NULL)
//								func(NodesData[ClickChoose].FuncData,this,stat,ClickChoose);
//							stat=1;
//							Win->NeedSolvePosEvent=0;
//							Win->NeedFreshScreen=1;
//							Win->PresentLimit|=GetNodePosize(ClickChoose);
//						}
//						break;
//					
//					case SDL_MOUSEMOTION:
//						int pos=GetNodeFromPos(Win->NowPos.y);
//						if (pos!=-1)
//						{
//							if (stat==0)
//							{
//								stat=1;
//								SetNeedLoseFocus();
//							}
//							else if (pos!=FocusChoose)
//								if (FocusChoose!=-1)
//									Win->PresentLimit|=GetNodePosize(FocusChoose);
//							Win->PresentLimit|=GetNodePosize(pos);
//							FocusChoose=pos;
//							Win->NeedFreshScreen=1;
//						}
//						else
//							if (FocusChoose!=-1)
//							{
//								Win->PresentLimit|=GetNodePosize(FocusChoose);
//								FocusChoose=-1;
//								Win->NeedFreshScreen=1;
//							}
//						Win->NeedSolvePosEvent=0;
//						break;
//				}
//			}
//			
//			virtual void Show(Posize &lmt)
//			{
//				if (NodeCnt==0) return;
//				int ForL=EnsureInRange(-fa->GetrPS().y/EachHeight,0,NodeCnt-1),
//					ForR=EnsureInRange(ForL+fa2->GetgPS().h/EachHeight+1,0,NodeCnt-1);
//				for (int i=ForL;i<=ForR;++i)
//				{
//					Posize NodePs=GetNodePosize(i);
//					TreeViewData &tvd=NodesData[i];
//					int col=ClickChoose==i?2:FocusChoose==i;
//					RenderFillRect(NodePs&lmt,!tvd.NodeColor[col]?(!NodeColor[col]?ThemeColor[col*2]:NodeColor[col]):tvd.NodeColor[col]);
//					RenderDrawText(tvd.text,Posize(NodePs.x+tvd.dep*TabWidth+EachHeight*2,NodePs.y,NodePs.w-tvd.dep*TabWidth-EachHeight*2,EachHeight),lmt,-1,!tvd.textColor?(!textColor?ThemeColor.MainTextColor[0]:textColor):tvd.textColor);
//					if (tvd.unfoldble)
//					{
//						int p=stat==5?2:FocusChoose==i;
//						if (tvd.unfolded)
//							p+=3;
//						if (!TriTex[p])
//							GetTriTex(p);
//						RenderCopyWithLmt(TriTex[p].GetPic(),Posize(NodePs.x+tvd.dep*TabWidth,NodePs.y,EachHeight,EachHeight),lmt); 
//					}
//					if (tvd.pic.GetPic()!=NULL)
//						RenderCopyWithLmt(tvd.pic.GetPic(),Posize(NodePs.x+tvd.dep*TabWidth+EachHeight,NodePs.y,EachHeight,EachHeight),lmt);
//				}
//				if (DEBUG_DisplayBorderFlag)
//					Debug_DisplayBorder(gPS);
//			}
//			
//			virtual void CalcPsEx()//??
//			{
//				Posize lastPs=gPS;
//				if (PsEx!=NULL)	
//					PsEx->GetrPS(rPS);
//				rPS.w=fa2->GetrPS().w;
//				rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
//				if (fa!=NULL)
//					gPS=rPS+fa->GetgPS();
//				else gPS=rPS;
//				CoverLmt=gPS&GetFaCoverLmt();
//				fa2->ResizeLL(rPS.w,rPS.h);
//				if (!(lastPs==gPS))
//					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
//			}
//			
//		public:
//			inline void SetTextColor(const RGBA &co)
//			{
//				textColor=co;
//				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=gPS;
//			}
//
//			inline void SetNodeColor(int p,const RGBA &co)
//			{
//				if (InRange(p,0,2))
//				{
//					NodeColor[p]=co;
//					Win->NeedFreshScreen=1;
//					Win->PresentLimit|=gPS;
//				}
//				else DD<<"[Error] SimpleTreeView: SetNodeColor: p "<<p<<" is not in range[0,2]"<<endl;
//			}			
//			
//			void SetBackGroundColor(const RGBA &co)
//			{fa2->SetLargeAreaColor(co);}
//
//			inline void SetNodeHeight(int h)
//			{
//				EachHeight=h;
//				Win->NeedUpdatePosize=1;
//				Win->NeedFreshScreen=1;
//			}
//			
//			inline void SetTreeViewFunc(void (*_func)(T&,SimpleTreeView1<T>*,int,int))
//			{func=_func;}
//			
//			int GetSubNodeCnt(int faNode,bool recursive=0)
//			{
//				faNode=EnsureInRange(faNode,-1,NodeCnt-1);
//				int re=0,subDep=faNode==-1?0:NodesData[faNode].dep+1;
//				for (int i=faNode+1;i<NodeCnt&&NodesData[i].dep>=subDep;++i)
//					if (NodesData[i].dep==subDep||recursive&&NodesData[i].dep>subDep)
//						++re;
//				return re;
//			}
//			
//			int SetSubNode(int faNode,int p,TreeViewData &data)//if faNode==-1 insert in top level(dep 0)
//			{
//				DD<<"[Info] SimpleTreeView "<<ID<<" SetSubNode "<<faNode<<" "<<p<<endl;
//				faNode=EnsureInRange(faNode,-1,NodeCnt);
//				if (faNode!=-1)
//					data.dep=NodesData[faNode].dep+1;
//				p=max(p,0);
//				for (int i=faNode+1;i<=NodeCnt;++i)
//				{
//					if (p==0||i==NodeCnt||NodesData[i].dep<data.dep)
//					{
//						NodesData.insert(NodesData.begin()+i,data);
//						++NodeCnt;
//						if (ClickChoose>=i) ++ClickChoose;
//						rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
//						fa2->ResizeLL(-1,rPS.h);
//						Win->NeedUpdatePosize=1;
//						Win->NeedFreshScreen=1;
//						Win->PresentLimit|=gPS;
//						return i;
//					}
//					if (NodesData[i].dep==data.dep)
//						--p;
//				}
//			}
//			
//			int PushbackNode(int faNode,TreeViewData &data)
//			{return SetSubNode(faNode,1e9,data);}
//			
//			int SetSubNodeMulti(int faNode,int p,vector <TreeViewData> &data)
//			{
//				DD<<"[Info] SimpleTreeView "<<ID<<" SetSubNodeMulti "<<faNode<<" "<<p<<endl;
//				faNode=EnsureInRange(faNode,-1,NodeCnt);
//				int dataDep=faNode==-1?0:NodesData[faNode].dep+1;
//				if (faNode!=-1)
//					for (int i=0;i<data.size();++i)
//						data[i].dep=dataDep;
//				p=max(p,0);
//				for (int i=faNode+1;i<=NodeCnt;++i)
//				{
//					if (p==0||i==NodeCnt||NodesData[i].dep<dataDep)
//					{
//						NodesData.insert(NodesData.begin()+i,data.begin(),data.end());
//						NodeCnt+=data.size();
//						if (ClickChoose>=i) ClickChoose+=data.size();
//						rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
//						fa2->ResizeLL(-1,rPS.h);
//						Win->NeedUpdatePosize=1;
//						Win->NeedFreshScreen=1;
//						Win->PresentLimit|=gPS;
//						return i;
//					}
//					if (NodesData[i].dep==dataDep)
//						--p;
//				}
//			}
//			
//			void DeleteSubNodeMulti(int faNode,int p,int cnt)
//			{
//				if (NodeCnt==0||cnt<=0) return;
//				DD<<"[Info] SimpleTreeView "<<ID<<" DeleteSubNodeMulti "<<faNode<<" "<<p<<" "<<cnt<<endl;
//				faNode=EnsureInRange(faNode,-1,NodeCnt-1);
//				int delL=-1,delR=-1,delDep=faNode==-1?0:NodesData[faNode].dep+1;
//				p=max(p,0);
//				for (int i=faNode+1;i<NodeCnt;++i)
//				{
//					if (p==0)
//						delL=i;
//					if (NodesData[i].dep==delDep)
//						if (p==0)
//							--cnt;
//						else --p;
//					else if (NodesData[i].dep<delDep)
//						break;
//					delR=i;
//					if (cnt==0)
//						break;
//				}
//				if (delL!=-1)
//				{
//					NodesData.erase(NodesData.begin()+delL,NodesData.begin()+delR+1);
//					NodeCnt-=delR-delL+1;
//					if (FocusChoose>=NodeCnt) FocusChoose=-1;
//					if (ClickChoose>=delR+1) ClickChoose-=delR-delL+1;
//					else if (ClickChoose>delL) ClickChoose=-1;
//					rPS.h=EnsureInRange(NodeCnt*EachHeight,fa2->GetrPS().h,1e9);
//					fa2->ResizeLL(-1,rPS.h);
//					Win->NeedUpdatePosize=1;
//					Win->NeedFreshScreen=1;
//					Win->PresentLimit|=gPS;
//				}
//			}
//			
//			void DeleteAllSubNode(int faNode)
//			{DeleteSubNodeMulti(faNode,0,GetSubNodeCnt(faNode));}
//			
//			void DeleteSubNode(int faNode,int p)
//			{DeleteSubNodeMulti(faNode,p,1);}
//			
//			void ClearAllNode()
//			{
//				if (!NodeCnt) return;
//				DD<<"[Info] SimpleTreeView "<<ID<<" ClearAllNode "<<NodeCnt<<endl;
//				FocusChoose=ClickChoose=-1;
//				NodesData.clear();
//				NodeCnt=0;
//				rPS.h=fa2->GetrPS().h;
//				fa2->ResizeLL(-1,rPS.h);
//				fa2->SetViewPort(1,0);
//				fa2->SetViewPort(2,0);
//				Win->NeedUpdatePosize=1;
//				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=gPS;
//			}
//			
//			void SetSelectNode(int p)
//			{
//				if (!InRange(p,0,NodeCnt-1))
//					return;
//				ClickChoose=p;
//				if (!InRange(p*EachHeight+fa->GetrPS().y,0,fa2->GetrPS().h-EachHeight))
//					fa2->SetViewPort(2,(p-1)*EachHeight);
//				Win->NeedUpdatePosize=1;
//				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=gPS;
//			}
//			
//			TreeViewData& GetNodeData(int p)
//			{return NodesData[EnsureInRange(p,0,NodeCnt-1)];}
//			
//			T& GetFuncData(int p)
//			{return NodesData[EnsureInRange(p,0,NodeCnt-1)].FuncData;}
//			
//			T& operator [] (int p)
//			{return NodesData[EnsureInRange(p,0,NodeCnt-1)].FuncData;}
//			
//			inline int GetNodeCnt()
//			{return NodeCnt;}
//			
//			void AddPsEx(PosizeEX *psex)
//			{fa2->AddPsEx(psex);}
//			
//			virtual void SetrPS(const Posize &ps)
//			{fa2->SetrPS(ps);}
//			
//			SimpleTreeView1(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,SimpleTreeView1<T>*,int,int)=NULL)
//			:func(_func)
//			{
//				SetID(_ID);
//				DD<<"[Info] Create SimpleTreeView "<<ID<<endl;
//				Type=WidgetType_SimpleTreeView;
//				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
//				SetFa(fa2->LargeArea());
//				rPS={0,0,_rps.w,0};
//				for (int i=0;i<=2;++i)
//					NodeColor[i]=RGBA_NONE;
//			}
//			
//			SimpleTreeView1(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,SimpleTreeView1<T>*,int,int)=NULL)
//			:func(_func)
//			{
//				SetID(_ID);
//				DD<<"[Info] Create SimpleTreeView "<<ID<<endl;
//				Type=WidgetType_SimpleTreeView;
//				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
//				SetFa(fa2->LargeArea());
//				rPS=ZERO_POSIZE;
//				for (int i=0;i<=2;++i)
//					NodeColor[i]=RGBA_NONE;
//			}
//	};
	
	template <class T> class DetailedListView:public Widgets
	{
		/*	Example:
				name  |   size   |    date    |    type    |   ....
			1	xxx			xxx			xxx			xxx
			2	xxx			xxx			xxx			xxx
		*/
		public:
			struct DetailedListViewData
			{
//				friend DetailedListView;
//				protected:
					vector <string> Text;
					vector <RGBA> SpecificTextColor;//if Empty,use the defalut Color
					SharedTexturePtr pic;
					T FuncData;
				
				public:
					DetailedListViewData() {}
			};
			
			struct DetailedListViewColumn
			{
//				friend DetailedListView;
				string TagName;
				int TextDisplayMode=0,
					Width=100;
				RGBA ColumnTextColor=RGBA_NONE;
			
				DetailedListViewColumn(const string &tagname,const int &w=100,const int &displaymode=0,const RGBA &co=RGBA_NONE)
				:TagName(tagname),TextDisplayMode(displaymode),Width(w),ColumnTextColor(co) {}
				
				DetailedListViewColumn() {}
			};
		
		protected:
			vector <DetailedListViewColumn> ColumnInfo;
			vector <DetailedListViewData> ListData;
			int ListCnt=0;
			void (*func)(T&,int,int)=NULL;//int1:Pos(CountFrom 0,-1 means background)   int2: 0:None 1:Left_Click 2:Left_Double_Click 3:Right_Click
			T BackgroundFuncData;
//			int (*CompFunc)(const string&,const string&,const string&,int)=NULL;//ItemA,ItemB,TagName,ColumnPos  //<:-1 ==:0 >:1 if ==:Sort by ColumnOrder 
			void (*SortFunc)(vector <DetailedListViewData>&,const string&,int,bool)=NULL;//TagName,ColumnPos,SortDirection
			LargeLayerWithScrollBar *fa2=NULL;
			int stat=0,//0:Up_NoFocus 1:Up_Focus_Row 2:Down_Left_TwiceClick 3:Down_Right 4:Down_Left_OnceClick 
					   //5:Up_Focus_Top 6:Down_LeftClick_Top 7:Down_RightClick_Top 8:Up_Focus_ResizeBar 9:Down_Scroll_ResizeBar
				ColumnAndResizeChoose=-1,
				FocusChoose=-1,
				ClickChoose=-1,
				ResizeColumnLeftX=0;
			int TopHeigth=30,
				TopResizeWidth=2,
				EachHeight=24,
				Interval=2,
				MainTextPos=0,
				SortByColumn=0;
			bool SortDirection=0;//0:small to big 1:big to small
			bool EnableAutoMainTextPos=1,
				 EnableCheckBox=0,
				 EnableDrag=0,
				 EnableKeyboardEvent=0,
				 EnableSerialNumber=0;
			RGBA TopResizeBarColor[3],
				 TopColumnColor[3],
				 EachRowColor[3],
				 MainTextColor=RGBA_NONE,
				 SubTextColor=RGBA_GRAY_8[5];
			
			inline int GetLineFromPos(int y)
			{
				int re=(Win->NowPos.y-gPS.y-TopHeigth)/(EachHeight+Interval);
				if ((y-gPS.y-TopHeigth)%(EachHeight+Interval)<EachHeight&&InRange(re,0,ListCnt-1))
					return re;
				else return -1;
			}
			
			virtual void CheckEvent()
			{
				
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							if (stat<=4)
								Win->PresentLimit|=GetLinePosize(FocusChoose);
							else Win->PresentLimit|=Posize(fa2->GetgPS().x,fa2->GetgPS().y,fa2->GetgPS().w,TopHeigth);
							stat=0;
							FocusChoose=-1;
							ColumnAndResizeChoose=-1;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (Win->NowPos.y-fa2->GetgPS().y<=TopHeigth)
						{
							stat=5;
							int x=Win->NowPos.x-gPS.x,s=gPS.x;
							if (FocusChoose!=-1)
								Win->PresentLimit|=GetLinePosize(FocusChoose);
							FocusChoose=ColumnAndResizeChoose=-1;
							for (int i=0;i<ColumnInfo.size();x-=ColumnInfo[i].Width,s+=ColumnInfo[i].Width,++i)
								if (x<=ColumnInfo[i].Width-TopResizeWidth)
								{
									ColumnAndResizeChoose=i;
									if (event.button.button==SDL_BUTTON_LEFT)
									{
										stat=6;
										if (SortByColumn==i)
											SortDirection=!SortDirection;
										else SortByColumn=i,SortDirection=0;
										if (EnableAutoMainTextPos)
											MainTextPos=SortByColumn;
									}
									else if (event.button.button==SDL_BUTTON_RIGHT)
										stat=7;
									break;
								}
								else if (x<=ColumnInfo[i].Width)
								{
									stat=9;
									ResizeColumnLeftX=s;
									ColumnAndResizeChoose=i;
									Win->OccupyPosWg=this;
									break;
								}
							Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TopHeigth);
						}
						else
						{
							if (ClickChoose!=-1)
								Win->PresentLimit|=GetLinePosize(ClickChoose);
							if (FocusChoose!=-1)
								Win->PresentLimit|=GetLinePosize(FocusChoose);
							ClickChoose=FocusChoose=GetLineFromPos(Win->NowPos.y);
							if (event.button.button==SDL_BUTTON_LEFT)
								if (event.button.clicks==2)
									stat=2;
								else stat=4;
							else if (event.button.button==SDL_BUTTON_RIGHT)
								stat=3;
							else stat=4,DD<<"[Waring] DetailedListView: Unknown click button,use it as left click once"<<endl;
							if (ClickChoose!=-1)
								Win->PresentLimit|=GetLinePosize(ClickChoose);
							if (FocusChoose!=-1)
								Win->PresentLimit|=GetLinePosize(FocusChoose);
						}
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							if (stat<=4)
							{
								DD<<"[Info] DetailedListView "<<ID<<" func "<<ClickChoose<<" "<<stat<<endl;
								if (func!=NULL)
									func(ClickChoose==-1?BackgroundFuncData:ListData[ClickChoose].FuncData,ClickChoose,stat==4?1:stat);
								stat=1;
							}
							else if (stat==6)
							{
								DD<<"[Info] DetailedListView "<<ID<<" Sort by "<<ColumnInfo[SortByColumn].TagName<<" "<<SortByColumn<<" "<<SortDirection<<endl;
								if (SortFunc!=NULL)
									SortFunc(ListData,ColumnInfo[SortByColumn].TagName,SortByColumn,SortDirection);
								Win->PresentLimit|=gPS;
								stat=5;
							}
							else if (stat==7)
							{
								DD<<"[Error] DetailedListView "<<ID<<" RightClick ToColumn Cannot be used yet!"<<endl;
								stat=5;
							}
							else if (stat==9)
							{
								stat=8;
								Win->OccupyPosWg=NULL;
								Win->PresentLimit|=Posize(gPS.x,gPS.y,gPS.w,TopHeigth);
							}
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
						}
						break;
					
					case SDL_MOUSEMOTION:
						if (stat==9)
						{
							ColumnInfo[ColumnAndResizeChoose].Width=EnsureInRange(Win->NowPos.x-ResizeColumnLeftX,10,1e9);
							Win->PresentLimit|=CoverLmt;
							Win->NeedFreshScreen=1;
							Win->NeedUpdatePosize=1;
						}
						else 
						{
							if (stat==0)
							{
								stat=1;
								SetNeedLoseFocus();
								Win->NeedFreshScreen=1;
							}
							
							if (InRange(Win->NowPos.y,fa2->GetgPS().y,fa2->GetgPS().y+TopHeigth))
							{
								if (FocusChoose!=-1)
								{
									Win->PresentLimit|=GetLinePosize(FocusChoose);
									FocusChoose=-1;
									Win->NeedFreshScreen=1;
								}
								
								if (stat==6||stat==7)
								{
									
								}
								else
								{
									int x=Win->NowPos.x-gPS.x,lastChoose=ColumnAndResizeChoose,lastStat=stat;
									ColumnAndResizeChoose=-1;
									stat=5;
									for (int i=0;i<ColumnInfo.size();x-=ColumnInfo[i].Width,++i)
										if (x<=ColumnInfo[i].Width-TopResizeWidth)
										{
											ColumnAndResizeChoose=i;
											stat=5;
											break;
										}
										else if (x<=ColumnInfo[i].Width)
										{
											ColumnAndResizeChoose=i;
											stat=8;
											break;
										}
									if (lastChoose!=ColumnAndResizeChoose||lastStat!=stat)
									{
										Win->PresentLimit|=Posize(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);
										Win->NeedFreshScreen=1;
									}
								}
							}
							else
							{
								if (ColumnAndResizeChoose!=-1)
								{
									ColumnAndResizeChoose=-1;
									Win->PresentLimit|=Win->PresentLimit|=Posize(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);
									Win->NeedFreshScreen=1;
								}
								
								int pos=GetLineFromPos(Win->NowPos.y);
								if (pos!=-1)
								{
									if (pos!=FocusChoose)
									{
										if (FocusChoose!=-1)
											Win->PresentLimit|=GetLinePosize(FocusChoose);
										Win->PresentLimit|=GetLinePosize(pos);
										FocusChoose=pos;
										Win->NeedFreshScreen=1;
									}
								}
								else
									if (FocusChoose!=-1)
									{
										Win->PresentLimit|=GetLinePosize(FocusChoose);
										FocusChoose=-1;
										Win->NeedFreshScreen=1;
									}
							}
						}
						Win->NeedSolvePosEvent=0;
						break;	
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (ListCnt!=0)
				{
					int ForL=(-fa->GetrPS().y)/(EachHeight+Interval),
						ForR=ForL+(fa2->GetgPS().h-TopHeigth)/(EachHeight+Interval)+1;
					ForL=EnsureInRange(ForL,0,ListCnt-1);
					ForR=EnsureInRange(ForR,0,ListCnt-1);
					if (!InRange(ClickChoose,0,ListCnt-1)) ClickChoose=-1;
					if (!InRange(FocusChoose,0,ListCnt-1)) FocusChoose=-1;
	
					Posize ps(gPS.x,gPS.y+TopHeigth+ForL*(EachHeight+Interval),0,EachHeight);
					for (int i=ForL;i<=ForR;++i)
					{
						ps.w=gPS.w;
						Win->RenderFillRect(ps&lmt,EachRowColor[ClickChoose==i?2:FocusChoose==i]?EachRowColor[ClickChoose==i?2:FocusChoose==i]:(ClickChoose==i?ThemeColor[3]:(FocusChoose==i?ThemeColor[1]:RGBA_NONE)));
						Win->Debug_DisplayBorder(ps);
						for (int j=0;j<ColumnInfo.size()&&j<=ListData[i].Text.size();++j)
						{
							ps.w=ColumnInfo[j].Width;
							Win->RenderDrawText(ListData[i].Text[j],ps,lmt,ColumnInfo[j].TextDisplayMode,j<ListData[i].SpecificTextColor.size()?ListData[i].SpecificTextColor[j]:(j==MainTextPos?(MainTextColor?MainTextColor:ThemeColor.MainTextColor[0]):SubTextColor));
							Win->Debug_DisplayBorder(ps);
							ps.x+=ps.w;
						}
						if (!!ListData[i].pic)
							Win->RenderCopyWithLmt(ListData[i].pic(),Posize(gPS.x,ps.y,EachHeight,EachHeight),lmt);
						ps.x=gPS.x;
						ps.y+=EachHeight+Interval;
					}
				}

				Posize ps(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);
				Win->Debug_DisplayBorder(ps);
				for (int i=0;i<ColumnInfo.size();++i)
				{
					ps.w=ColumnInfo[i].Width-TopResizeWidth;
					Win->RenderFillRect(lmt&ps,TopColumnColor[i==ColumnAndResizeChoose&&InRange(stat,5,7)?(stat==5?1:2):0]?TopColumnColor[i==ColumnAndResizeChoose&&InRange(stat,5,7)?(stat==5?1:2):0]:ThemeColor[i==ColumnAndResizeChoose&&InRange(stat,5,7)?(stat==5?2:4):0]);
					Win->RenderDrawText(ColumnInfo[i].TagName,ps,lmt,ColumnInfo[i].TextDisplayMode,ColumnInfo[i].ColumnTextColor?ColumnInfo[i].ColumnTextColor:ThemeColor.MainTextColor[0]);
					ps.x+=ps.w+TopResizeWidth;
					Win->RenderFillRect(lmt&Posize(ps.x-TopResizeWidth,ps.y,TopResizeWidth,ps.h),TopResizeBarColor[i==ColumnAndResizeChoose&&InRange(stat,8,9)?(stat==9?2:1):0]?TopResizeBarColor[i==ColumnAndResizeChoose&&InRange(stat,8,9)?(stat==9?2:1):0]:ThemeColor[i==ColumnAndResizeChoose&&InRange(stat,8,9)?(stat==9?5:3):1]);
				}
				Win->Debug_DisplayBorder(gPS);
			}

			virtual void CalcPsEx()//??
			{
				Posize lastPs=gPS;
				if (PsEx!=NULL)	
					PsEx->GetrPS(rPS);
				rPS.w=0;
				for (auto vp:ColumnInfo)//??
					rPS.w+=vp.Width;
				rPS.h=EnsureInRange(ListCnt*(EachHeight+Interval)-Interval+TopHeigth,fa2->GetrPS().h,1e9);
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				CoverLmt=gPS&GetFaCoverLmt();
				fa2->ResizeLL(rPS.w,rPS.h);
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
			}

		public:
			inline Posize GetLinePosize(int pos)
			{
				if (pos==-1) return ZERO_POSIZE;
				else return Posize(gPS.x,gPS.y+TopHeigth+pos*(EachHeight+Interval),gPS.w,EachHeight);
			}
			
			inline void SetTopResizeBarColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TopResizeBarColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=Posize(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);
				}
				else DD<<"[Error] DetailedListView: SetTopResizeBarColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetTopColumnColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TopColumnColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=Posize(gPS.x,fa2->GetgPS().y,gPS.w,TopHeigth);
				}
				else DD<<"[Error] DetailedListView: SetTopColumnColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetEachRowColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					EachRowColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=CoverLmt;
				}
				else DD<<"[Error] DetailedListView: SetEachRowColor: p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetMainTextColor(const RGBA &co)
			{
				MainTextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			inline void SetSubTextColor(const RGBA &co)
			{
				SubTextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			inline void SetBackgroundColor(const RGBA &co)
			{fa2->SetLargeAreaColor(co);}
			
			void SortBy(int sortbycolumn,bool sortdirection)
			{
				SortByColumn=sortbycolumn;
				SortDirection=sortdirection;
				DD<<"[Info] DetailedListView "<<ID<<" Sort by "<<ColumnInfo[SortByColumn].TagName<<" "<<SortByColumn<<" "<<SortDirection<<endl;
				if (SortFunc!=NULL)
					SortFunc(ListData,ColumnInfo[SortByColumn].TagName,SortByColumn,SortDirection);
				Win->PresentLimit|=gPS;
			}
			
			inline void SetMainTextPos(int pos)
			{
				if (MainTextPos!=pos)
				{
					MainTextPos=pos;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=CoverLmt;
				}
			}
			
			inline void SetEnableAutoMainTextPos(bool en)
			{
				EnableAutoMainTextPos=en;
				SetMainTextPos(SortByColumn);
			}

			inline void SetTopHeightAndResizeWidth(int topheight=30,int topresizewidth=2)
			{
				TopHeigth=topheight;
				TopResizeWidth=topresizewidth;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}

			inline void SetEachHeightAndInterval(int eachheight=24,int interval=2)
			{
				EachHeight=eachheight;
				Interval=interval;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			inline void SetFunc(void (*_func)(T&,int,int))
			{func=_func;}

			inline void SetBackgroundFuncdata(const T &bgdata)
			{BackgroundFuncData=bgdata;}
			
			void SetSortFunc(void (*_SortFunc)(vector <DetailedListViewData>&,const string&,int,bool))
			{SortFunc=_SortFunc;}

			void SetColumnInfo(int p,const DetailedListViewColumn &column)
			{
				p=EnsureInRange(p,0,ColumnInfo.size());
				ColumnInfo.insert(ColumnInfo.begin()+p,column);
				if (ColumnAndResizeChoose>=p) ++ColumnAndResizeChoose;
				if (MainTextPos>=p) ++MainTextPos; 
				if (SortByColumn>=p) ++SortByColumn;
				Win->NeedUpdatePosize=1;//Is it just OK?
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			void DeleteColumnInfo(int p)
			{
				if (ColumnInfo.emtpy()) return;
				p=EnsureInRange(p,0,ColumnInfo.size()-1);
				ColumnInfo.erase(ColumnInfo.begin()+p);
				if (ColumnAndResizeChoose>p) --ColumnAndResizeChoose;
				else if (ColumnAndResizeChoose==p) ColumnAndResizeChoose=-1;
				if (SortByColumn>p) --SortByColumn;
				else if (SortByColumn==p) SortByColumn=0;
				if (EnableAutoMainTextPos) MainTextPos=SortByColumn;
				else
					if (MainTextPos>p) --MainTextPos;
					else if (MainTextPos==p) MainTextPos=0;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			DetailedListView* PushbackColumnInfo(const DetailedListViewColumn &column)
			{
				SetColumnInfo(1e9,column);
				return this;
			}
			
			inline int GetColumnCnt()
			{return ColumnInfo.size();}

			void SetListContent(int p,const DetailedListViewData &info)
			{
				p=EnsureInRange(p,0,ListCnt);
				ListData.insert(ListData.begin()+p,info);
				++ListCnt;
				if (ClickChoose>=p) ++ClickChoose;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			DetailedListView* PushbackListContent(const DetailedListViewData &info)
			{
				SetListContent(1e9,info);
				return this;
			}
			
			void DeleteListContent(int p)
			{
				if (!ListCnt) return;
				p=EnsureInRange(p,0,ListCnt-1);
				ListData.erase(ListData.begin()+p);
				--ListCnt;
				if (ClickChoose>p) --ClickChoose;
				else if (ClickChoose==p) ClickChoose=-1;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;			 
			}
			
			void ClearListContent()
			{
				if (!ListCnt) return;
				DD<<"[Info] Clear DetailedListView "<<ID<<" Content: ListCnt "<<ListCnt<<endl;
				FocusChoose=ClickChoose=-1;
				ListData.clear();
				ListCnt=0;
				fa2->SetViewPort(2,0);
				Win->NeedFreshScreen=1;
				Win->NeedUpdatePosize=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			void UpdateListContent(int p,const DetailedListViewData &info)
			{
				p=EnsureInRange(p,0,ListCnt-1);
				ListData[p]=info;
				if (ClickChoose>=p) ++ClickChoose;
				Win->NeedUpdatePosize=1;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=CoverLmt;
			}
			
			inline int GetListCnt()
			{return ListCnt;}
				
			inline T& GetFuncData(int p)
			{return ListData[EnsureInRange(p,0,ListCnt-1)].FuncData;}

			inline T& operator [] (int p)
			{return ListData[EnsureInRange(p,0,ListCnt-1)].FuncData;}
			
			int Find(const T &_funcdata)
			{
				for (int i=0;i<ListCnt;++i)
					if (ListData[i]==_funcdata)
						return i;
				return -1;
			}
			
			inline DetailedListViewData& GetListData(int p)//For breakthrough class limit??
			{return ListData[EnsureInRange(p,0,ListCnt-1)];}
			
			inline DetailedListViewColumn& GetColumnData(int p)
			{return ColumnInfo[EnsureInRange(p,0,ColumnInfo.size())];}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			DetailedListView(int _ID,Widgets *_fa,const Posize &_rps,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create DetailedListView "<<ID<<endl;
				Type=WidgetType_DetailedListView;
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,TopHeigth};
				for (int i=0;i<=2;++i)
					TopResizeBarColor[i]=TopColumnColor[i]=EachRowColor[i]=RGBA_NONE;
			}
			
			DetailedListView(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(T&,int,int)=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create DetailedListView "<<ID<<endl;
				Type=WidgetType_DetailedListView;
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				for (int i=0;i<=2;++i)
					TopResizeBarColor[i]=TopColumnColor[i]=EachRowColor[i]=RGBA_NONE;
			}
	};
	
	class Menu1;
	struct MenuData
	{
		friend class Menu1;
		protected:
			int type=0;//0:none 1:normal_func 2:divide_line 3:sub_menu1(user funcdata storage the next *menudata)
			void (*func)(void*)=NULL;
			void *funcData=NULL;//Most funcData won't be auto deleted
			char hotKey=0;//case-insensitive
//			pair <SDL_Texture*,int*> pic={NULL,NULL};//pic tex and count,Pic is small that it would be deleted autoly 
			SharedTexturePtr pic;
			bool enable=1,
				 autoDeleteSubMenuData=0;
			string text;
			int subMenuW=200;
			/*
				Plan:Use union to storage data,and subMenu storage vector rather than vector's pointer which is easier to solve
			*/
			
		public:
			~MenuData()
			{
				if (autoDeleteSubMenuData)
					delete (vector <MenuData>*)(funcData);
			}
			
			MenuData(const string &_text,void (*_func)(void*),void *_funcdata,char _hotkey=0,const SharedTexturePtr &_pic=SharedTexturePtr(NULL),bool _enable=1)
			:type(1),text(_text),func(_func),funcData(_funcdata),hotKey(_hotkey),pic(_pic),enable(_enable) {}
			
			MenuData(int p)//any num is OK
			:type(2) {}
			
			MenuData(vector <MenuData> *submenu,bool autoDeleteThisVector,const string &_text,int _submenuWidth,char _hotkey=0,const SharedTexturePtr &_pic=SharedTexturePtr(NULL),bool _enable=1)
			:type(3),text(_text),autoDeleteSubMenuData(autoDeleteThisVector),subMenuW(_submenuWidth),funcData(submenu),hotKey(_hotkey),pic(_pic),enable(_enable) {}
	};
	
	class Menu1:public Widgets//This widget have many differences,it is not so easy to solve it  QAQ
	{
		protected:
			vector <MenuData> *MenuDataArray=NULL;
			int stat=0,//0:Up_NoFocus 1:Up_Focus 2:Down
				spos1=0,spos2=0,//show pos start(1) and end(2), when not exceed ,spos1 is 0, spos2 is ManudataCnt-1,means all in range
				pos=-1,//-1:no pos -2:ExceedTop -3:ExceedButtom
				subMenupos=-1,//-1 means not have subMenu
				EachHeight=20,//Each/2 is the height of exceed_button
				DivideLineHeight=3,
				BorderWidth=4,
				MenudataCnt=0;
			bool Exceed=0,
				 IsSubMenu=0,
				 autoDeleteMenuData=0,
				 autoAdjustMenuPosition=1,
				 NeedDelete=0; 
			Menu1 *rootMenu=NULL;
			SDL_Texture *triTex[2]={NULL,NULL};//enable,disable
			RGBA TextColor[2]={RGBA_NONE,RGBA_NONE},//enable,disable
				 DivideLineColor=RGBA_NONE,
				 MenuColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE};//no focus(background),focus,click
			
			void SetDelSubMenu()//create or delete subMenu
			{
				if (!InRange(pos,0,MenudataCnt-1))
				{
					if (subMenupos!=-1)
					{
						delete childWg;
						subMenupos=-1;
					}
					return;
				}
				
				if (subMenupos==-1||pos!=subMenupos)
				{
					if (pos!=subMenupos)//delete current subMenu
					{
						delete childWg;
						subMenupos=-1;
					}
					MenuData &md=(*MenuDataArray)[pos];
					if (md.type==3&&md.enable)
						if (childWg==NULL)
						{
							Point pt(rPS.w-2*BorderWidth,BorderWidth);
							if (Exceed)
								pt.y+=BorderWidth/2;
							for (int i=spos1;i<pos-1;++i)
								switch (md.type)
								{
									case 1:	case 3:	pt.y+=EachHeight;	break;
									case 2:	pt.y+=DivideLineHeight;		break;
								}
							new Menu1(0,this,pt,(vector <MenuData> *)md.funcData,md.autoDeleteSubMenuData,md.subMenuW);
							Win->NeedUpdatePosize=1;
							Win->NeedFreshScreen=1;
							subMenupos=pos;						
						}
				}
			}

			int GetPos(int y)
			{
				if (!InRange(y,BorderWidth,rPS.h-BorderWidth-1))
					return -1;
				if (Exceed)
				{
					if (InRange(y-BorderWidth,0,EachHeight/2-1))//Is top exceed_button
						return -2;
					if (InRange(rPS.h-y-BorderWidth-1,0,EachHeight/2-1))//Is buttom exceed_button
						return -3;
					y-=EachHeight/2+BorderWidth;
				}
				else y-=BorderWidth;
				for (int i=spos1;i<MenudataCnt&&y>BorderWidth&&i<=spos2;++i)
					switch ((*MenuDataArray)[i].type)
					{
						case 1:	case 3:
							if (InRange(y,0,EachHeight-1))
								return i;
							y-=EachHeight;
							break;
						case 2:
							if (InRange(y,0,DivideLineHeight-1))
								return i;
							y-=DivideLineHeight;
							break;
					}
				return -1;
			}
			
			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_KEYDOWN:
						char hotKeyPressed=-1;
						switch (event.key.keysym.sym)
						{
							case SDLK_a: hotKeyPressed='A';	break;
							case SDLK_b: hotKeyPressed='B';	break;
							case SDLK_c: hotKeyPressed='C';	break;
							case SDLK_d: hotKeyPressed='D';	break;
							case SDLK_e: hotKeyPressed='E';	break;
							case SDLK_f: hotKeyPressed='F';	break;
							case SDLK_g: hotKeyPressed='G';	break;
							case SDLK_h: hotKeyPressed='H';	break;
							case SDLK_i: hotKeyPressed='I';	break;
							case SDLK_j: hotKeyPressed='J';	break;
							case SDLK_k: hotKeyPressed='K';	break;
							case SDLK_l: hotKeyPressed='L';	break;
							case SDLK_m: hotKeyPressed='M';	break;
							case SDLK_n: hotKeyPressed='N';	break;
							case SDLK_o: hotKeyPressed='O';	break;
							case SDLK_p: hotKeyPressed='P';	break;
							case SDLK_q: hotKeyPressed='Q';	break;
							case SDLK_r: hotKeyPressed='R';	break;
							case SDLK_s: hotKeyPressed='S';	break;
							case SDLK_t: hotKeyPressed='T';	break;
							case SDLK_u: hotKeyPressed='U';	break;
							case SDLK_v: hotKeyPressed='V';	break;
							case SDLK_w: hotKeyPressed='W';	break;
							case SDLK_x: hotKeyPressed='X';	break;
							case SDLK_y: hotKeyPressed='Y';	break;
							case SDLK_z: hotKeyPressed='Z';	break;
							
							case SDLK_ESCAPE:
								SetDelayDeleteThis();
								Win->NeedSolveEvent=0;
								break;
							
							case SDLK_UP:
								stat=1;
								pos=(pos-1+MenudataCnt)%MenudataCnt;
								if ((*MenuDataArray)[pos].type==2)//Only once because normally there are not two divideline near
									pos=(pos-1+MenudataCnt)%MenudataCnt;
								Win->NeedFreshScreen=1;
								Win->NeedSolveEvent=0;
								break;
							
							case SDLK_DOWN:
								stat=1;
								pos=(pos+1+MenudataCnt)%MenudataCnt;
								if ((*MenuDataArray)[pos].type==2)
									pos=(pos+1+MenudataCnt)%MenudataCnt;
								Win->NeedFreshScreen=1;
								Win->NeedSolveEvent=0;
								break;
							
							case SDLK_LEFT:
								if (rootMenu!=this)
								{
									SetDelayDeleteThis();
									Win->NeedSolveEvent=0;
								}
								break;
							
							case SDLK_RIGHT:
								if (InRange(pos,0,MenudataCnt-1))
									if ((*MenuDataArray)[pos].type==3)
									{
										SetDelSubMenu();
										Win->NeedSolveEvent=0;
									}
								break;
							
							case SDLK_RETURN:
							case SDLK_SPACE:
								if (InRange(pos,0,MenudataCnt-1))
								{
									MenuData &md=(*MenuDataArray)[pos];
									if (md.enable)
									{
										if (md.type==1)
										{
											if (md.func!=NULL)
												md.func(md.funcData);
											SetDelayDeleteWidget(rootMenu);
										}
										else if (md.type==3)
											SetDelSubMenu();
										Win->NeedFreshScreen=1;
										Win->NeedSolveEvent=0;
									}
								}
								break;
						}
						if (hotKeyPressed!=-1)
						{
							for (int i=0;i<MenudataCnt;++i)
							{
								char ch=(*MenuDataArray)[i].hotKey;
								if (InRange(ch,'a','z'))
									ch+='A'-'a';
								if (ch==hotKeyPressed)
								{
									pos=i;
									MenuData &md=(*MenuDataArray)[i];
									if (md.enable)
										if (md.type==1)
										{
											if (md.func!=NULL)
												md.func(md.funcData);
											SetDelayDeleteWidget(rootMenu);
										}
										else if (md.type==3)
											SetDelSubMenu();
									Win->NeedFreshScreen=1;
									Win->NeedSolveEvent=0;
									break;
								}
							}
							if (Win->NeedSolveEvent)
								cout<<'\a';
						}
						break;
				}
			}

			virtual void CheckPos()//There still exists some bugs I think,I will fix it when found
			{
//				if (!Win->NeedSolvePosEvent||MenuDataArray==NULL) return;
				if (MenuDataArray==NULL) return;
				SDL_Event &event=*Win->NowSolvingEvent;
				Posize LMT=Win->WinPS.ToOrigin();
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						DD<<"[Info] Menu1 "<<ID<<" click"<<endl;
						if ((gPS&LMT).In(Win->NowPos))
						{
							if (event.button.button==SDL_BUTTON_LEFT)
								if (gPS.Shrink(BorderWidth).In(Win->NowPos))
								{
									stat=2;
									pos=GetPos(Win->NowPos.y-gPS.y);
									Win->NeedFreshScreen=1;
								}
								Win->NeedSolvePosEvent=0;
						}
						else
						{
							stat=0;
							Win->NeedFreshScreen=1;
//							delete this;
							SetDelayDeleteThis();
						}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2&&Win->NeedSolvePosEvent)
						{
							DD<<"[Info] Menu1 "<<ID<<" function"<<endl;
							if (gPS.Shrink(BorderWidth).In(Win->NowPos))
							{
								stat=1;
								pos=GetPos(Win->NowPos.y-gPS.y);
								SetDelSubMenu();
								Win->NeedFreshScreen=1;
								Win->NeedSolvePosEvent=0; 
								if (InRange(pos,0,MenudataCnt-1))
								{
									MenuData &md=(*MenuDataArray)[pos];
									if (md.type==1&&md.enable)
									{
										if (md.func!=NULL)
											md.func(md.funcData);
//										delete rootMenu;
										SetDelayDeleteWidget(rootMenu);
									}
								}
								else if (pos==-2)
								{
									DD<<"[Error] not usable yet"<<endl;
								}
								else if (pos==-3)
								{
									DD<<"[Error] not usable yet"<<endl;
								}
							}
						}
						break;
					
					case SDL_MOUSEMOTION:
						if ((gPS&LMT).In(Win->NowPos))
						{
							int pos_r=pos;
							pos=GetPos(Win->NowPos.y-gPS.y);
							if (pos!=pos_r)
							{
//								stat=InRange(pos,0,MenudataCnt-1);
								stat=1;
								SetDelSubMenu();
								Win->NeedFreshScreen=1;
							}
							Win->NeedSolvePosEvent=0;
						}
						else
						{
							if (stat==1)
								Win->NeedFreshScreen=1;
							stat=0;
							pos=-1;
						}
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (MenuDataArray==NULL) return;
				
				lmt=Win->WinPS.ToOrigin();
				Win->RenderFillRect(gPS.Shrink(1)&lmt,MenuColor[0]?MenuColor[0]:ThemeColor.BackgroundColor[0]);
				Win->RenderDrawRectWithLimit(gPS,MenuColor[stat]?MenuColor[stat]:ThemeColor[stat*2],lmt);
				int y=BorderWidth;
				if (Exceed)
				{

				}
				
				for (int i=spos1;i<MenudataCnt;++i)
				{
					if (y+EachHeight>rPS.h-BorderWidth)
						break;
					MenuData &md=(*MenuDataArray)[i];
					switch (md.type)
					{
						case 1:
							if (stat!=0&&pos==i)
								Win->RenderFillRect((Posize(BorderWidth,y,rPS.w-BorderWidth*2,EachHeight)+gPS)&lmt,MenuColor[stat]?MenuColor[stat]:ThemeColor[stat*2]);
							if (md.pic()!=NULL)
								Win->RenderCopyWithLmt(md.pic(),Posize(BorderWidth,y,EachHeight,EachHeight).Shrink(2)+gPS,lmt);
							Win->RenderDrawText(md.hotKey!=0?md.text+"("+md.hotKey+")":md.text,Posize(BorderWidth+EachHeight+8,y,rPS.w-EachHeight-8-BorderWidth*2,EachHeight)+gPS,lmt,-1,TextColor[!md.enable]?TextColor[!md.enable]:ThemeColor.MainTextColor[!md.enable]);
							y+=EachHeight;
							break;
						case 2:
							Win->RenderFillRect((Posize(BorderWidth+EachHeight+4,y+DivideLineHeight/3,rPS.w-BorderWidth*2-EachHeight-4,max(DivideLineHeight/3,1))+gPS)&lmt,DivideLineColor?DivideLineColor:ThemeColor[1]);
							y+=DivideLineHeight;
							break;
						case 3:
							if (stat!=0&&pos==i)
								Win->RenderFillRect((Posize(BorderWidth,y,rPS.w-BorderWidth*2,EachHeight)+gPS)&lmt,MenuColor[stat]?MenuColor[stat]:ThemeColor[stat*2]);
							if (md.pic()!=NULL)
								Win->RenderCopyWithLmt(md.pic(),Posize(BorderWidth,y,EachHeight,EachHeight).Shrink(2)+gPS,lmt);
							Win->RenderDrawText(md.hotKey!=0?md.text+"("+md.hotKey+")":md.text,Posize(BorderWidth+EachHeight+8,y,rPS.w-EachHeight-8-BorderWidth*2-EachHeight/2,EachHeight)+gPS,lmt,-1,TextColor[!md.enable]?TextColor[!md.enable]:ThemeColor.MainTextColor[!md.enable]);
							if (triTex[!md.enable]==NULL)
								triTex[!md.enable]=CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(EachHeight/4,EachHeight/2,Point(0,0),Point(0,EachHeight/2-1),Point(EachHeight/4-1,EachHeight/4),TextColor[!md.enable]?TextColor[!md.enable]:ThemeColor.MainTextColor[!md.enable]));
							Win->RenderCopyWithLmt(triTex[!md.enable],Posize(rPS.w-BorderWidth*2-EachHeight/4,y+EachHeight/4,EachHeight/4,EachHeight/2)+gPS,lmt);
							y+=EachHeight;
							break;
					}
					spos2=i;
				}
				
				if (Exceed)
				{
					
				}

				Win->Debug_DisplayBorder(gPS);
			}

			int GetHFromData()
			{
				int re=BorderWidth*2;
				if (MenuDataArray!=NULL)
					for (int i=0;i<MenudataCnt;++i)//??
						switch ((*MenuDataArray)[i].type)
						{
							case 1: case 3:	re+=EachHeight;	break;
							case 2:	re+=DivideLineHeight;	break;
						}
				return re;
			}
			
			Menu1(int _ID,Menu1 *_faMenu1,const Point &pt,vector <MenuData> *menudata,bool autodeleteData,int w=200)
			{
				SetID(_ID);
				DD<<"[Info] Create (Sub)Menu1 "<<ID<<endl;
				Type=WidgetType_Menu1;
				SetFa(_faMenu1);
				SetMenuData(menudata,autodeleteData);
				SetrPS((Win->WinPS.ToOrigin()-fa->GetgPS()).Shrink(1).EnsureIn({pt.x,pt.y,w,GetHFromData()}));
				LimitPosIngPS=0;
				IsSubMenu=1;
				rootMenu=_faMenu1->rootMenu;
			}
			
		public:
			void SetTextColor(bool p,const RGBA &co)
			{
				TextColor[p]=co;
				Win->NeedFreshScreen=1;
				if (triTex[p]!=NULL)
					SDL_DestroyTexture(triTex[p]),triTex[p]=NULL;
			}

			inline void SetDivideLineColor(const RGBA &co)
			{
				DivideLineColor=co;
				Win->NeedFreshScreen=1;
			}

			inline void SetMenuColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
					MenuColor[p]=co,Win->NeedFreshScreen=1;
				else DD<<"[Error] Menu1: SetMenuColor: p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			void SetEachHeight(int h)
			{
				EachHeight=h;
				GetHFromData();
				if (triTex[0]!=NULL)
					SDL_DestroyTexture(triTex[0]),triTex[0]=NULL;
				if (triTex[1]!=NULL)
					SDL_DestroyTexture(triTex[1]),triTex[1]=NULL;		
				Win->NeedFreshScreen=1;
			}
			
			void SetDivideLineHeight(int h)
			{
				DivideLineHeight=h;
				GetHFromData();
				Win->NeedFreshScreen=1;
			}
			
			void SetBorderWidth(int w)
			{
				BorderWidth=w;
				GetHFromData();
				Win->NeedFreshScreen=1;
			}

			void SetMenuData(vector <MenuData> *data,bool autoDelete=1)
			{
				if (data==NULL) return;
				if (autoDeleteMenuData)
					if (MenuDataArray!=NULL)
						delete MenuDataArray;
				MenuDataArray=data;
				autoDeleteMenuData=autoDelete;
				MenudataCnt=MenuDataArray->size();
				rPS.h=GetHFromData();
				Win->NeedFreshScreen=1;
			}
			
			virtual ~Menu1()
			{
				DD<<"[Info] Delete Menu1 "<<ID<<endl;
				if (autoDeleteMenuData)
					delete MenuDataArray;
				if (IsSubMenu)
					((Menu1*)fa)->subMenupos=-1;
			}
			
			Menu1(int _ID,vector <MenuData> *menudata,bool autodeleteData,int w=256,PUI_Window *_win=CurrentWindow)
			{
				SetID(_ID);
				DD<<"[Info] Create Menu1 "<<ID<<endl;
				Type=WidgetType_Menu1;
				SetFa(_win->MenuLayer);
				SetMenuData(menudata,autodeleteData);
				SetrPS(Win->WinPS.ToOrigin().EnsureIn({Win->NowPos.x,Win->NowPos.y,w,GetHFromData()}));
				LimitPosIngPS=0;
				rootMenu=this;
			}
	};
	
	class MessageBox1:public Widgets
	{
		protected:
			string MessageBoxTitle,
				   MessageBoxInfo;
			int ButtonCount,
				Preset=0;//0: not use preset 1:   2:   ...
			RGBA BackgroundColor,
				 TextColor;
			
			
			
		public:
			
			
	};
	
	class TitleMenu:public Widgets
	{
		protected:
			struct TabLayerData
			{
				Layer *lay=NULL;
				string title;
				RGBA TabColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},//none,focus/current,click
					 TextColor=RGBA_NONE;
				int TabWidth=50;
				SharedTexturePtr pic;
				
				TabLayerData() {}
				
				TabLayerData(Layer *_lay,const string &_title,const SharedTexturePtr &_pic)
				:lay(_lay),title(_title),pic(_pic) {}
			};
			
			vector <TabLayerData> EachTabLayer;
			int TabCnt=0;
			int stat=0,//0:none 1:focus 2:leftclick 3:rightclick
				TabHeight=24,
				LeftMostShowLayer=0,
				FocusingPos=-1,
				CurrentShowLayerPos=-1;//-1:means no layer
			Range TabLayerWidthRange={100,400};
			bool ShowCloseX=0,
				 EnableDrag=0,
				 EnableTabScrollAnimation=0,
				 EnableSwitchGradientAnimation=0;
			RGBA TabBarBackgroundColor=RGBA_NONE;
			void (*RigthClickFunc)(void*,int)=NULL;
			void *funcData=NULL; 
		
		
		public:
			
			
			
	};
	
	template <class T> class AddressSection:public Widgets
	{
		public:
			struct AddressSectionData
			{
				friend class AddressSection <T>;
				protected:
					string text;
					int w=0;
				
				public:
					T data;
					
					void SetText(const string &str)
					{
						text=str;
						TTF_SizeUTF8(PUI_Font(),text.c_str(),&w,NULL);
						w+=6;
					}
					
					inline const string GetText()
					{return text;}
					
					AddressSectionData(const string &str,const T &da)
					:text(str),data(da)
					{
						TTF_SizeUTF8(PUI_Font(),text.c_str(),&w,NULL);
						w+=6;
					}
			};
			
		protected:
			//  |>|XXX|<|YYYYYYYYY|<|ZZZZ|
			//Defualt used for FileAddress(Section)
			
			int stat=0;//0:Up_NoFocus 1:Up_Focus i 2:LeftClick i
			vector <AddressSectionData> AddrSecData;
			void (*func)(void*,AddressSection*,T&,int)=NULL;//funcData,this,data,NowFocus
			void *funcData=NULL;
			int SectionCnt=0,
				JointWidth=14,
				NowFocus=0;//0:not in +i:in i -i:in i list     //Do remember +-1 when using AddrSecData
			SharedTexturePtr TriangleTex[3][2];
			RGBA BackgroundColor[6],//0:no_focus_Joint 1:focus_Joint 2:down_Joint 3:no_focus_Section 4:focus_Section 5:down_Section
				 JointTriangleColor[3],//0:no_focus_Joint 1:focus_Joint 2:down_Joint
				 TextColor=RGBA_NONE;
			
			int GetChosenSection(int x)
			{
				if (SectionCnt==0) return 0;
				int p=SectionCnt-1,w=0;
				while (p>=0&&w<=gPS.w)
					w+=AddrSecData[p--].w+JointWidth;
				p++;
				while (p<SectionCnt&&x>AddrSecData[p].w+JointWidth)
					x-=AddrSecData[p++].w+JointWidth;
				if (p==SectionCnt) return 0;
				else if (x<=JointWidth) return -p-1;
				else return p+1;
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (stat!=0)
						if (!CoverLmt.In(Win->NowPos))
						{
							stat=0;
							NowFocus=0;
							RemoveNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						if (event.button.button==SDL_BUTTON_LEFT)
							if ((NowFocus=GetChosenSection(Win->NowPos.x-gPS.x))!=0)
							{
								DD<<"[Info] AddressSection "<<ID<<" click "<<NowFocus<<endl;
								stat=2;
								Win->NeedSolvePosEvent=0;
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
						break;
					
					case SDL_MOUSEBUTTONUP:
						if (stat==2)
						{
							DD<<"AddressSection "<<ID<<" function "<<NowFocus<<endl;
							if (func!=NULL)
								func(funcData,this,AddrSecData[abs(NowFocus)-1].data,NowFocus);
							stat=1;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
							Win->NeedSolvePosEvent=0;
						}
						break;
						
					case SDL_MOUSEMOTION:
						int lastFocus=NowFocus;
						if ((NowFocus=GetChosenSection(Win->NowPos.x-gPS.x))!=lastFocus)
						{
							if (NowFocus==0)
							{
								stat=0;
								RemoveNeedLoseFocus();
							}
							else
							{
								stat=1;
								SetNeedLoseFocus();
							}
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						if (NowFocus!=0)
							Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
				if (SectionCnt!=0)
				{
					int p=SectionCnt-1,w=0;
					while (p>=0&&w<=gPS.w)
						w+=AddrSecData[p--].w+JointWidth;
					Posize ps(gPS.x,gPS.y,0,gPS.h);
					for (int i=p+1;i<SectionCnt;++i)
					{
						ps.w=JointWidth;
						int col=stat!=0&&-NowFocus-1==i?stat:0;
						Win->RenderFillRect(ps&lmt,BackgroundColor[col]?BackgroundColor[col]:ThemeColor[2*col+1]);
						Win->RenderCopyWithLmt(TriangleTex[col][i==0](),ps,lmt);
						ps.x+=ps.w;
						
						ps.w=AddrSecData[i].w;
						col=stat!=0&&NowFocus-1==i?stat:0;
						Win->RenderFillRect(ps&lmt,BackgroundColor[col+3]?BackgroundColor[col+3]:ThemeColor[2*(col+1)]);
						Win->RenderDrawText(AddrSecData[i].text,ps,lmt,0,TextColor?TextColor:ThemeColor.MainTextColor[0]);
						ps.x+=ps.w;
					}
				}

				Win->Debug_DisplayBorder(gPS);
			}
			
			void ReCreateTriangleTex()
			{
				if (rPS.h<=0||JointWidth<=0) return;
				for (int i=0;i<=2;++i)
				{
					TriangleTex[i][0]=TriangleTex[i][1]=SharedTexturePtr(NULL);
					TriangleTex[i][0]=SharedTexturePtr(CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(JointWidth,gPS.h,{3,gPS.h>>1},{JointWidth-3,4},{JointWidth-3,gPS.h-4},JointTriangleColor[i])));
					TriangleTex[i][1]=SharedTexturePtr(CreateTextureFromSurfaceAndDelete(CreateTriangleSurface(JointWidth,gPS.h,{3,4},{3,gPS.h-4},{JointWidth-3,gPS.h>>1},JointTriangleColor[i])));
				}
			}
			
		public:
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,5))
				{
					BackgroundColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] SetBackgroundColor error : p "<<p<<" is not in range[0,5]"<<endl;
			}
			
			void SetJointTriangleColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					JointTriangleColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
					ReCreateTriangleTex();
				}
				else DD<<"[Error] SetJointTriangleColor error : p "<<p<<" is not in range[0,2]"<<endl;
			}
			
			inline void SetTextColor(const RGBA &co)
			{
				TextColor=co;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void Clear()
			{
				AddrSecData.clear();
				SectionCnt=0;
				stat=0;
				NowFocus=0;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			AddressSection* PushbackSection(const string &str,const T &da)
			{
				AddrSecData.push_back(AddressSectionData(str,da));
				SectionCnt++;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
				return this;
			}
			
			void Popback()
			{
				AddrSecData.pop_back();
				SectionCnt--;
				if (abs(NowFocus)>SectionCnt)
					NowFocus=0;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void PopbackMulti(int cnt)
			{
				if (cnt<=0) return;
				cnt=min(cnt,SectionCnt);
				AddrSecData.erase(AddrSecData.begin()+SectionCnt-cnt,AddrSecData.end());
				SectionCnt-=cnt;
				if (abs(NowFocus)>SectionCnt)
					NowFocus=0;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			T& GetSectionData(int p)
			{return AddrSecData[p].data;}
			
			inline int GetSectionCnt()
			{return SectionCnt;}
			
			void SetJointWidth(int w)
			{
				if (JointWidth==w)
					return;
				JointWidth=w;
				ReCreateTriangleTex();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetFunc(void (*_func)(void*,AddressSection*,T&,int),void *_funcData=NULL)
			{
				func=_func;
				funcData=_funcData;
			}
			
			AddressSection(int _ID,Widgets *_fa,const Posize &_rPS,void (*_func)(void*,AddressSection*,T&,int)=NULL,void *_funcData=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create AddressSection "<<ID<<endl;
				Type=WidgetType_AddressSection;
				SetFa(_fa);
				SetrPS(_rPS);
				for (int i=0;i<=2;++i)
					for (int j=0;j<=1;++j)
						TriangleTex[i][j]=SharedTexturePtr(NULL);
				for (int i=0;i<=6;++i)
					BackgroundColor[i]=RGBA_NONE;
				JointTriangleColor[0]={250,250,250,255};
				JointTriangleColor[1]={240,240,240,255};
				JointTriangleColor[2]={230,230,230,255};
				ReCreateTriangleTex();
			}
			
			AddressSection(int _ID,Widgets *_fa,PosizeEX *psex,void (*_func)(void*,AddressSection*,T&,int)=NULL,void *_funcData=NULL)
			:func(_func)
			{
				SetID(_ID);
				DD<<"[Info] Create AddressSection "<<ID<<endl;
				Type=WidgetType_AddressSection;
				SetFa(_fa);
				AddPsEx(psex);
				for (int i=0;i<=2;++i)
					for (int j=0;j<=1;++j)
						TriangleTex[i][j]=SharedTexturePtr(NULL);
				for (int i=0;i<=6;++i)
					BackgroundColor[i]=RGBA_NONE;
				JointTriangleColor[0]={250,250,250,255};
				JointTriangleColor[1]={240,240,240,255};
				JointTriangleColor[2]={230,230,230,255};
				ReCreateTriangleTex();
			}
	};

	#ifdef PAL_BASICFUNCTIONS_FILE_CPP
	class FileAddressSection:public AddressSection <string>
	{
		protected:
			void (*func0)(void*,const string&)=NULL;
			void *funcdata0=NULL;
			
			struct FileAddrSecFuncForMenuData
			{
				FileAddressSection *FileAddrSec=NULL;
				string path,name;
				int pos;
				
				FileAddrSecFuncForMenuData(FileAddressSection *_FileAddrSec,const string &_path,const string &_name,int _pos)
				:FileAddrSec(_FileAddrSec),path(_path),name(_name),pos(_pos) {}
			};
			
			static void FileAddrSecFuncForMenu(void *_funcdata)
			{
//				DD<<"[Debug] FileAddrSecFuncForMenu"<<endl;
				FileAddrSecFuncForMenuData *data=(FileAddrSecFuncForMenuData*)_funcdata;
				if (data->pos==1)
				{
					data->FileAddrSec->Clear();
					data->FileAddrSec->PushbackSection(data->name,data->name);
					if (data->FileAddrSec->func0!=NULL)
						data->FileAddrSec->func0(data->FileAddrSec->funcdata0,data->name);
				}
				else
				{
					data->FileAddrSec->PopbackMulti(data->FileAddrSec->GetSectionCnt()-data->pos+1);
					data->FileAddrSec->PushbackSection(data->name,data->path+"\\"+data->name);
					if (data->FileAddrSec->func0!=NULL)
						data->FileAddrSec->func0(data->FileAddrSec->funcdata0,data->path+"\\"+data->name);
				}
				delete data;
			}
			
			static void FileAddrSecFunc(void *_funcdata,AddressSection <string> *addrSec,string &path,int pos)
			{
//				DD<<"[Debug] FileAddrSecFunc. pos "<<pos<<endl;
				FileAddressSection *FileAddrSec=(FileAddressSection*)_funcdata;
				if (pos>0)
				{
					addrSec->PopbackMulti(addrSec->GetSectionCnt()-pos);
					if (FileAddrSec->func0!=NULL)
						FileAddrSec->func0(FileAddrSec->funcdata0,path);
				}
				else
				{
					vector <MenuData> *menudata=new vector<MenuData>;
					if (pos==-1)
					{
						for (char panfu[3]="C:";panfu[0]<='Z';++panfu[0])
							(*menudata).push_back(MenuData(panfu,FileAddrSecFuncForMenu,new FileAddrSecFuncForMenuData(FileAddrSec,"",panfu,1)));
					}
					else
					{
						string pa=GetPreviousBeforeBackSlash(path);
						vector <string>* allDir=GetAllFile_UTF8(pa,0);
						for (auto vp:*allDir)
							(*menudata).push_back(MenuData(vp,FileAddrSecFuncForMenu,new FileAddrSecFuncForMenuData(FileAddrSec,pa,vp,-pos)));
						delete allDir;
					}
					new Menu1(0,menudata,1);
				}
			}
			
		public:
			inline void SetFunc(void (*_func0)(void*,const string&),void *_funcdata0=NULL)
			{
				func0=_func0;
				funcdata0=_funcdata0;
			}
			
			void SetAddress(const string &path)
			{
				Clear();
				int p=0,q=0;
				while (q<path.length())
				{
					if (path[q]=='\\')
					{
						PushbackSection(path.substr(p,q-p),path.substr(0,q));
						p=q+1;
					}
					++q;
				}
				PushbackSection(path.substr(p,q-p),path);
			}
			
			FileAddressSection(int _ID,Widgets *_fa,const Posize &_rps,const string &initPath,void (*_func0)(void*,const string&)=NULL,void *_funcdata0=NULL)
			:AddressSection(_ID,_fa,_rps),func0(_func0),funcdata0(_funcdata0)
			{
				DD<<"[Info] Create FileAddressSection "<<ID<<endl;
				Type=WidgetType_FileAddressSection;
				AddressSection<string>::SetFunc(FileAddrSecFunc,this);
				SetAddress(initPath);
			}
			
			FileAddressSection(int _ID,Widgets *_fa,PosizeEX *psex,const string &initPath,void (*_func0)(void*,const string&)=NULL,void *_funcdata0=NULL)
			:AddressSection(_ID,_fa,psex),func0(_func0),funcdata0(_funcdata0)
			{
				DD<<"[Info] Create FileAddressSection "<<ID<<endl;
				Type=WidgetType_FileAddressSection;
				AddressSection<string>::SetFunc(FileAddrSecFunc,this);
				SetAddress(initPath);
			}
	};
	#endif
	
//	class SpinBox:public Widgets
//	{
//		protected:
//			
//			
//		public:
//			
//			
//			
//	};
	
	class SimpleTextBox:public Widgets//Cannot edit
	{
			
			
			
			
	};
	
	class TextEditLine:public Widgets
	{
		protected:
			stringUTF8 Text,
					   editingText;
			vector <int> ChWidth;
//			vector <RGBA> ChColor;
			bool Editing=0,
				 IntervalTimerOn=0,
				 ShowBorder=1;
			int stat=0,//0:no focus 1:focus 2:down_left 3:down_right
				StateInput=0,
				ChHeight=0,
				SumWidth=0,
				BorderWidth=3,
				LengthLimit=128,
				EditingTextCursorPos; 
//				LastEditingTimeStamp=0;
			Range pos={0,0},//this condition l>r is allowed //pos1:pos.l pos2:pos.r(also means current cursor)
				  ShowPos={0,0};
//			Uint32 UpdateSposInterval=300;
//			SDL_TimerID IntervalTimerID;
			void (*EnterFunc)(void*,const stringUTF8&,bool)=NULL;//bool:0:change 1:enter
			void *funcData=NULL;
			RGBA TextColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 BackgroundColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE},//bg,choosepart stat0,1,2/3
				 BorderColor[5]={RGBA_NONE,RGBA_NONE,RGBA_NONE,{255,0,0,200}};//stat0,1,2/3 overlimit
			
//			void TurnOnOffIntervalTimer(bool on)
//			{
//				if (IntervalTimerOn==on) return;
//				if (IntervalTimerOn=on)
//					IntervalTimerID=SDL_AddTimer(UpdateSposInterval,PUI_UpdateTimer,new PUI_UpdateTimerData(this,-1));
//				else
//				{
//					SDL_RemoveTimer(IntervalTimerID);
//					IntervalTimerID=0;
//				}
//			}
			
			void SetSposFromAnother(bool fromSposL)//used when one edge of the showPos is set,to calc the other side
			{
				int i,s;
				if (fromSposL)
				{
					for (i=ShowPos.l,s=BorderWidth;i<Text.length()&&s<=rPS.w-BorderWidth;++i)
						s+=ChWidth[i];
					if (s<=rPS.w-BorderWidth) ShowPos.r=Text.length();
					else ShowPos.r=i-1;
				}
				else
				{
					for (i=ShowPos.r-1,s=rPS.w-BorderWidth;i>=0&&s>=BorderWidth;--i)
						s-=ChWidth[i];
					if (s>=BorderWidth) ShowPos.l=0;
					else ShowPos.l=i+2;
				}
			}
		
			void SetSposFromPos2()//adjust ShowPos to ensure pos2 in ShowPos
			{
				if (pos.r<ShowPos.l)
				{
					ShowPos.l=pos.r;
					SetSposFromAnother(1);
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else if (ShowPos.r<pos.r)
				{
					ShowPos.r=pos.r;
					SetSposFromAnother(0);
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
			}
			
			int GetPos(int x)//return char interval index:  0,ch0,1,ch1,2,ch2,3
			{
//				LastPos2X=x;
				if (x<BorderWidth)
				{
					for (int s=BorderWidth,i=0,j=ShowPos.l;j>=0;--j)
					{
						if (x>s-ChWidth[j]/2) return i+ShowPos.l;
						s-=ChWidth[j];
						--i;
						if (x>=s) return max(0,i+ShowPos.l);
					}
					return 0;
				}
				else
				{
					for (int s=BorderWidth,i=0,j=ShowPos.l;j<ChWidth.size();++j)
					{
						if (x<s+ChWidth[j]/2) return i+ShowPos.l;
						s+=ChWidth[j];
						++i;
						if (x<=s) return i+ShowPos.l;
					}
					return Text.length();
				}
			}
		public:
			void ClearText(bool triggerFunc=1)
			{
				SumWidth=0;
				ChWidth.clear();
				Text.clear();
				pos=ShowPos=ZERO_RANGE;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,0);
			}
			
			int AddText(int p,const stringUTF8 &strUtf8)//p means the interval index,it will be auto Ensured in range[0,Text.length()]
			{
				p=EnsureInRange(p,0,Text.length());
				int len=min(LengthLimit-Text.length(),strUtf8.length());
				Text.insert(p,strUtf8,0,len);
				vector <int> chw;
				for (int i=0,w,h;i<len;++i)
				{
					TTF_SizeUTF8(PUI_Font(),strUtf8[i].c_str(),&w,&h);
					chw.push_back(w);
					SumWidth+=w;
					ChHeight=max(ChHeight,h);
				}
				ChWidth.insert(ChWidth.begin()+p,chw.begin(),chw.end());
				pos.l=pos.r=p+len;
				SetSposFromPos2();
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,0);
			}
			
			void SetText(const stringUTF8 &strUtf8)
			{
				ClearText(0);
				Text=strUtf8;
				if (Text.length()>LengthLimit)
					Text.erase(LengthLimit,Text.length()-LengthLimit);
				for (int i=0,w,h;i<Text.length();++i)
				{
					TTF_SizeUTF8(PUI_Font(),Text[i].c_str(),&w,&h);
					ChWidth.push_back(w);
					SumWidth+=w;
					ChHeight=max(ChHeight,h);
				}
				pos.l=pos.r=Text.length();
//				SetSposFromPos2();//??
//				if (SumWidth>rPS.w-BorderWidth*2)
//				{
//					ShowPos.r=Text.length();
//					SetSposFromAnother(0);
//				}
//				else 
//				{
					ShowPos.l=0;
					SetSposFromAnother(1);
//				}
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,0);
			}
			
			void append(const stringUTF8 &strUtf8)
			{AddText(Text.length(),strUtf8);}
			
			void DeleteText(int p,int len)
			{
				for (int i=p;i<p+len;++i)
					SumWidth-=ChWidth[i];
				ChWidth.erase(ChWidth.begin()+p,ChWidth.begin()+p+len);
				Text.erase(p,len);
				pos.l=pos.r=p;
				if (ShowPos.r>Text.length())//??next 8 lines?
					ShowPos.r=Text.length();
				SetSposFromPos2();
				if (ShowPos.l>Text.length())
				{
					ShowPos.r=Text.length();
					SetSposFromAnother(0);
				}
				if (EnterFunc!=NULL)
					EnterFunc(funcData,Text,0);
			}
			
			void DeleteTextBack()
			{
				if (Text.length()==0)
					DD<<"[Error] TextEditLine: "<<ID<<" DeleteTextBack error: Text is empty"<<endl;
				else DeleteText(Text.length()-1,1);
			}
			
			void DeleteTextCursor()
			{
				if (pos.Len0())
					if (pos.r==0) return;
					else DeleteText(pos.r-1,1);
				else DeleteText(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			void AddTextCursor(const stringUTF8 &strUtf8)
			{
				if (!pos.Len0())
					DeleteTextCursor();
				AddText(pos.r,strUtf8);
			}
			
			void SetCursorPos(int p)
			{
				pos.l=pos.r=p;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			stringUTF8 GetSelectedTextUTF8()
			{
				if (pos.Len0()) return stringUTF8();
				else return Text.substrUTF8(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			string GetSelectedText()
			{
				if (pos.Len0()) return "";
				else return Text.substr(min(pos.l,pos.r),abs(pos.Length()));
			}
			
			const stringUTF8& GetTextUTF8()
			{return Text;}
			
			string GetText()
			{return Text.cppString();}
			
		protected:
			virtual void ReceiveKeyboardInput()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_TEXTINPUT:
						{
							string str=event.text.text;
							GetRidOfEndChar0(str);
							if (!str.empty())
								AddTextCursor(stringUTF8(str));
							Win->NeedSolveEvent=0;
						}
						break;
					
					case SDL_TEXTEDITING:
						DD<<"[Info] TextEditLine: "<<ID<<" Editing text: start "<<event.edit.start<<", len "<<event.edit.length<<" "<<event.edit.length<<endl;
						editingText=event.edit.text;
						EditingTextCursorPos=event.edit.start;
//						LastEditingTimeStamp=event.edit.timestamp;
						Win->NeedSolveEvent=0; 
						
						if (!pos.Len0())
							DeleteTextCursor();					
						break;
				}
				Editing=editingText.length()!=0;
//				if (!Editing) 
//					DD<<"[Info] TextEditLine: "<<ID<<" Not editing"<<endl; 
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_KEYDOWN:
						if (StateInput)
							if (!Editing)
							{
//								if (event.key.timestamp-LastEditingTimeStamp>0)//?? 
									switch (event.key.keysym.sym)
									{
										case SDLK_BACKSPACE:
											DD<<"[Info] TextEditLine:"<<ID<<" DeleteTextBack"<<endl;
											DeleteTextCursor();
											Win->NeedFreshScreen=1;
											Win->PresentLimit|=gPS;
											break;
											
										case SDLK_LEFT:
											if (pos.r>0)
											{
												pos.l=--pos.r;
												SetSposFromPos2();
												Win->NeedFreshScreen=1;
												Win->PresentLimit|=gPS;
											}
											break;
										
										case SDLK_RIGHT:
											if (pos.r<Text.length())
											{
												pos.l=++pos.r;
												SetSposFromPos2();
												Win->NeedFreshScreen=1;
												Win->PresentLimit|=gPS;
											}
											break;
											
										case SDLK_RETURN:
										if (EnterFunc!=NULL)
											EnterFunc(funcData,Text,1);	
											break;
										
										case SDLK_HOME:
											pos.l=pos.r=0;
											SetSposFromPos2();
											Win->NeedFreshScreen=1;
											Win->PresentLimit|=gPS;
											break;
										
										case SDLK_END:
											pos.l=pos.r=Text.length();
											SetSposFromPos2();
											Win->NeedFreshScreen=1;
											Win->PresentLimit|=gPS;
											break;
										
										case SDLK_ESCAPE:
											StateInput=0;
											Editing=0;
											Win->NeedFreshScreen=1;
											Win->PresentLimit|=gPS;
											Win->KeyboardInputWg=NULL;
											editingText.clear();
											DD<<"[Info] TextEditLine:"<<ID<<" Stop input"<<endl;
											break;
											
										case SDLK_v:
											if (event.key.keysym.mod&KMOD_CTRL)
											{
												char *s=SDL_GetClipboardText();
												stringUTF8 str=s;
												SDL_free(s);
												if (str.empty())
													break;
												if (!pos.Len0())
													DeleteTextCursor();
												AddText(pos.r,str);
												SetSposFromPos2();
											}
											break;
										
										case SDLK_c:
											if (event.key.keysym.mod&KMOD_CTRL)
												if (!pos.Len0())
													SDL_SetClipboardText(GetSelectedText().c_str());
											break;
											
										case SDLK_x:
											if (event.key.keysym.mod&KMOD_CTRL)
												if (!pos.Len0())
												{
													SDL_SetClipboardText(GetSelectedText().c_str());
													DeleteTextCursor();
													Win->NeedFreshScreen=1;
													Win->PresentLimit|=gPS;
												}
											break;
											
										case SDLK_z:
											if (event.key.keysym.mod&KMOD_CTRL)
											{
												DD<<"[Error] TextEditLine: "<<ID<<" ctrl z cannot use yet"<<endl;
											}
											break;
									}
								Win->NeedSolveEvent=0; 
							}
						break;
						
					case SDL_USEREVENT:
						if (event.user.type==PUI_EVENT_UpdateTimer)
							if (event.user.data1==this)
							{
								SetSposFromPos2();
								pos.r=GetPos(Win->NowPos.y-gPS.y);
//								pos2=GetPos(LastPos2X);
							}
						break;
				}
			}
			
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (!CoverLmt.In(Win->NowPos))
						if (StateInput)
						{
							if (Win->NowSolvingEvent->type==SDL_MOUSEBUTTONDOWN)
								if (Win->NowSolvingEvent->button.button==SDL_BUTTON_LEFT)
								{
									stat=0;
									StateInput=0;
									Editing=0;
									Win->NeedFreshScreen=1;
									Win->PresentLimit|=gPS;
									Win->KeyboardInputWg=NULL;
									RemoveNeedLoseFocus();
									editingText.clear();
									DD<<"[Info] TextEditLine:"<<ID<<" Stop input"<<endl;
								}
						}
						else if (stat!=0)
						{
							stat=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
							RemoveNeedLoseFocus();
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						DD<<"[Info] TextEditLine "<<ID<<" click"<<endl;
						if (!Editing)
							if (event.button.button==SDL_BUTTON_LEFT)
							{
								stat=2;
								Win->OccupyPosWg=this;
								pos.l=pos.r=GetPos(Win->NowPos.x-gPS.x);
								DD<<"[Info] TextEditLine: "<<ID<<" Start  input"<<endl;
							}
							else if (event.button.button==SDL_BUTTON_RIGHT)
							{
								stat=3;
								//Not usable yet
							}
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						Win->PresentLimit|=gPS;
						break;
						
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							DD<<"[Info] TextEditLine:"<<ID<<" up"<<endl;
							if (stat==2)
								Win->OccupyPosWg=NULL;
							stat=1;
							StateInput=1;
//							Win->KeyboardInputWgState=1;
							Win->KeyboardInputWg=this;
//							SDL_Rect rct=PosizeToSDLRect(gPS);
//							SDL_SetTextInputRect(&rct);
//							SDL_StartTextInput();
//							TurnOnOffIntervalTimer(0);
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat>=2)
						{
							if (!Editing)
							{
								pos.r=GetPos(Win->NowPos.x-gPS.x);
//								DD<<"[Debug] pos.r "<<pos.r<<endl;
								SetSposFromPos2();
//								if (InRange(Win->NowPos.x,gPS.x+BorderWidth,gPS.x2()-BorderWidth))
//									TurnOnOffIntervalTimer(0);
//								else TurnOnOffIntervalTimer(1);
								Win->NeedFreshScreen=1;
								Win->PresentLimit|=gPS;
							}
						}
						else if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}

			virtual void Show(Posize &lmt)
			{
				int editingTextLen=-Text.length();
				if (StateInput&&Editing)
				{
					AddText(pos.r,editingText);
					editingTextLen+=Text.length();
					SetCursorPos(pos.r-editingTextLen+EditingTextCursorPos);
				}
				else editingTextLen=0;
				
				Win->RenderFillRect(lmt&gPS,BackgroundColor[0]?BackgroundColor[0]:ThemeColor.BackgroundColor[0]);
				if (ShowBorder)
					Win->RenderDrawRectWithLimit(gPS,BorderColor[Text.length()==LengthLimit?3:EnsureInRange(stat,0,2)]?BorderColor[Text.length()==LengthLimit?3:EnsureInRange(stat,0,2)]:ThemeColor[stat*2+1],lmt);
				int x=BorderWidth,w=0,m=min(pos.l,pos.r),M=max(pos.l,pos.r);
				for (int i=ShowPos.l;i<Text.length()&&i<=ShowPos.r;++i)
					if (i<m) x+=ChWidth[i];
					else if (i>=M) break;
					else w+=ChWidth[i];
				if (StateInput&&pos.Len0())//Draw cursor 
					Win->RenderFillRect(Posize(x+gPS.x,BorderWidth+gPS.y,2,rPS.h-BorderWidth*2)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
				Win->RenderFillRect(Posize(x+gPS.x,BorderWidth+gPS.y,w,rPS.h-BorderWidth*2)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
				
				int i,s;
				if (Text.length()!=0)
					for (i=ShowPos.l,s=BorderWidth;i<Text.length()&&s+ChWidth[i]<=rPS.w-BorderWidth;s+=ChWidth[i++])
					{
						int colorPos=editingTextLen!=0&&InRange(i,pos.r-EditingTextCursorPos,pos.r-EditingTextCursorPos+editingTextLen-1)?2:InRange(i,m,M-1);
						SDL_Texture *tex=CreateRGBATextTexture(Text[i].c_str(),TextColor[colorPos]?TextColor[colorPos]:(colorPos==2?ThemeColor[6]:ThemeColor.MainTextColor[colorPos]));
						Win->RenderCopyWithLmt(tex,Posize(s,rPS.h-ChHeight>>1,ChWidth[i],ChHeight)+gPS,lmt&gPS.Shrink(BorderWidth));
						SDL_DestroyTexture(tex);
					}
				ShowPos.r=i;

				if (StateInput&&Editing)
					DeleteText(pos.r-EditingTextCursorPos,editingTextLen);

				Win->Debug_DisplayBorder(gPS);
			}
			
		public:
			inline void SetTextColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TextColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditLine: "<<ID<<" SetTextColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditLine: "<<ID<<" SetBackgroundColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BorderColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditLine: "<<ID<<" SetBorderColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetShowBorder(bool bo)
			{
				ShowBorder=bo;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetBorderWidth(int w)
			{
				BorderWidth=w;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetLengthLimit(int len)
			{
				LengthLimit=len;
				if (len<Text.length())
				{
					DD<<"[Error] TextExitLine: "<<ID<<" set LengthLimit less than Text.length() cannot use yet"<<endl;
//					Win->NeedFreshScreen=1;
//					Win->PresentLimit|=gPS;
				}				
			}
			
			inline void SetEnterFunc(void (*_enterfunc)(void*,const stringUTF8&,bool),void *_funcdata=NULL)
			{
				EnterFunc=_enterfunc;
				funcData=_funcdata;
			}

			~TextEditLine()
			{
				DD<<"[Info] Delete TextEditLine "<<ID<<endl;
//				TurnOnOffIntervalTimer(0);
				if (StateInput)
					Win->KeyboardInputWg=NULL;
			}
			
			TextEditLine(int _ID,Widgets *_fa,const Posize &_rps,void (*_enterfunc)(void*,const stringUTF8&,bool)=NULL,void *_funcdata=NULL)
			:EnterFunc(_enterfunc)
			{
				SetID(_ID);
				DD<<"[Info] Create TextEditLine: "<<ID<<endl;
				Type=WidgetType_TextEditLine;
				SetFa(_fa);
				SetrPS(_rps);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
			
			TextEditLine(int _ID,Widgets *_fa,PosizeEX *psex,void (*_enterfunc)(void*,const stringUTF8&,bool)=NULL,void *_funcdata=NULL)
			:EnterFunc(_enterfunc)
			{
				SetID(_ID);
				DD<<"[Info] Create TextEditLine: "<<ID<<endl;
				Type=WidgetType_TextEditLine;
				SetFa(_fa);
				AddPsEx(psex);
				if (_funcdata==CONST_THIS)
					funcData=this;
				else funcData=_funcdata;
			}
	};
	
	class TextEditBox:public Widgets
	{
		protected:
			struct EachChData
			{
				SharedTexturePtr tex[2];
				int timeCode[2]={0,0};
			};
			
			SharedTexturePtr ASCIItex[128][2];
			int timeCodeOfASCIItex[128][2];
			
			SplayTree <stringUTF8_WithData <EachChData> > Text;
			stringUTF8 editingText;
			vector <int> TextLengthCount;
			stack <int> TextLengthChange;
			int MaxTextLength=0;
			int EachChWidth=12,//chinese character is twice the english character width
				EachLineHeight=24,
				FontSize=18;
			string FontPath="";
			LargeLayerWithScrollBar *fa2=NULL;
			bool Editing=0,
				 ShowBorder=1,
				 AutoNextLine=0,//realize it ten years later =_=|| 
				 ShowSpecialChar=0,
				 EnableScrollResize=0,
				 EnableEdit=1;
			int stat=0,//0:no focus 1:focus 2:down_left 3:down_right
				StateInput=0,
				BorderWidth=3,
				EditingTextCursorPos,
				NewestCode=1;
			void (*EachChangeFunc)(void*,TextEditBox*,bool)=NULL;//bool:Is inner change//It is not so beautiful...
			void *EachChangeFuncdata=NULL;
			void (*RightClickFunc)(void*,const Point&,TextEditBox*)=NULL;
			void *RightClickFuncData=NULL;
			Point Pos=ZERO_POINT,Pos_0=ZERO_POINT,//This pos(_0) is not related to pixels
				  EditingPosL=ZERO_POINT,EditingPosR=ZERO_POINT;
			RGBA TextColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE},
				 BackgroundColor[4]={RGBA_NONE,RGBA_NONE,RGBA_NONE,RGBA_NONE},//bg,choosepart stat0,1,2/3
				 BorderColor[3]={RGBA_NONE,RGBA_NONE,RGBA_NONE};//stat0,1,2/3
			
			inline bool CheckWidth(const string &str)
			{return str.length()>=2;}
			
			int GetChLenOfEachWidth(const string &str)
			{
				if (str=="\t") return 4;
				return 1+CheckWidth(str);
			}
			
			int GetStrLenOfEachWidth(const stringUTF8_WithData <EachChData> &strUtf8)
			{
				int re=0;
				for (int i=0;i<strUtf8.length();++i)
					re+=GetChLenOfEachWidth(strUtf8[i]);
				return re;
			}
			
			int GetChWidth(const string &str)
			{
				if (str=="\t") return 4*EachChWidth;
				return EachChWidth+CheckWidth(str)*EachChWidth;
			}
			
			inline int StrLenWithoutSlashNR(const stringUTF8_WithData <EachChData> &strUtf8)
			{
				if (strUtf8.length()==0) return 0;
				if (strUtf8[strUtf8.length()-1]=="\n")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\r")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else if (strUtf8[strUtf8.length()-1]=="\r")
					if (strUtf8.length()>=2&&strUtf8[strUtf8.length()-2]=="\n")
						return strUtf8.length()-2;
					else return strUtf8.length()-1;
				else return strUtf8.length();
			}
			
			inline Point EnsurePosValid(const Point &pt)
			{return Point(EnsureInRange(pt.x,0,StrLenWithoutSlashNR(Text[pt.y])),EnsureInRange(pt.y,0,Text.size()-1));}
			
			int GetTextLength(int line,int len)
			{
				stringUTF8_WithData <EachChData> &strUtf8=Text[line];
				int w=0;
				for (int i=0;i<len;++i)
					w+=GetChWidth(strUtf8[i]);
				return w;
			}
			
			Point GetPos(const Point &pt)
			{
				int i=0,w,LineY=EnsureInRange((pt.y-gPS.y)/EachLineHeight,0,Text.size()-1),
					x=pt.x-gPS.x-BorderWidth;
				stringUTF8_WithData <EachChData> &strUtf8=Text[LineY];
				int strlen=StrLenWithoutSlashNR(strUtf8);
				while (i<strlen)
				{
					w=GetChWidth(strUtf8[i]);
					if (x<=w/2) return Point(i,LineY);
					++i;
					x-=w;
				}
				return Point(i,LineY);
			}
			
			void SetShowPosFromPos()//??
			{
				if (BorderWidth+EachLineHeight*Pos.y<-fa->GetrPS().y)
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*Pos.y);
				else if (BorderWidth+EachLineHeight*(Pos.y+1)>-fa->GetrPS().y+fa2->GetrPS().h-fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth())
					fa2->SetViewPort(2,BorderWidth+EachLineHeight*(Pos.y+1)-fa2->GetrPS().h+fa2->GetButtomBarEnableState()*fa2->GetScrollBarWidth());
				int w=GetTextLength(Pos.y,Pos.x);
				if (BorderWidth+w<-fa->GetrPS().x+EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-EachChWidth);
				else if (BorderWidth+w>-fa->GetrPS().x+fa2->GetgPS().w-fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()-EachChWidth)
					fa2->SetViewPort(1,BorderWidth+w-fa2->GetgPS().w+fa2->GetRightBarEnableState()*fa2->GetScrollBarWidth()+EachChWidth);
			}
			
			void ResizeTextBoxHeight()
			{
				fa2->ResizeLL(0,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
			}
			
			void ApplyTextLenChange()
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" ApplyTextLenChange Start"<<endl;
				int newMaxLen=MaxTextLength;
				while (!TextLengthChange.empty())
				{
					int len=TextLengthChange.top();
					TextLengthChange.pop();
					if (len>=0)
					{
						if (len>=TextLengthCount.size())
							TextLengthCount.resize(len+1,0);
						TextLengthCount[len]++;
						newMaxLen=max(newMaxLen,len);
					}
					else TextLengthCount[-len]--;
				}
//				DD<<"#";
//				for (int i=0;i<TextLengthCount.size();++i)
//					if (TextLengthCount[i]!=0)
//						DD<<i<<":"<<TextLengthCount[i]<<"-";
//						DD<<endl;
				
				if (TextLengthCount[newMaxLen]==0)
					for (int i=newMaxLen-1;i>=0;--i)
						if (TextLengthCount[i]>0)
						{
							newMaxLen=i;
							break;
						}
				if (newMaxLen!=MaxTextLength)
				{
					MaxTextLength=newMaxLen;
					fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);
					rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				}
//				DD<<"[Debug] TextEditBox "<<ID<<" ApplyTextLenChange End"<<endl;
			}
			
			void AddTextLenCnt(int len)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" AddTextLenCnt Start "<<len<<endl;
				TextLengthChange.push(len);
//				DD<<"[Debug] TextEditBox "<<ID<<" AddTextLenCnt End"<<endl;
			}
			
			void SubtractTextLenCnt(int len)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" SubtractTextLenCnt Start "<<len<<endl;
				TextLengthChange.push(-len);
//				DD<<"[Debug] TextEditBox "<<ID<<" SubtractTextLenCnt End"<<endl;
			}

			virtual void CalcPsEx()
			{
				if (PsEx!=NULL)
					PsEx->GetrPS(rPS);
				Posize lastPs=gPS;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,0);//??
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				ResizeTextBoxHeight();
				if (fa!=NULL)
					gPS=rPS+fa->GetgPS();
				else gPS=rPS;
				if (!(lastPs==gPS))
					Win->PresentLimit=Win->PresentLimit|lastPs|gPS;
				Posize fa2gPS=fa2->GetgPS();//??
				if (fa2->GetRightBarEnableState())
					fa2gPS.w-=fa2->GetScrollBarWidth();
				if (fa2->GetButtomBarEnableState())
					fa2gPS.h-=fa2->GetScrollBarWidth();
				CoverLmt=gPS&GetFaCoverLmt()&fa2gPS;
			}
			
//			int MaintainNearbyLines(int p,int cnt)
//			{
//				
//			}

			bool InSelectedRange(const Point &pt0,const Point &pt1,const Point &pt2)//[L,R)
			{
				if (pt1==pt2) return 0;
				if (pt1.y==pt2.y)
					return pt0.y==pt1.y&&InRange(pt0.x,min(pt1.x,pt2.x),max(pt1.x,pt2.x)-1);
				else
					if (InRange(pt0.y,min(pt1.y,pt2.y),max(pt1.y,pt2.y)))
						if (pt1.y<pt2.y)
							if (pt0.y==pt1.y)
								return pt0.x>=pt1.x;
							else if (pt0.y==pt2.y)
								return pt0.x<pt2.x;
							else return 1;
						else
							if (pt0.y==pt2.y)
								return pt0.x>=pt2.x;
							else if (pt0.y==pt1.y)
								return pt0.x<pt1.x;
							else return 1;
					else return 0;
			}
		
			void _Clear(bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				Text.clear();
				editingText.clear();
				TextLengthCount.clear();
				MaxTextLength=0;
				Text.insert(0,stringUTF8("\r\n"));
				AddTextLenCnt(2);
				Pos=Pos_0={0,0};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Editing=0;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
			}
			
			void _AddNewLine(int p,const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
//				DD<<"[Debug] TextEditBox "<<ID<<" AddNewLine Start"<<endl;
				int i=0,j=0;
				while (i<strUtf8.length())
				{
					if (strUtf8[i]=="\r")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,substr);
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					else if (strUtf8[i]=="\n")
					{
						if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
							++i;
						stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
						Text.insert(p++,substr);
						AddTextLenCnt(GetStrLenOfEachWidth(substr));
						j=i+1;
					}
					++i;
				}
				if (j<strUtf8.length())
				{
					stringUTF8 substr=strUtf8.substrUTF8(j,strUtf8.length()-j)+stringUTF8("\r\n");
					Text.insert(p++,substr);
					AddTextLenCnt(GetStrLenOfEachWidth(substr));
				}
				Pos=Pos_0={StrLenWithoutSlashNR(Text[p-1]),p-1};
				ApplyTextLenChange();
				ResizeTextBoxHeight();
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" AddNewLine End"<<endl;
			}
			
			void _AddText(Point pt,const stringUTF8 &strUtf8,bool isInnerChange)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" AddText Start"<<endl;
				if (strUtf8.length()==0||!EnableEdit&&isInnerChange) return;
				if (InRange(pt.y,0,Text.size()-1))
				{
					stringUTF8_WithData <EachChData> &str2=Text[pt.y];
					stringUTF8_WithData <EachChData> str3=str2.substrUTF8_WithData(pt.x,str2.length()-pt.x);
					SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
					str2.erase(pt.x,str2.length()-pt.x);
					AddTextLenCnt(GetStrLenOfEachWidth(str2));
					
					int i=0,j=0;
					while (i<strUtf8.length())
					{
						if (strUtf8[i]=="\r")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\n")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,substr);
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						else if (strUtf8[i]=="\n")
						{
							if (i<strUtf8.length()-1&&strUtf8[i+1]=="\r")
								++i;
							if (j==0)
							{
								SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
								str2.append(strUtf8.substrUTF8(j,i-j+1));
								AddTextLenCnt(GetStrLenOfEachWidth(str2));
								pt.y++;
								pt.x=0;
							}
							else
							{
								stringUTF8 substr=strUtf8.substrUTF8(j,i-j+1);
								Text.insert(pt.y++,substr);
								AddTextLenCnt(GetStrLenOfEachWidth(substr));
							}
							j=i+1;
						}
						++i;
					}
					if (j<strUtf8.length())
						if (j==0)
						{
							SubtractTextLenCnt(GetStrLenOfEachWidth(str2));
							Pos=Pos_0={str2.length()+strUtf8.length(),pt.y};
							str2.append(strUtf8);
							str2.append(str3);
							AddTextLenCnt(GetStrLenOfEachWidth(str2));
						}
						else
						{
							stringUTF8_WithData <EachChData>  str4=strUtf8.substrUTF8(j,strUtf8.length()-j);
							str4.append(str3);
							Text.insert(pt.y,str4);
							AddTextLenCnt(GetStrLenOfEachWidth(str4));
							Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y]),pt.y};
							ResizeTextBoxHeight();
						}
					else
					{
						Text.insert(pt.y,str3);
						AddTextLenCnt(GetStrLenOfEachWidth(str3));
						Pos=Pos_0={StrLenWithoutSlashNR(Text[pt.y]),pt.y};
						ResizeTextBoxHeight();
					}
					ApplyTextLenChange();
					SetShowPosFromPos();
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
					if (EachChangeFunc!=NULL)
						EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
				}
				else _AddNewLine(EnsureInRange(pt.y,0,Text.size()),strUtf8,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" AddText End"<<endl;
			}
			
			void _SetText(const stringUTF8 &strUtf8,bool isInnerChange)
			{
				if (!EnableEdit&&isInnerChange) return;
				_Clear(isInnerChange);
				_AddText({0,0},strUtf8,isInnerChange);
			}
			
			void _AppendNewLine(const stringUTF8 &strUtf8,bool isInnerChange)
			{_AddNewLine(Text.size(),strUtf8,isInnerChange);}
			
			void _DeleteLine(int p,bool isInnerChange)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteLine Start"<<endl;
				if (!EnableEdit&&isInnerChange) return;
				p=EnsureInRange(p,0,Text.size()-1);
				SubtractTextLenCnt(GetStrLenOfEachWidth(Text[p]));
				Text.erase(p);
				Pos=p==0?Point(0,0):Point(p-1,StrLenWithoutSlashNR(Text[p-1]));
				Pos=Pos_0=EnsurePosValid(Pos);
				ResizeTextBoxHeight();
				ApplyTextLenChange();
				SetShowPosFromPos();
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteLine End"<<endl;
			}
			
			void _DeleteText(Point pt1,Point pt2,bool isInnerChange)
			{
				if (pt1==pt2||!EnableEdit&&isInnerChange) return;
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteText Start"<<endl;
				if (pt1.y==pt2.y)
				{
					if (pt1.x>pt2.x)
						swap(pt1,pt2);
					Pos=pt1;
					stringUTF8_WithData <EachChData>  &strUtf8=Text[pt1.y];
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.erase(pt1.x,pt2.x-pt1.x);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
				}
				else
				{
					if (pt1.y>pt2.y)
						swap(pt1,pt2);
					Pos=pt1;
					if (InRange(pt1.x,0,Text[pt1.y].length()-1))
					{
						stringUTF8_WithData <EachChData>  &strUtf8=Text[pt1.y];
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(pt1.x,strUtf8.length()-pt1.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					if (InRange(pt2.x,1,StrLenWithoutSlashNR(Text[pt2.y])))
					{
						stringUTF8_WithData <EachChData> &strUtf8=Text[pt2.y];
						SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
						strUtf8.erase(0,pt2.x);
						AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					}
					stringUTF8_WithData <EachChData> &strUtf8=Text[pt1.y];
					SubtractTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					strUtf8.append(Text[pt2.y]);
					AddTextLenCnt(GetStrLenOfEachWidth(strUtf8));
					SubtractTextLenCnt(GetStrLenOfEachWidth(Text[pt2.y]));
					Text.erase(pt2.y);
					for (int i=pt2.y-1;i>=pt1.y+1;--i)//It is necessary use --i that the index in SplayTree would change after erase
					{
						SubtractTextLenCnt(GetStrLenOfEachWidth(Text[i]));
						Text.erase(i);
					}
					ResizeTextBoxHeight();
				}
				Pos=Pos_0=EnsurePosValid(Pos);
				ApplyTextLenChange();
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;//...
				if (EachChangeFunc!=NULL)
					EachChangeFunc(EachChangeFuncdata,this,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteText End"<<endl;
			}
			
			void _DeleteTextBack(int len,bool isInnerChange)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteTextBack Start"<<endl;
				if (!EnableEdit&&isInnerChange) return;
				Point pt=Pos;
				if (len>Pos.x)
				{
					len-=Pos.x+1;
					pt.y--;
					if (pt.y<0)
						pt={0,0};
					else
					{
						while (len>StrLenWithoutSlashNR(Text[pt.y])&&pt.y>=1)
							len-=StrLenWithoutSlashNR(Text[pt.y--])+1;
						pt.x=max(0,StrLenWithoutSlashNR(Text[pt.y])-len);
					}
				}
				else pt.x-=len;
				_DeleteText(EnsurePosValid(pt),Pos,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteTextBack End"<<endl;
			}
			
			void _DeleteTextCursor(bool isInnerChange)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteTextCursor Start"<<endl;
				if (Pos==Pos_0)
					_DeleteTextBack(1,isInnerChange);
				else _DeleteText(Pos,Pos_0,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" DeleteTextCursor End"<<endl;
			}
			
			void _AddTextCursor(const stringUTF8 &strUtf8,bool isInnerChange)
			{
//				DD<<"[Debug] TextEditBox "<<ID<<" AddTextCursor Start"<<endl;
				if (!(Pos==Pos_0))
					_DeleteTextCursor(isInnerChange);
				_AddText(Pos,strUtf8,isInnerChange);
//				DD<<"[Debug] TextEditBox "<<ID<<" AddTextCursor End"<<endl;
			}
		
		public:
			inline void Clear()
			{_Clear(0);}
			
			inline void AddNewLine(int p,const stringUTF8 &strUtf8)
			{_AddNewLine(p,strUtf8,0);}
			
			inline void AddText(const Point &pt,const stringUTF8 &strUtf8)
			{_AddText(pt,strUtf8,0);}
			
			inline void SetText(const stringUTF8 &strUtf8)
			{_SetText(strUtf8,0);}
			
			inline void AppendNewLine(const stringUTF8 &strUtf8)
			{_AppendNewLine(strUtf8,0);}
			
			inline void DeleteLine(int p)
			{_DeleteLine(p,0);}
			
			inline void DeleteText(const Point &pt1,const Point &pt2)
			{_DeleteText(pt1,pt2,0);}
			
			inline void DeleteTextBack(int len=1)
			{_DeleteTextBack(len,0);}
			
			inline void DeleteTextCursor()
			{_DeleteTextCursor(0);}
			
			inline void AddTextCursor(const stringUTF8 &strUtf8)
			{_AddTextCursor(strUtf8,0);}
			
			void MoveCursorPos(int delta)//?? 
			{
				if (Text.size()==0) return;
//				DD<<"[Debug] TextEditBox "<<ID<<" MoveCursorPos Start "<<delta<<endl;
				if (delta>0)
					if (delta>StrLenWithoutSlashNR(Text[Pos.y])-Pos.x)
					{
						delta-=StrLenWithoutSlashNR(Text[Pos.y])-Pos.x+1;
						Pos.y++;
						if (Pos.y>=Text.size())
							Pos={StrLenWithoutSlashNR(Text[Text.size()-1]),Text.size()-1};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y])&&Pos.y<Text.size()-1)
								delta-=StrLenWithoutSlashNR(Text[Pos.y++])+1;
							Pos.x=min(delta,StrLenWithoutSlashNR(Text[Pos.y]));
						}
					}
					else Pos.x+=delta;
				else
					if ((delta=-delta)>Pos.x)
					{
						delta-=Pos.x+1;
						Pos.y--;
						if (Pos.y<0)
							Pos={0,0};
						else
						{
							while (delta>StrLenWithoutSlashNR(Text[Pos.y])&&Pos.y>0)
								delta-=StrLenWithoutSlashNR(Text[Pos.y--])+1;
							Pos.x=max(0,StrLenWithoutSlashNR(Text[Pos.y])-delta);
						}
					}
					else Pos.x-=delta;
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
//				DD<<"[Debug] TextEditBox "<<ID<<" MoveCursorPos End "<<delta<<endl;
			}
			
			void MoveCursorPosUpDown(int downDelta)
			{
				downDelta=EnsureInRange(downDelta,-Pos.y,Text.size()-1-Pos.y);
				if (downDelta==0) return;
				int w=GetTextLength(Pos.y,Pos.x);
				Pos.y+=downDelta;
				Pos.x=0;
				stringUTF8_WithData <EachChData> &strUtf8=Text[Pos.y];
				int strlen=StrLenWithoutSlashNR(strUtf8);
				for (int i=0;i<strlen&&w>0;Pos.x=++i)
					w-=GetChWidth(strUtf8[i]);
				Pos_0=Pos;
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetCursorPos(const Point &pt)
			{
				Pos=Pos_0=EnsurePosValid(pt);
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;//...
			}
			
			void SetCursorPos(const Point &pt1,const Point &pt2)
			{
				Pos=pt1;
				Pos_0=pt2;
				SetShowPosFromPos();
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			stringUTF8 GetSelectedTextUTF8()
			{
				stringUTF8 re;
				if (Pos==Pos_0)
					return re;
				Point pt1=Pos,pt2=Pos_0;
				if (pt1.y>pt2.y||pt1.y==pt2.y&&pt1.x>pt2.x)
					swap(pt1,pt2);
				if (pt1.y==pt2.y)
					re.append(Text[pt1.y].substrUTF8(pt1.x,pt2.x-pt1.x));
				else
				{
					re.append(Text[pt1.y].substrUTF8(pt1.x,Text[pt1.y].length()-pt1.x));
					for (int i=pt1.y+1;i<=pt2.y-1;++i)
						re.append(Text[i].StringUTF8());
					re.append(Text[pt2.y].substrUTF8(0,pt2.x));
				}
				return re;	
			}
			
			string GetSelectedText()
			{return GetSelectedTextUTF8().cppString();}
			
			stringUTF8 GetLineUTF8(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].StringUTF8();}
			
			string GetLine(int p)
			{return Text[EnsureInRange(p,0,Text.size()-1)].cppString();}
			
			stringUTF8 GetAllTextUTF8()
			{
				stringUTF8 re;
				for (int i=0;i<Text.size();++i)
					re.append(Text[i].StringUTF8());
				return re;
			}
			
			string GetAllText()
			{return GetAllTextUTF8().cppString();}
			
			int SetEachCharSize(int w,int h)
			{
				EachChWidth=w;
				EachLineHeight=h;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
			inline void SetFontSize(int size)
			{
				if (FontSize==size) return;
				FontSize=size;
				++NewestCode;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline int GetLinesCount()
			{return Text.size();}
			
			inline Point GetCursorPos()
			{return Pos;}
			
		protected:
			virtual void ReceiveKeyboardInput()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_TEXTINPUT:
						{
							string str=event.text.text;
							GetRidOfEndChar0(str);
							if (!str.empty())
								_AddTextCursor(stringUTF8(str),1);
							Win->NeedSolveEvent=0;
						}
						break;
					
					case SDL_TEXTEDITING:
						{
							DD<<"[Info] TextEditBox: "<<ID<<" Editing text: start "<<event.edit.start<<", len "<<event.edit.length<<" "<<event.edit.length<<endl;
							editingText=event.edit.text;
							EditingTextCursorPos=event.edit.start;
							Win->NeedSolveEvent=0;
							
							Editing=editingText.length()!=0;
							if (Editing)
							{
								if (!(Pos==Pos_0))
									_DeleteTextCursor(1);
								if (!(EditingPosL==EditingPosR))
									_DeleteText(EditingPosL,EditingPosR,1);
								EditingPosL=Pos;
								_AddTextCursor(editingText,1);
								EditingPosR=Pos;
								MoveCursorPos(EditingTextCursorPos-editingText.length());
							}
							else if (!(EditingPosL==EditingPosR)) 
							{
								Point oldPos=EditingPosR;//It's sukoshi ugly =_=|| 
								_DeleteText(EditingPosL,EditingPosR,1);
								SetCursorPos(oldPos);
								EditingPosL=EditingPosR=ZERO_POINT;
							}
						}
						break;
				}
				Win->NeedFreshScreen=1;
//				Win->PresentLimit|=;
				Win->PresentLimit|=gPS;
			}
			
			virtual void CheckEvent()
			{
				SDL_Event &event=*Win->NowSolvingEvent;
				switch (event.type)
				{
					case SDL_KEYDOWN:
						if (StateInput)
							if (!Editing)
							{
								bool flag=1;
								switch (event.key.keysym.sym)
								{
									case SDLK_BACKSPACE: _DeleteTextCursor(1);	break;
									case SDLK_LEFT:	MoveCursorPos(-1);	break;
									case SDLK_RIGHT: MoveCursorPos(1);	break;
									case SDLK_UP: MoveCursorPosUpDown(-1);	break;
									case SDLK_DOWN: MoveCursorPosUpDown(1);	break;
									case SDLK_PAGEUP: MoveCursorPosUpDown(-fa2->GetrPS().h/EachLineHeight);	break;
									case SDLK_PAGEDOWN:	MoveCursorPosUpDown(fa2->GetrPS().h/EachLineHeight);break;
									case SDLK_TAB: _AddTextCursor("\t",1);	break;
									case SDLK_RETURN:
										if (Pos.x>=StrLenWithoutSlashNR(Text[Pos.y]))
											_AddNewLine(Pos.y+1,"\r\n",1);
										else 
										{
											Point pt=Pos;
											_AddNewLine(Pos.y+1,Text[Pos.y].substr(Pos.x,StrLenWithoutSlashNR(Text[Pos.y])-Pos.x),1);
											_DeleteText(pt,Point(StrLenWithoutSlashNR(Text[pt.y]),pt.y),1);
											Pos_0=Pos={0,pt.y+1};
											Win->NeedFreshScreen=1;
											Win->PresentLimit|=gPS;
										}
										break;
									
									case SDLK_ESCAPE:
										StateInput=0;
										Editing=0;
										Win->NeedFreshScreen=1;
										Win->PresentLimit|=gPS;
										Win->KeyboardInputWg=NULL;
										editingText.clear();
										DD<<"[Info] TextEditBox:"<<ID<<" Stop input"<<endl;
										
									case SDLK_v:
										if (event.key.keysym.mod&KMOD_CTRL)
										{
											char *s=SDL_GetClipboardText();
											stringUTF8 str=s;
											SDL_free(s);
											if (str.empty())
												break;
											DD<<"OK"<<endl;
											_AddTextCursor(str,1);
										}
										break;
									
									case SDLK_c:
										if (event.key.keysym.mod&KMOD_CTRL)
											if (!(Pos==Pos_0))
												SDL_SetClipboardText(GetSelectedText().c_str()),DD<<"Copy"<<endl;
										break;
										
									case SDLK_x:
										if (event.key.keysym.mod&KMOD_CTRL)
											if (!(Pos==Pos_0))
											{
												SDL_SetClipboardText(GetSelectedText().c_str());
												_DeleteTextCursor(1);
												DD<<"Cut"<<endl;
											}
										break;
									
									case SDLK_z:
										if (event.key.keysym.mod&KMOD_CTRL)
										{
											DD<<"[Error] TextEditBox: "<<ID<<" ctrl z cannot use yet"<<endl;
										}
										break;
									
									case SDLK_a:
										if (event.key.keysym.mod&KMOD_CTRL)
											SetCursorPos({StrLenWithoutSlashNR(Text[Text.size()-1]),Text.size()-1},{0,0});
										break;
									
									default:
										flag=0;
								}
								if (flag)
									Win->NeedSolveEvent=0;
							}
						break;
					
					case SDL_MOUSEWHEEL:
						if (EnableScrollResize)
							if (SDL_GetModState()&KMOD_CTRL)
							{
								double lambda=1+event.wheel.y*0.1;
								int h=EnsureInRange(EachLineHeight*lambda,10,160),w=h/2;
								SetEachCharSize(w,h);
								SetFontSize(h*0.75);
								Win->NeedSolveEvent=0;
							}
						break;
				}
			}
		
			virtual void CheckPos()
			{
				const SDL_Event &event=*Win->NowSolvingEvent;
				if (Win->NowSolvingPosEventMode==1)
				{
					if (!CoverLmt.In(Win->NowPos))
						if (StateInput)
						{
							if (Win->NowSolvingEvent->type==SDL_MOUSEBUTTONDOWN)
								if (Win->NowSolvingEvent->button.button==SDL_BUTTON_LEFT)
								{
									stat=0;
									StateInput=0;
									Editing=0;
									Win->NeedFreshScreen=1;
									Win->PresentLimit|=gPS;
									Win->KeyboardInputWg=NULL;
									editingText.clear();
									RemoveNeedLoseFocus();
									DD<<"[Info] TextEditBox:"<<ID<<" Stop input"<<endl;
								}
						}
						else if (stat!=0)
						{
							stat=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
							RemoveNeedLoseFocus();
						}
					return;
				}
				
				switch (event.type)
				{
					case SDL_MOUSEBUTTONDOWN:
						DD<<"[Info] TextEditBox "<<ID<<" click"<<endl;
						if (!Editing)
							if (event.button.button==SDL_BUTTON_LEFT)
							{
								stat=2;
								Pos=Pos_0=GetPos(Win->NowPos);
								SetShowPosFromPos();
								Win->OccupyPosWg=this;
								DD<<"[Info] TextEditBox "<<ID<<": Start  input"<<endl;
							}
							else if (event.button.button==SDL_BUTTON_RIGHT)
							{
								stat=3;
								if (RightClickFunc!=NULL)
								{
									RightClickFunc(RightClickFuncData,GetPos(Win->NowPos),this);
									DD<<"[Info] TextEditBox "<<ID<<": RightClickFunc"<<endl;
								}
							}
						Win->NeedSolvePosEvent=0;
						Win->NeedFreshScreen=1;
						Win->PresentLimit|=gPS;
//						Win->PresentLimit|=;
						break;
						
					case SDL_MOUSEBUTTONUP:
						if (stat>=2)
						{
							DD<<"[Info] TextEditBox "<<ID<<": up"<<endl;
							if (stat==2)
								Win->OccupyPosWg=NULL;
							stat=1;
							StateInput=1;
							SDL_Rect rct=PosizeToSDLRect(gPS);
							SDL_SetTextInputRect(&rct);
							Win->KeyboardInputWg=this;
							Win->NeedSolvePosEvent=0;
							Win->NeedFreshScreen=1;
							Win->PresentLimit|=gPS;
//							Win->PresentLimit|=;
						}
						break;
						
					case SDL_MOUSEMOTION:
						if (stat>=2)
						{
							if (!Editing)
							{
								Pos=GetPos(Win->NowPos);
								SetShowPosFromPos();
								Win->NeedFreshScreen=1;
//								Win->PresentLimit|=;
								Win->PresentLimit|=gPS;
							}
						}
						else if (stat==0)
						{
							stat=1;
							SetNeedLoseFocus();
							Win->NeedFreshScreen=1;
//							Win->PresentLimit|=;
							Win->PresentLimit|=gPS;
						}
						Win->NeedSolvePosEvent=0;
						break;
				}
			}
			
			virtual void Show(Posize &lmt)
			{
//				Point EditingPosL,EditingPosR;
//				
//				if (StateInput&&Editing)
//				{
//					EditingPosL=Pos;
//					AddText(Pos,editingText);
//					EditingPosR=Pos;
//					MoveCursorPos(EditingTextCursorPos-editingText.length());
//				}
				
				Win->RenderFillRect(lmt&gPS,BackgroundColor[0]?BackgroundColor[0]:ThemeColor.BackgroundColor[0]);
				if (ShowBorder)
					Win->RenderDrawRectWithLimit(gPS,BorderColor[Editing?3:EnsureInRange(stat,0,2)]?BorderColor[Editing?3:EnsureInRange(stat,0,2)]:ThemeColor[(Editing?3:EnsureInRange(stat,0,2))*2+1],lmt);
				
				int ForL=-fa->GetrPS().y/EachLineHeight,
					ForR=ForL+fa2->GetgPS().h/EachLineHeight+1;
				ForL=EnsureInRange(ForL,0,Text.size()-1);
				ForR=EnsureInRange(ForR,0,Text.size()-1);
				
				Posize CharPs=Posize(gPS.x+BorderWidth,gPS.y+BorderWidth+ForL*EachLineHeight,EachChWidth,EachLineHeight);
				for (int i=ForL,flag=0;i<=ForR;++i)
				{
					stringUTF8_WithData <EachChData> &strUtf8=Text[i];
					for (int j=0;j<strUtf8.length();++j)
					{
						string str=strUtf8[j];
						CharPs.w=GetChWidth(str);
						if ((CharPs&lmt).Size()!=0)
						{
							if (Editing)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
								flag=InSelectedRange({j,i},EditingPosL,EditingPosR);
							}
							else if (Pos==Pos_0)
							{
								if (Pos==Point(j,i)&&StateInput)
									Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
							}
							else flag=InSelectedRange({j,i},Pos,Pos_0);
							
							if (!Editing&&flag)
								Win->RenderFillRect(CharPs&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
							
							SDL_Texture *tex=NULL;
							int chCol=flag?(Editing?2:1):0;
							if (chCol==2)
								Win->RenderDrawText(str,CharPs,lmt,0,TextColor[2]?TextColor[2]:ThemeColor[6],FontSize);
							else
							{
								RGBA &co=TextColor[chCol]?TextColor[chCol]:ThemeColor.MainTextColor[chCol];
								if (str=="\r")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\r'][chCol]!=NewestCode||!ASCIItex['\r'][chCol])
										{
											ASCIItex['\r'][chCol]=SharedTexturePtr(CreateRGBATextTexture("\\r",co,FontSize));
											timeCodeOfASCIItex['\r'][chCol]=NewestCode;
										}
										tex=ASCIItex['\r'][chCol]();
									}
									else tex=NULL;
								else if (str=="\n")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\n'][chCol]!=NewestCode||!ASCIItex['\n'][chCol])
										{
											ASCIItex['\n'][chCol]=SharedTexturePtr(CreateRGBATextTexture("\\n",co,FontSize));
											timeCodeOfASCIItex['\n'][chCol]=NewestCode;
										}
										tex=ASCIItex['\n'][chCol]();
									}
									else tex=NULL;
								else if (str=="\t")
									if (ShowSpecialChar)
									{
										if (timeCodeOfASCIItex['\t'][chCol]!=NewestCode||!ASCIItex['\t'][chCol])
										{
											ASCIItex['\t'][chCol]=SharedTexturePtr(CreateRGBATextTexture("\\t",co,FontSize));
											timeCodeOfASCIItex['\t'][chCol]=NewestCode;
										}
										tex=ASCIItex['\t'][chCol]();
									}
									else tex=NULL;
								else
								{
									if (strUtf8(j).timeCode[chCol]!=NewestCode||!strUtf8(j).tex[chCol])
										if (str.length()==1&&str[0]>0)
										{
											if (timeCodeOfASCIItex[str[0]][chCol]!=NewestCode||!ASCIItex[str[0]][chCol])
											{
												ASCIItex[str[0]][chCol]=SharedTexturePtr(CreateRGBATextTexture(str.c_str(),co,FontSize));
												timeCodeOfASCIItex[str[0]][chCol]=NewestCode;
											}
											strUtf8(j).tex[chCol]=ASCIItex[str[0]][chCol];
											strUtf8(j).timeCode[chCol]=NewestCode;
										}
										else
										{
											strUtf8(j).tex[chCol]=SharedTexturePtr(CreateRGBATextTexture(str.c_str(),co,FontSize));
											strUtf8(j).timeCode[chCol]=NewestCode;
										}
									tex=strUtf8(j).tex[chCol]();
								}
								if (tex!=NULL)
									Win->RenderCopyWithLmt_Centre(tex,CharPs,lmt);
							}
						}
						CharPs.x+=CharPs.w;
					}
					
					if (Editing)
					{
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
					}
					else if (Pos==Pos_0)
						if (Pos==Point(strUtf8.length(),i))
							Win->RenderFillRect(Posize(CharPs.x-1,CharPs.y,2,CharPs.h)&lmt,BackgroundColor[1+EnsureInRange(stat,0,2)]?BackgroundColor[1+EnsureInRange(stat,0,2)]:ThemeColor[2*EnsureInRange(stat,0,2)+1]);
					
					CharPs.x=gPS.x+BorderWidth;
					CharPs.y+=EachLineHeight;
				}
				
//				if (StateInput&&Editing)
//				{
//					MoveCursorPos(editingText.length()-EditingTextCursorPos+editingText.length());
//					DeleteTextBack(editingText.length());
//				}

				Win->Debug_DisplayBorder(gPS);
			}
		
		public:
			inline void SetTextColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					TextColor[p]=co;
					++NewestCode;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditBox "<<ID<<": SetTextColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void SetBackgroundColor(int p,const RGBA &co)
			{
				if (InRange(p,0,3))
				{
					BackgroundColor[p]=co;
					++NewestCode;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditBox "<<ID<<": SetBackgroundColor p "<<p<<" is not in Range[0,3]"<<endl;
			}
			
			inline void SetBorderColor(int p,const RGBA &co)
			{
				if (InRange(p,0,2))
				{
					BorderColor[p]=co;
					Win->NeedFreshScreen=1;
					Win->PresentLimit|=gPS;
				}
				else DD<<"[Error] TextEditBox "<<ID<<": SetBorderColor p "<<p<<" is not in Range[0,2]"<<endl;
			}
			
			inline void EnableShowBorder(bool en)
			{
				if (ShowBorder==en) return;
				ShowBorder=en;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			void SetBorderWidth(int w)
			{
				BorderWidth=w;
				fa2->ResizeLL(MaxTextLength*EachChWidth+BorderWidth*2,EachLineHeight*Text.size()+2*BorderWidth);
				rPS.w=max(fa2->GetrPS().w,MaxTextLength*EachChWidth+BorderWidth*2);
				rPS.h=max(fa2->GetrPS().h,EachLineHeight*Text.size()+2*BorderWidth);
				SetShowPosFromPos();
			}
			
			inline void EnableShowSpecialChar(bool en)
			{
				if (en==ShowSpecialChar) return;
				ShowSpecialChar=en;
				Win->NeedFreshScreen=1;
				Win->PresentLimit|=gPS;
			}
			
			inline void SetEachChangeFunc(void (*_eachchangefunc)(void*,TextEditBox*,bool),void *eachchangefuncdata=NULL)
			{
				EachChangeFunc=_eachchangefunc;
				EachChangeFuncdata=eachchangefuncdata;
			}
			
			inline void SetRightClickFunc(void (*_rightClickFunc)(void*,const Point&,TextEditBox*),void *funcdata=NULL)
			{
				RightClickFunc=_rightClickFunc;
				RightClickFuncData=funcdata;
			}
			
			inline void SetEnableScrollResize(bool enable)
			{EnableScrollResize=enable;}
			
			inline void SetEnableEdit(bool enable)
			{EnableEdit=enable;}
			
			void AddPsEx(PosizeEX *psex)
			{fa2->AddPsEx(psex);}
			
			virtual void SetrPS(const Posize &ps)
			{fa2->SetrPS(ps);}
			
			inline LargeLayerWithScrollBar* GetFa2()
			{return fa2;}
			
			TextEditBox(int _ID,Widgets *_fa,PosizeEX *psex)
			{
				SetID(_ID);
				DD<<"[Info] Create TextEditBox: "<<ID<<endl;
				Type=WidgetType_TextEditBox;
				fa2=new LargeLayerWithScrollBar(0,_fa,psex,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS=ZERO_POSIZE;
				TextLengthCount.resize(10,0);
				memset(timeCodeOfASCIItex,0,sizeof timeCodeOfASCIItex);
				_Clear(1);
			}
			
			TextEditBox(int _ID,Widgets *_fa,const Posize &_rps)
			{
				SetID(_ID);
				DD<<"[Info] Create TextEditBox: "<<ID<<endl;
				Type=WidgetType_TextEditBox;
				fa2=new LargeLayerWithScrollBar(0,_fa,_rps,ZERO_POSIZE);
				SetFa(fa2->LargeArea());
				rPS={0,0,_rps.w,_rps.h};
				TextLengthCount.resize(10,0);
				memset(timeCodeOfASCIItex,0,sizeof timeCodeOfASCIItex);
				_Clear(1);
			}
	};
	
	class ListViewTemplate:public Widgets//Auto create widgets by the template widgets
	{
		
	};
	
	class BlockViewTemplate:public Widgets
	{
		
	};
	
	//Special:
	//..
	
	class MessageBox2:public Widgets
	{
		protected:
			
			
		public:
			
			
	};
	
	class SelectFileBox:public Widgets
	{
		protected:
			
			
		public:
			
			
			
	};
	
	
	//Driver:
	//.. 
	
	void UpdateWidgetsPosize(Widgets *wg)
	{
		if (wg==NULL) return;
		UpdateWidgetsPosize(wg->nxtBrother);
		if (!wg->Enabled) return;
		wg->CalcPsEx();
		UpdateWidgetsPosize(wg->childWg);
	}
	
	void UpdateWidgetsPosize(PUI_Window *win=NULL)
	{
		if (win!=NULL)
		{
			CurrentWindow=win;
			if (!(win->BackGroundLayer->gPS==win->WinPS.ToOrigin()))
			{
				win->PresentLimit=win->WinPS.ToOrigin();
				win->BackGroundLayer->gPS=win->BackGroundLayer->rPS=win->BackGroundLayer->CoverLmt=win->WinPS.ToOrigin();
				win->MenuLayer->gPS=win->MenuLayer->rPS=win->MenuLayer->CoverLmt=win->WinPS.ToOrigin();
			}
			UpdateWidgetsPosize(win->BackGroundLayer->childWg);
			UpdateWidgetsPosize(win->MenuLayer->childWg);
			win->NeedUpdatePosize=0;
			win->NeedFreshScreen=1;
		}
		else
			for (auto sp:PUI_Window::AllWindow)
				if (sp->NeedUpdatePosize)
					UpdateWidgetsPosize(sp);
	}
	
	bool PresentWidgets()//RecommendThis
	{
//		static Uint64 LastPresentTime=0;
//		Uint64 CurrentPresentTime=SDL_GetPerformanceCounter();
		bool re=0;
		for (auto sp:PUI_Window::AllWindow)
			if (sp->NeedFreshScreen||PUI_Window::NeedFreshScreenAll)
			{
				CurrentWindow=sp;
				if (DEBUG_DisplayPresentLimitFlag)
				{
					sp->BackGroundLayer->_PresentWidgets(sp->WinPS.ToOrigin());
					sp->MenuLayer->_PresentWidgets(sp->WinPS.ToOrigin());
				}
				else
				{
					sp->BackGroundLayer->_PresentWidgets(sp->PresentLimit);
					sp->MenuLayer->_PresentWidgets(sp->PresentLimit);
				}
				sp->NeedFreshScreen=0;
				if (DEBUG_DisplayPresentLimitFlag)
					sp->DEBUG_DisplayPresentLimit(),
					DD<<"[Debug] PresentLimit "<<sp->PresentLimit.x<<" "<<sp->PresentLimit.y<<" "<<sp->PresentLimit.w<<" "<<sp->PresentLimit.h<<endl;
				SDL_RenderPresent(sp->ren);
//				sp->PresentLimit=sp->WinPS.ToOrigin();
				sp->PresentLimit=ZERO_POSIZE;
				re=1;
			}
		PUI_Window::NeedFreshScreenAll=0;
		return re;
	}
	
	void PresentWidgets(PUI_Window *win,Posize lmt=ZERO_POSIZE)
	{
		CurrentWindow=win;
		if (lmt==ZERO_POSIZE)
			lmt=win->PresentLimit;
		win->BackGroundLayer->_PresentWidgets(lmt);
		win->MenuLayer->_PresentWidgets(lmt);
		SDL_RenderPresent(win->ren);
		win->NeedFreshScreen=0;
		win->PresentLimit=win->WinPS.ToOrigin();
	}
	
	void PresentWidgets(Widgets *tar)//Update specific widgets tree;ignore PresentLimit and lmt
	{
		CurrentWindow=tar->Win;
		tar->_PresentWidgets(tar->gPS);
		tar->Win->MenuLayer->_PresentWidgets(tar->gPS);
		SDL_RenderPresent(tar->Win->ren);
		tar->Win->NeedFreshScreen=0;
	}
	
	int SolveEvent(SDL_Event &event)//0:Solve event successfully 1:Unknown event to solve 2:Known event but not solved
	{
		int re=1;
		switch (event.type)
		{
			case SDL_WINDOWEVENT:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.window.windowID];
				CurrentWindow->NowSolvingEvent=&event;
				CurrentWindow->NeedSolveEvent=1;
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						DD<<"[Info] Window Resize to "<<event.window.data1<<" "<<event.window.data2<<endl;
						CurrentWindow->WinPS.w=event.window.data1;
						CurrentWindow->WinPS.h=event.window.data2;
						CurrentWindow->NeedUpdatePosize=1;
						CurrentWindow->NeedSolveEvent=0;
						re=0;
						break;
						
			        case SDL_WINDOWEVENT_SHOWN:
			            DD<<"[Info] Window "<<event.window.windowID<<" shown"<<endl;
			            break;
			        case SDL_WINDOWEVENT_HIDDEN:
			            DD<<"[Info] Window "<<event.window.windowID<<" hidden"<<endl;
			            break;
			        case SDL_WINDOWEVENT_EXPOSED:
			            DD<<"[Info] Window "<<event.window.windowID<<" exposed"<<endl;
			            break;
			        case SDL_WINDOWEVENT_MOVED:
			            DD<<"[Info] Window "<<CurrentWindow->WindowTitle<<" moved to "<<event.window.data1<<","<<event.window.data2<<endl;
			            CurrentWindow->WinPS.x=event.window.data1;
			            CurrentWindow->WinPS.y=event.window.data2;
			            re=0;
			            break;
			        case SDL_WINDOWEVENT_SIZE_CHANGED:
			            DD<<"[Info] Window "<<event.window.windowID<<" size changed to "<<event.window.data1<<"x"<<event.window.data2<<endl;
			            break;
			        case SDL_WINDOWEVENT_MINIMIZED:
			            DD<<"[Info] Window "<<event.window.windowID<<" minimized"<<endl;
			            break;
			        case SDL_WINDOWEVENT_MAXIMIZED:
			            DD<<"[Info] Window "<<event.window.windowID<<" maximized"<<endl;
			            break;
			        case SDL_WINDOWEVENT_RESTORED:
			            DD<<"[Info] Window "<<event.window.windowID<<" restored"<<endl;
			            break;
			        case SDL_WINDOWEVENT_ENTER:
			            DD<<"[Info] Mouse entered window "<<event.window.windowID<<endl;
			            break;
			        case SDL_WINDOWEVENT_LEAVE:
			            DD<<"[Info] Mouse left window "<<CurrentWindow->WindowTitle<<endl;
//			            CurrentWindow->NowPos=GetGlobalMousePoint()-CurrentWindow->WinPS.GetLU();
//			            DD<<"[Debug] # "<<CurrentWindow->NowPos<<endl;
//			            goto StartSolvePosEventFlag;
			            break;
			        case SDL_WINDOWEVENT_FOCUS_GAINED:
			            DD<<"[Info] Window "<<event.window.windowID<<" gained keyboard focus"<<endl;
			            break;
			        case SDL_WINDOWEVENT_FOCUS_LOST:
			            DD<<"[Info] Window "<<event.window.windowID<<" lost keyboard focus"<<endl;
			            break;
			        case SDL_WINDOWEVENT_CLOSE:
			            DD<<"[Info] Window "<<event.window.windowID<<" closed"<<endl;
			            break;
			        case SDL_WINDOWEVENT_TAKE_FOCUS:
			            DD<<"[Info] Window "<<event.window.windowID<<" is offered a focus"<<endl;
			            break;
			        case SDL_WINDOWEVENT_HIT_TEST:
			            DD<<"[Info] Window "<<event.window.windowID<<" has a special hit test"<<endl;
			            break;
			        default:
			            DD<<"[Info] Window "<<event.window.windowID<<" got unknown event "<<event.window.event<<endl;
			            break;
				}
				break;	
			
			case SDL_TEXTINPUT:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.text.windowID];
			case SDL_TEXTEDITING:
				if (event.type==SDL_TEXTEDITING)
				{
					if (PUI_Window::WindowCnt==1)
						CurrentWindow=MainWindow;
					else CurrentWindow=PUI_Window::WinOfSDLWinID[event.edit.windowID];
				}
				CurrentWindow->NowSolvingEvent=&event;
				CurrentWindow->NeedSolveEvent=1;
				if (CurrentWindow->KeyboardInputWg!=NULL)
					CurrentWindow->KeyboardInputWg->ReceiveKeyboardInput();
				if (CurrentWindow->NeedSolveEvent)
				{
					CurrentWindow->MenuLayer->_SolveEvent();
					CurrentWindow->BackGroundLayer->_SolveEvent();
				}
				re=CurrentWindow->NeedSolveEvent?2:0;
				break;
			
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.key.windowID];
				CurrentWindow->NowSolvingEvent=&event;
				CurrentWindow->NeedSolveEvent=1;
				CurrentWindow->MenuLayer->_SolveEvent();
				CurrentWindow->BackGroundLayer->_SolveEvent();
				re=CurrentWindow->NeedSolveEvent?2:0;
				break;
			
//			case SDL_FINGERMOTION://Testing now
//				if (!PosEventGetFlag)
//				{
//	//				if (PUI_Window::WindowCnt==1)
//						CurrentWindow=MainWindow;
//	//				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.tfinger.windowID];
//					CurrentWindow->NowPos={event.tfinger.x*CurrentWindow->WinPS.w,event.tfinger.y*CurrentWindow->WinPS.h};
//					PosEventGetFlag=1;
//				}		
		
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.button.windowID];
				CurrentWindow->NowPos={event.button.x,event.button.y};
				goto StartSolvePosEventFlag;
				
			case SDL_MOUSEMOTION:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.motion.windowID];
				CurrentWindow->NowPos={event.motion.x,event.motion.y};
				goto StartSolvePosEventFlag;
				
			case SDL_FINGERMOTION:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.motion.windowID];
				CurrentWindow->NowPos={event.tfinger.x*CurrentWindow->WinPS.w,event.tfinger.y*CurrentWindow->WinPS.h};
				goto StartSolvePosEventFlag;
				
			StartSolvePosEventFlag:
				CurrentWindow->NowSolvingEvent=&event;
				CurrentWindow->NeedSolvePosEvent=1;	
				
				if (CurrentWindow->OccupyPosWg!=NULL)
					if (CurrentWindow->OccupyPosWg->Enabled)
					{
						CurrentWindow->NowSolvingPosEventMode=2;
						CurrentWindow->OccupyPosWg->CheckPos();
					}
					else DD<<"[Error] OccupyPosWg "<<CurrentWindow->OccupyPosWg->ID<<" id disabled!"<<endl; 
				else
				{
					CurrentWindow->NowSolvingPosEventMode=1;
					for (PUI_Window::LoseFocusLinkTable *p=CurrentWindow->LoseFocusWgHead;p;p=p->nxt)
						if (p->wg!=NULL)
							if (p->wg->Enabled)
								p->wg->CheckPos();
							else DD<<"[Error] NeedLoseFocusWg "<<p->wg->ID<<" is disabled!"<<endl; 
						else DD<<"[Error] NeedLoseFocusWidgets is NULL"<<endl;
					
					CurrentWindow->NowSolvingPosEventMode=0;
					CurrentWindow->MenuLayer->_SolvePosEvent();
					CurrentWindow->BackGroundLayer->_SolvePosEvent();
				}
				re=CurrentWindow->NeedSolvePosEvent?2:0;
				CurrentWindow->NeedSolvePosEvent=0;
				break;
			
			case SDL_MOUSEWHEEL:
				if (PUI_Window::WindowCnt==1)
					CurrentWindow=MainWindow;
				else CurrentWindow=PUI_Window::WinOfSDLWinID[event.wheel.windowID];
				CurrentWindow->NowSolvingEvent=&event;
				CurrentWindow->NeedSolveEvent=1;
				CurrentWindow->MenuLayer->_SolveEvent();
				CurrentWindow->BackGroundLayer->_SolveEvent();
				re=CurrentWindow->NeedSolveEvent?2:0;
				break;
				
			case SDL_USEREVENT:
				if (event.user.type==PUI_EVENT_UpdateTimer)
					((Widgets*)event.user.data1)->CheckEvent();
				re=0;
				break;
			//Maybe need fix 
			
			default:
				DD<<"[Warning] Such event was not considered yet!"<<endl;
//				EventSolvedFlag=0;
//				MenusLayer->_SolveEvent(event);
//				BackGroundLayer->_SolveEvent(event);
				
		}
		while (!Widgets::WidgetsToDeleteAfterEvent.empty())
		{
			delete Widgets::WidgetsToDeleteAfterEvent.front();
			Widgets::WidgetsToDeleteAfterEvent.pop();
		}
		return re;
	}
	
	void DebugEventSolve(const SDL_Event &event)
	{
		if (event.type==SDL_KEYDOWN)
			if (event.key.keysym.mod&KMOD_CTRL)
				switch (event.key.keysym.sym)
				{
					case SDLK_F2:
						DEBUG_DisplayBorderFlag=!DEBUG_DisplayBorderFlag,PUI_Window::NeedFreshScreenAll=1;
						break;
					
					case SDLK_F3:
						DD.SwitchToContrary();
						break;
						
					case SDLK_F4:
						DEBUG_DisplayPresentLimitFlag=!DEBUG_DisplayPresentLimitFlag;
						break;
					
//					case SDLK_F5:
//						DEBUG_EnableWidgetsShowInTurn=!DEBUG_EnableWidgetsShowInTurn;
//						break;
				}
	}
	
	void EasyEventLoop()
	{
		bool QuitFlag=0;
		while (!QuitFlag)
		{
			SDL_Event mainEvent;
			SDL_WaitEvent(&mainEvent);
			
			if (SolveEvent(mainEvent)!=0)
			{
				if (mainEvent.type==SDL_QUIT)
					QuitFlag=1;
				if (mainEvent.type==SDL_KEYDOWN)
					if (mainEvent.key.keysym.sym==SDLK_ESCAPE)
						QuitFlag=1;
				DebugEventSolve(mainEvent);
			}
			UpdateWidgetsPosize();
			PresentWidgets();
		}
	}
	
	int PAL_GUI_Init(const Posize &winps,const string &title="",unsigned int flagWin=SDL_WINDOW_RESIZABLE,unsigned int flagRen=SDL_RENDERER_ACCELERATED)
	{
		DD<<"[Info] PAL_GUI_Init"<<endl;
		SDL_Init(SDL_INIT_EVERYTHING);
		
		PUI_EVENT_UpdateTimer=SDL_RegisterEvents(1);
		
		MainWindow=CurrentWindow=new PUI_Window(winps,title,flagWin,flagRen);
		DD<<"[Info] PAL_GUI_Init: OK"<<endl;
	}
	
	int PAL_GUI_Quit()
	{
		DD<<"AllWindowCnt "<<PUI_Window::AllWindow.size()<<endl;
		while (!PUI_Window::AllWindow.empty())
			delete *PUI_Window::AllWindow.begin();
		while (!Widgets::WidgetsID.empty())
			delete Widgets::WidgetsID.begin()->second;
		SDL_Quit();
	}
}
#endif
