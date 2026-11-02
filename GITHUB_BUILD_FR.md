# Compiler BvM avec GitHub Actions

Le workflow `.github/workflows/build.yml` compile BvM sur deux machines cloud :

- Windows : VST3 et application Standalone 64 bits.
- macOS : VST3, AU et application Standalone. Le projet demande une compilation universelle Intel + Apple Silicon.

## Première mise en ligne

1. Créer un compte sur https://github.com si nécessaire.
2. Créer un nouveau dépôt, par exemple `BvM-Vocal-Mix`.
3. Décompresser l'archive et ouvrir le dossier `BvM` qui contient `CMakeLists.txt`.
4. Envoyer tout le contenu de ce dossier à la racine du dépôt GitHub. Le dossier `.github/workflows/` doit être conservé.
5. Ouvrir l'onglet **Actions**, choisir **Build BvM**, puis **Run workflow**.
6. Attendre la fin des deux tâches. Une coche verte indique que la compilation et les tests ont réussi ; une croix rouge signifie qu'il faut lire les journaux et corriger l'erreur.
7. Dans la page de l'exécution terminée, télécharger les artefacts `BvM-Windows-Release` et `BvM-macOS-Release` dans la section **Artifacts**.

Chaque nouveau push sur `main` ou `master` relance également la compilation.

## Important

- Ce workflow automatise la compilation ; il ne garantit pas à lui seul que le plugin soit exempt de bugs ou compatible avec tous les DAW.
- AU est un format réservé à macOS. Windows produit VST3 et Standalone.
- Avant toute distribution commerciale, vérifier les conditions de licence de JUCE 8 et remplacer/configurer correctement le système de licence BvM. Le secret de licence actuellement présent dans le code est un texte de remplacement et ne doit pas être considéré comme une protection commerciale.
- Les artefacts GitHub sont temporaires (14 jours avec cette configuration). Télécharger et conserver les versions validées.
