// Prevent Windows SDK min/max macros from breaking std::min in MSVC.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>
#include "PluginProcessor.h"
#include "PluginEditor.h"
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
#if JUCE_WINDOWS
 #include <windows.h>
#endif

VocalSauceProcessor::VocalSauceProcessor():
    juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "17TESS_PARAMETERS", makeLayout()) {
    for(size_t i=0;i<rawParameters.size();++i)
        rawParameters[i]=parameters.getRawParameterValue(SauceDSP::ids[i]);
}
juce::AudioProcessorValueTreeState::ParameterLayout VocalSauceProcessor::makeLayout(){
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto make=[&](const char* id,const char* name,float min,float max,float step,float def,
                  const char* unit){
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id,1},name,juce::NormalisableRange<float>{min,max,step},def,
            juce::AudioParameterFloatAttributes().withLabel(unit)));
    };
    make("sat","Saturation",0,100,0.1f,0,"%");
    make("dist","Distortion",0,100,0.1f,0,"%");
    make("air","Air",0,100,0.1f,0,"%");
    make("punch","Punch",0,100,0.1f,0,"%");
    make("wet","Wet",0,100,0.1f,100,"%");
    make("deess","De-ess",0,100,0.1f,0,"%");
    make("color","Color EQ",0,100,0.1f,0,"%");
    make("chorus","Chorus",0,100,0.1f,0,"%");
    make("eq_low","EQ Low",-12,12,0.1f,0,"dB");
    make("eq_mid","EQ Mid",-12,12,0.1f,0,"dB");
    make("eq_high","EQ High",-12,12,0.1f,0,"dB");
    make("dist_hp","Distortion HP",20,1000,1,65,"Hz");
    make("dist_lp","Distortion LP",800,16000,1,15000,"Hz");
    make("chorus_rate","Chorus Rate",0.1f,5,0.01f,0.8f,"Hz");
    make("chorus_depth","Chorus Depth",0,100,0.1f,40,"%");
    make("color_tilt","Color Tilt",-100,100,0.1f,0,"%");
    make("deess_freq","De-ess Frequency",3500,10500,1,6600,"Hz");
    make("punch_speed","Punch Attack",20,180,1,90,"Hz");
    make("eq_freq_low","EQ Low Frequency",30,680,1,160,"Hz");
    make("eq_freq_mid","EQ Mid Frequency",350,3750,1,1200,"Hz");
    make("eq_freq_high","EQ High Frequency",2500,16500,1,7000,"Hz");
    make("eq_width","EQ Width",0.3f,3,0.01f,1,"");
    return layout;
}
void VocalSauceProcessor::prepareToPlay(double sampleRate,int) {
    engine.prepare(sampleRate,getTotalNumOutputChannels());
}
void VocalSauceProcessor::releaseResources() {engine.reset();}
bool VocalSauceProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    auto mainIn=layout.getMainInputChannelSet();
    auto mainOut=layout.getMainOutputChannelSet();
    return (mainIn==juce::AudioChannelSet::mono() || mainIn==juce::AudioChannelSet::stereo()) && mainIn==mainOut;
}
void VocalSauceProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int count=(std::min)(2,audio.getNumChannels());
    if(count<=0 || audio.getNumSamples()==0)return;
    std::array<float,SauceDSP::numParams> values{};
    for(size_t i=0;i<values.size();++i)values[i]=rawParameters[i]->load();
    auto* const* channels=audio.getArrayOfWritePointers();
    engine.process(channels,count,audio.getNumSamples(),values);
    for(int i=count;i<audio.getNumChannels();++i)audio.clear(i,0,audio.getNumSamples());
}
juce::AudioProcessorEditor* VocalSauceProcessor::createEditor(){return new VocalSauceEditor(*this);}
void VocalSauceProcessor::getStateInformation(juce::MemoryBlock& block){
    auto xml=parameters.copyState().createXml();
    if(xml)copyXmlToBinary(*xml,block);
}
void VocalSauceProcessor::setStateInformation(const void* data,int size){
    if(auto xml=getXmlFromBinary(data,size)){
        if(xml->hasTagName(parameters.state.getType().toString()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}
juce::File VocalSauceProcessor::getPresetDirectory() const {
    auto fallback=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("17TESS").getChildFile("VocalSauce").getChildFile("Presets");
#if JUCE_WINDOWS
    HMODULE h=nullptr;
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(reinterpret_cast<const void*>(&createPluginFilter)),&h) && h){
        wchar_t path[32768]{};
        DWORD count=GetModuleFileNameW(h,path,32768);
        if(count>0 && count<32768){
            auto module=juce::File(juce::String(path));
            auto dir=module.getParentDirectory();
            // For standard Windows VST3 bundles: Bundle.vst3/Contents/x86_64-win/Bundle.vst3
            if(dir.getFileName().equalsIgnoreCase("x86_64-win")){
                auto candidate=dir.getParentDirectory().getParentDirectory().getChildFile("Presets");
                if(candidate.createDirectory().wasOk()){
                    auto probe=candidate.getChildFile(".17tess_write_test");
                    juce::FileOutputStream out(probe);
                    if(out.openedOk()){out.writeByte('1');out.flush();probe.deleteFile();return candidate;}
                }
            }
        }
    }
#endif
    fallback.createDirectory();
    return fallback;
}
bool VocalSauceProcessor::savePreset(const juce::File& file,juce::String& error){
    if(!file.hasFileExtension("17tesspreset")){
        error="Use the .17tesspreset extension.";return false;
    }
    if(auto xml=parameters.copyState().createXml()){
        if(xml->writeTo(file))return true;
    }
    error="Cannot save preset. Check folder permissions.";return false;
}
bool VocalSauceProcessor::loadPreset(const juce::File& file,juce::String& error){
    auto xml=juce::XmlDocument::parse(file);
    if(!xml || !xml->hasTagName(parameters.state.getType().toString())){
        error="Invalid 17TESS preset.";return false;
    }
    auto tree=juce::ValueTree::fromXml(*xml);
    if(!tree.isValid()){error="Preset format error.";return false;}
    parameters.replaceState(tree);
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {return new VocalSauceProcessor();}
