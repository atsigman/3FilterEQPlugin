/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider
{
    
    CustomRotarySlider(): juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                       juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
        
    }

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
    CustomRotarySlider peakFreqSlider,
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
