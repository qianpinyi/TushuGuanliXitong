#include <PAL_BasicFunctions/PAL_BasicFunctions_Charset.cpp>
#include <PAL_GUI/PAL_GUI_0.cpp>
#include <PAL_BasicFunctions/PAL_BasicFunctions_Config.cpp>
using namespace PAL_GUI;
using namespace Charset;

struct BookData
{
	string ISBN,
		   BookName,
		   Author,
		   Publisher,
		   BoughtTime,
		   Annotation;
	int CurrentInventory=0;
	double WholeSalePrice=0,
		   SalePrice=0;
	bool Deleted=0;
	int NumID=0;
	   
	void WriteBinary(ofstream &fout)
	{
		int len;
		#define W(x) len=x.length();\
					 fout.write((char*)&len,sizeof(int)).write(x.c_str(),len)
		W(ISBN);W(BookName);W(Author);W(Publisher);W(BoughtTime);W(Annotation);
		#undef W
		fout.write((char*)&CurrentInventory,sizeof(int));
		fout.write((char*)&WholeSalePrice,sizeof(double));
		fout.write((char*)&SalePrice,sizeof(double));
	}
	
	void ReadBinary(ifstream &fin)
	{
		int len,currentMaxLen=256;
		char *buffer=new char[256];
		for (int i=1;i<=6;++i)
		{
			fin.read((char*)&len,sizeof(int));
			if (len>=currentMaxLen)
			{
				delete[] buffer;
				buffer=new char[len*2];
				currentMaxLen=len*2;
			}
			fin.read(buffer,len);
			buffer[len]='\0';
			switch (i)
			{
				case 1:	ISBN=buffer;		break;
				case 2:	BookName=buffer;	break;
				case 3:	Author=buffer;		break;
				case 4:	Publisher=buffer;	break;
				case 5:	BoughtTime=buffer;	break;
				case 6:	Annotation=buffer;	break;
			}
		}
		delete[] buffer;
		fin.read((char*)&CurrentInventory,sizeof(int));
		fin.read((char*)&WholeSalePrice,sizeof(double));
		fin.read((char*)&SalePrice,sizeof(double));
	}
	
	BookData() {}
};
vector <BookData> BookLibrary;//It won't be deleted,the deleted book will be tagged and won't be write to file.

bool BookDataModifyRealtimeUpdate=0,
	 EnableAboutPageTEBscrollResize=0,
	 EnableShowTEBSpeacialChar=0,
	 EnableCompleteCopy=0,
	 EnableBackgroundPic=0,
	 EnableDebugOut=1,
	 EnableAccelerate=1;
string BackgroundPicPath="";
double BgPicOpacity=0;

PAL_Config cfg("cfg.txt");

bool ReadWriteBookDataFile(const string &filename,bool ModeWrite)
{
	int count=0;
	if (ModeWrite)
	{
		ofstream fout(filename);
		if (fout.is_open())
		{
			for (auto &vp:BookLibrary)
				if (!vp.Deleted)
					++count;
			fout.write((char*)&count,sizeof(int));
			for (auto &vp:BookLibrary)
				vp.WriteBinary(fout);
			if (fout.fail())
				return DD<<"[Error] Some error happen,write file failed!"<<endl,0;
		}
		else return DD<<"[Error] Cannot write BookData file!"<<endl,0;
	}
	else
	{
		ifstream fin(filename);
		if (fin.is_open())
		{
			fin.read((char*)&count,sizeof(int));
			for (int i=0;i<count;++i)
			{
				BookLibrary.push_back(BookData());
				BookLibrary.back().ReadBinary(fin);
				BookLibrary.back().NumID=i;
			}
			if (fin.fail())
				return DD<<"[Error] Some error happen,read file failed!"<<endl,0;
		}
		else return DD<<"[Error] Cannot read BookDataFile!"<<endl,0;
	}
	return 1;
}

namespace PUI_Widget_Name
{
	PictureBox *PicBo_Background=NULL;
	TwinLayerWithDivideLine *TwinLay_MainView=NULL;
	SimpleListView_MultiColor <int> *SLV_FunctionList=NULL;
	
	Layer *Lay_Welcome=NULL;
	TwinLayerWithDivideLine *TwinLay_BookSearchLayer=NULL;
	SimpleListView <int> *SLV_BookSearchResult=NULL;
	Layer *Lay_BookInfoEdit=NULL;
	TwinLayerWithDivideLine *TwinLay_OverviewPage=NULL;
	DetailedListView <int> *DLV_BookDetail=NULL;
	Layer *Lay_Options=NULL,
		  *Lay_PageAbout=NULL;
};
using namespace PUI_Widget_Name;

int SCB_BookSearchMode=0;

enum 
{
	FunctionList_Welcome,
	FunctionList_FastSale,
	FunctionList_BookSearch,
	FunctionList_Overview,
	FunctionList_Options,
	FunctionList_About
};
int CurrentFunctionPage=FunctionList_Welcome;

