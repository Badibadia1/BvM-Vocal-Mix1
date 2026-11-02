#pragma once
#include "PluginProcessor.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace bvmui {

using juce::Colour;
inline juce::Font F(float h, bool bold = false) { return juce::Font(juce::FontOptions(h, bold ? juce::Font::bold : juce::Font::plain)); }
inline juce::String U(const char* s) { return juce::String::fromUTF8(s); }

namespace Pal {
    const Colour bg1(0xff0B0D1A), bg2(0xff1B1035), panel(0xff151934), track(0xff2A2F4A), text(0xffA6ACCD);
    const Colour cyan(0xff22D3EE), sky(0xff38BDF8), violet(0xff8B5CF6), amber(0xffFBBF24),
                 pink(0xffF472B6), lilac(0xffC084FC), green(0xff34D399), teal(0xff2DD4BF);
}

// ---------------------------------------------------------------- Look & feel (boutons rotatifs)
class Look : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override {
        auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(5.f);
        float r = std::min(b.getWidth(), b.getHeight()) / 2.f; auto c = b.getCentre();
        float ang = a0 + pos * (a1 - a0);
        juce::PathStrokeType st(4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::Path tr; tr.addCentredArc(c.x, c.y, r, r, 0.f, a0, a1, true);
        g.setColour(Pal::track); g.strokePath(tr, st);
        juce::Path v; v.addCentredArc(c.x, c.y, r, r, 0.f, a0, ang, true);
        auto col = s.findColour(juce::Slider::rotarySliderFillColourId);
        g.setColour(col.withAlpha(0.25f)); g.strokePath(v, juce::PathStrokeType(8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(col); g.strokePath(v, st);
        float kr = r - 9.f;
        g.setGradientFill(juce::ColourGradient(Colour(0xff3C4270), c.x, c.y - kr, Colour(0xff141830), c.x, c.y + kr, false));
        g.fillEllipse(c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
        juce::Path p; p.addRoundedRectangle(-1.6f, -kr + 2.f, 3.2f, kr * 0.5f, 1.5f);
        g.setColour(juce::Colours::white); g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
    }
};

// ---------------------------------------------------------------- Écran de verrouillage (fin d'essai)
class LockPanel : public juce::Component {
public:
    std::function<void()> onUnlocked;
    explicit LockPanel(bvm::License& l) : lic(l) {
        auto lab = [this](juce::Label& L, const juce::String& t, float sz, bool bold, Colour c) {
            L.setText(t, juce::dontSendNotification); L.setFont(F(sz, bold)); L.setColour(juce::Label::textColourId, c);
            L.setJustificationType(juce::Justification::centred); L.setMinimumHorizontalScale(1.f); addAndMakeVisible(L);
        };
        lab(title, bvm::License::kTrialSec > 0 ? U("Période d'essai terminée") : U("Activation requise"), 30, true, Pal::pink);
        lab(body, U("Pour utiliser BvM, achetez votre code d'activation par Mobile Money, "
                    "puis envoyez votre ID machine (ci-dessous) avec la preuve de paiement. Vous recevrez un code valable uniquement pour cet ordinateur.\n\n"
                    "MTN MoMo : +2290190512342\nMoov Money : +22965494385\nCeltis Cash : +2290140168542"), 16, false, juce::Colours::white);
        lab(idLabel, "ID machine", 13, false, Pal::text);
        lab(status, "", 15, true, Pal::amber);
        idBox.setReadOnly(true); idBox.setText(lic.machineId()); idBox.setFont(F(18, true)); idBox.setJustification(juce::Justification::centred);
        codeBox.setTextToShowWhenEmpty("XXXXX-XXXXX-XXXXX-XXXXX", Pal::text); codeBox.setFont(F(18, true)); codeBox.setJustification(juce::Justification::centred);
        for (auto* t : { &idBox, &codeBox }) {
            t->setColour(juce::TextEditor::backgroundColourId, Pal::panel); t->setColour(juce::TextEditor::textColourId, juce::Colours::white);
            t->setColour(juce::TextEditor::outlineColourId, Pal::violet); addAndMakeVisible(*t);
        }
        btn.setButtonText(U("Activer BvM")); btn.setColour(juce::TextButton::buttonColourId, Pal::violet);
        btn.onClick = [this] {
            if (lic.activate(codeBox.getText())) { status.setColour(juce::Label::textColourId, Pal::green); status.setText(U("Activé ✔ Merci !"), juce::dontSendNotification); if (onUnlocked) onUnlocked(); }
            else { status.setColour(juce::Label::textColourId, Pal::pink); status.setText(U("Code invalide pour cet ordinateur."), juce::dontSendNotification); }
        };
        addAndMakeVisible(btn);
    }
    void paint(juce::Graphics& g) override { g.fillAll(Colour(0xf2090B16)); }
    void resized() override {
        auto r = getLocalBounds().withSizeKeepingCentre(620, 520);
        title.setBounds(r.removeFromTop(50)); body.setBounds(r.removeFromTop(170)); r.removeFromTop(8);
        idLabel.setBounds(r.removeFromTop(20)); idBox.setBounds(r.removeFromTop(38)); r.removeFromTop(10);
        codeBox.setBounds(r.removeFromTop(42)); r.removeFromTop(10);
        btn.setBounds(r.removeFromTop(42).withSizeKeepingCentre(220, 42)); status.setBounds(r.removeFromTop(30));
    }
private:
    bvm::License& lic;
    juce::Label title, body, idLabel, status; juce::TextEditor idBox, codeBox; juce::TextButton btn;
};

} // namespace bvmui

// ---------------------------------------------------------------- Éditeur
class BvMEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit BvMEditor(BvMProcessor& p) : juce::AudioProcessorEditor(&p), proc(p), lock(p.license) {
        using namespace bvmui;
        setLookAndFeel(&look);
        struct Def { const char* title; juce::uint32 col; std::vector<std::pair<const char*, const char*>> k; };
        const std::vector<Def> defs = {
            { "IA & SORTIE",      0xff22D3EE, { {"ai", "IA Voix"}, {"outGain", "Sortie dB"}, {"limCeil", "Plafond dB"}, {"limRel", "Relâche limiteur"} } },
            { "GATE",             0xff22D3EE, { {"gateThr", "Seuil dB"}, {"gateRel", "Relâche ms"} } },
            { "ANTI-BOOM",        0xff38BDF8, { {"boomHpf", "Coupe Hz"}, {"boomAmt", "Anti-boom"} } },
            { "DE-ESSER",         0xff38BDF8, { {"dsFreq", "Freq Hz"}, {"dsThr", "Seuil dB"}, {"dsAmt", "Réduc. dB"} } },
            { "COMPRESSEUR",      0xff8B5CF6, { {"cThr", "Seuil dB"}, {"cRatio", "Ratio"}, {"cAtk", "Attaque ms"}, {"cRel", "Relâche ms"} } },
            { "ÉQUALISEUR",       0xff8B5CF6, { {"eqLow", "Grave"}, {"eqBody", "Corps"}, {"eqPres", "Présence"}, {"eqAir", "Air"} } },
            { "SATURATION",       0xffFBBF24, { {"satDrive", "Drive"}, {"satMix", "Mix"} } },
            { "SOFT · ADOUCIR",   0xffF472B6, { {"soft", "Douceur"} } },
            { "CORRECTION TONALE",0xffF472B6, { {"ptSpeed", "Vitesse ms"}, {"ptAmt", "Force"} } },
            { "HARMONIES",        0xffC084FC, { {"h1", "Voix 1"}, {"h2", "Voix 2"}, {"hMix", "Niveau"} } },
            { "DELAY",            0xff34D399, { {"dTime", "Temps ms"}, {"dFb", "Répét."}, {"dMix", "Mix"} } },
            { "REVERB",           0xff2DD4BF, { {"rSize", "Taille"}, {"rDamp", "Amorti"}, {"rMix", "Mix"} } },
        };
        for (auto& d : defs) {
            Section s; s.title = U(d.title); s.col = Colour(d.col);
            for (auto& kv : d.k) {
                auto k = std::make_unique<Knob>();
                k->s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                k->s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
                k->s.setRotaryParameters(juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
                k->s.setColour(juce::Slider::rotarySliderFillColourId, s.col);
                k->s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
                k->s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
                k->l.setText(U(kv.second), juce::dontSendNotification); k->l.setFont(F(12.5f));
                k->l.setJustificationType(juce::Justification::centred); k->l.setColour(juce::Label::textColourId, Pal::text);
                k->l.setInterceptsMouseClicks(false, false);
                addAndMakeVisible(k->s); addAndMakeVisible(k->l);
                k->a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, kv.first, k->s);
                s.k.push_back(std::move(k));
            }
            secs.push_back(std::move(s));
        }
        // contrôles spécifiques à la correction tonale
        pitchBtn.setButtonText("AUTO-TUNE"); pitchBtn.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        pitchBtn.setColour(juce::ToggleButton::tickColourId, Pal::pink);
        keyBox.addItemList({ "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" }, 1);
        scaleBox.addItemList({ "Majeur", "Mineur", "Chromatique", "Pentat. mineure" }, 1);
        for (auto* c : { (juce::Component*) &pitchBtn, (juce::Component*) &keyBox, (juce::Component*) &scaleBox }) addAndMakeVisible(c);
        pitchA = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "pitchOn", pitchBtn);
        keyA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "key", keyBox);
        scaleA = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.apvts, "scale", scaleBox);

        // présets
        {
            const auto& pl = bvm::presets();
            for (size_t i = 0; i < pl.size(); ++i) presetBox.addItem(U(pl[i].name), (int) i + 1);
            const int cur = proc.getPresetIndex();
            if (cur >= 0 && cur < (int) pl.size()) presetBox.setSelectedId(cur + 1, juce::dontSendNotification);
        }
        presetBox.setTextWhenNothingSelected(U("— Choisir un préset —"));
        presetBox.onChange = [this] { proc.applyPreset(presetBox.getSelectedId() - 1); };
        addAndMakeVisible(presetBox);
        trial.setFont(F(13)); trial.setJustificationType(juce::Justification::centredRight); trial.setColour(juce::Label::textColourId, Pal::amber);
        addAndMakeVisible(trial);

        addChildComponent(lock);
        setSize(1080, 700);
        timerCallback(); startTimerHz(2);
    }
    ~BvMEditor() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        using namespace bvmui;
        g.setGradientFill(juce::ColourGradient(Pal::bg1, 0, 0, Pal::bg2, (float) getWidth(), (float) getHeight(), false)); g.fillAll();
        g.setGradientFill(juce::ColourGradient(Pal::violet, 20, 10, Pal::cyan, 170, 60, false));
        g.setFont(F(52, true)); g.drawText("BvM", 22, 8, 170, 62, juce::Justification::centredLeft);
        g.setColour(juce::Colours::white); g.setFont(F(15, true)); g.drawText("BADI-BADIA VOCAL MIX", 190, 16, 320, 22, juce::Justification::left);
        g.setColour(Pal::text); g.setFont(F(13)); g.drawText(U("Votre voix. Niveau studio mondial. En un clic."), 190, 38, 380, 20, juce::Justification::left);
        for (auto& s : secs) {
            auto r = s.r.toFloat();
            g.setColour(Pal::panel.withAlpha(0.92f)); g.fillRoundedRectangle(r, 14.f);
            g.setColour(s.col.withAlpha(0.35f)); g.drawRoundedRectangle(r.reduced(0.5f), 14.f, 1.2f);
            g.setColour(s.col); g.fillEllipse(r.getX() + 14, r.getY() + 14, 8, 8);
            g.setFont(F(13.5f, true)); g.drawText(s.title, (int) r.getX() + 28, (int) r.getY() + 8, (int) r.getWidth() - 36, 20, juce::Justification::left);
        }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(16);
        auto head = area.removeFromTop(78);
        presetBox.setBounds(head.removeFromRight(320).withTrimmedTop(8).withHeight(30));   // y = 24..54
        trial.setBounds(getWidth() - 16 - 320, 58, 320, 20);                                // sous le menu, sans chevauchement
        const int gap = 10, w = (area.getWidth() - gap * 3) / 4, h = (area.getHeight() - gap * 2) / 3;
        for (size_t i = 0; i < secs.size(); ++i) {
            auto& s = secs[i];
            s.r = { area.getX() + (int) (i % 4) * (w + gap), area.getY() + (int) (i / 4) * (h + gap), w, h };
            auto in = s.r.reduced(10, 8); in.removeFromTop(26);
            if (i == 8) {
                auto row = in.removeFromTop(26);
                pitchBtn.setBounds(row.removeFromLeft(96)); keyBox.setBounds(row.removeFromLeft(52).reduced(2));
                scaleBox.setBounds(row.reduced(2)); in.removeFromTop(2);
            }
            int n = (int) s.k.size(), kw = in.getWidth() / std::max(1, n);
            for (int j = 0; j < n; ++j) {
                auto col = in.withX(in.getX() + j * kw).withWidth(kw);
                s.k[(size_t) j]->l.setBounds(col.removeFromBottom(16));
                s.k[(size_t) j]->s.setBounds(col);
            }
        }
        lock.setBounds(getLocalBounds());
    }

private:
    struct Knob { juce::Slider s; juce::Label l; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a; };
    struct Section { juce::String title; juce::Colour col; std::vector<std::unique_ptr<Knob>> k; juce::Rectangle<int> r; };

    void timerCallback() override {
        const bool lk = proc.isLocked();
        if (lock.isVisible() != lk) lock.setVisible(lk);
        juce::String t;
        if (proc.license.isActivated()) t = bvmui::U("Licence activée ✔");
        else if (bvm::License::kTrialSec == 0) t = bvmui::U("Non activé");
        else { auto s = proc.license.secondsLeft(); t = bvmui::U("Essai gratuit : ") + juce::String((int) (s / 3600)) + " h " + juce::String((int) ((s % 3600) / 60)) + " min"; }
        trial.setText(t, juce::dontSendNotification);
    }

    BvMProcessor& proc;
    bvmui::Look look;
    std::vector<Section> secs;
    juce::ToggleButton pitchBtn; juce::ComboBox keyBox, scaleBox, presetBox; juce::Label trial;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pitchA;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyA, scaleA;
    bvmui::LockPanel lock;
};
