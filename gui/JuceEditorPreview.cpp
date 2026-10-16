#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

class PrimeEightEditorPreview final : public juce::Component
{
public:
    PrimeEightEditorPreview() { setSize (940, 490); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour::fromRGB (14, 15, 15));
        g.setColour (juce::Colour::fromRGB (232, 218, 195));
        g.setFont (juce::FontOptions (39.0f, juce::Font::bold));
        g.drawText ("Prime", 24, 22, 135, 52, juce::Justification::centredLeft);
        g.setColour (juce::Colour::fromRGB (237, 48, 37));
        g.drawText ("Eight", 140, 22, 180, 52, juce::Justification::centredLeft);
        g.setColour (juce::Colour::fromRGB (184, 182, 175));
        g.setFont (juce::FontOptions (14.0f));
        g.drawText ("A N A L O G   D E L A Y   P R O C E S S O R",
                    25, 79, 620, 22, juce::Justification::centredLeft);
        g.setColour (juce::Colour::fromRGB (113, 109, 102));
        g.drawRect (24, 112, 892, 48);
        g.setColour (juce::Colour::fromRGB (223, 215, 202));
        g.drawText ("DELAY ENGINE", 39, 122, 240, 28, juce::Justification::centredLeft);
    }
};

class PrimeEightPreviewApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "PrimeEight JUCE Editor Preview"; }
    const juce::String getApplicationVersion() override { return "0.133"; }
    void initialise (const juce::String&) override
    {
        window = std::make_unique<PreviewWindow>();
    }
    void shutdown() override { window.reset(); }
private:
    class PreviewWindow final : public juce::DocumentWindow
    {
    public:
        PreviewWindow()
            : juce::DocumentWindow ("PrimeEight Editor Preview",
                                    juce::Colours::black,
                                    juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new PrimeEightEditorPreview(), true);
            centreWithSize (940, 490);
            setVisible (true);
        }
        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
    std::unique_ptr<PreviewWindow> window;
};

START_JUCE_APPLICATION (PrimeEightPreviewApplication)