struct EditBookInfoFuncdata
{
	int pos,TagNum,pInSLV;
	TextEditLine *tel=NULL;
	TextEditBox *teb=NULL;
	Button *bu=NULL;
	
	EditBookInfoFuncdata(int _pos,int tagnum,int _pInSLV,TextEditLine *_tel):pos(_pos),TagNum(tagnum),pInSLV(_pInSLV),tel(_tel) {}
	
	EditBookInfoFuncdata(int _pos,int tagnum,int _pInSLV,TextEditBox *_teb,Button *_bu):pos(_pos),TagNum(tagnum),pInSLV(_pInSLV),teb(_teb),bu(_bu) {}
	
	EditBookInfoFuncdata() {}
};
EditBookInfoFuncdata *EditTELfuncData[9]={0,0,0,0,0,0,0,0,0};

struct CurrentEditingBookInfoStruct
{
	int pos,
		pInListView;
}CurrentEditingBookInfo;

void SetBookToDLV(int DLVpos,int BookPos)
{
	DetailedListView<int>::DetailedListViewData dlvd;
	dlvd.Text.push_back(BookLibrary[BookPos].ISBN);
	dlvd.Text.push_back(BookLibrary[BookPos].BookName);
	dlvd.Text.push_back(BookLibrary[BookPos].Author);
	dlvd.Text.push_back(BookLibrary[BookPos].Publisher);
	dlvd.Text.push_back(BookLibrary[BookPos].BoughtTime);
	dlvd.Text.push_back(llTOstr(BookLibrary[BookPos].CurrentInventory));
	dlvd.Text.push_back(llTOstr(int(BookLibrary[BookPos].WholeSalePrice))+"."+llTOstr(((int)BookLibrary[BookPos].WholeSalePrice*100)%100));
	dlvd.Text.push_back(llTOstr(int(BookLibrary[BookPos].SalePrice))+"."+llTOstr(((int)BookLibrary[BookPos].SalePrice*100)%100));
	dlvd.Text.push_back(BookLibrary[BookPos].Annotation);
	dlvd.FuncData=BookPos;
	PUI_Widget_Name::DLV_BookDetail->SetListContent(DLVpos,dlvd);
}

void CreateLayer_BookInfoEdit(int p,int pInSLV);

void ButtonFunc_DelBook(void *funcdata)
{
	BookLibrary[CurrentEditingBookInfo.pos].Deleted=1;
	CreateLayer_BookInfoEdit(-1,CurrentEditingBookInfo.pInListView);
	if (CurrentFunctionPage==FunctionList_BookSearch)
		SLV_BookSearchResult->DeleteListContent(CurrentEditingBookInfo.pInListView);
	else if (CurrentFunctionPage==FunctionList_Overview)
		DLV_BookDetail->DeleteListContent(CurrentEditingBookInfo.pInListView);
}

void ButtonFunc_CreateCopy(void *funcdata)
{
	BookLibrary.push_back(BookLibrary[CurrentEditingBookInfo.pos]);
	if (!EnableCompleteCopy)
	{
		BookLibrary.back().BookName+=GetRidOfEndChar0_Re(AnsiToUtf8("(副本)"));
		BookLibrary.back().CurrentInventory=0;
		BookLibrary.back().Annotation+=GetRidOfEndChar0_Re(AnsiToUtf8("(快速创建的副本)"));
	}
	if (CurrentFunctionPage==FunctionList_BookSearch)
		SLV_BookSearchResult->SetListContent(CurrentEditingBookInfo.pInListView+1,BookLibrary.back().BookName,BookLibrary.size()-1);
	else if (CurrentFunctionPage==FunctionList_Overview)
		SetBookToDLV(CurrentEditingBookInfo.pInListView+1,BookLibrary.size()-1);
}

