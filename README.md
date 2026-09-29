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
./build.sh       # sharkive-updater.3dsx
./build.sh cia   # zusätzlich sharkive-updater.cia
./build.sh clean
```

GitHub Actions baut bei jedem Push beide Dateien (Artifacts). Ein Tag `v*` erstellt automatisch ein Release.

## Installieren

- **.3dsx:** nach `sd:/3ds/` kopieren, über den Homebrew Launcher starten.
- **.cia:** mit FBI o. ä. installieren, erscheint im HOME-Menü.

WLAN muss verbunden sein; Luma3DS muss installiert sein.
