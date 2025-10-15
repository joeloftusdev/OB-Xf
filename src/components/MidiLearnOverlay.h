#ifndef MIDILEARNOVERLAY_H
#define MIDILEARNOVERLAY_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <algorithm>

enum class AnchorPosition
{
    Below,
    Above,
    Overlay,
    Left,
    Right
};

class MidiLearnOverlay : public juce::Component
{
  public:
    MidiLearnOverlay(Component *anchor,
                     std::function<int(Component *)> getCC,
                     const AnchorPosition position = AnchorPosition::Overlay,
                     const int customWidth_ = -1,
                     const int customHeight_ = -1,
                     const float customPadding_ = -1.0f)
        : anchorComp(anchor),
          getCCForComp(std::move(getCC)),
          anchorPosition(position),
          customWidth(customWidth_),
          customHeight(customHeight_),
          customPadding(customPadding_)
    {
    }

    void setCustomOverlayWidth(const int width) { customWidth = width; updatePosition(); }
    void setCustomOverlayHeight(const int height) { customHeight = height; updatePosition(); }
    void setCustomOverlayPadding(const float pad) { customPadding = pad; repaint(); }

    void setScaleFactor(const float sf)
    {
        scaleFactor = std::max(0.01f, sf);
        updatePosition();
    }

    void setSelected(const bool selected)
    {
        isSelected = selected;
        juce::MessageManager::callAsync([this] { repaint(); });
    }

    bool getSelected() const { return isSelected; }

    void mouseDown(const juce::MouseEvent &event) override
    {
        if (onSelectionCallback)
            onSelectionCallback(this);

        if (getCCForComp(anchorComp) >= 0)
        {
            const auto contentRect = getContentRect();
            const int xSize = scaledXSize();
            const float leftPadding = scaledLeftPadding();
            const float xLeft = contentRect.getX() + leftPadding;
            const float yCenter = contentRect.getCentreY();

            if (const juce::Rectangle clearArea(xLeft, yCenter - xSize / 2.0f,
                                                      static_cast<float>(xSize),
                                                      static_cast<float>(xSize));
                clearArea.contains(event.position))
            {
                if (onClearCallback)
                    onClearCallback(anchorComp);
            }
        }
    }

    void paint(juce::Graphics &g) override
    {
        const auto boxRect = getLocalBounds();
        const int tailSize = tailSizeCached;

        juce::Path bubblePath;
        juce::Rectangle<float> bubbleRect;

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
            bubbleRect = boxRect.toFloat().withTrimmedTop(tailSize);
            bubblePath.addRoundedRectangle(bubbleRect, cornerRadiusCached);
            bubblePath.addTriangle(bubbleRect.getCentreX() - tailSize / 2.0f, bubbleRect.getY(),
                                   bubbleRect.getCentreX() + tailSize / 2.0f, bubbleRect.getY(),
                                   bubbleRect.getCentreX(), bubbleRect.getY() - tailSize);
            break;
        case AnchorPosition::Above:
            bubbleRect = boxRect.toFloat().withTrimmedBottom(tailSize);
            bubblePath.addRoundedRectangle(bubbleRect, cornerRadiusCached);
            bubblePath.addTriangle(
                bubbleRect.getCentreX() - tailSize / 2.0f, bubbleRect.getBottom(),
                bubbleRect.getCentreX() + tailSize / 2.0f, bubbleRect.getBottom(),
                bubbleRect.getCentreX(), bubbleRect.getBottom() + tailSize);
            break;
        case AnchorPosition::Left:
            bubbleRect = boxRect.toFloat().withTrimmedRight(tailSize);
            bubblePath.addRoundedRectangle(bubbleRect, cornerRadiusCached);
            bubblePath.addTriangle(bubbleRect.getRight(), bubbleRect.getCentreY() - tailSize / 2.0f,
                                   bubbleRect.getRight(), bubbleRect.getCentreY() + tailSize / 2.0f,
                                   bubbleRect.getRight() + tailSize, bubbleRect.getCentreY());
            break;
        case AnchorPosition::Right:
            bubbleRect = boxRect.toFloat().withTrimmedLeft(tailSize);
            bubblePath.addRoundedRectangle(bubbleRect, cornerRadiusCached);
            bubblePath.addTriangle(bubbleRect.getX(), bubbleRect.getCentreY() - tailSize / 2.0f,
                                   bubbleRect.getX(), bubbleRect.getCentreY() + tailSize / 2.0f,
                                   bubbleRect.getX() - tailSize, bubbleRect.getCentreY());
            break;
        default:
            bubbleRect = boxRect.toFloat();
            bubblePath.addRoundedRectangle(bubbleRect, cornerRadiusCached);
            break;
        }