void CreateLayer_BookInfoEdit(int p,int pInSLV)
{
	DD<<"[Debug] CreateLayer_BookInfoEdit "<<p<<" "<<pInSLV<<endl;
	static int lastP=-1,lastPage=-1;
	if (lastPage==CurrentFunctionPage&&lastP==p)
		return;
	lastPage=CurrentFunctionPage;
	lastP=p;
	if (Lay_BookInfoEdit!=NULL)
	{
//		delete Lay_BookInfoEdit;
		Lay_BookInfoEdit->DelayDelete();
		Lay_BookInfoEdit=NULL;
	}
	if (pInSLV!=-1&&InRange(p,0,BookLibrary.size()-1)&&!BookLibrary[p].Deleted)
	{
		if (CurrentFunctionPage==FunctionList_BookSearch)
			Lay_BookInfoEdit=new Layer(0,TwinLay_BookSearchLayer->AreaA(),new PosizeEX_Fa6(2,2,0,0,240,0));
		else if (CurrentFunctionPage==FunctionList_Overview)
		{
			TwinLay_OverviewPage->SetDivideLineMode(2,-0.6,-0.2);
			TwinLay_OverviewPage->SetDivideLinePosition(0.618);
			Lay_BookInfoEdit=new Layer(0,TwinLay_OverviewPage->AreaB(),new PosizeEX_Fa6(2,2,0,0,50,0));
			UpdateWidgetsPosize(TwinLay_OverviewPage);
		}
		
		auto EditBookInfoFunc=[](void *funcdata,const stringUTF8 &strUTF8,bool isenter)->void
		{
			auto data=(EditBookInfoFuncdata*)funcdata;
			BookData &bo=BookLibrary[data->pos];
			string str=strUTF8.cppString();
			if (!BookDataModifyRealtimeUpdate)
				if (!isenter)
				{
					data->tel->SetBorderColor(0,{255,198,139,255});
					data->tel->SetBorderColor(1,{255,156,89,255});
					data->tel->SetBorderColor(2,{255,89,0,255});
					return;
				}
				else
					for (int i=0;i<=2;++i)
						data->tel->SetBorderColor(i,RGBA_NONE);
			switch (data->TagNum)
			{
				case 0:	bo.BookName=str;	break;
				case 1:	bo.Author=str;		break;
				case 2:	bo.Publisher=str;	break;
				case 3:	bo.ISBN=str;		break;
				case 4:	bo.BoughtTime=str;	break;
				case 5:	bo.CurrentInventory=strTOint(str);	break;
				case 6:	bo.WholeSalePrice=strTOdb(str);		break;
				case 7:	bo.SalePrice=strTOdb(str);			break;
			}
			if (CurrentFunctionPage==FunctionList_BookSearch)
				SLV_BookSearchResult->SetText(data->pInSLV,bo.BookName);
			else if (CurrentFunctionPage==FunctionList_Overview)
			{
				DLV_BookDetail->DeleteListContent(data->pInSLV);
				SetBookToDLV(data->pInSLV,data->pos);
			}
		};
		
		for (int i=0;i<=8;EditTELfuncData[i++]=NULL)
			if (EditTELfuncData[i]!=NULL)
				delete (EditBookInfoFuncdata*)EditTELfuncData[i];
		TextEditLine *tel=NULL;
		#define TT(x,y) (new TinyText(0,Lay_BookInfoEdit,{0,(y),80,30},AnsiToUtf8(x),1))
		#define TEL(x,y,z) (tel=new TextEditLine(0,Lay_BookInfoEdit,new PosizeEX_Fa6(2,3,100,50,(y),30)))->SetText(x);\
							tel->SetEnterFunc(EditBookInfoFunc,EditTELfuncData[z]=new EditBookInfoFuncdata(p,z,pInSLV,tel))
		TT("书名:",10);			TEL(BookLibrary[p].BookName,10,0);
		TT("作者:",40);			TEL(BookLibrary[p].Author,40,1);
		TT("出版社:",70);		TEL(BookLibrary[p].Publisher,70,2);
		TT("ISBN号:",100);		TEL(BookLibrary[p].ISBN,100,3);
		TT("进书日期:",130);	TEL(BookLibrary[p].BoughtTime,130,4);
		TT("库存量:",160);		TEL(llTOstr(BookLibrary[p].CurrentInventory),160,5);
		TT("批发价:",190);		TEL(llTOstr(int(BookLibrary[p].WholeSalePrice))+"."+llTOstr(((int)BookLibrary[p].WholeSalePrice*100)%100),190,6);
		TT("零售价:",220);		TEL(llTOstr(int(BookLibrary[p].SalePrice))+"."+llTOstr(((int)BookLibrary[p].SalePrice*100)%100),220,7);
		TT("注释:",250);		
		#undef TEL
		#undef TT
		auto TEBofAnnotation=new TextEditBox(0,Lay_BookInfoEdit,new PosizeEX_Fa6(2,3,100,50,250,100));
		TEBofAnnotation->SetText(BookLibrary[p].Annotation);
		TEBofAnnotation->SetEachChangeFunc([](void *funcdata,TextEditBox *teb,bool isInnner)->void
			{
				auto data=(EditBookInfoFuncdata*)funcdata;
				if (!BookDataModifyRealtimeUpdate)
				{
					#define Seco(x,y) data->teb->SetBorderColor(x,y);data->bu->SetButtonColor(x,y)
					Seco(0,RGBA(255,198,139,255));Seco(1,RGBA(255,156,89,255));Seco(2,RGBA(255,89,0,255));
					#undef Seco
				}
				else
				{
					BookLibrary[data->pos].Annotation=teb->GetAllText();
					while (BookLibrary[data->pos].Annotation.back()=='\n'||BookLibrary[data->pos].Annotation.back()=='\r')
						BookLibrary[data->pos].Annotation.erase(BookLibrary[data->pos].Annotation.length()-1,1);
					if (CurrentFunctionPage==FunctionList_Overview)
					{
						DLV_BookDetail->DeleteListContent(data->pInSLV);
						SetBookToDLV(data->pInSLV,data->pos);
					}
				}
			},EditTELfuncData[8]=new EditBookInfoFuncdata(p,8,pInSLV,TEBofAnnotation,NULL));
		TEBofAnnotation->EnableShowSpecialChar(EnableShowTEBSpeacialChar);
		
		if (!BookDataModifyRealtimeUpdate)
			EditTELfuncData[8]->bu=new Button(0,Lay_BookInfoEdit,new PosizeEX_Fa6(1,3,100,50,360,30),AnsiToUtf8("更新注释"),
				[](void *funcdata)->void
				{
					auto data=(EditBookInfoFuncdata*)funcdata;
					BookLibrary[data->pos].Annotation=data->teb->GetAllText();
					while (BookLibrary[data->pos].Annotation.back()=='\n'||BookLibrary[data->pos].Annotation.back()=='\r')
						BookLibrary[data->pos].Annotation.erase(BookLibrary[data->pos].Annotation.length()-1,1);
					for (int i=0;i<=2;++i)
						data->teb->SetBorderColor(i,RGBA_NONE),
						data->bu->SetButtonColor(i,RGBA_NONE);
					if (CurrentFunctionPage==FunctionList_Overview)
					{
						DLV_BookDetail->DeleteListContent(data->pInSLV);
						SetBookToDLV(data->pInSLV,data->pos);
					}
				},EditTELfuncData[8]);
		new Button(0,Lay_BookInfoEdit,{100,360,80,30},AnsiToUtf8("删除"),ButtonFunc_DelBook);
		new Button(0,Lay_BookInfoEdit,CurrentFunctionPage==FunctionList_Overview&&!BookDataModifyRealtimeUpdate?Posize(100,400,80,30):Posize(200,360,80,30),AnsiToUtf8("创建副本"),ButtonFunc_CreateCopy,NULL);
		if (CurrentFunctionPage==FunctionList_Overview)
			new Button(0,Lay_BookInfoEdit,new PosizeEX_Fa6(1,3,100,50,400,30),AnsiToUtf8("关闭"),[](void *funcdata)->void{CreateLayer_BookInfoEdit(-1,-1);});
	}
	else if (pInSLV==-1&&CurrentFunctionPage==FunctionList_Overview)
	{
		TwinLay_OverviewPage->SetDivideLineMode(2,-1,0);
		TwinLay_OverviewPage->SetDivideLinePosition(1);
	}
}

