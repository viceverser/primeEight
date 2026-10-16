#include "PrimeEightClapJuce.h"
#include "PrimeEightEditorComponent.h"
#include <sst/clap_juce_shim/clap_juce_shim.h>
#include <cmath>
#include <memory>

struct PrimeEightJuceBridge final : sst::clap_juce_shim::EditorProvider
{
    sst::clap_juce_shim::ClapJuceShim shim{this};
    bool created{false};
    std::unique_ptr<juce::Component> createEditor() override
    {
        return std::make_unique<PrimeEightEditorComponent>();
    }
    bool registerOrUnregisterTimer(clap_id&, int, bool) override { return false; }
    bool registerOrUnregisterPosixFd(int, clap_posix_fd_flags_t, bool) override { return false; }
};

PrimeEightJuceBridge* primeeightJuceBridgeCreate() { return new PrimeEightJuceBridge; }
void primeeightJuceBridgeDelete(PrimeEightJuceBridge* b)
{
    if (!b) return;
    if (b->created) b->shim.guiDestroy();
    delete b;
}
bool primeeightJuceGuiCreate(PrimeEightJuceBridge* b, const char* api, bool floating)
{
    if (!b || b->created || !api || !b->shim.guiIsApiSupported(api, floating)) return false;
    b->created = b->shim.guiCreate(api, floating);
    return b->created;
}
void primeeightJuceGuiDestroy(PrimeEightJuceBridge* b)
{
    if (b && b->created) { b->shim.guiDestroy(); b->created = false; }
}
bool primeeightJuceGuiParent(PrimeEightJuceBridge* b, const clap_window_t* window)
{
    return b && b->created && window && window->api &&
           b->shim.guiIsApiSupported(window->api, false) &&
           b->shim.guiSetParent(window);
}
bool primeeightJuceGuiScale(PrimeEightJuceBridge* b, double scale)
{
    return b && b->created && std::isfinite(scale) && scale > 0 && b->shim.guiSetScale(scale);
}
bool primeeightJuceGuiSize(PrimeEightJuceBridge* b, uint32_t* w, uint32_t* h)
{
    return b && b->created && w && h && b->shim.guiGetSize(w,h);
}
bool primeeightJuceGuiSetSize(PrimeEightJuceBridge* b, uint32_t w, uint32_t h)
{
    return b && b->created && w == PrimeEightEditorComponent::editorWidth &&
           h == PrimeEightEditorComponent::editorHeight && b->shim.guiSetSize(w,h);
}
bool primeeightJuceGuiShow(PrimeEightJuceBridge* b)
{
    return b && b->created && b->shim.guiShow();
}
bool primeeightJuceGuiHide(PrimeEightJuceBridge* b)
{
    // The upstream shim has no guiHide() method; detaching here would violate
    // the host's create/parent/destroy lifecycle. Leave ownership intact.
    return b && b->created;
}
