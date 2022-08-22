/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>

template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert( std::is_same_v<T, juce::AudioBuffer<float>>,
                      "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for( auto& buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false,   //clear everything?
                           true,    //including the extra space?
                           true);   //avoid reallocating if you can?
            buffer.clear();
        }
    }
    
    void prepare(size_t numElements)
    {
        static_assert( std::is_same_v<T, std::vector<float>>,
                      "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for( auto& buffer : buffers )
        {
            buffer.clear();
            
            // Fill buffer with 0s to initialise: 
            buffer.resize(numElements, 0);
        }
    }
    
    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if( write.blockSize1 > 0 )
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        
        return false;
    }
    
    bool pull(T& t)
    {
        auto read = fifo.read(1);
        if( read.blockSize1 > 0 )
        {
            t = buffers[read.startIndex1];
            return true;
        }
        
        return false;
    }
    
    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo {Capacity};
};

enum Channel
{
  Right, // 0
  Left   // 1
};

template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch): channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);
        bufferToFill.setSize(1,             //channel
                             bufferSize,    //num samples
                             false,         //keepExistingContent
                             true,          //clear extra space
                             true);         //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const {return audioBufferFifo.getNumAvailableForReading();}
    bool isPrepared() const {return prepared.get();}
    int getSize() const {return size.get();}
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }
    private:
        Channel channelToUse;
        int fifoIndex = 0;
        Fifo<BlockType> audioBufferFifo;
        BlockType bufferToFill;
        juce::Atomic<bool> prepared = false;
        juce::Atomic<int> size = 0;
            
        void pushNextSampleIntoFifo(float sample)
        {
            if (fifoIndex == bufferToFill.getNumSamples())
            {
                auto ok = audioBufferFifo.push(bufferToFill);

                juce::ignoreUnused(ok);
                    
                fifoIndex = 0;
            }
                
            bufferToFill.setSample(0, fifoIndex, sample);
            ++fifoIndex;
        }
    };

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
    float peakFreq{0}, peakGainInDecibels{0}, peakQuality{1.f};
    float lowCutFreq{0}, highCutFreq{0};
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope {Slope::Slope_12};
    
    bool lowCutBypassed{false}, highCutBypassed{false}, peakBypassed{false};
};


// helper function for extracting filter parameter values (returns data struct):
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

// Aliases:
using Filter = juce::dsp::IIR::Filter<float>;

//chain of filters (for LP or HP, 12dB/octave):

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

// Mono channel chain:
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);

// Helper function:
template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

// Template function, given unknown param types:
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

// Use inline keyword to avoid the compiler producing a definition in each module in which this
// .h file is included, thus confusing the linker, use inline keyword:

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);
    
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, sampleRate, (chainSettings.highCutSlope + 1) * 2);
    
}

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
    
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};
    
    using BlockType = juce::AudioBuffer<float>;
    SingleChannelSampleFifo<BlockType> leftChannelFifo{Channel::Left};
    SingleChannelSampleFifo<BlockType> rightChannelFifo{Channel::Right};
    
private:
    MonoChain leftChain, rightChain;
    
    void updatePeakFilter(const ChainSettings &chainSettings);
        
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();
    
    // Osc to verify FFT spectrum analyser accuracy:
    
    juce::dsp::Oscillator<float> osc; 

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessor)
};
