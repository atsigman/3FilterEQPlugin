/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// extracted filter parameters:
struct ChainSettings
{
    float peakFreq{ 0 }, peakGainInDecibels { 0 }, peakQuality {1.f};
    float lowCutFreq { 0 }, highCutFreq { 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope { Slope::Slope_12 };
};


// helper function for extracting filter parameter values (returns data struct):
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class SimpleEQAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters",
        createParameterLayout()};

private:
    //aliases:
    using Filter = juce::dsp::IIR::Filter<float>;
    
    //chain of filters (for LP or HP, 12dB/octave):
    
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    
    // mono channel chain:
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    MonoChain leftChain, rightChain;
    
    enum ChainPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    void updatePeakFilter(const ChainSettings &chainSettings);
    using Coefficients = Filter::CoefficientsPtr;
    void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    // helper function:
    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients)
    {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }
    
    // template function, given unkmnown param types:
    template<typename ChainType, typename CoefficientType>
    void updateCutFilter(ChainType& leftLowCut,
                         const CoefficientType& CutCoefficients,
                         const Slope lowCutSlope)
    {
        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);
        
        switch ( lowCutSlope )
        {
            // rather than break after each case, passes to next case:
            case Slope_48:
            {
                update<3>(leftLowCut, CutCoefficients);
            }
                
            case Slope_36:
            {
                update<2>(leftLowCut, CutCoefficients);
            }
                
            case Slope_24:
            {
                update<1>(leftLowCut, CutCoefficients);
            }
                
                
            case Slope_12:
            {
                update<0>(leftLowCut, CutCoefficients);
            }
                
        };

    }
    
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters(); 

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
