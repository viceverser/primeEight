#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Reusable JUCE component: no standalone application or CLAP lifecycle ownership.
// The CLAP bridge can instantiate this component through an editor provider.
class PrimeEightEditorComponent final : public juce::Component
{
public:
    static constexpr int editorWidth = 940;
    static constexpr int editorHeight = 490;

    PrimeEightEditorComponent() { setSize(editorWidth, editorHeight); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(14, 15, 15));
        g.setColour(juce::Colour::fromRGB(232, 218, 195));
        g.setFont(juce::FontOptions(39.0f, juce::Font::bold));
        g.drawText("Prime", 24, 22, 135, 52, juce::Justification::centredLeft);
        g.setColour(juce::Colour::fromRGB(237, 48, 37));
        g.drawText("Eight", 140, 22, 180, 52, juce::Justification::centredLeft);
        g.setColour(juce::Colour::fromRGB(184, 182, 175));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("A N A L O G   D E L A Y   P R O C E S S O R",
                   25, 79, 620, 22, juce::Justification::centredLeft);
        g.setColour(juce::Colour::fromRGB(113, 109, 102));
        g.drawRect(24, 112, 892, 48);
        g.setColour(juce::Colour::fromRGB(223, 215, 202));
        g.drawText("DELAY ENGINE", 39, 122, 240, 28, juce::Justification::centredLeft);
    }
};
