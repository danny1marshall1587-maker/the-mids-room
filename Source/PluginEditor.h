#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct NeonLookAndFeel : public juce::LookAndFeel_V4
{
    NeonLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFA000FF)); // Neon Violet
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF1A0033)); // Very Dark Purple
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFD400FF)); // Bright Neon Purple
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                          const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 6.0f;
        auto arcRadius = radius - lineW * 0.5f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(outline);
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(fill);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            
            // Glow
            g.setColour(fill.withAlpha(0.3f));
            g.strokePath(valueArc, juce::PathStrokeType(lineW * 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        auto thumbWidth = 10.0f;
        juce::Path thumb;
        thumb.addRectangle(-thumbWidth * 0.5f, -radius - 4.0f, thumbWidth, 12.0f);
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillPath(thumb, juce::AffineTransform::rotation(toAngle).translated(bounds.getCentreX(), bounds.getCentreY()));
    }
};

struct LavaBlob
{
    juce::Point<float> pos, velocity;
    float radius;
    juce::Colour colour;

    void update(int w, int h)
    {
        pos += velocity;
        if (pos.x < 0 || pos.x > w) velocity.x *= -1;
        if (pos.y < 0 || pos.y > h) velocity.y *= -1;
    }
};

class TheMidsRoomAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    TheMidsRoomAudioProcessorEditor (TheMidsRoomAudioProcessor&);
    ~TheMidsRoomAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    TheMidsRoomAudioProcessor& audioProcessor;

    NeonLookAndFeel neonLookAndFeel;
    std::vector<LavaBlob> blobs;

    juce::Slider midLevelSlider, sideLevelSlider, spaceSlider, airSlider, moveSlider, mixSlider, extraWidthSlider;
    juce::Label midLevelLabel, sideLevelLabel, spaceLabel, airLabel, moveLabel, mixLabel, extraWidthLabel;
    juce::ToggleButton dtButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midLevelAttachment, sideLevelAttachment, spaceAttachment, airAttachment, moveAttachment, mixAttachment, extraWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dtAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheMidsRoomAudioProcessorEditor)
};
