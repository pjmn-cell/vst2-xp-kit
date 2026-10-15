#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "vst2_min.h"
static intptr_t cb(AEffect*, int32_t op, int32_t, intptr_t, void*, float){ return op==1?2300:0; }
typedef AEffect* (__cdecl *MainFn)(audioMasterCallback);
static float rms(const std::vector<float>& v,int a,int b){ double s=0; for(int i=a;i<b;i++) s+=v[i]*v[i]; return (float)sqrt(s/(b-a)); }
static float db(float x){ return 20*log10f(x+1e-12f); }
static void disp(AEffect* e,int i,char* d){ d[0]=0; e->dispatcher(e,7,i,0,d,0); }
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0);
  if(getenv("FPUTRAP")){ unsigned c; __asm__ volatile("fnstcw %0":"=m"(c)); c&=~0x0Du; c&=~0x300u; __asm__ volatile("fldcw %0"::"m"(c)); puts("FPU strict"); }
  HMODULE h=LoadLibraryA(argv[1]); MainFn m=(MainFn)GetProcAddress(h,"main");
  const char* lab[3]={"Send (kick inchange)","Cmp+Send, prise Input (avant)","Cmp+Send, prise Output (apres)"};
  float modeV[3]={0.375f,0.625f,0.625f}, tapV[3]={0,0,1};
  printf("params=%d ; effIdentify=%#x\n", m(cb)->numParams, 0);
  for(int s=0;s<3;s++){
    AEffect *A=m(cb), *B=m(cb);
    for(AEffect* e : {A,B}){ e->dispatcher(e,0,0,0,0,0); e->dispatcher(e,10,0,0,0,44100.f); e->dispatcher(e,12,0,1,0,0); }
    A->setParameter(A,5,modeV[s]); A->setParameter(A,7,tapV[s]);               // kick : seuil -20, 4:1 par défaut, attack 10 ms
    B->setParameter(B,5,1.0f); B->setParameter(B,1,0.607f); B->setParameter(B,2,0.4337f); B->setParameter(B,3,0.5396f);
    char d1[64],d2[64],d3[64]; disp(A,5,d1); disp(A,7,d2); disp(B,5,d3);
    const float FS=44100; const int Bk=512,N=88200; std::vector<float> K(N),Bs(N),oK(N),oB(N),sink(N);
    for(int i=0;i<N;i++){ int ph=i%22050; K[i]=(ph<4410)?0.9f*sinf(6.2831853f*60*i/FS):0.f; Bs[i]=0.3f*sinf(6.2831853f*80*i/FS); }
    for(int p=0;p<N;p+=Bk){ int n=(N-p<Bk)?N-p:Bk; float* ki[2]={&K[p],&K[p]}; float* ko[2]={&oK[p],&sink[p]}; float* bi[2]={&Bs[p],&Bs[p]}; float* bo[2]={&oB[p],&sink[p]};
      A->processReplacing(A,ki,ko,n); B->processReplacing(B,bi,bo,n); }
    int a=22050+2205,b=22050+4410;
    printf("%-32s [A=%s/%s, B=%s] kick %5.1f -> %5.1f dB (%+5.1f) | basse reduite de %5.1f dB\n",lab[s],d1,d2,d3,db(rms(K,a,b)),db(rms(oK,a,b)),db(rms(oK,a,b))-db(rms(K,a,b)),db(rms(Bs,a,b))-db(rms(oB,a,b)));
    A->dispatcher(A,1,0,0,0,0); B->dispatcher(B,1,0,0,0,0);
  }
  puts("FIN OK"); return 0; }
