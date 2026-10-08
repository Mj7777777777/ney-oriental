# Ney Oriental

Instrument virtuel (VST3 / AU) de flûte de roseau orientale — ney, kawala — par modèle physique, sans samples.
Pensé pour FL Studio, sur Windows et Mac.

## Obtenir le plugin sans rien installer

1. Créez un dépôt GitHub et déposez-y tout le contenu de ce dossier.
2. GitHub compile tout seul (onglet **Actions**, environ 10 minutes).
3. Les fichiers prêts à installer apparaissent dans **Releases** : un zip Windows, un zip Mac.
4. Suivez `INSTALLATION.txt`.

## Compiler soi-même

Il faut CMake 3.22+ et un compilateur (Visual Studio 2022 sur Windows, Xcode sur Mac). JUCE 8.0.9 est téléchargé automatiquement.

```
cmake -B build
cmake --build build --config Release --target NeyOriental_VST3
```

Le plugin se trouve ensuite dans `build/NeyOriental_artefacts/Release/VST3/`.

## Ce qu'il y a dedans

| Fichier | Rôle |
|---|---|
| `Source/NeyEngine.h` | Le modèle physique : tube (ligne à retard), jet d'air non linéaire, souffle, vibrato, attaque, registre aigu |
| `Source/NeyTuning.h` | Table de correction de justesse du modèle (mesurée : environ ±3 cents sur toute la tessiture) |
| `Source/NeySynth.h` | Voix mono liée / poly 4 voix, pédale, suréchantillonnage ×2 |
| `Source/PluginProcessor.*` | Paramètres, MIDI, maqams, presets, réverbération, sauvegarde |
| `Source/PluginEditor.*` | Interface |
| `tests/dsp_tests.cpp` | Tests automatiques du moteur (justesse, quarts de ton, stabilité) |
| `tests/demo.cpp` | Rend une phrase de démonstration en WAV avec le moteur seul |

## Si le son paraît trop synthétique

- Montez **Bruit** et baissez **Brillance** : le ney est un instrument très soufflé et plutôt sombre.
- Mettez un peu d'**Attaque** et de **Glide**, et jouez lié.
- Laissez le **Vibrato** bas et dosez-le à la molette, note par note.
- Les réglages de fond sont dans `NeyEngine.h`, fonction `control()` : `noiseGain` et `airGain` (souffle),
  `fc` (pertes du tube), `tfc` (filtre de timbre), `evenAmt` (harmoniques pairs).

## Limites connues

- Le timbre est synthétisé : il évoque un ney, il ne remplace pas un ney enregistré.
- Pas de MPE. Le pitch bend, l'aftertouch de canal et les contrôleurs 1, 2, 11 et 64 sont pris en charge.
