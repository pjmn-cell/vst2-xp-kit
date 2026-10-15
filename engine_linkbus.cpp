#include "bus.h"
#include <cstdio>
#include <cmath>
#include <vector>
static unsigned NOW=1000; static const float FS=44100; static const int B=512;
static float rms(const std::vector<float>&v,int a,int b){double s=0;for(int i=a;i<b;i++)s+=v[i]*v[i];return (float)sqrt(s/(b-a));}
static float db(float x){return 20*log10f(x+1e-12f);}
struct R { float kickDur, bassDur, bassRef, bassTail; bool transparent; };
// kick : sinus 60 Hz, enveloppe qui décroit (0.9 -> ~0.1 en 400 ms) ; bruit de fond (bleed) 0.01 en permanence ; basse 0.3
static R run(float thr, float hold, bool withKick, float bleed){
  for(int i=0;i<4;i++){bf::g_localBus.links[i].ready=0;bf::g_localBus.links[i].n[0]=bf::g_localBus.links[i].n[1]=0;}
  bf::Sender S; bf::Receiver Rc; S.setSampleRate(FS);Rc.setSampleRate(FS);
  S.thresh=thr;S.holdMs=hold;S.linkId=0; Rc.thresh=-20;Rc.ratio=8;Rc.attackMs=2;Rc.releaseMs=120;Rc.linkId=0;
  int N=88200; std::vector<float> K(N),Bs(N),oK(N),oB(N),sink(N);
  for(int i=0;i<N;i++){ int ph=i%22050; float env=(ph<17640)? 0.9f*expf(-ph/(0.12f*FS)) : 0.f;
     K[i]=(withKick?env*sinf(6.2831853f*60*i/FS):0.f)+bleed*sinf(6.2831853f*1000*i/FS); Bs[i]=0.3f*sinf(6.2831853f*80*i/FS); }
  bool tr=true;
  for(int p=0;p<N;p+=B){int n=std::min(B,N-p); float*ki[2]={&K[p],&K[p]};float*ko[2]={&oK[p],&sink[p]};float*bi[2]={&Bs[p],&Bs[p]};float*bo[2]={&oB[p],&sink[p]};NOW+=11;
    S.process(bf::busGet(),NOW,ki,ko,n); Rc.process(bf::busGet(),NOW,bi,bo,n);
    for(int i=0;i<n;i++) if(oK[p+i]!=K[p+i]) tr=false; }
  R r; int base=22050;
  r.bassRef=rms(Bs,base,base+4410);
  r.kickDur=db(rms(K,base+400,base+2000));   // début du corps du kick
  r.bassDur=db(r.bassRef)-db(rms(oB,base+2205,base+4410));          // réduction 50-100 ms après l'attaque
  r.bassTail=db(r.bassRef)-db(rms(oB,base+8820,base+11025));        // réduction 200-250 ms après l'attaque (le kick a décru)
  r.transparent=tr; return r; }
int main(){
  puts("Kick : sinus 60 Hz, crête 0.9 qui décroit (const. de temps 120 ms). Récepteur : seuil -20 dB, 8:1, attack 2 ms, release 120 ms.");
  printf("%-46s | réduction basse à +50..100 ms | à +200..250 ms | Send transparent\n","réglage du Send");
  struct {const char* l; float thr,hold;} c[]={{"seuil -40 dB, hold 0 ms (suit le niveau)",-40,0},{"seuil -40 dB, hold 50 ms",-40,50},{"seuil -40 dB, hold 150 ms",-40,150},{"seuil -40 dB, hold 300 ms",-40,300}};
  for(auto&x:c){R r=run(x.thr,x.hold,true,0); printf("%-46s | %6.1f dB                      | %6.1f dB      | %s\n",x.l,r.bassDur,r.bassTail,r.transparent?"oui":"NON");}
  puts("\nBruit de fond (bleed) d'un autre instrument à -40 dBFS crête 0.01, SANS kick :");
  for(float thr : {-60.f,-30.f}){ R r=run(thr,50,false,0.01f); printf("   seuil du Send %4.0f dB -> réduction de la basse = %.1f dB\n",thr,r.bassDur); }
  puts("\nBleed à 0.1 (-20 dBFS) sans kick, et récepteur seuil -20 :");
  for(float thr : {-40.f,-10.f}){ R r=run(thr,50,false,0.1f); printf("   seuil du Send %4.0f dB -> réduction de la basse = %.1f dB\n",thr,r.bassDur); }
}
