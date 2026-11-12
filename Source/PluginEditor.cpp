#include "PluginEditor.h"

namespace
{
const juce::Colour kCyan (0xff00e5ff);
const juce::Colour kViolet (0xffa855ff);
const juce::Colour kMagenta (0xffff2bd6);
const juce::Colour kBlue (0xff3d8bff);
const juce::Colour kGreen (0xff39ff9a);

struct KnobSpec
{
    const char* id;
    const char* name;
};

const KnobSpec kSpecs[8] = {
    { "piano", "PIANO" },
    { "pad", "PAD" },
    { "cutoff", "CUTOFF" },
    { "delay", "DELAY" },
    { "delayTime", "TIME" },
    { "reverb", "REVERB" },
    { "reverbSize", "SIZE" },
    { "volume", "VOLUME" },
};

juce::Font smallCapsFont (float size, float kerning)
{
    return juce::Font (juce::FontOptions (size).withStyle ("Bold").withKerningFactor (kerning));
}
} // namespace

//==============================================================================
LenWorshipAudioProcessorEditor::LenWorshipAudioProcessorEditor (LenWorshipAudioProcessor& p)
    : AudioProcessorEditor (&p), owner (p),
      panels { Panel { "LAYERS", kCyan, 0, 2, {} },
               Panel { "FILTER", kViolet, 2, 1, {} },
               Panel { "DELAY", kMagenta, 3, 2, {} },
               Panel { "REVERB", kBlue, 5, 2, {} },
               Panel { "MASTER", kGreen, 7, 1, {} } }
{
    setLookAndFeel (&lnf);

    for (const auto& panel : panels)
    {
        for (int i = panel.firstKnob; i < panel.firstKnob + panel.numKnobs; ++i)
        {
            auto& k = knobs[(size_t) i];

            k.slider.setColour (juce::Slider::rotarySliderFillColourId, panel.colour);
            k.slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd7dbe6));
            k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
            k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
            k.slider.setTextBoxIsEditable (false);
            addAndMakeVisible (k.slider);

            k.label.setText (kSpecs[i].name, juce::dontSendNotification);
            k.label.setJustificationType (juce::Justification::centred);
            k.label.setFont (smallCapsFont (13.0f, 0.18f));
            k.label.setColour (juce::Label::textColourId, panel.colour.brighter (0.35f));
            k.label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (k.label);

            k.attachment = std::make_unique<SliderAttachment> (owner.apvts, kSpecs[i].id, k.slider);
        }
    }

    setSize (1000, 400);
}

