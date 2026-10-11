#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class PixelLook final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
};

class EQGraph final : public juce::Component {
public:
    explicit EQGraph(VocalSauceProcessor& p): processor(p) {setMouseCursor(juce::MouseCursor::PointingHandCursor);}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    juce::Point<float> point(int band) const;
    void move(const juce::MouseEvent&);
    VocalSauceProcessor& processor;
    int selected=-1;
};

class VocalSauceEditor final : public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit VocalSauceEditor(VocalSauceProcessor&);
    ~VocalSauceEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using Attachment=juce::AudioProcessorValueTreeState::SliderAttachment;
    struct MainEffect {
        juce::Slider knob;
        juce::TextButton more{"+"};
        juce::Label name;
        juce::Label value;
        std::unique_ptr<Attachment> attachment;
    };
    struct Detail {
        juce::Slider slider;
        juce::Label label;
        int owner=-1;
        std::unique_ptr<Attachment> attachment;
    };
    static constexpr int width=1040, height=798;
    VocalSauceProcessor& processor;
    PixelLook look;
    EQGraph graph;
    std::array<MainEffect,8> effects;
    std::array<juce::Slider,3> eqKnobs;
    std::array<juce::Label,3> eqLabels;
    std::array<std::unique_ptr<Attachment>,3> eqAttachments;
    std::array<Detail,11> details;
    juce::TextButton eqExpand{"+"};
    juce::TextButton saveButton{"SAVE PRESET"};
    juce::TextButton loadButton{"LOAD PRESET"};
    juce::Label detailHint;
    juce::Label version;
    int expanded=-1; // -1 = closed, 0..7 main effects, 8 = graph EQ
    std::unique_ptr<juce::FileChooser> chooser;
    void setExpanded(int);
    void savePreset();
    void loadPreset();
    void timerCallback() override;
    void showError(const juce::String&);
    static void stone(juce::Graphics&,juce::Rectangle<int>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalSauceEditor)
};
