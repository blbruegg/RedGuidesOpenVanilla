// MQ2TimerColor.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.

#include "../MQ2Plugin.h"
PreSetup("MQ2TimerColor");

void CreateTimerWnd();
void CreateAlphaWnd();
void SaveTimerColor(ARGBCOLOR color);

ARGBCOLOR TimerColor;
ARGBCOLOR Palette[]=
{
   {0x00,0x00,0xF0,0xFF}, //Red
   {0x00,0xF0,0x00,0xFF}, //Green
   {0xF0,0x00,0x00,0xFF}, //Blue
   {0x00,0xF0,0xF0,0xFF}, //Yellow
   {0xF0,0xF0,0xF0,0xFF}, //White
   {0x00,0x00,0x7F,0xFF}, //Dark Red
   {0x00,0x7F,0x00,0xFF}, //Dark Green
   {0x7F,0x00,0x00,0xFF}, //Dark Blue
   {0x00,0x7F,0x7F,0xFF}, //Dark Yellow
   {0x7F,0x7F,0x7F,0xFF}, //Gray
   {0xF0,0xF0,0x00,0xFF}, //Teal
   {0x7F,0x7F,0x00,0xFF}, //Dark Teal
   {0x00,0x7F,0xF0,0xFF}, //Orange
   {0x00,0x40,0x7F,0xFF}, //Brown
   {0x00,0x00,0x00,0xFF}, //Black
};

class CTimerColorWnd : public CCustomWnd
{
public:
   CTimerColorWnd():CCustomWnd("ColorPickerWnd")
   {
      SetWndNotification(CTimerColorWnd);
      BitOn(WindowStyle,CWS_CLOSE);
      BitOff(WindowStyle,CWS_MINIMIZE);
      
      for(int i=0; i<15; i++)
      {
         sprintf(Temp,"CPW_TC%dButton",i);
         ColorBox[i]=(CButtonWnd*)GetChildItem(Temp);
         ((PCBUTTONWND)ColorBox[i])->Color.ARGB=Palette[i].ARGB;
      }
      
      CurrentColor=(CButtonWnd*)GetChildItem("CPW_TC15Button");
      ButtonAccept=(CButtonWnd*)GetChildItem("CPW_Accept_Button");
      SliderR=(CSliderWnd*)GetChildItem("CPW_RedSlider");
      SliderG=(CSliderWnd*)GetChildItem("CPW_GreenSlider");
      SliderB=(CSliderWnd*)GetChildItem("CPW_BlueSlider");
      InputR=(CTextEntryWnd*)GetChildItem("CPW_RedSliderInput");
      InputG=(CTextEntryWnd*)GetChildItem("CPW_GreenSliderInput");
      InputB=(CTextEntryWnd*)GetChildItem("CPW_BlueSliderInput");

      ((PCBUTTONWND)CurrentColor)->Color.ARGB=TimerColor.ARGB;
      SliderR->SetNumTicks(256); SliderR->SetValue(TimerColor.R);
      SliderG->SetNumTicks(256); SliderG->SetValue(TimerColor.G);
      SliderB->SetNumTicks(256); SliderB->SetValue(TimerColor.B);
      SetCXStr(&InputR->InputText,itoa(TimerColor.R,Temp,10));
      SetCXStr(&InputG->InputText,itoa(TimerColor.G,Temp,10));
      SetCXStr(&InputB->InputText,itoa(TimerColor.B,Temp,10));
      SetCXStr(&CurrentColor->Tooltip,"Click to set Alpha level");
   }
   ~CTimerColorWnd()
   {
   }

