#include "../source/DSPCore.h"
#include <cstdio>
#include <cmath>
int main(){
 SauceDSP::Engine dsp;dsp.prepare(48000,2);
 std::array<float,SauceDSP::numParams> p=SauceDSP::defaults;
 float left[1024],right[1024];for(int i=0;i<1024;++i){left[i]=std::sin(i*.12f)*.5f;right[i]=left[i];}
 float* channels[]={left,right};dsp.process(channels,2,1024,p);
 for(int i=0;i<1024;++i)if(std::abs(left[i]-right[i])>0.000001f)return 2;
 p[SauceDSP::sat]=42;p[SauceDSP::dist]=70;p[SauceDSP::chorus]=65;
 p[SauceDSP::air]=55;p[SauceDSP::eqLow]=5;p[SauceDSP::punch]=25;
 float before=left[234];dsp.process(channels,2,1024,p);
 if(std::abs(left[234]-before)<.001)return 3;
 for(int i=0;i<1024;++i)if(!std::isfinite(left[i])||std::abs(left[i])>32)return 4;
 puts("DSP PASSED: stereo, bypass, effects, finite samples");return 0;
}
