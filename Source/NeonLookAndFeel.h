#pragma once

#include "Common.h"

// Perillas negras con anillo de luz neon
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffd7dbe6));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, juce::Colour (0xffd7dbe6));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        using juce::Colour;
        using juce::Path;
        using juce::PathStrokeType;

        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const auto neon = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        const float arcR = radius * 0.90f;

        // halo suave detras de la perilla
        {
            juce::ColourGradient halo (neon.withAlpha (0.20f * (0.35f + 0.65f * sliderPos)), c.x, c.y,
                                       neon.withAlpha (0.0f), c.x, c.y - radius * 1.2f, true);
            g.setGradientFill (halo);
            g.fillEllipse (c.x - radius * 1.2f, c.y - radius * 1.2f, radius * 2.4f, radius * 2.4f);
        }

        // pista
        Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Colour (0xff1a1a24));
        g.strokePath (track, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));

        // arco de valor con brillo neon
        if (sliderPos > 0.001f)
        {
            Path val;
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, rotaryStartAngle, angle, true);

            for (int i = 4; i >= 1; --i)
            {
                g.setColour (neon.withAlpha (0.07f));
                g.strokePath (val, PathStrokeType (5.0f + (float) i * 3.5f, PathStrokeType::curved, PathStrokeType::rounded));
            }

            g.setColour (neon.withAlpha (0.95f));
            g.strokePath (val, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.strokePath (val, PathStrokeType (1.4f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // cuerpo negro
        const float bodyR = radius * 0.68f;
        {
            juce::ColourGradient body (Colour (0xff30323d), c.x - bodyR * 0.5f, c.y - bodyR * 0.7f,
                                       Colour (0xff060608), c.x + bodyR * 0.6f, c.y + bodyR * 0.8f, false);
            g.setGradientFill (body);
            g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

            g.setColour (juce::Colours::black);
            g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 2.0f);

            const float capR = bodyR * 0.80f;
            juce::ColourGradient cap (Colour (0xff15161c), c.x, c.y - capR, Colour (0xff050507), c.x, c.y + capR, false);
            g.setGradientFill (cap);
            g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);

            g.setColour (neon.withAlpha (0.30f));
            g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);
        }

        // indicador
        Path p;
        p.addRoundedRectangle (-1.8f, -bodyR * 0.92f, 3.6f, bodyR * 0.42f, 1.8f);
        const auto t = juce::AffineTransform::rotation (angle).translated (c.x, c.y);
        g.setColour (neon.withAlpha (0.25f));
        g.strokePath (p, PathStrokeType (5.0f), t);
        g.setColour (neon);
        g.fillPath (p, t);
    }
};
