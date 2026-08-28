// Offscreen render of the real editor. Dev tool: verifies the panel without a
// host. Writes panel.png into the directory given as argv[1].
#include <juce_gui_basics/juce_gui_basics.h>

#include "Panel.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::String ("."));

    CapicolaProcessor proc;
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize ((int) capi::panel::designW, (int) capi::panel::designH);

    juce::Image img (juce::Image::ARGB, editor->getWidth() * 2, editor->getHeight() * 2, true);
    juce::Graphics g (img);
    g.addTransform (juce::AffineTransform::scale (2.0f));
    editor->paintEntireComponent (g, false);

    const auto out = outDir.getChildFile ("panel.png");
    juce::FileOutputStream stream (out);
    stream.setPosition (0);
    stream.truncate();
    juce::PNGImageFormat().writeImageToStream (img, stream);
    std::printf ("%s\n", out.getFullPathName().toRawUTF8());

    return 0;
}
