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

void LookAndFeel::drawToggleButton(juce::Graphics &g, juce::ToggleButton &toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    using namespace juce;
    
    // Power button-like design:
    Path powerButton;
    
    auto bounds = toggleButton.getLocalBounds();
    
    // Somewhat reduced size:
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 4;
    
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();
    
    float ang = 30.f;
    
    size -= 6; 
    
    powerButton.addCentredArc(r.getCentreX(),
                              r.getCentreY(),
                              size * 0.5,
                              size * 0.5,
                              0.f,
                              degreesToRadians(ang),
                              degreesToRadians(360.f - ang),
                              true);
    
    
    // Vertical line:
    
    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());
    
    // Customise joint style to rounded edges:
    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);
    
    auto colour = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
    g.setColour(colour);
    g.strokePath(powerButton, pst);
    
    // Draw ellipse around button:
    g.drawEllipse(r, 2);
    
    
    
    
    
    
    
    
    
    
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
    
    
// Slider bounding boxes (just for reference): 
//    g.setColour(Colours::red);
//    g.drawRect(getLocalBounds());
//
//    // Different outline colour for sliders:
//    g.setColour(Colours::yellow);
//    g.drawRect(sliderBounds);
    
    // Map slider value to normalised range:
    getLookAndFeel().drawRotarySlider(g,  sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), startAng, endAng, *this);
    
    // Param min/max val labels:
    
    // Bounding box:
    
    auto centre = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    
    // Iterate through labels:
    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        // Place centre at edge of slider bounding box, not colliding with circle:
        auto c = centre.getPointOnCircumference(radius + getTextHeight() * 0.5 + 1, ang);
        
        Rectangle<float> r;
        
        auto str = labels[i].label;
        
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        
        // Shift down (along y axis) from circle:
        r.setY(r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
        
    }
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
//    return juce::String(getValue());
    
    if (auto * choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    // Whether or not to express freq value as Khz:
    bool addK = false;
    
    if (auto * floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();
        
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        // 2 decimal places if AddK is true, otherwise the number of places required:
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; // should not happen, but just in case...
    }
    
    // Does not apply in the case of the Q param:
    if (suffix.isNotEmpty())
    {
        // Add a space:
        str << " ";
        
        if (addK)
            str << "K";
        
        str << suffix;
    }
    
    return str;
}

// =========================================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p):
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();

    // Add listener for each param:
    for (auto param : params)
    {
        
        param->addListener(this);
    }
    
   
    // Update MonoChain (at launch/reopening of plugin GUI):
    updateChain();
    
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

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    
    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0)
    {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer))
        {
            auto size  = tempIncomingBuffer.getNumSamples();
            
            // shift all content of increment num_samples - size to the 0 index:
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);
            
            // copy most recent block from tempIncomingBuffer to end of monoBuffer;
            // position in buffer depends upon size of tempIncomingBuffer:
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
        
    }
    
    /*
     If there are FFT data buffers to pull,
     and if a buffer can be pulled,
     generate a path via pathProducer.
     */
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    
    // bin width = 48000 / 2048 = 23 Hz
    const auto binWidth = sampleRate / (double)fftSize;
    
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    
    /* While there are paths that can be pulled,
       pull as many as possible;
       display most recent path.
     */
    
    while (pathProducer.getNumPathsAvailable())
    {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback()
{
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
    
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);
    
    if(parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
        
        // Signal a repaint (draw new response curve)
//        repaint();
    }
    // Pepaint all the time, as paths are being produced continuously:
    repaint();
}

