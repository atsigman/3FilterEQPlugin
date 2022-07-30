/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics & g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider & slider)
{
    using namespace juce;
    
    auto bounds = Rectangle<float>(x, y, width, height);
    
    // Create circles:
    
    // Fill colour:
    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);
    
    // Outline colour:
    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);
    
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto centre = bounds.getCentre();
        Path p;
        
        Rectangle<float> r;
        
        // Set Rectangle left and right 2 pixels offset from x value (on either side):
        r.setLeft(centre.getX() - 2);
        r.setRight(centre.getX() + 2);
        
        // Set Rectangle top and bottom to top of bounding box and the centre, respectively:
        r.setTop(bounds.getY());
        // To avoid text being occluded, offset by text height * some constant:
        r.setBottom(centre.getY() - rswl->getTextHeight() * 1.5);
        
        // Add Rectangle to Path:
        p.addRoundedRectangle(r, 2.f);
        
        jassert(rotaryStartAngle < rotaryEndAngle);
        
        // Rotate:
        
        // Map normalised pos value to rotary slider range values (in radians):
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        
        
        // Apply transform to rotate position about centre (x, y) point:
        p.applyTransform(AffineTransform().rotated(sliderAngRad, centre.getX(), centre.getY()));
        
        // Draw:
        
        g.fillPath(p);
        
        // Set text font:
        
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);
        
        // Set dimensions to somewhat wider/taller than text width and height:
        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());
        
        
        // Text box background colour:
        g.setColour(Colours::black);
        g.fillRect(r);
        
        //Text colour:
        
        g.setColour(Colours::white);
        // 1 line of text:
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
    
}

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    // start angle = ca. 7:00
    auto startAng = degreesToRadians(180.f + 45.f);
    
    // 5:00: opposite side of 12:00, + 1 full rotation:
    auto endAng = degreesToRadians(180.f-45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    g.setColour(Colours::red);
    g.drawRect(getLocalBounds());
    
    // Different outline colour for sliders:
    g.setColour(Colours::yellow);
    g.drawRect(sliderBounds);
    
    // Map slider value to normalised range:
    getLookAndFeel().drawRotarySlider(g,  sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);
    
    
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    // Shrink bounding box:
    size -= getTextHeight() * 2;
    
    juce::Rectangle<int> r;
    // A square:
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    
    // Vertical coordinate 2 pixels below origin:
    r.setY(2);
    
    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    return juce::String(getValue()); 
    
}

// =========================================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p): audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();

    // Add listener for each param:
    for (auto param : params)
    {
        
        param->addListener(this);
    }
    
    // Start timer:
    startTimerHz(60);
    
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    
    for (auto param : params)
    {
        
        param-> removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if(parametersChanged.compareAndSetBool(false, true))
        
    {
        // Update the Monochain
        
        double sampleRate = audioProcessor.getSampleRate();
        
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
        auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
        
        updateCutFilter(monoChain.get<LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        
        // Signal a repaint (draw new response curve)
        repaint();
    }
}
    
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
       
    // Black background:
    g.fillAll(Colours::black);
        
    auto responseArea = getLocalBounds();
        
    auto w = responseArea.getWidth();
        
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
        
    auto sampleRate = audioProcessor.getSampleRate();
        
    std::vector<double> mags;
        
    mags.resize(w);
        
    // Iterate through magnitude vector, convert from pixels to Hz:
        
    for (int i = 0; i < w; ++i)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
            
        // If filter is not bypassed, the magnitude = the magnitude of the given frequency:
        if (!monoChain.isBypassed<ChainPositions::Peak>())
        {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
                
        if (!lowcut.isBypassed<0>())
        {
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<1>())
        {
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<2>())
        {
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<3>())
        {
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
            
        if (!highcut.isBypassed<0>())
        {
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<1>())
        {
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<2>())
        {
            mag *=highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<3>())
        {
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
            
        // Convert magnitude from gain value to dB and store value to mags vector:
            
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    // Create path:
    Path responseCurve;
        
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
        
    // Map (input) magnitude value to the range of (-24, 24) (peak gain range), and rescale range
    // to editor window Y axis range:
    auto map = [outputMin, outputMax] (double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
        
    // Start new subpath and run map function on first magnitude value:
        
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
        
    // Iterate over all magnitudes and map:
    for (size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i , map(mags[i]));
    }
        
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
        
    // Outline colour:
        
    g.setColour(Colours::white);
        
    // Draw response curve:
    g.strokePath(responseCurve, PathStrokeType(2.f));
}
    

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
// Initialise response curve and slider parameter attachments:
peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),

responseCurveComponent(audioProcessor),
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
//    setSize (400, 300);
    
    // Add and make visible each slider:
    for ( auto* comp: getComps() )
    {
        addAndMakeVisible(comp);
    }
    

    // Embiggen the editor window:
    setSize(600, 400);
}



SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
   
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
   
    // Black background:
    g.fillAll(Colours::black);
    
//    g.setColour (juce::Colours::white);
//    g.setFont (15.0f);
//    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    // Set relative positions for each slider (bounds is stateful, and thus updates after each assignment):
    
    auto bounds = getLocalBounds();
    // Response area = 1/3 down from top (the rectangle in which the response curve will be situated):
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    responseCurveComponent.setBounds(responseArea); 
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    
    // Offset to 1/2 of remaining width:
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(bounds.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(bounds.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea); 
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    
    // Offset to 1/2 of remaining height:
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    
    // Remaining bounds:
    peakQualitySlider.setBounds(bounds);
 }


std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}