        g.setColour(isSelected ? juce::Colour(0xFF404040) : juce::Colour(0xFF2A2A2A));
        g.fillPath(bubblePath);

        g.setColour(isSelected ? juce::Colour(0xFF666666) : juce::Colour(0xFF444444));
        g.strokePath(bubblePath, juce::PathStrokeType(std::max(1.0f, tailSizeCached * 0.18f)));

        const auto contentRect = getContentRect();

        if (const int cc = getCCForComp(anchorComp); cc < 0)
        {
            g.setColour(isSelected ? juce::Colours::yellow : juce::Colours::red);
            const int dashY = static_cast<int>(contentRect.getCentreY());
            g.drawLine(contentRect.getX() + 4.0f, static_cast<float>(dashY),
                       contentRect.getRight() - 4.0f, static_cast<float>(dashY),
                       std::max(1.0f, tailSizeCached * 0.18f));
            if (isSelected)
            {
                g.setColour(juce::Colours::white);
                g.setFont(std::max(8.0f, overlayHeightCached * 0.35f));
            }
        }
        else
        {
            const float w = contentRect.getWidth();
            const float h = contentRect.getHeight();
            const float yCenter = contentRect.getCentreY();

            const bool isSide = (anchorPosition == AnchorPosition::Left || anchorPosition == AnchorPosition::Right);
            const float xBoxSize = h * (isSide ? 0.35f : 0.55f);
            const float gap = w * (isSide ? 0.08f : 0.13f);
            const float sepWidth = w * (isSide ? 0.012f : 0.018f);
            const float fontSize = h * (isSide ? 0.45f : 0.75f);
            const float ccGap = gap * (isSide ? 0.9f : 0.5f);

            const float totalWidth = xBoxSize + gap + sepWidth + ccGap + fontSize * (isSide ? 2.5f : 2.2f);
            const float startX = contentRect.getX() + (w - totalWidth) * 0.5f;

            // x
            const juce::Rectangle xRect(startX, yCenter - xBoxSize * 0.5f, xBoxSize, xBoxSize);
            g.setColour(juce::Colours::white);
            const float xStroke = std::max(1.0f, h * (isSide ? 0.09f : 0.12f));
            g.drawLine(xRect.getX(), xRect.getY(), xRect.getRight(), xRect.getBottom(), xStroke);
            g.drawLine(xRect.getRight(), xRect.getY(), xRect.getX(), xRect.getBottom(), xStroke);

            // sep
            const float sepX = xRect.getRight() + gap + sepWidth * 0.5f;
            g.drawLine(sepX, contentRect.getY(), sepX, contentRect.getBottom(), sepWidth);

            // cc
            g.setFont(fontSize);
            const juce::Rectangle ccRect(sepX + ccGap, contentRect.getY(), fontSize * (isSide ? 2.5f : 2.2f), h);
            g.drawText(juce::String(cc), ccRect, juce::Justification::centredLeft, false);
        }
    }

    void parentHierarchyChanged() override { updatePosition(); }