void ListViewOfBookFunc(int &funcdata,int pos,int click)
{
	DD<<"[Debug] ListViewOfBookFunc "<<funcdata<<" "<<pos<<" "<<click<<endl;
	CurrentEditingBookInfo.pos=funcdata;
	CurrentEditingBookInfo.pInListView=pos;
	if (click==1)
		CreateLayer_BookInfoEdit(funcdata,pos);
	else if (click==3)
	{
		auto menudata=new vector <MenuData>;
		if (pos!=-1)
		{
			menudata->push_back(MenuData(GetRidOfEndChar0_Re(AnsiToUtf8("查看")),
				[](void *funcdata)->void
				{CreateLayer_BookInfoEdit(CurrentEditingBookInfo.pos,CurrentEditingBookInfo.pInListView);}
				,NULL,'V'));
			menudata->push_back(MenuData(0));
			menudata->push_back(MenuData(GetRidOfEndChar0_Re(AnsiToUtf8("删除")),ButtonFunc_DelBook,NULL,'D'));
			menudata->push_back(MenuData(GetRidOfEndChar0_Re(AnsiToUtf8("创建副本")),ButtonFunc_CreateCopy,NULL,'C'));
		}
		
		if (CurrentFunctionPage==FunctionList_Overview)
		{
			if (pos!=-1)
				menudata->push_back(MenuData(0));
			menudata->push_back(MenuData(GetRidOfEndChar0_Re(AnsiToUtf8("快速新建")),
				[](void *funcdata)->void
				{
					BookLibrary.push_back(BookData());
					BookLibrary.back().BookName=GetRidOfEndChar0_Re(AnsiToUtf8("(新图书)"));
					BookLibrary.back().Annotation=GetRidOfEndChar0_Re(AnsiToUtf8("(快速新建的图书)"));
					SetBookToDLV(CurrentEditingBookInfo.pInListView+1,BookLibrary.size()-1);
				},NULL,'N'));
		}
		if (menudata->empty())
			delete menudata;
		else new Menu1(0,menudata,1);
	}
}

