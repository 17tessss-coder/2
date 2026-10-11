#include "PluginEditor.h"
#include <cmath>

namespace {
const auto dirt=juce::Colour::fromRGB(43,34,28);
const auto black=juce::Colour::fromRGB(12,12,12);
const auto stoneGray=juce::Colour::fromRGB(108,109,109);
const auto stoneLight=juce::Colour::fromRGB(171,173,172);
const auto stoneDark=juce::Colour::fromRGB(52,53,54);
const auto grass=juce::Colour::fromRGB(113,190,84);
const auto gold=juce::Colour::fromRGB(252,240,49);
const auto pale=juce::Colour::fromRGB(234,235,220);
const std::array<const char*,8> names{{"SATURATION","DISTORTION","AIR","PUNCH","WET","DE-ESS","COLOR EQ","CHORUS"}};
const std::array<int,8> pids{{SauceDSP::sat,SauceDSP::dist,SauceDSP::air,SauceDSP::punch,
                              SauceDSP::wet,SauceDSP::deess,SauceDSP::color,SauceDSP::chorus}};
struct DetailSpec {int owner;const char* id;const char* title;};
const std::array<DetailSpec,11> specs{{
 {1,"dist_hp","LOW CUT"},{1,"dist_lp","HIGH CUT"},
 {3,"punch_speed","ATTACK"},{5,"deess_freq","SIBILANCE FREQ"},
 {6,"color_tilt","TILT"},{7,"chorus_rate","RATE"},{7,"chorus_depth","DEPTH"},
 {8,"eq_freq_low","LOW FREQ"},{8,"eq_freq_mid","MID FREQ"},{8,"eq_freq_high","HIGH FREQ"},{8,"eq_width","EQ WIDTH"}
}};
juce::Font pixelFont(float size,bool bold=true){
    return juce::Font(juce::Font::getDefaultMonospacedFontName(),size,
                      bold ? juce::Font::bold : juce::Font::plain);
}
}

void PixelLook::drawRotarySlider(juce::Graphics&g,int x,int y,int w,int h,float position,
                                  float,float,juce::Slider&) {
    auto bounds=juce::Rectangle<int>(x,y,w,h);
    const int size=std::min(w,h)-8;
    const int left=bounds.getCentreX()-size/2,top=bounds.getCentreY()-size/2;
    g.setColour(black);g.fillRect(left-3,top-3,size+6,size+6);
    g.setColour(stoneGray);g.fillRect(left,top,size,size);
    g.setColour(stoneLight);g.fillRect(left,top,size,4);g.fillRect(left,top,4,size);
    g.setColour(stoneDark);g.fillRect(left,top+size-4,size,4);g.fillRect(left+size-4,top,4,size);
    g.setColour(juce::Colour::fromRGB(58,58,58));g.fillRect(left+8,top+8,size-16,size-16);
    const auto angle=(-2.35f+position*4.7f);
    const int bx=left+size/2+static_cast<int>(std::sin(angle)*(size*.29f));
    const int by=top+size/2-static_cast<int>(std::cos(angle)*(size*.29f));
    g.setColour(gold);g.fillRect(bx-4,by-4,8,8);
    g.setColour(black);g.fillRect(left+size/2-3,top+size/2-3,6,6);
}
void PixelLook::drawButtonBackground(juce::Graphics&g,juce::Button&b,const juce::Colour&,bool over,bool down){
    auto r=b.getLocalBounds();
    g.setColour(black);g.fillRect(r.translated(2,3));
    g.setColour(stoneDark);g.fillRect(r.reduced(1).withTrimmedBottom(2));
    g.setColour(down?grass:(over?stoneLight:stoneGray));g.fillRect(r.reduced(4).withTrimmedBottom(2));
    g.setColour(stoneLight);g.fillRect(r.reduced(4).removeFromTop(2));
}
void PixelLook::drawButtonText(juce::Graphics&g,juce::TextButton&b,bool,bool){
    g.setFont(pixelFont(b.getHeight()<32?13.f:17.f));
    g.setColour(pale);g.drawText(b.getButtonText(),b.getLocalBounds(),juce::Justification::centred,true);
}

