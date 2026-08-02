//****************************************************************// 
// MQ2Clickies v1.02
// by s0rCieR 2005.07.24
//****************************************************************// 
// I was real tired of having to edit my UI files each time i get
// a new clicky items, making lots of custom folders for each of
// my toons that i play on regular basis.
//
// I Wake Up this morning with idea of making a plugin to display 
// and manage clickies easily. A 2 windows design, one to display
// clickies, and one to manage them.
//
// I know it's real ugly on first time startup, windows positions
// and size are a mess so suggest you position them way you likes
// them and use /clickies save
//****************************************************************// 
// Usage: /clickies           -> Display Setting window if active.
// Usage: /clickies on        -> Turn on Clickies support.
// Usage: /clickies off       -> Turn off Clickies support.
// Usage: /clickies save      -> Save clickies and windows.
//****************************************************************// 
// v1.00 initial write up
// v1.01 fixed escape closing up clickies window
// v1.02 added a test to not try to remove xml if no loaded
//       added more display status msg.

#include "../MQ2Plugin.h" 

PreSetup("MQ2Clickies");

#define      MAXI_INVSLOT 30      // Maximum Inventory Slot #
bool         InGame=false;        // InGame Flag?
bool         Active=false;        // Active Flag?
bool         cBOOL[MAXI_INVSLOT]; // Clickies Flag?

void ReadINI() {
  CHAR zBuff[MAX_STRING];  
  GetPrivateProfileString("Clickies","Active","off",zBuff,MAX_STRING,INIFileName);
  Active=!strnicmp(zBuff,"on",2);
  for(int c=0; c<MAXI_INVSLOT; c++) {
    GetPrivateProfileString("Clickies",szItemSlot[c],"on",zBuff,MAX_STRING,INIFileName);
    cBOOL[c]=!strnicmp(zBuff,"on",2);
  }
}

void SaveINI() {
  WritePrivateProfileString("Clickies",NULL,NULL,INIFileName);
  WritePrivateProfileString("Clickies","Active",Active?"on":"off",INIFileName);
  for(int c=0; c<MAXI_INVSLOT; c++)
  WritePrivateProfileString("Clickies",szItemSlot[c],cBOOL[c]?"on":"off",INIFileName);
}

class CMyClickWnd : public CCustomWnd {
public:
  CInvSlotWnd *cSLOT[MAXI_INVSLOT];
  RECT         cRECT[MAXI_INVSLOT];
  RECT         cBACK;               
  CMyClickWnd():CCustomWnd("ClickWnd") {
    this->Visible(false);
    cBACK.left=-250; cBACK.top=-250; cBACK.right=-200; cBACK.bottom=-200;
    CHAR zBuff[MAX_STRING];
    for(int c=0; c<MAXI_INVSLOT; c++) {
      sprintf(zBuff,"ClickSlot%d",c);
    	cSLOT[c]=(CInvSlotWnd*)GetChildItem(zBuff);
    	cRECT[c]=cSLOT[c]->Location;
    }
    BitOff(this->WindowStyle,(CWS_CLOSE|CWS_MINIMIZE|CWS_RESIZEALL));
    SetWndNotification(CMyClickWnd);
  }
  void Updates() {
  	long Lo=0; long x2=0; long y2=0; long ct=0; RECT ws;
    for(int c=0; c<MAXI_INVSLOT; c++) if(this->cSLOT[c]) {
      if(cBOOL[c]) {
      	ws=this->cRECT[Lo++];
      	this->cSLOT[c]->Location=ws;
      	if(ws.right>x2) x2=ws.right;
      	if(ws.bottom>y2) y2=ws.bottom;
      	ct++;
      } else this->cSLOT[c]->Location=this->cBACK;
    }
    if(ct>2) {
    	if(((this->WindowStyle)&(CWS_TITLE))==CWS_TITLE)   { y2+=16; }
    	if(((this->WindowStyle)&(CWS_BORDER))==CWS_BORDER) { y2+=8; x2+=8; }
    	this->Location.bottom=this->Location.top+y2;
    	this->Location.right=this->Location.left+x2;
    }
  }
  void Visible(bool Wanted) {
    ((CXWnd*)this)->Show(Wanted,Wanted);
    if(Wanted) this->Updates();
  }
  int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown) {
    if(!pWnd && Message==XWM_CLOSE && Active) this->Visible(true);
    return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);    
  }
  ~CMyClickWnd() {} 
}; 

CMyClickWnd *MyClickWnd = 0;