int DLVsortColumn=0,DLVsortDirection=0;
void DLVsortFunc(vector <DetailedListView<int>::DetailedListViewData> &listData,const string &sortTagName,int columnPos,bool sortDirection)
{
	DD<<"[Info] Sort by "<<sortTagName<<" "<<sortDirection<<endl;
	DLVsortColumn=columnPos;
	DLVsortDirection=sortDirection;
	sort(listData.begin(),listData.end(),
		[](const DetailedListView<int>::DetailedListViewData &a,const DetailedListView<int>::DetailedListViewData &b)->bool
		{
			#define CMP(x,y) DLVsortDirection?((x)>(y)):((x)<(y))//Careful!:d?a>b:a<b not equiv (a<b)^d
			if (DLVsortColumn<=4||DLVsortColumn==8)
				return CMP(a.Text[DLVsortColumn],b.Text[DLVsortColumn]);
			else switch(DLVsortColumn)
			{
				case 5: return CMP(BookLibrary[a.FuncData].CurrentInventory,BookLibrary[b.FuncData].CurrentInventory);
				case 6: return CMP(BookLibrary[a.FuncData].WholeSalePrice,BookLibrary[b.FuncData].WholeSalePrice);
				case 7: return CMP(BookLibrary[a.FuncData].SalePrice,BookLibrary[b.FuncData].SalePrice);
			}
			#undef CMP
		});
}

void SetBackgroundPic(string str)
{
//	MainWindow->SetBackgroundColor(RGBA_NONE);
	PicBo_Background->SetPicture(CreateTextureFromSurfaceAndDelete(IMG_Load(str.c_str()),MainWindow),1,0);
}

TextEditLine *TEL_SetBgPic=NULL;