juce::Point<float> EQGraph::point(int band) const{
    const int gains[]={SauceDSP::eqLow,SauceDSP::eqMid,SauceDSP::eqHigh};
    const int freqs[]={SauceDSP::eqFreqLow,SauceDSP::eqFreqMid,SauceDSP::eqFreqHigh};
    const double freq=processor.param(freqs[band]);
    const double gain=processor.param(gains[band]);
    const auto r=getLocalBounds().reduced(17,10).toFloat();
    const float x=r.getX()+static_cast<float>(std::log(freq/30.0)/std::log(16500.0/30.0))*r.getWidth();
    const float y=r.getY()+static_cast<float>((12.0-gain)/24.0)*r.getHeight();
    return {x,y};
}
void EQGraph::paint(juce::Graphics&g){
    auto r=getLocalBounds();
    g.fillAll(juce::Colour::fromRGB(27,36,27));
    g.setColour(juce::Colour::fromRGB(50,74,53));
    for(int i=1;i<7;++i)g.fillRect(r.getWidth()*i/7,0,1,r.getHeight());
    for(int i=1;i<5;++i)g.fillRect(0,r.getHeight()*i/5,r.getWidth(),1);
    g.setColour(juce::Colour::fromRGB(100,126,88));g.fillRect(0,r.getHeight()/2,r.getWidth(),2);
    juce::Path curve;
    const double f0=processor.param(SauceDSP::eqFreqLow),f1=processor.param(SauceDSP::eqFreqMid),f2=processor.param(SauceDSP::eqFreqHigh);
    const double gs[]={processor.param(SauceDSP::eqLow),processor.param(SauceDSP::eqMid),processor.param(SauceDSP::eqHigh)};
    const double fs[]={f0,f1,f2};
    for(int x=0;x<r.getWidth();x+=3){
        const double f=30.0*std::pow(16500.0/30.0,double(x)/std::max(1,r.getWidth()));
        double gain=0;
        for(int k=0;k<3;++k){const double delta=std::log2(f/fs[k]);
            const double width=(k==1?1.1:1.55)*processor.param(SauceDSP::eqWidth);
            gain+=gs[k]*std::exp(-.5*delta*delta/(width*width));
        }
        const float y=float(r.getHeight()*.5-gain*r.getHeight()/24.0);
        if(x==0)curve.startNewSubPath(float(x),y);else curve.lineTo(float(x),y);
    }
    g.setColour(grass);g.strokePath(curve,juce::PathStrokeType(2.5f));
    for(int k=0;k<3;++k){
        auto p=point(k);g.setColour(black);g.fillRect(int(p.x)-9,int(p.y)-9,18,18);
        g.setColour(k==1?gold:grass);g.fillRect(int(p.x)-6,int(p.y)-6,12,12);
    }
    g.setColour(pale);g.setFont(pixelFont(11));
    g.drawText("30 HZ",6,r.getHeight()-18,65,16,juce::Justification::centredLeft);
    g.drawText("16.5 KHZ",r.getWidth()-86,r.getHeight()-18,83,16,juce::Justification::centredRight);
    g.setColour(stoneLight);g.drawRect(r,2);
}
void EQGraph::mouseDown(const juce::MouseEvent&e){
    float best=625;
    selected=-1;
    for(int k=0;k<3;++k){const float dist=point(k).getDistanceSquaredFrom(e.position);
        if(dist<best){best=dist;selected=k;}}
    if(selected>=0){
        const int gains[]={SauceDSP::eqLow,SauceDSP::eqMid,SauceDSP::eqHigh};
        const int freqs[]={SauceDSP::eqFreqLow,SauceDSP::eqFreqMid,SauceDSP::eqFreqHigh};
        processor.parameters.getParameter(SauceDSP::ids[gains[selected]])->beginChangeGesture();
        processor.parameters.getParameter(SauceDSP::ids[freqs[selected]])->beginChangeGesture();
        move(e);
    }
}
void EQGraph::mouseDrag(const juce::MouseEvent&e){if(selected>=0)move(e);}
void EQGraph::move(const juce::MouseEvent&e){
    if(selected<0)return;
    const int gains[]={SauceDSP::eqLow,SauceDSP::eqMid,SauceDSP::eqHigh};
    const int freqs[]={SauceDSP::eqFreqLow,SauceDSP::eqFreqMid,SauceDSP::eqFreqHigh};
    const float fmin[]={30,350,2500},fmax[]={680,3750,16500};
    auto r=getLocalBounds().reduced(17,10).toFloat();
    const double xf=std::clamp(double((e.position.x-r.getX())/r.getWidth()),0.0,1.0);
    const double yf=std::clamp(double((e.position.y-r.getY())/r.getHeight()),0.0,1.0);
    const double frequency=30.0*std::pow(16500.0/30.0,xf);
    const float hz=static_cast<float>(std::clamp(frequency,double(fmin[selected]),double(fmax[selected])));
    const float db=static_cast<float>(12.0-yf*24.0);
    auto* freqParam=processor.parameters.getParameter(SauceDSP::ids[freqs[selected]]);
    auto* gainParam=processor.parameters.getParameter(SauceDSP::ids[gains[selected]]);
    freqParam->setValueNotifyingHost(freqParam->convertTo0to1(hz));
    gainParam->setValueNotifyingHost(gainParam->convertTo0to1(db));
    repaint();
}
void EQGraph::mouseUp(const juce::MouseEvent&){
    if(selected>=0){
        const int gains[]={SauceDSP::eqLow,SauceDSP::eqMid,SauceDSP::eqHigh};
        const int freqs[]={SauceDSP::eqFreqLow,SauceDSP::eqFreqMid,SauceDSP::eqFreqHigh};
        processor.parameters.getParameter(SauceDSP::ids[gains[selected]])->endChangeGesture();
        processor.parameters.getParameter(SauceDSP::ids[freqs[selected]])->endChangeGesture();
        selected=-1;
    }
}

