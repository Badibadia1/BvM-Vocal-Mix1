#pragma once
// BvM - Licence : essai 24 h, activation liée à l'ordinateur, persistante après réinstallation.
// L'état est stocké HORS du dossier du plugin (2 fichiers + registre sous Windows), signé par HMAC-like SHA-256.
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <mutex>
#include <atomic>
#include <limits>
#include <vector>
#include <algorithm>

namespace bvm {

class License {
public:
    // !!! CHANGEZ ce secret avant de compiler, et mettez le MÊME dans tools/keygen.py. Ne le publiez jamais.
    static constexpr const char* kSecret = "CHANGE-MOI-SECRET-LONG-ET-ALEATOIRE-2026";
    // 0 = pas d'essai : le plugin reste muet tant que le code d'activation de CET ordinateur n'est pas saisi.
    // Mettre 24 * 3600 pour réactiver un essai de 24 h.
    static constexpr juc = 0;

    std::atomic<juce::int64> expiry { 0 };     // secondes epoch ; max() si activé
    std::atomic<bool> activated { false };

    License() { id = makeId(); refresh(); }

    const juce::String& machineId() const { return id; }
    bool isActivated() const { return activated.load(); }
    static juce::int64 now() { return juce::Time::currentTimeMillis() / 1000; }
    juce::int64 secondsLeft() const {
        auto e = expiry.load();
        return e == std::numeric_limits<juce::int64>::max() ? e : std::max<juce::int64>(0, e - now());
    }

    bool activate(const juce::String& code) {
        auto norm = [](const juce::String& s) { return s.toUpperCase().retainCharacters("0123456789ABCDEF"); };
        if (norm(code).length() != 20 || norm(code) != norm(expectedCode())) return false;
        { std::lock_guard<std::mutex> lk(m); actFlag = true; }
        refresh();
        return true;
    }

    // Relit/écrit les stockages, applique les règles d'essai. Appeler hors thread audio.
    void refresh() {
        std::lock_guard<std::mutex> lk(m);
        const juce::int64 t = now();
        Rec r; bool have = false, any = false;
        for (auto& s : readAll()) {
            if (s.trim().isEmpty()) continue;
            any = true; Rec x;
            if (!parse(s, x)) continue;
            if (!have) { r = x; have = true; }
            else { r.start = std::min(r.start, x.start); r.last = std::max(r.last, x.last); r.act = r.act || x.act; }
        }
        if (!have) {
            // aucun enregistrement valide : première installation... ou falsification (=> essai expiré)
            r.start = any ? t - kTrialSec - 1 : t; r.last = t; r.act = false;
        }
        if (actFlag) r.act = true;
        const bool rollback = t + 300 < r.last;           // horloge reculée
        r.last = std::max(r.last, t);
        writeAll(serialize(r));
        activated = r.act;
        expiry = r.act ? std::numeric_limits<juce::int64>::max() : (rollback ? 0 : r.start + kTrialSec);
    }

private:
    struct Rec { juce::int64 start = 0, last = 0; bool act = false; };
    juce::String id; std::mutex m; bool actFlag = false;

    static juce::String groupStr(const juce::String& s, int n) {
        juce::String o; for (int i = 0; i < s.length(); i += n) { if (i) o << "-"; o << s.substring(i, i + n); } return o;
    }
    static juce::String hashHex(const juce::String& s) { return juce::SHA256(s.toUTF8()).toHexString().toUpperCase(); }
    static juce::String makeId() {
        return groupStr(hashHex(juce::SystemStats::getUniqueDeviceID() + "|" + juce::SystemStats::getComputerName() + "|BvM").substring(0, 16), 4);
    }
    juce::String expectedCode() const { return groupStr(hashHex(juce::String(kSecret) + ":" + id).substring(0, 20), 5); }

    juce::String sign(const Rec& r) const {
        return hashHex(juce::String(kSecret) + "#" + id + "#" + juce::String(r.start) + "#" + juce::String(r.last) + "#" + (r.act ? "1" : "0")).substring(0, 24);
    }
    juce::String serialize(const Rec& r) const { return juce::String(r.start) + "|" + juce::String(r.last) + "|" + (r.act ? "1" : "0") + "|" + sign(r); }
    bool parse(const juce::String& s, Rec& r) const {
        auto t = juce::StringArray::fromTokens(s.trim(), "|", "");
        if (t.size() != 4) return false;
        r.start = t[0].getLargeIntValue(); r.last = t[1].getLargeIntValue(); r.act = (t[2] == "1");
        return t[3] == sign(r);
    }

    static std::vector<juce::File> files() {
        return { juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile(".bvm_cache").getChildFile("b1.dat"),
                 juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory).getChildFile("BvM").getChildFile("b2.dat") };
    }
    static std::vector<juce::String> readAll() {
        std::vector<juce::String> v;
        for (auto& f : files()) v.push_back(f.existsAsFile() ? f.loadFileAsString() : juce::String());
#if JUCE_WINDOWS
        v.push_back(juce::WindowsRegistry::getValue("HKEY_CURRENT_USER\\Software\\BvM\\sys"));
#endif
        return v;
    }
    static void writeAll(const juce::String& text) {
        for (auto& f : files()) { f.getParentDirectory().createDirectory(); f.replaceWithText(text); }
#if JUCE_WINDOWS
        juce::WindowsRegistry::setValue("HKEY_CURRENT_USER\\Software\\BvM\\sys", text);
#endif
    }
};

} // namespace bvm