void SLV_FuncListfunc(int &funcdata,int pos,int click)
{
	if (pos==-1) return;
	if (click==1)
	{
		static int lastPage=-1;
		if (lastPage==funcdata)
			return;
		CurrentFunctionPage=funcdata;
		if (TwinLay_BookSearchLayer!=NULL)
			SLV_BookSearchResult=NULL,Lay_BookInfoEdit=NULL;
		#define DELwg(x) if (x!=NULL) {/*delete x;*/x->DelayDelete();x=NULL;}
		DELwg(Lay_Welcome);
//		DELwg();
		DELwg(TwinLay_BookSearchLayer);
		DELwg(TwinLay_OverviewPage);
		DELwg(Lay_Options);
		DELwg(Lay_PageAbout);
		#undef DELwg
		
		switch (funcdata)
		{
			case FunctionList_Welcome:
			{
				Lay_Welcome=new Layer(0,TwinLay_MainView->AreaB(),new PosizeEX_Fa6(2,2,0,0,0,0));
				(new TinyText(0,Lay_Welcome,{62,60,300,30},AnsiToUtf8("C++ 课程设计"),-1,{137,188,255,255}))->SetFontSize(20);
				(new TinyText(0,Lay_Welcome,{60,100,520,50},AnsiToUtf8("图书管理系统(With PAL_GUI)"),-1,{48,149,255,255}))->SetFontSize(40);
				(new TinyText(0,Lay_Welcome,new PosizeEX_Fa6(3,1,65,300,30,200),AnsiToUtf8("By：qianpinyi"),-1,{63,147,201,255}))->SetFontSize(20);
				(new TinyText(0,Lay_Welcome,new PosizeEX_Fa6(3,1,65,300,30,160),AnsiToUtf8("学号：161910130"),-1,{63,147,201,255}))->SetFontSize(20);
				(new TinyText(0,Lay_Welcome,new PosizeEX_Fa6(3,1,65,300,30,120),AnsiToUtf8("班级：1619001"),-1,{63,147,201,255}))->SetFontSize(20);
				ShapedPictureButton *spb;
				static SDL_Surface *mask=IMG_Load("resources/StartPageButton-1.png");
				SDL_Surface *sur[3];
				for (int i=0;i<=2;++i)
					sur[i]=SDL_CreateRGBSurfaceWithFormat(0,160,160,32,SDL_PIXELFORMAT_RGBA32),
					SDL_SetSurfaceBlendMode(sur[i],SDL_BLENDMODE_BLEND);
				for (int i=0;i<160;++i)
					for (int j=0;j<160;++j)
					{
						RGBA co0=GetSDLSurfacePixel(mask,{i,j});
						for (int k=0;k<=2;++k)
						{
							RGBA co=ThemeColor[k*2+1];
							co.a=co0.a;
							SetSDLSurfacePixel(sur[k],{i,j},co);
						}
					}
				auto spbFunc=[](void *funcdata)->void
					{
						int pageNum=FunctionList_Welcome;
						if (funcdata==CONST_Ptr_0) pageNum=FunctionList_BookSearch;
						else if (funcdata==CONST_Ptr_1) pageNum=FunctionList_Overview;
						else if (funcdata==CONST_Ptr_2) pageNum=FunctionList_Options;
						else if (funcdata==CONST_Ptr_3) pageNum=FunctionList_About;
						SLV_FuncListfunc(pageNum,-2,1);
					};
				#define SPB(xr,yr,text,funcdata) spb=new ShapedPictureButton(0,Lay_Welcome,new PosizeEX_Fa6(1,1,160,xr,160,yr),spbFunc,funcdata);\
											spb->SetText(AnsiToUtf8(text));\
											spb->SetMaskPic(mask,0);\
											for (int i=0;i<=2;++i)\
												spb->SetButtonPic(i,CreateTextureFromSurface(sur[i]),1);
				SPB(160,260,"图书查询",CONST_Ptr_0);
				SPB(260,160,"图书一览",CONST_Ptr_1);
				SPB(60,160,"杂项设置",CONST_Ptr_2);
				SPB(160,60,"关于",CONST_Ptr_3);
				for (int i=0;i<=2;++i)
					SDL_FreeSurface(sur[i]);
				break;
			}
			case FunctionList_FastSale:
			{
				break;
			}
			case FunctionList_BookSearch:
			{
				TwinLay_BookSearchLayer=new TwinLayerWithDivideLine(0,TwinLay_MainView->AreaB(),new PosizeEX_Fa6(2,2,0,0,20,20),1,0.618);
				TwinLay_BookSearchLayer->SetDivideLineMode(0,320,-0.3);
				auto Lay_SearchFunctions=new Layer(0,TwinLay_BookSearchLayer->AreaA(),new PosizeEX_Fa6(2,3,0,0,0,240));
				(new TinyText(0,Lay_SearchFunctions,new PosizeEX_MidFa({0,-90,300,20}),AnsiToUtf8("图 书 查 询 模 块")))->SetFontSize(20);
				new TextEditLine(0,Lay_SearchFunctions,new PosizeEX_MidFa({0,-50,300,30}),
					[](void *funcdata,const stringUTF8 &str,bool isenter)->void
					{
						if (!isenter) return;
						SLV_BookSearchResult->ClearListContent();
						CreateLayer_BookInfoEdit(-1,-1);
						string keyword=str.cppString();
						for (int i=0;i<BookLibrary.size();++i)
							if (!BookLibrary[i].Deleted)
							{
								int result=0;
								#define REOR(x,y) result|=(int)(BookLibrary[i].x.find(keyword)!=BookLibrary[i].x.npos)<<y
								REOR(BookName,0);REOR(Author,1);REOR(ISBN,2);REOR(Publisher,3);REOR(Annotation,4);
								#undef REOR
								if (SCB_BookSearchMode==5&&result>0||(result&(1<<SCB_BookSearchMode)))
									SLV_BookSearchResult->PushbackContent(BookLibrary[i].BookName,i);
							}
					});
				auto SCB=new SingleChoiceButton(0,Lay_SearchFunctions,new PosizeEX_MidFa({0,40,200,140}),[](void*,const string&,int pos)->void{SCB_BookSearchMode=pos;});
				#define AddC(x) AddChoice(AnsiToUtf8(x))
				SCB->AddC("书名")->AddC("作者")->AddC("ISBN号")->AddC("出版社")->AddC("注释")->AddC("全部");
				#undef AddC
				SCB->SetChoice(SCB_BookSearchMode);
				
				new TinyText(0,TwinLay_BookSearchLayer->AreaB(),new PosizeEX_Fa6(2,3,10,10,10,30),AnsiToUtf8("查询结果"));
				SLV_BookSearchResult=new SimpleListView <int> (0,TwinLay_BookSearchLayer->AreaB(),new PosizeEX_Fa6(2,2,0,0,50,20),ListViewOfBookFunc);
				break;
			}
			case FunctionList_Overview:
			{
				TwinLay_OverviewPage=new TwinLayerWithDivideLine(0,TwinLay_MainView->AreaB(),new PosizeEX_Fa6(2,2,0,0,0,0),1,1);
				TwinLay_OverviewPage->SetDivideLineMode(2,-1,0);
				TwinLay_OverviewPage->SetDivideLinePosition(1);
				DLV_BookDetail=new DetailedListView <int> (0,TwinLay_OverviewPage->AreaA(),new PosizeEX_Fa6(2,2,0,0,10,0),ListViewOfBookFunc);
				#define Push(x,y) PushbackColumnInfo(DetailedListView <int>::DetailedListViewColumn(AnsiToUtf8(x),y))
				DLV_BookDetail->Push("ISBN号",170)->Push("书名",160)->Push("作者",80)->Push("出版社",140)->Push("进书日期",120)->Push("库存量",60)->Push("批发价",60)->Push("零售价",60)->Push("注释",300);
				#undef Push
				DLV_BookDetail->SetEnableAutoMainTextPos(0);
				DLV_BookDetail->SetMainTextPos(1);
				DLV_BookDetail->SetSortFunc(DLVsortFunc);
				
				for (int i=0;i<BookLibrary.size();++i)
					if (!BookLibrary[i].Deleted)
						SetBookToDLV(1e9,i);
				break;
			}
			case FunctionList_Options:
			{
				Lay_Options=new Layer(0,TwinLay_MainView->AreaB(),new PosizeEX_Fa6(2,2,0,0,0,0));
				(new TinyText(0,Lay_Options,{50,40,300,40},AnsiToUtf8("设置"),-1))->SetFontSize(18);
				TinyText *tt=NULL;
				SwitchButton *sb=NULL;
				#define TTSB(str,y,val) \
					tt=new TinyText(0,Lay_Options,{100,y,300,20},AnsiToUtf8(str),-1);\
					(sb=new SwitchButton(0,Lay_Options,new PosizeEX_Fa6(1,3,40,-0.618,(y)+2,16),val,[](void *funcdata,bool on)->void{val=on;}))->SetChooseColor({91,112,255,255});
				TTSB("文本修改实时更新",100,BookDataModifyRealtimeUpdate);
				TTSB("关于页面字体缩放",130,EnableAboutPageTEBscrollResize);
				TTSB("文本编辑框显示特殊字符",160,EnableShowTEBSpeacialChar);
				TTSB("创建完全相同的副本",190,EnableCompleteCopy);
				TTSB("使用下方设定的背景图片",220,EnableBackgroundPic);
				sb->SetFunc([](void *funcdata,bool on)->void
					{
						EnableBackgroundPic=on;
						SetBackgroundPic(on?TEL_SetBgPic->GetText():"");
					});
				(TEL_SetBgPic=new TextEditLine(0,Lay_Options,{100,250,290,30}))->SetText(BackgroundPicPath);
				TEL_SetBgPic->SetEnterFunc([](void *funcdata,const stringUTF8 &str,bool isenter)->void
					{
						if (isenter)
						{
							BackgroundPicPath=str.cppString();
							if (EnableBackgroundPic)
								SetBackgroundPic(str.cppString());
							for (int i=0;i<=2;++i)
								TEL_SetBgPic->SetBorderColor(i,RGBA_NONE);
						}
						else
						{
							TEL_SetBgPic->SetBorderColor(0,{255,198,139,255});
							TEL_SetBgPic->SetBorderColor(1,{255,156,89,255});
							TEL_SetBgPic->SetBorderColor(2,{255,89,0,255});
						}
					});
				new TinyText(0,Lay_Options,{100,290,300,20},AnsiToUtf8("背景图片透明度"),-1);
				(new Slider(0,Lay_Options,{100,320,290,10},0,
					[](void *funcdata,double per,bool looseMouse)->void
					{
						BgPicOpacity=per;
						TwinLay_MainView->SetAreaAColor({255,255,255,per*255});
						TwinLay_MainView->SetAreaBColor({255,255,255,per*255});
					}))->SetPercent(BgPicOpacity);
				TTSB("默认启用调试输出",340,EnableDebugOut);
				TTSB("默认启用硬件加速绘制",370,EnableAccelerate);
				#undef TTSB
				
				break;
			}
			case FunctionList_About:
			{
				Lay_PageAbout=new Layer(0,TwinLay_MainView->AreaB(),new PosizeEX_Fa6(2,2,0,0,0,0));
				(new TinyText(0,Lay_PageAbout,new PosizeEX_Fa6(2,3,0,0,40,30),AnsiToUtf8("C++ 课程设计")))->SetFontSize(20);
				(new TinyText(0,Lay_PageAbout,new PosizeEX_Fa6(2,3,0,0,75,40),AnsiToUtf8("图书管理系统")))->SetFontSize(30);
				(new TinyText(0,Lay_PageAbout,new PosizeEX_Fa6(2,3,0,0,120,30),AnsiToUtf8("(With PAL_GUI)")))->SetFontSize(20);
				(new TinyText(0,Lay_PageAbout,new PosizeEX_Fa6(2,3,0,0,155,30),AnsiToUtf8("By:qianpinyi")))->SetFontSize(20);
				(new Button(0,Lay_PageAbout,new PosizeEX_Fa6(3,3,100,220,190,30),AnsiToUtf8("邮箱:qianpinyi@outlook.com"),
					[](void *funcdata)->void
					{
						ShellExecuteW(0,L"open",L"mailto:qianpinyi@outlook.com",L"",L"",SW_SHOWNORMAL);
						((Button*)funcdata)->SetTextColor({0,100,255,255});
					},CONST_THIS))->AddPsEx(new PosizeEX_MidFa_Single(0));
				auto TEB_report=new TextEditBox(0,Lay_PageAbout,new PosizeEX_Fa6(2,2,50,50,230,30));
				TEB_report->SetEnableEdit(0);
				TEB_report->SetEnableScrollResize(EnableAboutPageTEBscrollResize);
				TEB_report->EnableShowSpecialChar(EnableShowTEBSpeacialChar);
				ifstream fin("report.txt");
				if (fin.is_open())
				{
					string str;
					char ch;
					while ((ch=fin.get())!=-1)
						if (ch=='\n')
						{
							TEB_report->AppendNewLine(str+"\n");
							str.clear();
						}
						else if (ch!='\r')
							str+=ch;
					TEB_report->DeleteLine(0);
				}
				else DD<<"[Error] Cannot open report file: report.txt"<<endl;
				break;
			}
		}
	}
}