void updatePosition()
{
    if (anchorComp && getParentComponent())
    {
        const auto anchorBounds = anchorComp->getBounds();
        computeScaledSizes(anchorBounds);

        const int offset = std::max(1, tailSizeCached / 2);

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
        {
            const int width = (customWidth > 0) ? juce::roundToInt(customWidth * scaleFactor) : anchorBounds.getWidth();
            const int x = anchorBounds.getX() + (anchorBounds.getWidth() - width) / 2;
            const int y = anchorBounds.getBottom() + offset;
            setBounds(x, y, width, overlayHeightCached);
            break;
        }
        case AnchorPosition::Above:
        {
            const int width = (customWidth > 0) ? juce::roundToInt(customWidth * scaleFactor) : anchorBounds.getWidth();
            const int x = anchorBounds.getX() + (anchorBounds.getWidth() - width) / 2;
            const int y = anchorBounds.getY() - overlayHeightCached - offset;
            setBounds(x, y, width, overlayHeightCached);
            break;
        }
        case AnchorPosition::Overlay:
        {
            const int width  = (customWidth  > 0) ? juce::roundToInt(customWidth  * scaleFactor) : anchorBounds.getWidth();
            const int height = (customHeight > 0) ? juce::roundToInt(customHeight * scaleFactor) : anchorBounds.getHeight();
            const int x = anchorBounds.getX() + (anchorBounds.getWidth() - width) / 2;
            const int y = anchorBounds.getY() + (anchorBounds.getHeight() - height) / 2;
            setBounds(x, y, width, height);
            break;
        }
        case AnchorPosition::Left:
        {
            const int w = sideOverlayWidthCached;
            const int h = overlayHeightCached;
            const int x = anchorBounds.getX() - w - offset;
            const int y = anchorBounds.getY() + (anchorBounds.getHeight() - h) / 2;
            setBounds(x, y, w, h);
            break;
        }
        case AnchorPosition::Right:
        {
            const int w = sideOverlayWidthCached;
            const int h = overlayHeightCached;
            const int x = anchorBounds.getRight() + offset;
            const int y = anchorBounds.getY() + (anchorBounds.getHeight() - h) / 2;
            setBounds(x, y, w, h);
            break;
        }
        }
    }
    repaint();
}

    std::function<void(MidiLearnOverlay *)> onSelectionCallback;
    std::function<void(Component *)> onClearCallback;

    Component *anchorComp;

    void setMidiLearnActive(const bool active)
    {
        midiLearnActive = active;
        repaint();
    }
    void setComponentSelected(const bool selected)
    {
        componentSelected = selected;
        repaint();
    }

  private:
    bool midiLearnActive = false;
    bool componentSelected = false;

    std::function<int(Component *)> getCCForComp;
    AnchorPosition anchorPosition;
    bool isSelected = false;

    int overlayHeightCached = 28;
    int sideOverlayWidthCached = 45;
    int tailSizeCached = 8;
    float cornerRadiusCached = 8.0f;

    int customWidth = -1;
    int customHeight = -1;
    float customPadding = -1.0f;

    float scaleFactor = 1.0f;

    void computeScaledSizes(const juce::Rectangle<int> &anchorBounds)
    {
        overlayHeightCached = (customHeight > 0) ? juce::jmax(12, juce::roundToInt(customHeight * scaleFactor))
                                                 : juce::jmax(12, juce::roundToInt(anchorBounds.getHeight() * 0.6f));

        if (anchorPosition == AnchorPosition::Left || anchorPosition == AnchorPosition::Right)
            sideOverlayWidthCached = (customWidth > 0) ? juce::jmax(36, juce::roundToInt(customWidth * scaleFactor))
                                                       : juce::jmax(36, juce::roundToInt(anchorBounds.getWidth() * 1.2f));
        else
            sideOverlayWidthCached = juce::jmax(20, juce::roundToInt(anchorBounds.getWidth() * 0.6f));

        tailSizeCached = juce::jmax(4, juce::roundToInt(overlayHeightCached * 0.28f));
        cornerRadiusCached = std::max(4.0f, overlayHeightCached * 0.28f);
    }

    int scaledXSize() const { return juce::jmax(6, juce::roundToInt(overlayHeightCached * 0.28f)); }
    float scaledLeftPadding() const { return std::max(4.0f, overlayHeightCached * 0.28f); }
    float scaledSepPadding() const { return std::max(6.0f, overlayHeightCached * 0.28f); }

    juce::Rectangle<float> getContentRect() const
    {
        const auto boxRect = getLocalBounds().toFloat();
        const int tailSize = tailSizeCached;

        const float horizontalPadding = (customPadding > 0.0f) ? (customPadding * scaleFactor)
                                                               : (overlayHeightCached * 0.12f);
        const float verticalPadding = overlayHeightCached * 0.08f;

        switch (anchorPosition)
        {
        case AnchorPosition::Below:
            return boxRect.withTrimmedTop(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Above:
            return boxRect.withTrimmedBottom(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Left:
            return boxRect.withTrimmedRight(tailSize).reduced(horizontalPadding, verticalPadding);
        case AnchorPosition::Right:
            return boxRect.withTrimmedLeft(tailSize).reduced(horizontalPadding, verticalPadding);
        default:
            return boxRect.reduced(horizontalPadding, verticalPadding);
        }
    }
};

#endif // MIDILEARNOVERLAY_H