VocalSauceEditor::VocalSauceEditor(VocalSauceProcessor& p):
  juce::AudioProcessorEditor(&p),processor(p),graph(p){
    setLookAndFeel(&look);
    setSize(width,height);
    for(size_t i=0;i<effects.size();++i){
        auto& tile=effects[i];
        addAndMakeVisible(tile.knob);addAndMakeVisible(tile.more);
        addAndMakeVisible(tile.name);addAndMakeVisible(tile.value);
        tile.name.setFont(pixelFont(16));tile.name.setColour(juce::Label::textColourId,pale);
        tile.name.setJustificationType(juce::Justification::centred);
        tile.name.setText(names[i],juce::dontSendNotification);
        tile.value.setFont(pixelFont(14));tile.value.setColour(juce::Label::textColourId,gold);
        tile.value.setJustificationType(juce::Justification::centred);
        tile.knob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        tile.knob.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        tile.knob.setDoubleClickReturnValue(true,(i==4)?100.0:0.0);
        tile.attachment=std::make_unique<Attachment>(processor.parameters,SauceDSP::ids[pids[i]],tile.knob);
        tile.knob.onValueChange=[this,i]{effects[i].value.setText(
            juce::String(effects[i].knob.getValue(),1)+" %",juce::dontSendNotification);};
        tile.knob.onValueChange();
        tile.more.onClick=[this,i]{setExpanded(int(i));};
    }
    addAndMakeVisible(graph);addAndMakeVisible(eqExpand);
    eqExpand.onClick=[this]{setExpanded(8);};
    const char* eqNames[]={"LOW","MID","HIGH"};
    for(size_t i=0;i<eqKnobs.size();++i){
        auto& s=eqKnobs[i];addAndMakeVisible(s);addAndMakeVisible(eqLabels[i]);
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,76,20);
        s.setColour(juce::Slider::textBoxTextColourId,gold);
        s.setColour(juce::Slider::textBoxOutlineColourId,black);
        s.setColour(juce::Slider::textBoxBackgroundColourId,dirt);
        eqLabels[i].setText(eqNames[i],juce::dontSendNotification);
        eqLabels[i].setFont(pixelFont(13));eqLabels[i].setColour(juce::Label::textColourId,pale);
        eqLabels[i].setJustificationType(juce::Justification::centred);
        eqAttachments[i]=std::make_unique<Attachment>(processor.parameters,SauceDSP::ids[SauceDSP::eqLow+int(i)],s);
    }
    for(size_t i=0;i<details.size();++i){
        auto& d=details[i];d.owner=specs[i].owner;
        addAndMakeVisible(d.slider);addAndMakeVisible(d.label);
        d.slider.setSliderStyle(juce::Slider::LinearHorizontal);
        d.slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,94,24);
        d.slider.setColour(juce::Slider::trackColourId,grass);
        d.slider.setColour(juce::Slider::thumbColourId,gold);
        d.slider.setColour(juce::Slider::textBoxTextColourId,gold);
        d.slider.setColour(juce::Slider::textBoxOutlineColourId,black);
        d.slider.setColour(juce::Slider::textBoxBackgroundColourId,dirt);
        d.label.setText(specs[i].title,juce::dontSendNotification);
        d.label.setFont(pixelFont(13));d.label.setColour(juce::Label::textColourId,pale);
        d.attachment=std::make_unique<Attachment>(processor.parameters,specs[i].id,d.slider);
    }
    addAndMakeVisible(detailHint);
    detailHint.setFont(pixelFont(14,false));
    detailHint.setColour(juce::Label::textColourId,pale);
    detailHint.setJustificationType(juce::Justification::centred);
    detailHint.setText("PRESS + TO OPEN EFFECT SETTINGS",juce::dontSendNotification);
    addAndMakeVisible(saveButton);addAndMakeVisible(loadButton);addAndMakeVisible(version);
    saveButton.onClick=[this]{savePreset();};loadButton.onClick=[this]{loadPreset();};
    version.setText("V0.2  |  WINDOWS X64 VST3  |  NO FACTORY PRESETS",juce::dontSendNotification);
    version.setFont(pixelFont(11));version.setColour(juce::Label::textColourId,stoneLight);
    resized();setExpanded(-1);startTimerHz(15);
}
VocalSauceEditor::~VocalSauceEditor(){stopTimer();chooser.reset();setLookAndFeel(nullptr);}
void VocalSauceEditor::stone(juce::Graphics&g,juce::Rectangle<int> r){
    g.setColour(black);g.fillRect(r.translated(3,4));g.setColour(stoneDark);g.fillRect(r);
    g.setColour(stoneGray);g.fillRect(r.reduced(4));
    g.setColour(stoneLight);g.fillRect(r.reduced(4).removeFromTop(3));
}
void VocalSauceEditor::paint(juce::Graphics&g){
    g.fillAll(dirt);
    // Pixel-dirt texture - drawn as original geometry, no Mojang assets included.
    juce::Random rng(1717);
    const auto grains[]={juce::Colour::fromRGB(51,41,32),juce::Colour::fromRGB(33,27,22),
                         juce::Colour::fromRGB(58,44,32)};
    for(int i=0;i<2500;++i){g.setColour(grains[i%3]);
        g.fillRect(rng.nextInt(width/5)*5,rng.nextInt(height/5)*5,5,5);}
    stone(g,{14,11,width-28,69});
    g.setFont(pixelFont(31));g.setColour(black);
    g.drawText("17TESS VOCAL SAUCE",29,28,560,41,juce::Justification::centredLeft);
    g.setColour(pale);g.drawText("17TESS VOCAL SAUCE",27,26,560,41,juce::Justification::centredLeft);
    g.setFont(pixelFont(15));g.setColour(gold);
    g.drawText("PIXEL EDITION",795,33,210,30,juce::Justification::centredRight);
    for(int i=0;i<8;++i){
        const int x=14+(i%4)*257,y=95+(i/4)*163;
        stone(g,{x,y,248,154});
        if(expanded==i){g.setColour(grass);g.drawRect(x+3,y+3,242,148,3);}
    }
    stone(g,{14,430,650,294});
    stone(g,{674,430,352,294});
    g.setColour(pale);g.setFont(pixelFont(17));
    g.drawText("EQ  /  DRAG THE PIXEL DOTS",29,441,450,27,juce::Justification::centredLeft);
    g.drawText(expanded==-1?"DETAIL SETTINGS":expanded==8?"EQ  /  ADVANCED":
               juce::String(names[expanded])+"  /  DETAILS",691,441,299,27,juce::Justification::centredLeft);
    g.setColour(stoneLight);g.fillRect(16,737,width-32,2);
    g.setFont(pixelFont(12));g.setColour(grass);
    g.drawText("NO FACTORY PRESETS",17,752,230,26,juce::Justification::centredLeft);
}
void VocalSauceEditor::resized(){
    for(int i=0;i<8;++i){
        const int x=14+(i%4)*257,y=95+(i/4)*163;
        auto& tile=effects[static_cast<size_t>(i)];
        tile.name.setBounds(x+13,y+12,203,24);
        tile.more.setBounds(x+208,y+9,29,29);
        tile.knob.setBounds(x+76,y+39,96,87);
        tile.value.setBounds(x+50,y+123,148,22);
    }
    graph.setBounds(29,469,620,155);
    eqExpand.setBounds(618,440,31,27);
    for(int i=0;i<3;++i){
        eqKnobs[static_cast<size_t>(i)].setBounds(57+202*i,625,102,77);
        eqLabels[static_cast<size_t>(i)].setBounds(171+202*i,647,58,26);
    }
    detailHint.setBounds(692,535,318,62);
    int visibleIndex=0;
    for(auto& d:details){
        if(d.owner==expanded){
            const int y=476+visibleIndex*48;
            d.label.setBounds(692,y,120,27);
            d.slider.setBounds(808,y,201,27);
            ++visibleIndex;
        }
    }
    saveButton.setBounds(674,744,168,44);
    loadButton.setBounds(851,744,176,44);
    version.setBounds(220,745,450,36);
}
void VocalSauceEditor::setExpanded(int selected){
    expanded=(selected==expanded?-1:selected);
    bool hasDetails=false;
    for(auto& d:details){const bool show=(d.owner==expanded);
        d.slider.setVisible(show);d.label.setVisible(show);hasDetails|=show;}
    detailHint.setVisible(!hasDetails);
    if(!hasDetails){
        juce::String msg="PRESS + TO OPEN EFFECT SETTINGS";
        if(expanded>=0 && expanded<8)
            msg=juce::String(names[expanded])+"  :  MAIN KNOB IS THE ONLY PARAMETER";
        detailHint.setText(msg,juce::dontSendNotification);
    }
    resized();repaint();
}
void VocalSauceEditor::showError(const juce::String& msg){
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
        "17TESS VOCAL SAUCE",msg);
}
void VocalSauceEditor::savePreset(){
    auto initial=processor.getPresetDirectory().getChildFile("my_vocal.17tesspreset");
    chooser=std::make_unique<juce::FileChooser>("Save 17TESS preset",initial,"*.17tesspreset");
    juce::Component::SafePointer<VocalSauceEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::canSelectFiles |
                         juce::FileBrowserComponent::warnAboutOverwriting,
        [safe](const juce::FileChooser& dialog){
            if(!safe)return;
            auto file=dialog.getResult();if(file==juce::File())return;
            if(!file.hasFileExtension("17tesspreset"))file=file.withFileExtension(".17tesspreset");
            juce::String error;
            if(!safe->processor.savePreset(file,error))safe->showError(error);
        });
}
void VocalSauceEditor::loadPreset(){
    chooser=std::make_unique<juce::FileChooser>("Load 17TESS preset",
        processor.getPresetDirectory(),"*.17tesspreset");
    juce::Component::SafePointer<VocalSauceEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                         juce::FileBrowserComponent::canSelectFiles,
        [safe](const juce::FileChooser& dialog){
            if(!safe)return;
            auto file=dialog.getResult();if(!file.existsAsFile())return;
            juce::String error;
            if(!safe->processor.loadPreset(file,error))safe->showError(error);
        });
}
void VocalSauceEditor::timerCallback(){graph.repaint();}
