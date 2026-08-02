#define EQ_GetWelcomeFailure 0x575490   // 09/15

#include "../MQ2Plugin.h"

DWORD addrGetWelcomeFailure = NULL;


// 56 68 ? ? ? ? 6A ? 68 ? ? ? ? 68
unsigned char* patternGetWelcomeFailure = (unsigned char*)"\x56\x68\x00\x00\x00\x00\x6A\x00\x68\x00\x00\x00\x00\x68";
char maskGetWelcomeFailure[] = "xx????x?x????x";

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


int __stdcall GetWelcomeFailure()
{
    DebugSpewAlways("GetWelcomeFailure: returning 0");
    return 0;
}

DETOUR_TRAMPOLINE_EMPTY(int __stdcall Tramp_GetWelcomeFailure());

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
    addrGetWelcomeFailure = dwFindPattern(0x570000,0x10000,patternGetWelcomeFailure, maskGetWelcomeFailure);

    DebugSpewAlways("Initializing MQ2Nowelcome 0x%x", addrGetWelcomeFailure);
    //EzDetour(EQ_GetWelcomeFailure, GetWelcomeFailure, Tramp_GetWelcomeFailure);
    if (addrGetWelcomeFailure) {
        EzDetour(addrGetWelcomeFailure, GetWelcomeFailure, Tramp_GetWelcomeFailure);
    }
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2Nowelcome");

    if (addrGetWelcomeFailure) {
        RemoveDetour(addrGetWelcomeFailure);
    }
}

