/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder
{
    order2048 = 11,
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /**
     produces the FFT data from an audio buffer.
     */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity)
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        // First apply a windowing function to our data:
        window->multiplyWithWindowingTable(fftData.data(), fftSize);       // [1]

        // Then render our FFT data:
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());  // [2]

        int numBins = (int)fftSize / 2;

        // Normalize the FFT values:
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] /= (float) numBins;
        }

        // Convert them to decibels:
        for( int i = 0; i < numBins; ++i )
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        // When you change order, recreate the window, forwardFFT, fifo, fftData.
        // Also reset the fifoIndex.
        // Things that need recreating should be created on the heap via std::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; }
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); }
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); }
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};

template<typename PathType>
struct AnalyzerPathGenerator
{
    /*
     converts 'renderData[]' into a juce::Path
     */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize,
                      float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v,
                              negativeInfinity, 0.f,
                              float(bottom+10),   top);
        };

        auto y = map(renderData[0]);

//        jassert( !std::isnan(y) && !std::isinf(y) );
        if( std::isnan(y) || std::isinf(y) )
            y = bottom;
        
        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels.

        for( int binNum = 1; binNum < numBins; binNum += pathResolution )
        {
            y = map(renderData[binNum]);

//            jassert( !std::isnan(y) && !std::isinf(y) );

            if( !std::isnan(y) && !std::isinf(y) )
            {
                auto binFreq = binNum * binWidth;
                auto normalizedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalizedBinX * width);
                p.lineTo(binX, y);
            }
        }

        pathFifo.push(p);
    }

    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }
private:
    Fifo<PathType> pathFifo;
};

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
    
    void drawToggleButton(juce::Graphics &g,
                          juce::ToggleButton &toggleButton,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;
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
    
    // Label for displaying min/max values for given param:
    struct LabelPos
    {
        float pos;
        juce::String label;
    };
    
    // 2 labels (min and max) per param (rotary slider): 
    juce::Array<LabelPos> labels; 
    
    
    void paint(juce::Graphics& g) override; 
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const {return 14;}
    juce::String getDisplayString() const; 
    
private:
    
    // LookAndFeel instance:
    LookAndFeel lnf;
    // base class, with access to all relevant methods:
    juce::RangedAudioParameter* param;
    juce::String suffix;

};

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf):
    leftChannelFifo(&scsf)
    {
        /*
           sample rate = 48K
           For an FFT order of 2048, freq range divided into
           ca. 23 Hz bins (48000/2048).
           This results in low resultion for lower frequencies. Raising the no. of bins
           -> increase in resource (CPU) consumption.
            */
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        
        // Initialise buffer at new size:
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    
    void process(juce::Rectangle<float> fftBounds, double sampleRate);
    juce::Path getPath() {return leftChannelFFTPath;}; 
    
private:
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChannelFifo;
    
    juce::AudioBuffer<float> monoBuffer;
    
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;
    
    AnalyzerPathGenerator<juce::Path> pathProducer;
    
    juce::Path leftChannelFFTPath;
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
    void resized() override;
    
    void toggleAnalysisEnablement(bool enabled)
    {
        shouldShowFFTAnalysis = enabled;
    }
    
private:
    SimpleEQAudioProcessor& audioProcessor;
    
    // Atomic flag for updating response curve GUI:
    juce::Atomic<bool> parametersChanged { false };
    
    MonoChain monoChain;
    
    void updateChain();
    
    // response curve grid: 
    juce::Image background;
    
    juce::Rectangle<int> getRenderArea();
    
    // Area for actual response curve/grid lines: 
    juce::Rectangle<int> getAnalysisArea();
    
    PathProducer leftPathProducer, rightPathProducer;
    
    bool shouldShowFFTAnalysis = true;
    
};

//==============================================================================
struct PowerButton : juce::ToggleButton {};
struct AnalyserButton : juce::ToggleButton
{
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto insetRect = bounds.reduced(4);
        
        randomPath.clear();
        
        juce::Random r;
        
        auto y = insetRect.getY();
        auto height = insetRect.getHeight();
        
        // Random height between 0 and 1:
        randomPath.startNewSubPath(insetRect.getX(), y + height * r.nextFloat());
        
        // lineTo: random line seg every other pixel:
        
        for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2)
        {
            randomPath.lineTo(x, y + height * r.nextFloat());
        }
    }
    
    juce::Path randomPath;    
};

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
    
            
    
    PowerButton lowcutBypassButton, highcutBypassButton, peakBypassButton;
    AnalyserButton analyserEnabledButton; 
    
    using ButtonAttachment = APVTS::ButtonAttachment;
    
    ButtonAttachment lowcutBypassButtonAttachment,
                     highcutBypassButtonAttachment,
                     peakBypassButtonAttachment,
                     analyserEnabledButtonAttachment;
    
    // Retrieve all sliders as a vector for ease of iteration through all sliders:
    std::vector<juce::Component*> getComps();
    
    // LookAndFeel instance: 
    LookAndFeel lnf;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
