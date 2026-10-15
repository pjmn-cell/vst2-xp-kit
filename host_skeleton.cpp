// Test du squelette (gain) : 0.5 -> 0 dB, 0.75 -> +12 dB (x3.98), 0.25 -> -12 dB (x0.251) ; l'éditeur s'ouvre et se ferme.
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "vst2_min.h"
static intptr_t cb(AEffect*, int32_t op, int32_t, intptr_t, void*, float){ return op==1?2300:0; }
typedef AEffect* (__cdecl *MainFn)(audioMasterCallback);
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0);
  HMODULE h=LoadLibraryA(argv[1]); if(!h){puts("FAIL LoadLibrary");return 1;}
  AEffect* e=((MainFn)GetProcAddress(h,"main"))(cb); if(!e){puts("FAIL main NULL");return 1;}
  e->dispatcher(e,0,0,0,0,0); e->dispatcher(e,10,0,0,0,44100.f); e->dispatcher(e,12,0,1,0,0);
  float in[512],o[512]; for(int i=0;i<512;i++) in[i]=0.1f; float* I[2]={in,in}; float* O[2]={o,o};
  struct { float v, expect; } t[3]={{0.5f,0.1f},{0.75f,0.1f*3.981f},{0.25f,0.1f*0.2512f}}; int bad=0;
  for(auto& c:t){ e->setParameter(e,0,c.v); e->processReplacing(e,I,O,512); float got=o[100]; int ok=fabsf(got-c.expect)<0.002f; bad+=!ok;
    char d[64]={0}; e->dispatcher(e,7,0,0,d,0); printf("  param=%.2f (%s dB) sortie=%.4f attendu=%.4f %s\n",c.v,d,got,c.expect,ok?"ok":"ERREUR"); }
  ERect* r=0; int ed=(int)e->dispatcher(e,13,0,0,&r,0); HWND p=CreateWindowExA(0,"STATIC","p",WS_OVERLAPPEDWINDOW,0,0,300,200,0,0,GetModuleHandleA(0),0);
  int open=(int)e->dispatcher(e,14,0,0,p,0); e->dispatcher(e,15,0,0,0,0); printf("  editeur : rect=%ld open=%d\n",(long)ed,open); bad+=!(ed&&open);
  e->dispatcher(e,1,0,0,0,0); puts(bad?"FAIL":"PASS"); return bad; }
