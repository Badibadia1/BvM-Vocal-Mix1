# BvM — Guide de construction et de compatibilité

## Ce que contient cette livraison

- Le code source du plug-in JUCE.
- Le moteur DSP C++ indépendant de JUCE.
- Les tests automatiques du moteur audio.
- Une compilation CI prévue pour Windows, macOS et Linux.
- Les formats VST3 et Standalone ; AU en plus sur macOS.

## Compatibilité réaliste

BvM vise les ordinateurs **64 bits** récents ou raisonnablement récents :

- Windows 10/11 x64.
- macOS Intel x86_64 et Apple Silicon arm64.
- Linux x86_64 avec les bibliothèques audio et graphiques nécessaires.

Pour les DAW, utiliser le format VST3 lorsque l'application le prend en charge. Sur macOS, Logic Pro utilise le format AU. Pro Tools exige généralement un plug-in AAX : ce format n'est pas fourni dans cette version. Les systèmes 32 bits ne sont pas pris en charge.

Aucun développeur ne peut garantir « toutes les DAW et tous les ordinateurs » sans connaître leurs versions, leurs systèmes et leurs formats de plug-ins. Il faut donc valider les hôtes exacts avant une commercialisation.

## Construire les tests audio (sans Internet)

Installer CMake 3.22+ et un compilateur C++17, puis lancer :

```bash
cmake -S . -B build-dsp -DBVM_BUILD_PLUGIN=OFF -DBVM_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-dsp --config Release
ctest --test-dir build-dsp -C Release --output-on-failure
```

## Construire le plug-in

Installer CMake 3.22+, Git, un compilateur C++17 et les dépendances audio/GUI de la plateforme. La configuration télécharge JUCE 8.0.4 automatiquement :

```bash
cmake -S . -B build -DBVM_BUILD_PLUGIN=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Si JUCE est déjà installé localement :

```bash
cmake -S . -B build -DBVM_JUCE_SOURCE_DIR=/chemin/vers/JUCE -DCMAKE_BUILD_TYPE=Release
```

Les résultats sont dans `build/BvM_artefacts/Release/`.

## Réduire la consommation CPU

- Désactiver Auto-Tune et les harmonies quand ils ne sont pas utilisés : le détecteur YIN est alors complètement arrêté.
- Laisser le mix de réverbération à zéro si aucune réverbération n'est nécessaire : son traitement est alors contourné.
- Éviter d'activer simultanément plusieurs traitements lourds si la machine est ancienne.
- Garder à l'esprit que 96/192 kHz et les petits buffers augmentent la charge CPU.

## Avant toute vente

Compiler sur chaque système cible, scanner le plug-in dans les DAW choisies, vérifier la sauvegarde des projets et des paramètres, tester les automations, les fréquences d'échantillonnage, le mono/stéréo, les changements de presets, les longues sessions et les performances sur une machine modeste. Faire aussi une validation d'écoute professionnelle : les tests automatisés ne remplacent pas l'écoute.