class CMyClickSet : public CCustomWnd {
public:
  CListWnd *cLIST;
  CMyClickSet():CCustomWnd("ClickSet") {
    this->Visible(false);
    cLIST=(CListWnd*)GetChildItem("ClickList");
    SetWndNotification(CMyClickSet);
    BitOn(WindowStyle,CWS_CLOSE);
    BitOff(WindowStyle,CWS_MINIMIZE);    
  }
  void Updates() {
    CHAR zBuff[MAX_STRING];
    if(this->cLIST) {
    	this->cLIST->DeleteAll();
      for(int c=0; c<MAXI_INVSLOT; c++) {
        sprintf(zBuff,"  [%s]   %s",cBOOL[c]?"O":"X",szItemSlot[c]);
        this->cLIST->AddString(zBuff,cBOOL[c]?0xFF00FF00:0xFFFFFFFF,0,0);
      }
    }
  }
  void Visible(bool Wanted) {
    ((CXWnd*)this)->Show(Wanted,Wanted);
    if(Wanted) this->Updates();
  }
  int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown) {
  	if(pWnd) {
      if(Message==XWM_CLOSE) {
      	SaveINI();
      	this->Visible(false);
      }
      if((Message==XWM_LCLICK || Message==XWM_RCLICK)) {
        long CurSel=((CListWnd*)pWnd)->GetCurSel();
      	if(pWnd==(CXWnd*)cLIST && this->cLIST && CurSel>-1) {
         	cBOOL[CurSel]=!cBOOL[CurSel];
      	  if(MyClickWnd) MyClickWnd->Updates();
       	  this->Updates();
        }
      }
    }
    return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);    
  }
  ~CMyClickSet() {}
};
CMyClickSet *MyClickSet = 0;

void WindowLoad(PCSIDLWND pWindow, PCHAR FileSec, PCHAR WindowName) {
  pWindow->Location.top    = GetPrivateProfileInt(FileSec,"Top",         pWindow->Location.top   ,INIFileName); 
  pWindow->Location.bottom = GetPrivateProfileInt(FileSec,"Bottom",      pWindow->Location.bottom,INIFileName); 
  pWindow->Location.left   = GetPrivateProfileInt(FileSec,"Left",        pWindow->Location.left  ,INIFileName); 
  pWindow->Location.right  = GetPrivateProfileInt(FileSec,"Right",       pWindow->Location.right ,INIFileName); 
  pWindow->Locked          = GetPrivateProfileInt(FileSec,"Locked",      pWindow->Locked         ,INIFileName); 
  pWindow->Fades           = GetPrivateProfileInt(FileSec,"Fades",       pWindow->Fades          ,INIFileName); 
  pWindow->TimeMouseOver   = GetPrivateProfileInt(FileSec,"Delay",       pWindow->TimeMouseOver  ,INIFileName); 
  pWindow->FadeDuration    = GetPrivateProfileInt(FileSec,"Duration",    pWindow->FadeDuration   ,INIFileName); 
  pWindow->Alpha           = GetPrivateProfileInt(FileSec,"Alpha",       pWindow->Alpha          ,INIFileName); 
  pWindow->FadeToAlpha     = GetPrivateProfileInt(FileSec,"FadeToAlpha", pWindow->FadeToAlpha    ,INIFileName); 
  pWindow->BGType          = GetPrivateProfileInt(FileSec,"BGType",      pWindow->BGType         ,INIFileName); 
  pWindow->BGColor.R       = GetPrivateProfileInt(FileSec,"BGTint.red",  pWindow->BGColor.R      ,INIFileName); 
  pWindow->BGColor.G       = GetPrivateProfileInt(FileSec,"BGTint.green",pWindow->BGColor.G      ,INIFileName); 
  pWindow->BGColor.B       = GetPrivateProfileInt(FileSec,"BGTint.blue", pWindow->BGColor.B      ,INIFileName);
  SetCXStr(&pWindow->WindowText,WindowName);
}

