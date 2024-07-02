#ifndef CUSTOMLOOKANDFEEL_H
#define CUSTOMLOOKANDFEEL_H

#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawResizableFrame(juce::Graphics& g, int w, int h, const juce::BorderSize<int>& border) override
    {
        g.setColour(juce::Colours::blueviolet);
        g.drawRect(0, 0, w, h, 4); // X- Draw thicker border with thickness 4
    }

    void drawStretchableLayoutResizerBar(juce::Graphics& g, int w, int h, bool isVerticalBar, bool isMouseOver, bool isMouseDragging) override
    {
        if (isMouseOver || isMouseDragging)
            g.fillAll(juce::Colours::lightblue); // X- Set color for resizer indicator when hovered or dragging
        else
            g.fillAll(juce::Colours::grey); // X- Default color for resizer indicator

        g.setColour(juce::Colours::darkgrey);

        if (isVerticalBar)
        {
            for (int i = 3; i < h; i += 5)
                g.drawRect(0, i, w, 2); // X- Draw horizontal lines for vertical bar with thicker lines
        }
        else
        {
            for (int i = 3; i < w; i += 5)
                g.drawRect(i, 0, 2, h); // X- Draw vertical lines for horizontal bar with thicker lines
        }
    }
};

#endif // CUSTOMLOOKANDFEEL_H