// To ensure that current parameters are displayed in response curve at plugin launch:
void ResponseCurveComponent::updateChain()
{
    // Update the Monochain
    
    double sampleRate = audioProcessor.getSampleRate();
    
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    
    // Update curve with filter bypass settings
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    
    
    auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    
    updateCutFilter(monoChain.get<LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
    
}
    
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
       
    // Black background:
    g.fillAll(Colours::black);
    
    // Draw background grid: 
    g.drawImage(background, getLocalBounds().toFloat());
        
//    auto responseArea = getLocalBounds();

    // Reduced bounds:
    auto responseArea = getAnalysisArea();
        
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
                
        // Only check conditions/execute if lowcut filter is not bypassed:
        if (!monoChain.isBypassed<ChainPositions::LowCut>())
        {
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
        }
            
        if (!monoChain.isBypassed<ChainPositions::HighCut>())
        {
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
    
    
    auto leftChannelFFTPath = leftPathProducer.getPath();
    
    // Translate FFT spectrum analyser function to response area origin:
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() - 10.f));
    
    // Paint FFT analysis path for left channel:
    g.setColour(Colours::skyblue);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
    
    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY() - 10.f));
    
    // Paint FFT analysis path for right channel:
    g.setColour(Colours::darkcyan);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
        
    // Outline colour:
        
    g.setColour(Colours::white);
        
    // Draw response curve:
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized()
{
    using namespace juce;
    
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    
    // Create graphics context:
    Graphics g(background);
    
   
    Array<float> freqs
    {
        20, 50, 100,
        200,  500, 1000,
        2000, 5000, 10000,
        20000
    };
    auto renderArea = getAnalysisArea();
    
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    Array<float> xs;
    for (auto f: freqs)
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
        
    }
    
    g.setColour(Colours::dimgrey);
    
    for (auto x: xs)
    {
        // draw a vertical line:
        g.drawVerticalLine(x, top, bottom);
    }
    
    Array<float> gain
    {
        -24, -12, 0, 12, 24
    };
    
    for (auto gDb: gain)
    {
        // Bottom of range: bottom of component
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        // green line @ centre (0 dB):
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }
    
    // Frequency labels:
    
    g.setColour(Colours::lightgrey);
    
    // In area between top of editor and freq response box:
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    for (int i = 0; i < freqs.size(); ++i)
    {
        auto f = freqs[i];
        auto x = xs[i];
        
        bool addK = false;
        String str;
        
        if (f > 999.f)
        {
            addK = true;
            f /= 1000.f;
        }
        
        str << f;
        if (addK)
            str << "K";
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    // Gain labels:
    
    for (auto gDb: gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        String str;
        
        if (gDb > 0)
        {
            str << "+";
        }
        
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u): Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
        
        
        // Left side: dB values remapped to [-48, 0] dB relative scale:
        str.clear();
        
        str << (gDb - 24.f);
        
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    // Reduce top/bottom and side bounds, for labelling purposes:
    
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20); 
    
    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    
    return bounds;
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
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),

lowcutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowcutBypassButton),
highcutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highcutBypassButton),
peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
analyserEnabledButtonAttachment(audioProcessor.apvts, "Analyser Enabled", analyserEnabledButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
//    setSize (600, 480);
    
// Initialise param min/max labels:
    peakFreqSlider.labels.add({0.f, "20Hz"});
    peakFreqSlider.labels.add({1.f, "20KHz"});
    
    peakGainSlider.labels.add({0.f, "-24dB"});
    peakGainSlider.labels.add({1.f, "+24dB"});
    
    peakQualitySlider.labels.add({0.f, "0.1"});
    peakQualitySlider.labels.add({1.f, "10"});

    lowCutFreqSlider.labels.add({0.f, "20Hz"});
    lowCutFreqSlider.labels.add({1.f, "20KHz"});
    
    lowCutSlopeSlider.labels.add({0.f, "12dB/Oct"});
    lowCutSlopeSlider.labels.add({1.f, "48dB/Oct"});
    
    highCutFreqSlider.labels.add({0.f, "20Hz"});
    highCutFreqSlider.labels.add({1.f, "20KHz"});
    
    highCutSlopeSlider.labels.add({0.f, "12dB/Oct"});
    highCutSlopeSlider.labels.add({1.f, "48dB/Oct"});
    
    // Add and make visible each slider:
    for ( auto* comp: getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    lowcutBypassButton.setLookAndFeel(&lnf);
    highcutBypassButton.setLookAndFeel(&lnf);
    peakBypassButton.setLookAndFeel(&lnf);
    

    // Embiggen the editor window:
    setSize(480, 500);
}



SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    lowcutBypassButton.setLookAndFeel(nullptr);
    highcutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
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
    
    
    // Response area = Some height ratio down from top (the rectangle in which the response curve will be situated):
    float hRatio = 25.f / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * hRatio);
    
    responseCurveComponent.setBounds(responseArea);
    
    // offset current bounds a few pixels below bottom boundary of responseArea:
    bounds.removeFromTop(5);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    
    // Offset to 1/2 of remaining width:
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowcutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(bounds.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highcutBypassButton.setBounds(highCutArea.removeFromTop(25));
    
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(bounds.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakBypassButton.setBounds(bounds.removeFromTop(25)); 
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
        &responseCurveComponent,
        
        // Bypass buttons:
        &lowcutBypassButton,
        &highcutBypassButton,
        &peakBypassButton,
        &analyserEnabledButton
    };
}
