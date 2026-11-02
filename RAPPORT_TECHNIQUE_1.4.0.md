# Rapport technique — BvM 1.4.0

## Modifications réellement intégrées

1. **Économie CPU du suivi de hauteur :** `Channel::processBlock` calcule une fois par bloc si Auto-Tune ou une harmonie est demandée. Si les deux sont inactifs, le détecteur YIN n'est plus alimenté et aucune analyse YIN n'est exécutée.
2. **Économie CPU des effets temporels :** Delay et Reverb ne traitent plus le signal lorsque leur niveau wet est nul. Une remise à zéro est effectuée à la transition actif → inactif pour éviter de restituer une ancienne queue d'effet à la réactivation.
3. **Construction modulaire :** les tests DSP se configurent sans JUCE (`BVM_BUILD_PLUGIN=OFF`), donc sans accès réseau.
4. **Portabilité :** la configuration déclare les formats et architectures réellement ciblés au lieu de promettre une compatibilité universelle irréaliste.
5. **Tests :** le test ajoute une vérification que YIN ne s'exécute pas quand il est inutile, qu'il s'exécute lorsque le pitch est activé, et que le DSP reste fini à 44.1, 48, 88.2, 96 et 192 kHz avec blocs non standards.

## Limites connues à ne pas masquer

- La compilation du plug-in JUCE complet et les tests dans les DAW ne sont pas effectués dans cet environnement sans dépendances JUCE préinstallées / accès réseau garanti.
- Compatibilité annoncée : VST3 64 bits sur Windows/Linux, VST3 + AU 64 bits sur macOS, et application Standalone. Pas d'AAX, pas de CLAP, pas de VST2 ni de 32 bits dans cette version.
- La limitation est sample-peak, pas true-peak.
- L'analyse dite « IA » existante est une adaptation heuristique, pas un modèle neuronal.
- Le pitch shifter actuel est un algorithme simple à deux têtes ; sa qualité doit être évaluée sur des prises vocales réelles avant de le comparer à des outils spécialisés de correction tonale.

## Validation à effectuer avant commercialisation

- Compiler Release sur Windows 10/11 x64, macOS Intel + Apple Silicon, Linux x64.
- Scanner/charger dans REAPER, FL Studio, Ableton Live, Cubase, Studio One et Logic Pro (AU uniquement sur macOS).
- Tester mono/stéréo, 44.1–192 kHz, tailles de blocs variables, automation rapide, sauvegarde/rappel de session, bypass, silence, clipping, changements de presets et longues sessions.
- Mesurer CPU, latence, mémoire et stabilité sur machines modestes et récentes.
- Écouter des voix parlées, rap grave, chant, sibilantes, transitoires et harmonies dans des conditions de mix réelles.

## Résultat de compilation dans l'environnement de travail

- Compilation C++ du banc de tests DSP : **réussie**.
- CTest : **1/1 test réussi**.
- Configuration du plug-in complet : **non terminée**, car l'environnement n'a pas de résolution DNS vers `github.com` et n'a donc pas pu récupérer JUCE 8.0.4. Ce blocage est externe au code source ; il ne constitue pas une validation de compilation du plug-in.