LenWorshipAudioProcessorEditor::~LenWorshipAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void LenWorshipAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = b.getWidth();

    // ---------------------------------------------------------------- fondo negro
    g.fillAll (juce::Colour (0xff040406));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0d0d14), 0.0f, 0.0f,
                                             juce::Colour (0xff030304), 0.0f, b.getBottom(), false));
    g.fillRect (b);

    // neblina de neon en las esquinas
    g.setGradientFill (juce::ColourGradient (kMagenta.withAlpha (0.16f), 0.0f, 0.0f,
                                             kMagenta.withAlpha (0.0f), 420.0f, 0.0f, true));
    g.fillRect (b);
    g.setGradientFill (juce::ColourGradient (kCyan.withAlpha (0.14f), w, b.getBottom(),
                                             kCyan.withAlpha (0.0f), w - 460.0f, b.getBottom(), true));
    g.fillRect (b);

    // marco
    g.setColour (juce::Colour (0xff1c1c26));
    g.drawRoundedRectangle (b.reduced (1.5f), 10.0f, 1.5f);

    // ---------------------------------------------------------------- titulo grabado
    const juce::String title = "Len-Worship";
    const auto titleArea = juce::Rectangle<int> (0, 12, getWidth(), 70);
    g.setFont (juce::Font (juce::FontOptions (50.0f).withStyle ("Bold").withKerningFactor (0.10f)));

    // resplandor tenue de neon detras del grabado
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (kCyan.withAlpha (0.05f));
        g.drawText (title, titleArea.expanded (i * 2, i), juce::Justification::centred);
        g.setColour (kMagenta.withAlpha (0.03f));
        g.drawText (title, titleArea.expanded (i * 3, i * 2), juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.28f)); // borde de luz abajo
    g.drawText (title, titleArea.translated (0, 2), juce::Justification::centred);
    g.setColour (juce::Colours::black); // sombra interior arriba
    g.drawText (title, titleArea.translated (0, -1), juce::Justification::centred);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a90aa), 0.0f, (float) titleArea.getY() + 14.0f,
                                             juce::Colour (0xff1b1c26), 0.0f, (float) titleArea.getBottom() - 10.0f, false));
    g.drawText (title, titleArea, juce::Justification::centred);

    // subtitulo
    g.setFont (smallCapsFont (11.0f, 0.35f));
    g.setColour (kMagenta.withAlpha (0.75f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("DARK PIANO   \xc2\xb7   WORSHIP PADS")),
                juce::Rectangle<int> (0, 80, getWidth(), 16), juce::Justification::centred);

    // linea de neon bajo el titulo
    {
        const float ly = 104.0f;
        const float x0 = w * 0.12f, x1 = w * 0.88f;
        juce::ColourGradient line (kMagenta, x0, ly, kCyan, x1, ly, false);
        line.addColour (0.5, kViolet);
        for (int i = 3; i >= 1; --i)
        {
            g.setGradientFill (line);
            g.setOpacity (0.10f);
            g.fillRoundedRectangle (x0, ly - (float) i * 1.5f, x1 - x0, 2.0f + (float) i * 3.0f, 3.0f);
        }
        g.setGradientFill (line);
        g.setOpacity (1.0f);
        g.fillRoundedRectangle (x0, ly, x1 - x0, 2.0f, 1.0f);
    }

    // ---------------------------------------------------------------- paneles
    for (const auto& panel : panels)
    {
        const auto r = panel.bounds.toFloat();
        const auto col = panel.colour;

        g.setColour (juce::Colour (0xff08080d).withAlpha (0.92f));
        g.fillRoundedRectangle (r, 14.0f);

        for (int i = 3; i >= 1; --i)
        {
            g.setColour (col.withAlpha (0.05f * (float) (4 - i)));
            g.drawRoundedRectangle (r.expanded ((float) i * 1.5f), 14.0f + (float) i, 1.5f + (float) i * 0.8f);
        }

        g.setColour (col.withAlpha (0.60f));
        g.drawRoundedRectangle (r, 14.0f, 1.2f);

        g.setFont (smallCapsFont (11.0f, 0.35f));
        g.setColour (col);
        g.drawText (panel.name, panel.bounds.withHeight (34).withTrimmedTop (10), juce::Justification::centred);
    }
}

void LenWorshipAudioProcessorEditor::resized()
{
    const int margin = 24, gap = 12, top = 116;
    const int panelHeight = getHeight() - top - 24;

    int totalKnobs = 0;
    for (const auto& p : panels)
        totalKnobs += p.numKnobs;

    const int available = getWidth() - 2 * margin - gap * ((int) panels.size() - 1);
    const float unit = (float) available / (float) totalKnobs;

    float x = (float) margin;
    for (auto& panel : panels)
    {
        const int pw = juce::roundToInt (unit * (float) panel.numKnobs);
        panel.bounds = { juce::roundToInt (x), top, pw, panelHeight };
        x += (float) pw + (float) gap;

        auto area = panel.bounds.reduced (6, 0).withTrimmedTop (38).withTrimmedBottom (10);
        const int cellW = area.getWidth() / panel.numKnobs;

        for (int i = 0; i < panel.numKnobs; ++i)
        {
            auto cell = area.removeFromLeft (cellW);
            auto& k = knobs[(size_t) (panel.firstKnob + i)];
            k.label.setBounds (cell.removeFromTop (22));
            k.slider.setBounds (cell);
        }
    }
}
