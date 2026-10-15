#include <windows.h>
#include <stdio.h>
#include "vst2_min.h"
static intptr_t cb(AEffect*, int32_t op, int32_t, intptr_t, void*, float){ return op==1?2300:0; }
typedef AEffect* (__cdecl *MainFn)(audioMasterCallback);
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0);
  for(int i=1;i<argc;i++){
    HMODULE h=LoadLibraryA(argv[i]); if(!h){printf("%s : LoadLibrary KO\n",argv[i]);continue;}
    MainFn m=(MainFn)GetProcAddress(h,"main"); AEffect* e=m?m(cb):0; if(!e){printf("%s : main NULL\n",argv[i]);continue;}
    intptr_t r=e->dispatcher(e,22,0,0,0,0);
    unsigned u=(unsigned)r; printf("%-22s effIdentify(22) -> %#010x  '%c%c%c%c'\n",argv[i],u,(u>>24)&255,(u>>16)&255,(u>>8)&255,u&255);
  }
}
