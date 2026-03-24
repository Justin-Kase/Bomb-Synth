#include "WavetableDisplay.h"

WavetableDisplay::WavetableDisplay() {
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    // Force library init on construction (happens in UI thread, fine)
    (void)WavetableBankLibrary::get();
}

void WavetableDisplay::setBankIndex(int idx) {
    int nBanks = WavetableBankLibrary::get().numBanks();
    bankIdx_ = juce::jlimit(0, nBanks - 1, idx);
    repaint();
}

void WavetableDisplay::setMorphPos(float pos) {
    morphPos_ = juce::jlimit(0.f, 1.f, pos);
    repaint();
}

// ── Path builder ─────────────────────────────────────────────────────────────
juce::Path WavetableDisplay::buildWaveformPath(const juce::Rectangle<float>& area) const {
    const auto& bank     = WavetableBankLibrary::get().bank(bankIdx_);
    const int   nPoints  = 256;
    const float cx       = area.getCentreX();
    const float cy       = area.getCentreY();
    const float halfH    = area.getHeight() * 0.44f;

    juce::Path p;
    bool first = true;
    for (int i = 0; i <= nPoints; ++i) {
        float phase01 = (float)i / nPoints;
        float sample  = bank.getSample(phase01, morphPos_);   // [-1, 1]
        float x = area.getX() + (float)i / nPoints * area.getWidth();
        float y = cy - sample * halfH;
        if (first) { p.startNewSubPath(x, y); first = false; }
        else        p.lineTo(x, y);
    }
    return p;
}

