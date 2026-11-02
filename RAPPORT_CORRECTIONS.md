# Rapport technique — BvM 1.3.0

## Travail de cette passe

### Limiteur look-ahead : correction structurelle
L'ancienne détection calculait la réduction à partir du seul échantillon courant alors que le signal de sortie était retardé. Elle ne garantissait donc pas que les crêtes présentes dans la fenêtre de look-ahead soient anticipées au bon moment.

Le détecteur utilise maintenant une deque monotone préallouée pour suivre le maximum de crête stéréo liée sur la fenêtre complète `[n - lookahead, n]`. Le gain calculé s'applique à l'échantillon retardé correspondant. Les buffers de délai et de détection sont préparés hors du thread audio ; aucune allocation dynamique n'est effectuée dans `process()`.

### Tests
- Signal sinusoïdal : sortie finie.
- Entrées NaN/Inf : aucune contamination persistante de la chaîne.
- Limiteur stéréo lié : sortie finie et crête sous le plafond configuré dans le test.
- Transitoire isolé : vérification du délai exact et du plafonnement grâce à l'anticipation.
- Vérification de base des paramètres et de la gamme.

Commande exécutée avec succès :

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/dsp_smoke.cpp -o bvm_dsp_test
./bvm_dsp_test
```

Résultat : `BvM DSP tests passed; parameters=34; linked look-ahead limiter latency=240 samples`.

## Ce qui n'est pas encore certifié
- Compilation du plug-in JUCE complet en VST3/AU/Standalone.
- Validation dans FL Studio, Ableton Live, Cubase, Studio One et Reaper.
- Tests de charge CPU, stabilité sur longues sessions et compatibilité de rappel d'état/automatisation.
- True-peak limiting avec suréchantillonnage : non implémenté dans cette passe.
- Le module nommé « IA » reste un ensemble de règles DSP adaptatives, pas un modèle neuronal entraîné.

Ne pas distribuer comme version commerciale certifiée avant ces étapes.
