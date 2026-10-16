#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "PrimeEightEmbeddedArtwork.h"

// Reusable vector editor; original Inkscape SVG assets remain untouched.
class PrimeEightEditorComponent final : public juce::Component
{
public:
    static constexpr int editorWidth = 940;
    static constexpr int editorHeight = 490;
    PrimeEightEditorComponent()
    {
        setSize(editorWidth, editorHeight);
        auto headingXml = juce::parseXML(juce::String::fromUTF8(primeEightHeadingSvg));
        auto logoXml = juce::parseXML(juce::String::fromUTF8(primeEightLightLogoSvg));
        if (headingXml) heading = juce::Drawable::createFromSVG(*headingXml);
        if (logoXml) logo = juce::Drawable::createFromSVG(*logoXml);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour::fromRGB(14, 15, 15));
        // Both source SVGs are tightly cropped; margins are defined here.
        constexpr float margin = 26.0f;
        if (heading)
            heading->drawWithin(g, {margin, 24.0f, 480.0f, 124.0f},
                                juce::RectanglePlacement::centredLeft, 1.0f);
        if (logo)
            logo->drawWithin(g, {editorWidth - margin - 112.0f, 22.0f, 112.0f, 125.0f},
                             juce::RectanglePlacement::centred, 1.0f);

        g.setColour(juce::Colour::fromRGB(97, 93, 87));
        g.drawRoundedRectangle({margin, 166.0f, editorWidth - 2.0f * margin, 296.0f}, 7.0f, 1.0f);
        g.setColour(juce::Colour::fromRGB(240, 221, 198));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("DELAY ENGINE", 42, 177, 260, 25, juce::Justification::centredLeft);
    }
private:
    std::unique_ptr<juce::Drawable> heading, logo;
};