void InitUI()
{
//	using namespace PUI_Widget_Name;
	if (PicBo_Background==NULL)
	{
		PicBo_Background=new PictureBox(0,MainWindow->BackGroundLayer,new PosizeEX_Fa6(2,2,0,0,0,0));
		PicBo_Background->SetBackGroundColor(RGBA_NONE);
		if (EnableBackgroundPic)
			SetBackgroundPic(BackgroundPicPath);
	}
	
	TwinLay_MainView=new TwinLayerWithDivideLine(0,PicBo_Background,new PosizeEX_Fa6(2,2,0,0,0,0),1,0.2);
	TwinLay_MainView->SetDivideLineMode(1,140,-0.6);
	TwinLay_MainView->SetAreaAColor({255,255,255,BgPicOpacity*255});
	TwinLay_MainView->SetAreaBColor({255,255,255,BgPicOpacity*255});
	new Button(0,TwinLay_MainView->AreaA(),new PosizeEX_Fa6(2,3,10,10,10,35),AnsiToUtf8("图书管理系统"));
	SLV_FunctionList=new SimpleListView_MultiColor <int> (0,TwinLay_MainView->AreaA(),new PosizeEX_Fa6(2,2,0,0,55,5),SLV_FuncListfunc);
	SLV_FunctionList->SetTextMode(0);
	SLV_FunctionList->SetRowHeightAndInterval(40,4);
	#define P(x,y) PushbackContent(AnsiToUtf8(x),y)
	SLV_FunctionList->P("起始页",FunctionList_Welcome)
//					->P("快速销售",FunctionList_FastSale)
					->P("图书查询",FunctionList_BookSearch)
					->P("图书一览",FunctionList_Overview)
					->P("杂项设置",FunctionList_Options)
					->P("关于",FunctionList_About);
	#undef P
	SLV_FuncListfunc(SLV_FunctionList->GetFuncData(0),0,1);
}