   int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown)
   {
      if(pWnd==(CXWnd*)InputR)
      {
         if(Message==XWM_NEWVALUE)
         {
            GetCXStr((PCXSTR)InputR->InputText,Temp,MAX_STRING);
            HandleInputText(Temp,InputR->InputText->Length);
            SetCXStr(&InputR->InputText,Temp);
            SliderR->SetValue(atoi(Temp));
            ((PCTEXTENTRYWND)InputR)->CursorPos2=strlen(Temp);
            ((PCBUTTONWND)CurrentColor)->Color.R=atoi(Temp);
         }
      }
      else if(pWnd==(CXWnd*)InputG)
      {
         if(Message==XWM_NEWVALUE)
         {
            GetCXStr((PCXSTR)InputG->InputText,Temp,MAX_STRING);
            HandleInputText(Temp,InputG->InputText->Length);
            SetCXStr(&InputG->InputText,Temp);
            SliderG->SetValue(atoi(Temp));
            ((PCTEXTENTRYWND)InputG)->CursorPos2=strlen(Temp);
            ((PCBUTTONWND)CurrentColor)->Color.G=atoi(Temp);
         }
      }
      else if(pWnd==(CXWnd*)InputB)
      {
         if(Message==XWM_NEWVALUE)
         {
            GetCXStr((PCXSTR)InputB->InputText,Temp,MAX_STRING);
            HandleInputText(Temp,InputB->InputText->Length);
            SetCXStr(&InputB->InputText,Temp);
            SliderB->SetValue(atoi(Temp));
            ((PCTEXTENTRYWND)InputB)->CursorPos2=strlen(Temp);
            ((PCBUTTONWND)CurrentColor)->Color.B=atoi(Temp);
         }
      }
      else if(pWnd==(CXWnd*)SliderR)
      {
         if(Message==XWM_NEWVALUE)
         {
            SetCXStr(&InputR->InputText,itoa((int)unknown,Temp,10));
            ((PCBUTTONWND)CurrentColor)->Color.R=(int)unknown;   
         }
      }
      else if(pWnd==(CXWnd*)SliderG)
      {
         if(Message==XWM_NEWVALUE)
         {
            SetCXStr(&InputG->InputText,itoa((int)unknown,Temp,10));
            ((PCBUTTONWND)CurrentColor)->Color.G=(int)unknown;
         }
      }
      else if(pWnd==(CXWnd*)SliderB)
      {
         if(Message==XWM_NEWVALUE)
         {
            SetCXStr(&InputB->InputText,itoa((int)unknown,Temp,10));
            ((PCBUTTONWND)CurrentColor)->Color.B=(int)unknown;      
         }
      }
      else if(pWnd && pWnd->GetType()==UI_Button)
      {
         if(Message==XWM_LCLICK)
         {
            if(pWnd==(CXWnd*)ButtonAccept)
            {
               SaveTimerColor(((PCBUTTONWND)CurrentColor)->Color);
               Show=0;
            }
            else if(pWnd==(CXWnd*)CurrentColor)
            {
               CreateAlphaWnd();
            }
            else
            {
               ((PCBUTTONWND)CurrentColor)->Color.R=((PCBUTTONWND)pWnd)->Color.R;
               ((PCBUTTONWND)CurrentColor)->Color.G=((PCBUTTONWND)pWnd)->Color.G;
               ((PCBUTTONWND)CurrentColor)->Color.B=((PCBUTTONWND)pWnd)->Color.B;
               SetCXStr(&InputR->InputText,itoa(((PCBUTTONWND)pWnd)->Color.R,Temp,10));
               SetCXStr(&InputG->InputText,itoa(((PCBUTTONWND)pWnd)->Color.G,Temp,10));
               SetCXStr(&InputB->InputText,itoa(((PCBUTTONWND)pWnd)->Color.B,Temp,10));
               SliderR->SetValue(((PCBUTTONWND)pWnd)->Color.R);
               SliderG->SetValue(((PCBUTTONWND)pWnd)->Color.G);
               SliderB->SetValue(((PCBUTTONWND)pWnd)->Color.B);
            }
         }
      }
      return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);
   }

   char *HandleInputText(char *input, int length)
   {
      if(strlen(input)>3) input[3]='\0';
      if(length>0 && input[length-1]<'0' || input[length-1]>'9') input[length-1]='\0';
      if(atoi(input)>255) itoa(255,input,10);
      else if(atoi(input)<0) itoa(0,input,10);
      return input;
   }

   void UpdateAlpha(BYTE a)
   {
      ((PCBUTTONWND)CurrentColor)->Color.A=a;
      TimerColor.A=a;
   }

   CButtonWnd *ButtonAccept, *CurrentColor, *ColorBox[14];
   CSliderWnd *SliderR, *SliderG, *SliderB;
   CTextEntryWnd *InputR, *InputG, *InputB;
   CHAR Temp[MAX_STRING];
};

CTimerColorWnd *pTimerColorWnd=0;