// ── Paint ─────────────────────────────────────────────────────────────────────
void WavetableDisplay::paint(juce::Graphics& g) {
    const auto& lib   = WavetableBankLibrary::get();
    const auto& bank  = lib.bank(bankIdx_);
    const juce::Colour bankCol(bank.colourArgb);
    const int W = getWidth(), H = getHeight();

    // ── Background ────────────────────────────────────────────────────────────
    g.setColour(juce::Colour(0xFF0A0A14));
    g.fillRoundedRectangle(0.f, 0.f, (float)W, (float)H, 6.f);

    // ── Border ────────────────────────────────────────────────────────────────
    g.setColour(bankCol.withAlpha(0.5f));
    g.drawRoundedRectangle(0.5f, 0.5f, W - 1.f, H - 1.f, 6.f, 1.f);

    // ── Header: "WAVEFORM" label + load button ────────────────────────────────
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    g.setColour(bankCol.withAlpha(0.5f));
    g.drawText("WAVEFORM", 0, 0, W - kLoadW, kHdrH, juce::Justification::centred);

    // Load button (top-right corner of header bar)
    {
        juce::Colour lc = hoverLoad_
            ? bankCol.brighter(0.5f)
            : bankCol.withAlpha(0.35f);

        // Hover: fill background
        if (hoverLoad_) {
            g.setColour(bankCol.withAlpha(0.15f));
            g.fillRect(W - kLoadW, 0, kLoadW, kHdrH);
        }

        // Draw a small folder/upload icon (upward arrow over a line)
        const float bx = W - kLoadW + kLoadW * 0.5f;
        const float by = kHdrH * 0.5f;
        g.setColour(lc);
        juce::Path arrow;
        arrow.addTriangle(bx - 3.5f, by + 1.f,
                          bx + 3.5f, by + 1.f,
                          bx,        by - 3.f);
        g.fillPath(arrow);
        g.fillRect(bx - 1.f, by + 1.f, 2.f, 3.f);        // stem
        g.fillRect(bx - 4.f, by + 4.f, 8.f, 1.2f);       // base line
    }

    // Subtle separator under header
    g.setColour(bankCol.withAlpha(0.12f));
    g.drawHorizontalLine(kHdrH, 2.f, (float)(W - 2));

    // ── Waveform area ─────────────────────────────────────────────────────────
    const float dotRowY = H - kBarH - kDotRow;
    const juce::Rectangle<float> waveArea {4.f, (float)kHdrH + 2.f,
                                            (float)(W - 8), dotRowY - (float)kHdrH - 4.f};

    // Subtle center line
    g.setColour(juce::Colour(0xFF1A2030));
    g.drawHorizontalLine((int)waveArea.getCentreY(), waveArea.getX(), waveArea.getRight());

    // Subtle grid lines at ±0.5 amplitude
    g.setColour(juce::Colour(0xFF111820));
    float gridY1 = waveArea.getCentreY() - waveArea.getHeight() * 0.22f;
    float gridY2 = waveArea.getCentreY() + waveArea.getHeight() * 0.22f;
    g.drawHorizontalLine((int)gridY1, waveArea.getX(), waveArea.getRight());
    g.drawHorizontalLine((int)gridY2, waveArea.getX(), waveArea.getRight());

    // Clip drawing to wave area
    g.saveState();
    g.reduceClipRegion(waveArea.toNearestInt());

    juce::Path wavePath = buildWaveformPath(waveArea);

    // Pass 1: wide glow
    g.setColour(bankCol.withAlpha(0.12f));
    g.strokePath(wavePath, juce::PathStrokeType(6.f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    // Pass 2: medium glow
    g.setColour(bankCol.withAlpha(0.28f));
    g.strokePath(wavePath, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    // Pass 3: sharp line
    g.setColour(bankCol.withAlpha(0.92f));
    g.strokePath(wavePath, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));

    g.restoreState();

    // ── Frame dots ────────────────────────────────────────────────────────────
    const float dotY   = dotRowY + kDotRow * 0.5f;
    const float dotR   = 2.2f;
    const float dotTotalW = kWTFrames * (dotR * 2 + 3.f) - 3.f;
    float dotX = (W - dotTotalW) * 0.5f;

    float activeFFrame = morphPos_ * (kWTFrames - 1);
    for (int di = 0; di < kWTFrames; ++di) {
        float dist   = std::abs(activeFFrame - di);
        float bright = std::max(0.f, 1.f - dist);
        juce::Colour dotCol = (bright > 0.01f)
            ? bankCol.withAlpha(0.4f + bright * 0.6f)
            : juce::Colour(0xFF2A3040);
        g.setColour(dotCol);
        g.fillEllipse(dotX, dotY - dotR, dotR * 2, dotR * 2);
        dotX += dotR * 2 + 3.f;
    }

    // ── Navigation bar ────────────────────────────────────────────────────────
    const float barY = (float)(H - kBarH);

    // Subtle separator
    g.setColour(bankCol.withAlpha(0.2f));
    g.drawHorizontalLine((int)barY, 0.f, (float)W);

    // Left arrow
    {
        float ax = kNavW * 0.5f, ay = barY + kBarH * 0.5f;
        juce::Colour ac = hoverL_ ? bankCol.brighter(0.4f) : bankCol.withAlpha(0.55f);
        g.setColour(ac);
        juce::Path arrow;
        arrow.addTriangle(ax + 5.f, ay - 5.f, ax + 5.f, ay + 5.f, ax - 3.f, ay);
        g.fillPath(arrow);
    }

    // Right arrow
    {
        float ax = W - kNavW * 0.5f, ay = barY + kBarH * 0.5f;
        juce::Colour ac = hoverR_ ? bankCol.brighter(0.4f) : bankCol.withAlpha(0.55f);
        g.setColour(ac);
        juce::Path arrow;
        arrow.addTriangle(ax - 5.f, ay - 5.f, ax - 5.f, ay + 5.f, ax + 3.f, ay);
        g.fillPath(arrow);
    }

    // Bank name + index
    g.setFont(juce::Font(10.f, juce::Font::bold));
    g.setColour(bankCol);
    g.drawText(bank.name,
               kNavW, (int)barY, W - kNavW * 2, kBarH,
               juce::Justification::centred);

    // Bank index indicator (tiny dots — one per bank, capped at 16 for space)
    const float bDotR = 2.f, bDotGap = 2.f;
    const int   nb     = juce::jmin(lib.numBanks(), 16);
    float bDotTotalW   = nb * (bDotR * 2 + bDotGap) - bDotGap;
    float bDotX        = (W - bDotTotalW) * 0.5f;
    // draw at very bottom inside bar
    float bDotY = barY + kBarH - 5.f;
    for (int bi = 0; bi < nb; ++bi) {
        g.setColour(bi == bankIdx_ ? bankCol : bankCol.withAlpha(0.2f));
        g.fillEllipse(bDotX, bDotY - bDotR, bDotR * 2, bDotR * 2);
        bDotX += bDotR * 2 + bDotGap;
    }
}

// ── Mouse handling ────────────────────────────────────────────────────────────
void WavetableDisplay::mouseMove(const juce::MouseEvent& e) {
    const int H  = getHeight();
    const int W  = getWidth();
    bool wasL    = hoverL_, wasR = hoverR_, wasLd = hoverLoad_;
    hoverL_    = (e.x < kNavW    && e.y >= H - kBarH);
    hoverR_    = (e.x >= W - kNavW && e.y >= H - kBarH);
    hoverLoad_ = (e.x >= W - kLoadW && e.y < kHdrH);
    if (hoverL_ != wasL || hoverR_ != wasR || hoverLoad_ != wasLd) repaint();
}

void WavetableDisplay::mouseDown(const juce::MouseEvent& e) {
    const int H  = getHeight();
    const int W  = getWidth();

    // Load button (top-right header corner)
    if (e.y < kHdrH && e.x >= W - kLoadW) {
        if (onLoadWavetable) onLoadWavetable();
        return;
    }

    if (e.y < H - kBarH) return;   // ignore waveform area clicks

    const int nBanks = WavetableBankLibrary::get().numBanks();
    int newIdx = bankIdx_;
    if (e.x < kNavW)
        newIdx = (bankIdx_ - 1 + nBanks) % nBanks;
    else if (e.x >= W - kNavW)
        newIdx = (bankIdx_ + 1) % nBanks;

    if (newIdx != bankIdx_) {
        bankIdx_ = newIdx;
        repaint();
        if (onBankChanged) onBankChanged(bankIdx_);
    }
}

void WavetableDisplay::mouseExit(const juce::MouseEvent&) {
    hoverL_ = hoverR_ = hoverLoad_ = false;
    repaint();
}
