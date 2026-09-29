# Sharkive Updater

3DS-Homebrew-App, die die Cheat-Datenbank von [FlagBrew/Sharkive](https://github.com/FlagBrew/Sharkive) herunterlädt, entpackt und die Cheats nach `sd:/cheats/<TitleID>.txt` schreibt – genau dort, wo das Rosalina-Cheat-Menü von Luma3DS sie sucht.

Oben zeigt die App Uhrzeit, Datum, WLAN-Empfang und Akkustand (inkl. Laden) an.

## Bedienung

- **A** (oder Button antippen): Cheats herunterladen und installieren
- **B**: Laufendes Update abbrechen
- **START**: Beenden (bricht ein laufendes Update ab)

Im Spiel: **L + Steuerkreuz runter + SELECT** → Rosalina-Menü → *Cheats*.

## Bauen

Voraussetzung: Docker.

```sh
./build.sh
```

Ergebnis: `sharkive-updater.3dsx`. `./build.sh clean` räumt auf.

## Installieren

1. `sharkive-updater.3dsx` nach `sd:/3ds/` kopieren.
2. Auf dem 3DS den Homebrew Launcher starten und *Sharkive Updater* öffnen.
3. WLAN muss verbunden sein; Luma3DS muss installiert sein.
