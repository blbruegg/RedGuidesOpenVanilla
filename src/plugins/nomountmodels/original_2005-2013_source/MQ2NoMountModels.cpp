/*
* MQ2NoMountModels
* Thanks to ieatacid for the orginal plugin code.
* This plugin simply allows you to use a horse regardless of
* whether you have luclin modles on for that toon.
* You still need to have horse models enabled for this to work.
* Note the pattern/mask will need to be updated if offset can't be found.
*/

/* Version History
* 1.04 (20150523) Updated pattern and mask for 5/18/15 Live Client and added FixOffset() call to fix up start address - htw
* 1.03 (20140223) Updated pattern and mask for current client - dannuic
* 1.02 (20080609) Automatic offset finding - ieatacid
* 1.01 (2006120700) Given a dumb named by Jaq, posted in VIP
* 1.00 Original code by ieatacid
*/


#include "../MQ2Plugin.h"


PreSetup("MQ2NoMountModels");
PLUGIN_VERSION(1.04);

DWORD dwAddress=0;
char  mask[]="xxxxxxxxxxxxxxxxxxxxxxxxx????xx????xxxxxxxxxxxxxxxxxxxxx????x?xxxxxxxxxxxxxxxxx";
PBYTE pattern=(PBYTE)"\x51\x56\x8b\xf1\x8b\x06\x8b\x90\x88\x00\x00\x00\x8d\x4c\x24\x04"
                     "\x51\x8b\xce\xff\xd2\x8b\x00\x3b\x05\x00\x00\x00\x00\x0f\x84\x00"
                     "\x00\x00\x00\x8b\x16\x8b\x92\x88\x00\x00\x00\x8d\x44\x24\x04\x50"
                     "\x8b\xce\xff\xd2\x8b\x00\x3b\x05\x00\x00\x00\x00\x74\x00\x8b\x16"
                     "\x8b\x92\x88\x00\x00\x00\x8d\x44\x24\x04\x50\x8b\xce\xff\xd2";


// credit: radioactiveman/bunny771 ----------------------------------------
bool bDataCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
    for(;*szMask;++szMask,++pData,++bMask)
        if(*szMask=='x' && *pData!=*bMask )
            return false;
    return (*szMask) == NULL;
}

DWORD dwFindPattern(DWORD dwAddress,DWORD dwLen,BYTE *bMask,char * szMask)
{
    for(DWORD i=0; i < dwLen; i++)
        if( bDataCompare( (BYTE*)( dwAddress+i ),bMask,szMask) )
            return (DWORD)(dwAddress+i);
   
    return 0;
}
// ------------------------------------------------------------------------

class a
{
public:
   bool b();
   bool c()
   {
      return false;
   }
};

DETOUR_TRAMPOLINE_EMPTY(bool a::b(void));

PLUGIN_API VOID InitializePlugin(VOID)
{
   dwAddress=dwFindPattern(FixOffset(0x450000),0x200000,pattern,mask);
   if(dwAddress)
   {
      EzDetour(dwAddress,&a::c,&a::b);
   }
   else
   {
      WriteChatf("\arError:\ax Couldn't find offset.");
      EzCommand("/timed 1 /plugin mq2nomountmodels unload");
   }
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
   if(dwAddress)
   {
      RemoveDetour(dwAddress);
   }
}
