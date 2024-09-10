#ifndef CUSTOMLOOKANDFEEL_H
#define CUSTOMLOOKANDFEEL_H

#include <JuceHeader.h>
#include <juce_graphics/juce_graphics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawResizableFrame(juce::Graphics& g, int w, int h, const juce::BorderSize<int>& border) override
    {
        g.setColour(juce::Colours::blueviolet);
        g.drawRect(10, 10, w, h, 4); // Draw thicker border with thickness 4
    }

    void drawStretchableLayoutResizerBar(juce::Graphics& g, int w, int h, bool isVerticalBar, bool isMouseOver, bool isMouseDragging) override
    {
        if (isMouseOver || isMouseDragging)
            g.fillAll(juce::Colours::lightblue); // Set color for resizer indicator when hovered or dragging
        else
            g.fillAll(juce::Colours::grey); // Default color for resizer indicator

        g.setColour(juce::Colours::darkgrey);

        if (isVerticalBar)
        {
            for (int i = 3; i < h; i += 5)
                g.drawRect(0, i, w, 2); // Draw horizontal lines for vertical bar with thicker lines
        }
        else
        {
            for (int i = 3; i < w; i += 5)
                g.drawRect(i, 00 , 20, h); // Draw vertical lines for horizontal bar with thicker lines
        }
    }

    void drawTickBox(juce::Graphics& g, juce::Component& component,
                     float x, float y, float w, float h,
                     bool ticked,
                     bool isEnabled,
                     bool shouldDrawButtonAsHighlighted,
                     bool shouldDrawButtonAsDown) override
    {
        auto bounds = juce::Rectangle<float>(x, y, w, h).reduced(0.5f, 0.5f);

        auto tickWidth = juce::jmin(15.0f, bounds.getHeight() * 0.75f);
        auto tickBounds = bounds.removeFromLeft(tickWidth).reduced(2.0f);

        auto edge = 2.0f;

        g.setColour(component.findColour(juce::ToggleButton::tickColourId));
        g.drawRect(tickBounds, edge);

        if (ticked)
        {
            g.drawLine(tickBounds.getX() + edge,
                       tickBounds.getCentreY(),
                       tickBounds.getCentreX(),
                       tickBounds.getBottom() - edge,
                       edge);

            g.drawLine(tickBounds.getCentreX(),
                       tickBounds.getBottom() - edge,
                       tickBounds.getRight() - edge,
                       tickBounds.getY() + edge,
                       edge);
        }
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));

        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font(getLabelFont(label));

            g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
            g.setFont(font);

            auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());

            g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                             juce::jmax(1, static_cast<int>(textArea.getHeight() / font.getHeight())),
                             label.getMinimumHorizontalScale());

            g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(alpha));
        }
        else if (label.isEnabled())
        {
            g.setColour(label.findColour(juce::Label::outlineColourId));
        }

        g.drawRect(label.getLocalBounds());
    }

    // Customizing button appearance
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto buttonArea = button.getLocalBounds().toFloat();
        auto edge = 4.0f;

        auto offset = shouldDrawButtonAsDown ? -edge / 2.0f : -edge;
        auto cornerSize = juce::jmin(static_cast<float>(button.getHeight()), static_cast<float>(button.getWidth())) / 2.0f;

        g.setColour(backgroundColour);
        g.fillRoundedRectangle(buttonArea.reduced(offset), cornerSize);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(buttonArea.reduced(offset), cornerSize, 1.0f);
    }

    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto fontSize = juce::jmin(15.0f, static_cast<float>(button.getHeight()) * 0.75f);
        juce::Font font(juce::FontOptions().withHeight(static_cast<float>(fontSize)));
        g.setFont(font);
        g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                              : juce::TextButton::textColourOffId));

        auto yIndent = static_cast<float>(juce::jmin(4, static_cast<int>(button.proportionOfHeight(0.3f))));
        auto leftIndent = static_cast<float>(juce::jmin(button.getHeight(), button.getWidth())) / 4.0f;

        auto textArea = button.getLocalBounds().reduced(static_cast<int>(leftIndent), static_cast<int>(yIndent));

        g.drawFittedText(button.getButtonText(), textArea, juce::Justification::centred, 2);
    }
};

#endif // CUSTOMLOOKANDFEEL_H