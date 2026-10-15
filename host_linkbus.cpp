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
static void init(AEffect* e){ e->dispatcher(e,0,0,0,0,0); e->dispatcher(e,10,0,0,0,44100.f); e->dispatcher(e,11,0,512,0,0); e->dispatcher(e,12,0,1,0,0); }
static void params(AEffect* e,const char* tag){ printf("   %s : %d params, identify=%#x :",tag,e->numParams,(unsigned)e->dispatcher(e,22,0,0,0,0));
  for(int i=0;i<e->numParams;i++){ char n[64]={0},d[64]={0},l[64]={0}; e->dispatcher(e,8,i,0,n,0); e->dispatcher(e,7,i,0,d,0); e->dispatcher(e,6,i,0,l,0); printf(" %s=%s%s%s;",n,d,l[0]?" ":"",l); } puts(""); }
static void pump(){ MSG m; while(PeekMessageA(&m,0,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageA(&m);} }
static void capture(HWND c,int w,int h,const char* fn){
  HDC wdc=GetDC(c), mem=CreateCompatibleDC(wdc); HBITMAP bmp=CreateCompatibleBitmap(wdc,w,h); SelectObject(mem,bmp); BitBlt(mem,0,0,w,h,wdc,0,0,SRCCOPY);
  BITMAPINFOHEADER bi={sizeof bi,w,-h,1,24,BI_RGB,0,0,0,0,0}; int rb=((w*3+3)/4)*4; unsigned char* px=(unsigned char*)malloc(rb*h);
  GetDIBits(mem,bmp,0,h,px,(BITMAPINFO*)&bi,DIB_RGB_COLORS); BITMAPFILEHEADER fh={0x4D42,(DWORD)(54+rb*h),0,0,54};
  FILE* f=fopen(fn,"wb"); fwrite(&fh,sizeof fh,1,f); fwrite(&bi,sizeof bi,1,f); fwrite(px,rb,h,f); fclose(f); }
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0);
  if(getenv("FPUTRAP")){ unsigned c; __asm__ volatile("fnstcw %0":"=m"(c)); c&=~0x0Du; c&=~0x300u; __asm__ volatile("fldcw %0"::"m"(c)); puts("FPU strict (exceptions démasquées, précision 24 bits)"); }
  HMODULE hs=LoadLibraryA(argv[1]), hr=LoadLibraryA(argv[2]); if(!hs||!hr){puts("LoadLibrary KO");return 1;}
  MainFn ms=(MainFn)GetProcAddress(hs,"main"), mr=(MainFn)GetProcAddress(hr,"main");
  printf("Deux DLL distinctes chargées : %s (%p) et %s (%p)\n",argv[1],(void*)hs,argv[2],(void*)hr);
  AEffect *S1=ms(cb),*S2=ms(cb),*R1=mr(cb),*R2=mr(cb),*R3=mr(cb);
  if(!S1||!S2||!R1||!R2||!R3){puts("main NULL");return 2;}
  AEffect* all[5]={S1,S2,R1,R2,R3}; for(int k=0;k<5;k++) init(all[k]);
  params(S1,"LinkSend "); params(R1,"LinkRecv ");
  S1->setParameter(S1,0,0.0f);  S2->setParameter(S2,0,0.34f);                       // liens 1 et 2
  R1->setParameter(R1,5,0.0f);  R2->setParameter(R2,5,0.34f); R3->setParameter(R3,5,0.67f);   // liens 1, 2, 3
  for(AEffect* r : {R1,R2,R3}){ r->setParameter(r,1,0.607f); r->setParameter(r,2,0.4337f); r->setParameter(r,3,0.5396f); }  // 8:1, 2 ms, 120 ms
  const float FS=44100; const int Bk=512,N=88200;
  std::vector<float> K1(N),K2(N),Bs(N),o1(N),o2(N),oR1(N),oR2(N),oR3(N),sink(N);
  for(int i=0;i<N;i++){ K1[i]=(i%22050<4410)?0.9f*sinf(6.2831853f*60*i/FS):0.f; K2[i]=((i+11025)%22050<4410)?0.9f*sinf(6.2831853f*60*i/FS):0.f; Bs[i]=0.3f*sinf(6.2831853f*80*i/FS); }
  bool transp=true;
  for(int p=0;p<N;p+=Bk){ int n=(N-p<Bk)?N-p:Bk;
    float* k1[2]={&K1[p],&K1[p]}; float* k2[2]={&K2[p],&K2[p]}; float* b[2]={&Bs[p],&Bs[p]};
    float* q1[2]={&o1[p],&sink[p]}; float* q2[2]={&o2[p],&sink[p]};
    float* r1[2]={&oR1[p],&sink[p]}; float* r2[2]={&oR2[p],&sink[p]}; float* r3[2]={&oR3[p],&sink[p]};
    S1->processReplacing(S1,k1,q1,n); S2->processReplacing(S2,k2,q2,n);
    R1->processReplacing(R1,b,r1,n); R2->processReplacing(R2,b,r2,n); R3->processReplacing(R3,b,r3,n);
    for(int i=0;i<n;i++) if(o1[p+i]!=K1[p+i]||o2[p+i]!=K2[p+i]) transp=false; }
  int w1a=22050+2205,w1b=22050+4410, w2a=22050+11025+2205,w2b=22050+11025+4410;
  struct { const char* n; std::vector<float>* o; } rr[3]={{"R1 (lien 1)",&oR1},{"R2 (lien 2)",&oR2},{"R3 (lien 3, sans émetteur)",&oR3}};
  puts("== Réduction de la basse (dB) :   pendant le kick du lien 1 | pendant le kick du lien 2");
  for(auto& r:rr) printf("   %-28s %6.1f                      | %6.1f\n",r.n,db(rms(Bs,w1a,w1b))-db(rms(*r.o,w1a,w1b)),db(rms(Bs,w2a,w2b))-db(rms(*r.o,w2a,w2b)));
  printf("== LinkSend transparent (sortie == entrée, bit pour bit) : %s\n",transp?"oui":"NON");
  // émetteur arrêté : le récepteur doit revenir à 0 dB
  { std::vector<float> o(Bk),bb(Bk,0.3f); float* bi[2]={bb.data(),bb.data()}; float* bo[2]={o.data(),sink.data()};
    for(int k=0;k<4;k++) R1->processReplacing(R1,bi,bo,Bk); Sleep(400); for(int k=0;k<60;k++) R1->processReplacing(R1,bi,bo,Bk);
    printf("== Émetteur arrêté depuis 400 ms : gain de la basse (R1) = %.1f dB (0 attendu)\n",db(o[Bk-1]/0.3f)); }
  if(argc>3){
    struct E { AEffect* e; const char* fn; } eds[2]={{S1,"cap_send.bmp"},{R1,"cap_recv.bmp"}};
    for(auto& ed:eds){ ERect* r=0; ed.e->dispatcher(ed.e,13,0,0,&r,0); int W=r->right,H=r->bottom;
      HWND parent=CreateWindowExA(0,"STATIC","p",WS_OVERLAPPEDWINDOW|WS_VISIBLE,50,50,W+20,H+40,0,0,GetModuleHandleA(0),0);
      ed.e->dispatcher(ed.e,14,0,0,parent,0); HWND c=GetWindow(parent,GW_CHILD);
      if(ed.e==R1){ SendMessageA(c,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(150,6+0*30+18)); SendMessageA(c,WM_LBUTTONUP,0,MAKELPARAM(150,24)); }   // seuil -> -30 dB
      else        { SendMessageA(c,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(100,6+2*30+18)); SendMessageA(c,WM_LBUTTONUP,0,MAKELPARAM(100,24)); }   // hold
      char d[64]; if(ed.e==R1){ disp(R1,0,d); printf("   editeur Recv : clic sur Threshold -> %s dB\n",d);} else { disp(S1,2,d); printf("   editeur Send : clic sur Hold -> %s ms\n",d);} 
      // on fait « jouer » le kick en continu pendant ~0,5 s pour que les afficheurs bougent
      std::vector<float> kk(Bk),bb(Bk),oa(Bk),ob(Bk),sk(Bk); long ph=0;
      for(int it=0;it<50;it++){ for(int i=0;i<Bk;i++,ph++){ kk[i]=0.5f*sinf(6.2831853f*60*ph/FS); bb[i]=0.3f*sinf(6.2831853f*80*ph/FS); }
        float* ki[2]={kk.data(),kk.data()}; float* ko[2]={oa.data(),sk.data()}; float* bi[2]={bb.data(),bb.data()}; float* bo[2]={ob.data(),sk.data()};
        S1->processReplacing(S1,ki,ko,Bk); R1->processReplacing(R1,bi,bo,Bk); pump(); Sleep(10); }
      capture(c,W,H,ed.fn); ed.e->dispatcher(ed.e,15,0,0,0,0); }
  }
  for(int k=0;k<5;k++) all[k]->dispatcher(all[k],1,0,0,0,0);
  puts("FIN OK"); return 0; }