class CTimerAlphaWnd : public CCustomWnd
{
public:
   CTimerAlphaWnd():CCustomWnd("QuantityWnd")
   {
      SetWndNotification(CTimerAlphaWnd);
      BitOff(WindowStyle,CWS_MINIMIZE);

      SliderA=(CSliderWnd*)GetChildItem("QTYW_Slider");
      InputA=(CTextEntryWnd*)GetChildItem("QTYW_SliderInput");
      ButtonAcceptA=(CButtonWnd*)GetChildItem("QTYW_Accept_Button");

      InputA->Enabled=0;
      SliderA->SetNumTicks(256); SliderA->SetValue(TimerColor.A);
      SetCXStr(&InputA->InputText,itoa(TimerColor.A,Temp,10));
      SetCXStr(&WindowText,"Alpha Level");
   }
   ~CTimerAlphaWnd()
   {
   }

   int WndNotification(CXWnd *pWnd, unsigned int Message, void *unknown)
   {
      if(pWnd==(CXWnd*)SliderA)
      {
         if(Message==XWM_NEWVALUE)
         {
            SetCXStr(&InputA->InputText,itoa((int)unknown,Temp,10));
            pTimerColorWnd->UpdateAlpha((BYTE)unknown);
         }
      }
      else if(pWnd==(CXWnd*)ButtonAcceptA)
      {
         if(Message==XWM_LCLICK)
         {
            Show=0;
         }
      }
      return CSidlScreenWnd::WndNotification(pWnd,Message,unknown);
   }

   CSliderWnd *SliderA;
   CTextEntryWnd *InputA;
   CButtonWnd *ButtonAcceptA;
   CHAR Temp[MAX_STRING];
};

CTimerAlphaWnd *pTimerAlphaWnd=0;

void CreateColorWnd()
{
   if(!pTimerColorWnd) pTimerColorWnd=new CTimerColorWnd();
   pTimerColorWnd->Show=1;
   ((CXWnd*)pTimerColorWnd)->BringToTop(1);
}

void CreateAlphaWnd()
{
   if(!pTimerAlphaWnd) pTimerAlphaWnd=new CTimerAlphaWnd();
   pTimerAlphaWnd->Show=1;
   ((CXWnd*)pTimerAlphaWnd)->BringToTop(1);
}

void SaveTimerColor(ARGBCOLOR color)
{
   CHAR Temp[MAX_STRING]={0};
   TimerColor.ARGB=color.ARGB;
   WritePrivateProfileString("InventoryTimerColor","Alpha",itoa(color.A,Temp,10),INIFileName);
   WritePrivateProfileString("InventoryTimerColor",  "Red",itoa(color.R,Temp,10),INIFileName);
   WritePrivateProfileString("InventoryTimerColor","Green",itoa(color.G,Temp,10),INIFileName);
   WritePrivateProfileString("InventoryTimerColor", "Blue",itoa(color.B,Temp,10),INIFileName);
}

void TimerColorCmd(PSPAWNINFO pChar, PCHAR szLine)
{
   CreateColorWnd();
}

DETOUR_TRAMPOLINE_EMPTY(int DrawColoredRect_Tramp(class CXRect &, unsigned long, class CXRect &));
int DrawColoredRect_Detour(class CXRect &r1, unsigned long color, class CXRect &r2)
{
   if(color==0x40C0C0C0)
   {
      color=TimerColor.ARGB;
   }
   return DrawColoredRect_Tramp(r1, color, r2);
}

PLUGIN_API VOID OnCleanUI(VOID)
{
   if(pTimerColorWnd)
   {
      delete pTimerColorWnd;
      pTimerColorWnd=0;
   }
   if(pTimerAlphaWnd)
   {
      delete pTimerAlphaWnd;
      pTimerAlphaWnd=0;
   }
}

PLUGIN_API VOID InitializePlugin(VOID)
{
   DebugSpewAlways("Initializing MQ2TimerColor");
   EzDetour(CXWnd__DrawColoredRect, DrawColoredRect_Detour, DrawColoredRect_Tramp);
   TimerColor.A=GetPrivateProfileInt("InventoryTimerColor","Alpha",0x40,INIFileName);
   TimerColor.R=GetPrivateProfileInt("InventoryTimerColor",  "Red",0xc0,INIFileName);
   TimerColor.G=GetPrivateProfileInt("InventoryTimerColor","Green",0xc0,INIFileName);
   TimerColor.B=GetPrivateProfileInt("InventoryTimerColor", "Blue",0xc0,INIFileName);
   AddCommand("/timercolor",TimerColorCmd);
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
   DebugSpewAlways("Shutting down MQ2TimerColor");
   RemoveDetour(CXWnd__DrawColoredRect);
   RemoveCommand("/timercolor");
   OnCleanUI();
}