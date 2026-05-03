#include "PluginProcessor.h"
#include "PluginEditor.h"

TheMidsRoomAudioProcessorEditor::TheMidsRoomAudioProcessorEditor (TheMidsRoomAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 520);

    juce::Random r;
    for (int i = 0; i < 6; ++i)
    {
        LavaBlob blob;
        blob.pos = { r.nextFloat() * 600, r.nextFloat() * 520 };
        blob.velocity = { (r.nextFloat() - 0.5f) * 1.2f, (r.nextFloat() - 0.5f) * 1.2f };
        blob.radius = r.nextFloat() * 100 + 50;
        blob.colour = juce::Colour((juce::uint8)r.nextInt(256), (juce::uint8)r.nextInt(256), (juce::uint8)r.nextInt(256)).withAlpha(0.12f);
        blobs.push_back(blob);
    }

    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment, const juce::String& paramID) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        slider.setLookAndFeel(&neonLookAndFeel);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha(0.8f));
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font("Orbitron", 13.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, juce::Colour(0xFFD400FF)); // Neon Purple
        addAndMakeVisible(label);

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, slider);
    };

    setupSlider(sideLevelSlider, sideLevelLabel, "SIDE LEVEL", sideLevelAttachment, "sideLevel");
    setupSlider(midLevelSlider, midLevelLabel, "MID LEVEL", midLevelAttachment, "midLevel");
    setupSlider(extraWidthSlider, extraWidthLabel, "EXTRA WIDTH", extraWidthAttachment, "extraWidth");
    setupSlider(spaceSlider, spaceLabel, "SPACE", spaceAttachment, "space");
    setupSlider(airSlider, airLabel, "AIR", airAttachment, "air");
    setupSlider(moveSlider, moveLabel, "MOVE", moveAttachment, "move");
    setupSlider(mixSlider, mixLabel, "MIX", mixAttachment, "mix");

    dtButton.setButtonText("DOUBLE TRACK");
    dtButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    dtButton.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFA000FF));
    addAndMakeVisible(dtButton);
    dtAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "doubleTrack", dtButton);

    startTimerHz(60);
}

TheMidsRoomAudioProcessorEditor::~TheMidsRoomAudioProcessorEditor()
{
    stopTimer();
    midLevelSlider.setLookAndFeel(nullptr);
    sideLevelSlider.setLookAndFeel(nullptr);
    extraWidthSlider.setLookAndFeel(nullptr);
    spaceSlider.setLookAndFeel(nullptr);
    airSlider.setLookAndFeel(nullptr);
    moveSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
}

void TheMidsRoomAudioProcessorEditor::timerCallback()
{
    for (auto& blob : blobs) blob.update(getWidth(), getHeight());
    repaint();
}

void TheMidsRoomAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0A001A));
    for (const auto& blob : blobs)
    {
        g.setColour(blob.colour);
        g.fillEllipse(blob.pos.x - blob.radius, blob.pos.y - blob.radius, blob.radius * 2, blob.radius * 2);
    }
    
    g.setColour(juce::Colour(0xFFD400FF).withAlpha(0.2f));
    g.drawRoundedRectangle(getLocalBounds().reduced(15).toFloat(), 20.0f, 2.0f);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font("Orbitron", 28.0f, juce::Font::bold));
    g.drawText("THE MIDS ROOM", 0, 30, getWidth(), 40, juce::Justification::centred);
    
    g.setColour(juce::Colour(0xFFA000FF).withAlpha(0.7f));
    g.setFont(juce::Font(12.0f, juce::Font::italic));
    g.drawText("ULTRA-WIDE SPATIAL ENGINE", 0, 65, getWidth(), 20, juce::Justification::centred);
}

void TheMidsRoomAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(40);
    area.removeFromTop(70);

    dtButton.setBounds(area.removeFromTop(40).withSizeKeepingCentre(160, 30));
    area.removeFromTop(10);

    auto row1 = area.removeFromTop(area.getHeight() / 2);
    auto sliderW = row1.getWidth() / 4;

    sideLevelSlider.setBounds(row1.removeFromLeft(sliderW).reduced(8));
    sideLevelLabel.setBounds(sideLevelSlider.getX(), sideLevelSlider.getY() - 15, sideLevelSlider.getWidth(), 20);

    midLevelSlider.setBounds(row1.removeFromLeft(sliderW).reduced(8));
    midLevelLabel.setBounds(midLevelSlider.getX(), midLevelSlider.getY() - 15, midLevelSlider.getWidth(), 20);

    extraWidthSlider.setBounds(row1.removeFromLeft(sliderW).reduced(8));
    extraWidthLabel.setBounds(extraWidthSlider.getX(), extraWidthSlider.getY() - 15, extraWidthSlider.getWidth(), 20);

    spaceSlider.setBounds(row1.reduced(8));
    spaceLabel.setBounds(spaceSlider.getX(), spaceSlider.getY() - 15, spaceSlider.getWidth(), 20);

    auto row2 = area;
    sliderW = row2.getWidth() / 3;
    airSlider.setBounds(row2.removeFromLeft(sliderW).reduced(10));
    airLabel.setBounds(airSlider.getX(), airSlider.getY() - 15, airSlider.getWidth(), 20);

    moveSlider.setBounds(row2.removeFromLeft(sliderW).reduced(10));
    moveLabel.setBounds(moveSlider.getX(), moveSlider.getY() - 15, moveSlider.getWidth(), 20);

    mixSlider.setBounds(row2.reduced(10));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getY() - 15, mixSlider.getWidth(), 20);
}