void WindowSave(PCSIDLWND pWindow, PCHAR FileSec) {
  CHAR Buffer[MAX_STRING];
  RECT CurLoc=pWindow->Minimized?pWindow->OldLocation:pWindow->Location;
  GetCXStr(pWindow->WindowText,Buffer); 
  WritePrivateProfileString(FileSec,NULL,NULL,INIFileName);
  WritePrivateProfileString(FileSec,"WindowTitle",                                 Buffer,INIFileName); 
  WritePrivateProfileString(FileSec,"Top",         itoa(CurLoc.top,            Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Bottom",      itoa(CurLoc.bottom,         Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Left",        itoa(CurLoc.left,           Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Right",       itoa(CurLoc.right,          Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Locked",      itoa(pWindow->Locked,       Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Fades",       itoa(pWindow->Fades,        Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Delay",       itoa(pWindow->MouseOver,    Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Duration",    itoa(pWindow->FadeDuration, Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"Alpha",       itoa(pWindow->Alpha,        Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"FadeToAlpha", itoa(pWindow->FadeToAlpha,  Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"BGType",      itoa(pWindow->BGType,       Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"BGTint.red",  itoa(pWindow->BGColor.R,    Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"BGTint.green",itoa(pWindow->BGColor.G,    Buffer,10),INIFileName);
  WritePrivateProfileString(FileSec,"BGTint.blue", itoa(pWindow->BGColor.B,    Buffer,10),INIFileName);
}

void WindowCreate() {
	InGame=false;
	if(MQ2Globals::gGameState==GAMESTATE_INGAME) {
    if(GetCharInfo2() && GetCharInfo() && GetCharInfo()->pSpawn) {
      InGame=true;
    	sprintf(INIFileName,"%s\\MQ2clickies_%s.ini",gszINIPath,GetCharInfo()->Name);
      ReadINI();
      if(pSidlMgr->FindScreenPieceTemplate("ClickWnd")) {
    	  if(!MyClickWnd) MyClickWnd= new CMyClickWnd();
        WindowLoad((PCSIDLWND)MyClickWnd,"ClickWnd","Clickies");
      }
      if(pSidlMgr->FindScreenPieceTemplate("ClickSet")) {
        if(!MyClickSet) MyClickSet= new CMyClickSet();
        WindowLoad((PCSIDLWND)MyClickSet,"ClickSet","Settings");
      }
      if(MyClickWnd && Active) MyClickWnd->Visible(true);
    }
  }
} 

void WindowRemove() {
	SaveINI();
	if(MyClickWnd) {
		WindowSave((PCSIDLWND)MyClickWnd,"ClickWnd");
		delete MyClickWnd;
	  MyClickWnd=0;
	}
	if(MyClickSet) {
    WindowSave((PCSIDLWND)MyClickSet,"ClickSet");
	  delete MyClickSet;
	  MyClickSet=0;
	}
}

void ClickiesCommand(PSPAWNINFO pCHAR, PCHAR zLine) {
  CHAR zParm[MAX_STRING]; 
  GetArg(zParm,zLine,1);
  if(pSidlMgr->FindScreenPieceTemplate("ClickWnd") && MyClickWnd && MyClickSet) {
  	if(!strnicmp(zParm,"save",4)) {
      WriteChatf("MQ2Clickies:: Saving Settings...");
  		WindowSave((PCSIDLWND)MyClickWnd,"ClickWnd");
      WindowSave((PCSIDLWND)MyClickSet,"ClickSet");
      SaveINI();
    } else if(!strnicmp(zParm,"on",2)) {
    	Active=true;
    } else if(!strnicmp(zParm,"off",3)) {
    	Active=false;
    } else if(Active) {
      MyClickWnd->Visible(true);
    	MyClickSet->Visible(true);
    	WriteChatf("MQ2Clickies:: Popping settings window...");
    }
    MyClickWnd->Visible(Active);
  	if(!Active) MyClickSet->Visible(false);
  	WriteChatf("MQ2Clickies:: Clickes are %s...",Active?"enable":"disable");
	} else WriteChatf("MQ2Clickies:: XML not Loaded!");
}

PLUGIN_API VOID InitializePlugin(VOID) { 
  DebugSpewAlways("Initializing MQ2Clickies"); 
  AddXMLFile("MQUI_ClickiesWnd.xml");
  AddCommand("/Clickies",ClickiesCommand); 
} 

PLUGIN_API VOID ShutdownPlugin(VOID) { 
  DebugSpewAlways("Shutting down MQ2Clickies");
  WindowRemove();
  if(pSidlMgr->FindScreenPieceTemplate("ClickWnd"))
  	RemoveXMLFile("MQUI_ClickiesWnd.xml");
  RemoveCommand("/Clickies");
} 

PLUGIN_API VOID OnCleanUI(VOID) { 
  DebugSpewAlways("MQ2Clickies::OnCleanUI()"); 
  WindowRemove();
} 

PLUGIN_API VOID OnReloadUI(VOID) { 
  DebugSpewAlways("MQ2Clickies::OnReloadUI()"); 
  WindowCreate();
} 

PLUGIN_API VOID SetGameState(DWORD GameState) { 
  DebugSpewAlways("MQ2Clickies::SetGameState(%d)",GameState);
  WindowCreate();
}