void ReadWriteCfg(bool isRead)
{
	if (isRead)
	{
		if (cfg.Read()!=0)
			DD<<"[Error] Cannot read cfg file. Error code "<<cfg.GetLastStat()<<endl,exit(1);
		BookDataModifyRealtimeUpdate=cfg("BookDataModifyRealtimeUpdate")=="1";
		EnableAboutPageTEBscrollResize=cfg("EnableAboutPageTEBscrollResize")=="1";
		EnableShowTEBSpeacialChar=cfg("EnableShowTEBSpeacialChar")=="1";
		EnableCompleteCopy=cfg("EnableCompleteCopy")=="1";
		EnableBackgroundPic=cfg("EnableBackgroundPic")=="1";
		BackgroundPicPath=cfg("BackgroundPicPath");
		BgPicOpacity=strTOll(cfg("BgPicOpacity"))/255.0;
		EnableDebugOut=cfg("EnableDebugOut")=="1";
		EnableAccelerate=cfg("EnableAccelerate")=="1";
	}
	else
	{
		cfg("BookDataModifyRealtimeUpdate")=BookDataModifyRealtimeUpdate?"1":"0";
		cfg("EnableAboutPageTEBscrollResize")=EnableAboutPageTEBscrollResize?"1":"0";
		cfg("EnableShowTEBSpeacialChar")=EnableShowTEBSpeacialChar?"1":"0";
		cfg("EnableCompleteCopy")=EnableCompleteCopy?"1":"0";
		cfg("EnableBackgroundPic")=EnableBackgroundPic?"1":"0";
		cfg("BackgroundPicPath")=BackgroundPicPath;
		cfg("BgPicOpacity")=llTOstr(BgPicOpacity*255);
		cfg("EnableDebugOut")=EnableDebugOut?"1":"0";
		cfg("EnableAccelerate")=EnableAccelerate?"1":"0";
		if (cfg.Write()!=0)
			DD<<"[Error] Cannot write cfg file. Error code "<<cfg.GetLastStat()<<endl;
	}
}

int main(int argc,char **argv)
{
	SDL_SetMainReady();
	system("chcp 65001");
	system("cls");
	DD.SetLOGFile("Log.txt");
	DD%DebugOut_Channel::DebugOut_CERR_LOG;
	DD<<AnsiToUtf8("C++课程设计:图书管理系统 (With PAL_GUI)")<<endl
	  <<"By:qianpinyi"<<endl;
	
	ReadWriteCfg(1);

	if (!EnableDebugOut)
		DD%DebugOut_Channel::DebugOut_OFF;
	
	if (!ReadWriteBookDataFile("BookData.dat",0))
		return DD<<"[Error] Read BookData file failed"<<endl,1;
	
	
	if (EnableAccelerate) PAL_GUI_Init({300,200,1280,720},AnsiToUtf8("图书管理系统"));
	else PAL_GUI_Init({300,200,1280,720},AnsiToUtf8("图书管理系统"),SDL_WINDOW_RESIZABLE,SDL_RENDERER_SOFTWARE);
	
	InitUI();
	
	EasyEventLoop();
	
	PAL_GUI_Quit();
	if (!ReadWriteBookDataFile("BookData.dat",1))
		DD<<"[Error] Write BookData file failed"<<endl;
	ReadWriteCfg(0);
	return 0;
}
