/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
   void drawRotarySlider(juce::Graphics&,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                         juce::Slider&) override;
    
    
    
    
};

// Rotary Slider customisation:
struct RotarySliderWithLabels : juce::Slider
{
    
    // Set LookAndFeel in constructor:
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix):
       juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }
    
    // unset LookAndFeel in destructor:
    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }
    
    void paint(juce::Graphics& g) override; 
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const {return 14;}
    
private:
    
    // LookAndFeel instance:
    LookAndFeel lnf;
    // base class, with access to all relevant methods:
    juce::RangedAudioParameter* param;
    juce::String suffix;

};

// Response curve as separate component (to avoid exceeding editor boundaries):
struct ResponseCurveComponent: juce::Component,
juce::AudioProcessorParameter::Listener,
juce::Timer
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();
    
    // Listener callback functions:
    void parameterValueChanged (int parameterIndex, float newValue) override;
    
    // Not relevant: empty implementation
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {};
    
    void timerCallback() override;
    
    void paint(juce::Graphics&) override;
    
    SimpleEQAudioProcessor& audioProcessor;
    
    // Atomic flag for updating response curve GUI:
    juce::Atomic<bool> parametersChanged { false };
    
    MonoChain monoChain; 
};

//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;
    

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    
    // The rotary sliders:
    RotarySliderWithLabels peakFreqSlider,
    peakGainSlider,
    peakQualitySlider,
    lowCutFreqSlider,
    highCutFreqSlider,
    lowCutSlopeSlider,
    highCutSlopeSlider;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    // declare instance of ResponseCurveComponent:
    ResponseCurveComponent responseCurveComponent;
    
    
    // Attach sliders to parameters: 
    Attachment peakFreqSliderAttachment,
               peakGainSliderAttachment,
               peakQualitySliderAttachment,
               lowCutFreqSliderAttachment,
               highCutFreqSliderAttachment,
               lowCutSlopeSliderAttachment,
               highCutSlopeSliderAttachment;
                
   
    // Retrieve all sliders as a vector for ease of iteration through all sliders:
    std::vector<juce::Component*> getComps();
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
