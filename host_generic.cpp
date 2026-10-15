#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "vst2_min.h"
static intptr_t cb(AEffect*, int32_t op, int32_t idx, intptr_t val, void* ptr, float opt){
    printf("  [host callback] opcode=%d idx=%d val=%ld\n", op, idx, (long)val);
    if (op == 1) { const char* v=getenv("HOSTVER"); return v? atoi(v):2; }
    if (op == 3) return 0;
    return 0;
}
typedef AEffect* (__cdecl *MainFn)(audioMasterCallback);
static void dump(const char* name, const void* p, size_t n){
    const unsigned char* c=(const unsigned char*)p; int nz=0; for(size_t i=0;i<n;i++) if(c[i]) nz=1;
    if(nz && c[0]>=32 && c[0]<127) printf("      %s: \"%s\"\n", name, (const char*)p);
}
int main(int argc, char** argv){
    setvbuf(stdout,NULL,_IONBF,0); if(getenv("FPUTRAP")){ unsigned c; __asm__ volatile("fnstcw %0":"=m"(c)); c &= ~0x0Du; c &= ~0x300u; __asm__ volatile("fldcw %0"::"m"(c)); printf("FPU: exceptions invalid/zerodiv/overflow DEMASQUEES, precision 24 bits\n"); }
    HMODULE h = LoadLibraryA(argv[1]);
    if(!h){ printf("LoadLibrary ECHEC err=%lu\n", GetLastError()); return 1; }
    printf("LoadLibrary OK\n");
    MainFn m = (MainFn)GetProcAddress(h, "main");
    printf("GetProcAddress(main)=%p  (VSTPluginMain=%p)\n",(void*)m,(void*)GetProcAddress(h,"VSTPluginMain"));
    if(!m) return 2;
    AEffect* e = m(cb);
    if(!e){ printf("main() a renvoye NULL\n"); return 3; }
    printf("AEffect: magic=%#x numPrograms=%d numParams=%d in=%d out=%d flags=%#x\n", e->magic, e->numPrograms,e->numParams,e->numInputs,e->numOutputs,e->flags);
    printf("         initialDelay=%d ioRatio=%f uniqueID=%#x ('%c%c%c%c') version=%d\n", e->initialDelay, e->ioRatio, e->uniqueID,
           (e->uniqueID>>24)&255,(e->uniqueID>>16)&255,(e->uniqueID>>8)&255,e->uniqueID&255, e->version);
    printf("         process=%p processReplacing=%p object=%p user=%p\n",(void*)e->process,(void*)e->processReplacing,e->object,e->user);
    const char* nm[]={"effOpen","effClose","setProgram","getProgram","setProgName","getProgName","getParamLabel","getParamDisplay","getParamName","getVu","setSampleRate","setBlockSize","mainsChanged","editGetRect","editOpen","editClose","editDraw","editMouse","editKey","editIdle","editTop","editSleep","identify","getChunk","setChunk"};
    // ouverture dans l'ordre habituel d'un hôte
    printf("-- séquence d'init\n");
    printf(" effOpen -> %ld\n",(long)e->dispatcher(e,0,0,0,0,0));
    printf(" effSetSampleRate(44100) -> %ld\n",(long)e->dispatcher(e,10,0,0,0,44100.f));
    printf(" effSetBlockSize(512) -> %ld\n",(long)e->dispatcher(e,11,0,512,0,0));
    printf(" effMainsChanged(1) -> %ld\n",(long)e->dispatcher(e,12,0,1,0,0));
    printf("-- interrogation des opcodes\n");
    int ops[]={3,5,23,26,33,34,35,38,44,45,47,48,49,50,51,52,53,58,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80};
    for(unsigned k=0;k<sizeof(ops)/sizeof(*ops);k++){
        int op=ops[k]; char buf[256]; memset(buf,0,sizeof buf);
        intptr_t r=e->dispatcher(e,op,0,0,buf,0);
        printf(" op %2d -> %ld\n", op, (long)r); dump("buf",buf,sizeof buf);
    }
    const char* cando[]={"receiveVstEvents","receiveVstMidiEvent","sendVstEvents","sendVstMidiEvent","receiveVstTimeInfo","offline","plugAsChannelInsert","plugAsSend","mixDryWet","noRealTime","multipass","metapass","1in1out","1in2out","2in1out","2in2out","2in4out","4in2out","4in4out","midiProgramNames","conformsToWindowRules","bypass"};
    printf("-- canDo\n");
    for(unsigned k=0;k<sizeof(cando)/sizeof(*cando);k++) printf(" canDo(%s) -> %ld\n",cando[k],(long)e->dispatcher(e,51,0,0,(void*)cando[k],0));
    printf("-- paramètres\n");
    for(int i=0;i<e->numParams && i<8;i++){
        char n[256]={0},d[256]={0},l[256]={0};
        e->dispatcher(e,8,i,0,n,0); e->dispatcher(e,7,i,0,d,0); e->dispatcher(e,6,i,0,l,0);
        printf(" param %d: name='%s' display='%s' label='%s' value=%f canAutomate=%ld\n",i,n,d,l,e->getParameter(e,i),(long)e->dispatcher(e,26,i,0,0,0));
    }
    // traitement
    float inL[512],inR[512],oL[512],oR[512]; for(int i=0;i<512;i++){inL[i]=inR[i]=0.3f*(i%64<32?1:-1);oL[i]=oR[i]=9;}
    float* in[2]={inL,inR}; float* out[2]={oL,oR};
    if(e->processReplacing){ e->processReplacing(e,in,out,512);
      float pk=0; for(int i=0;i<512;i++){float a=oL[i]<0?-oL[i]:oL[i]; if(a>pk) pk=a;}
      printf("processReplacing OK, pic sortie=%f\n",pk);}
    printf(" effMainsChanged(0) -> %ld\n",(long)e->dispatcher(e,12,0,0,0,0));
    printf(" effClose -> %ld\n",(long)e->dispatcher(e,1,0,0,0,0));
    printf("FIN OK\n");
    return 0;
}
