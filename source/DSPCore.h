#pragma once
// Independent, allocation-free-in-process DSP core. One instance per plug-in.
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace SauceDSP {
constexpr int numParams = 22;
enum Id {sat, dist, air, punch, wet, deess, color, chorus, eqLow, eqMid, eqHigh,
         distHP, distLP, chorusRate, chorusDepth, colorTilt, deessFreq, punchSpeed,
         eqFreqLow, eqFreqMid, eqFreqHigh, eqWidth};
inline constexpr std::array<const char*, numParams> ids {{
    "sat", "dist", "air", "punch", "wet", "deess", "color", "chorus", "eq_low",
    "eq_mid", "eq_high", "dist_hp", "dist_lp", "chorus_rate", "chorus_depth",
    "color_tilt", "deess_freq", "punch_speed", "eq_freq_low", "eq_freq_mid",
    "eq_freq_high", "eq_width"
}};
inline constexpr std::array<float, numParams> defaults {{
    0.f,0.f,0.f,0.f,100.f,0.f,0.f,0.f,0.f,0.f,0.f,
    65.f,15000.f,0.8f,40.f,0.f,6600.f,90.f,160.f,1200.f,7000.f,1.f
}};
inline double clamp(double x, double lo, double hi) noexcept {return std::min(hi, std::max(lo, x));}
inline double onePole(double hz, double sampleRate) noexcept {
    hz = clamp(hz, 5.0, sampleRate * 0.45);
    return 1.0 - std::exp(-6.283185307179586 * hz / sampleRate);
}
inline double softclip(double x) noexcept {return std::tanh(clamp(x, -20.0, 20.0));}
inline double decibels(double db) noexcept {return std::pow(10.0, db / 20.0);}
struct Channel {
    double hp = 0, lp = 0, airLP = 0, deessLP = 0, deessEnvelope = 0;
    double fast = 0, slow = 0, colorLow = 0, colorMid = 0;
    double eqLP1 = 0, eqLP2 = 0, eqLP3 = 0, chorusPhase = 0;
    std::vector<float> delay;
    size_t index = 0;
    void reset() noexcept {
        hp = lp = airLP = deessLP = deessEnvelope = fast = slow = colorLow = colorMid = 0;
        eqLP1 = eqLP2 = eqLP3 = chorusPhase = 0; index = 0;
        std::fill(delay.begin(), delay.end(), 0.f);
    }
};
class Engine {
public:
    void prepare(double newRate, int channels) {
        sr = std::max(8000.0, newRate);
        for (auto& c : states) {
            c.delay.assign(static_cast<size_t>(std::ceil(sr * 0.08)) + 4, 0.f);
            c.reset();
        }
        usedChannels = std::clamp(channels, 1, 2);
    }
    void reset() noexcept {for (auto& c : states) c.reset();}
    void process(float* const* channels, int channelCount, int samples,
                 const std::array<float,numParams>& p) noexcept {
        if (!channels || samples<=0) return;
        const int number = std::min({channelCount,usedChannels,2});
        const double saturation = clamp(p[sat]*.01,0,1), distortion = clamp(p[dist]*.01,0,1);
        const double airMix = clamp(p[air]*.01,0,1), punchMix = clamp(p[punch]*.01,0,1);
        const double wetMix = clamp(p[wet]*.01,0,1), deessMix = clamp(p[deess]*.01,0,1);
        const double colorMix = clamp(p[color]*.01,0,1), chorusMix = clamp(p[chorus]*.01,0,1);
        // Discrete filters, 1-pole approximations; no oversampling in this first version.
        const double highPass = onePole(p[distHP],sr), lowPass = onePole(p[distLP],sr);
        const double highAir = onePole(4400,sr), deessCoeff = onePole(p[deessFreq],sr);
        const double punchCoeff = onePole(p[punchSpeed],sr), slowCoeff = onePole(9,sr);
        const double envCoeff = onePole(120,sr), colLow = onePole(280,sr), colMid=onePole(2400,sr);
        const double eq1 = onePole(p[eqFreqLow],sr),eq2=onePole(p[eqFreqMid],sr),eq3=onePole(p[eqFreqHigh],sr);
        const double gLow=decibels(p[eqLow]),gMid=decibels(p[eqMid]*p[eqWidth]),gHigh=decibels(p[eqHigh]);
        const double tilt = clamp(p[SauceDSP::colorTilt]*0.01,-1,1);
        const double rate = clamp(p[chorusRate],.10,5.0), depth = clamp(p[chorusDepth]*.01,0,1);
        const double phaseStep = 6.283185307179586*rate/sr;
        const bool eqOn = std::abs(gLow-1)>0.00001 || std::abs(gMid-1)>0.00001 || std::abs(gHigh-1)>0.00001;
        const bool processing = saturation>0 || distortion>0 || airMix>0 || punchMix>0 || deessMix>0 || colorMix>0 || chorusMix>0 || eqOn;
        if (!processing || wetMix<=0) return; // exact pass-through at default settings
        for (int ch=0;ch<number;++ch) {
            float* data = channels[ch];if (!data) continue;
            Channel& s=states[static_cast<size_t>(ch)];
            for(int i=0;i<samples;++i){
                const double dry = std::isfinite(data[i]) ? clamp(data[i],-32,32) : 0;
                double x = dry;
                if(saturation>0){
                    const double drive=1+1.65*saturation;
                    x = x*(1-saturation*.55) + softclip(x*drive)*(.55*saturation/softclip(drive));
                }
                if(distortion>0){
                    s.hp += highPass*(x-s.hp);
                    const double filtered = x-s.hp;
                    s.lp += lowPass*(filtered-s.lp);
                    x=(1-distortion)*x+distortion*softclip(s.lp*(1+35*distortion))*.85;
                }
                if(airMix>0){s.airLP += highAir*(x-s.airLP); x += .55*airMix*(x-s.airLP);}
                if(punchMix>0){
                    const double amplitude=std::abs(x);
                    s.fast+=punchCoeff*(amplitude-s.fast);
                    s.slow+=slowCoeff*(amplitude-s.slow);
                    x*=1 + punchMix*clamp(7*(s.fast-s.slow),-.33,1.4);
                }
                if(deessMix>0){
                    s.deessLP += deessCoeff*(x-s.deessLP);
                    const double high = x-s.deessLP;
                    s.deessEnvelope += envCoeff*(std::abs(high)-s.deessEnvelope);
                    const double gain = 1 - deessMix*clamp(28*(s.deessEnvelope-.012),0,.84);
                    x=s.deessLP+gain*high;
                }
                if(colorMix>0){
                    s.colorLow += colLow*(x-s.colorLow);
                    s.colorMid += colMid*(x-s.colorMid);
                    x += colorMix*(.35*tilt*(s.colorLow-(x-s.colorMid))+.08*(s.colorMid-s.colorLow));
                }
                if(eqOn){
                    s.eqLP1 += eq1*(x-s.eqLP1);
                    s.eqLP2 += eq2*(x-s.eqLP2);
                    s.eqLP3 += eq3*(x-s.eqLP3);
                    // Bass, presence and air. Mid is a broad band between low/high crossovers.
                    x = s.eqLP1*gLow + (s.eqLP3-s.eqLP1)*gMid + (x-s.eqLP3)*gHigh;
                }
                if(chorusMix>0 && s.delay.size()>4){
                    const size_t size=s.delay.size();
                    const double ms=15+depth*8*(1+std::sin(s.chorusPhase));
                    const double offset=clamp(ms*.001*sr,1,static_cast<double>(size-3));
                    const double readPos = static_cast<double>(s.index)+size-offset;
                    const size_t ix=static_cast<size_t>(readPos)%size;
                    const size_t ix2=(ix+1)%size;
                    const double frac=readPos-std::floor(readPos);
                    const double delayed=(1-frac)*s.delay[ix]+frac*s.delay[ix2];
                    s.delay[s.index]=static_cast<float>(x);s.index=(s.index+1)%size;
                    s.chorusPhase+=phaseStep;
                    if(s.chorusPhase>=6.283185307179586)s.chorusPhase-=6.283185307179586;
                    x=x*(1-.42*chorusMix)+delayed*(.42*chorusMix);
                }
                const double out=dry*(1-wetMix)+x*wetMix;
                data[i]=static_cast<float>(clamp(std::isfinite(out)?out:0,-32,32));
            }
        }
    }
private:
    double sr=44100.0;
    int usedChannels=2;
    std::array<Channel,2> states;
};
}
