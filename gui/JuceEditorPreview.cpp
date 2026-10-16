#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PrimeEightEditorComponent.h"

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
            setContentOwned (new PrimeEightEditorComponent(), true);
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
