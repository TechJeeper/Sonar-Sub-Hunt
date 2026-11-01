# Sonar Sub Hunt (Pebble)

Sweep your arm around the room. The watch uses the **compass** and **accelerometer** to aim an invisible sonar cone; auditory pings (speaker or vibration) get faster and higher as you lock onto hidden enemy subs. **Tap the screen** (or press Select) to fire a torpedo.

## Play

| Control | Action |
| --- | --- |
| Sweep / turn | Aim with compass heading + wrist tilt |
| Tap / Select | Fire (or start / next wave) |
| Up / Down | Same as Select |

Pings accelerate as you near a sub. When the HUD says **LOCK — FIRE!**, shoot.

You have limited torpedoes. Clear every sub to advance waves. Misses cost ammo; empty rack ends the run.

## Feedback

Open **Sonar Sub Hunt → Settings** in the Pebble app (Clay config page):

- **Both** — speaker chirps + vibration (default)
- **Sound** — speaker only
- **Vibrate** — vibration only

Watches without a speaker still vibrate when Both is selected.

## Touch

Tap to fire where supported; Select / Up / Down always work as fallback.

## Build

On Windows (Podman or Docker):

```powershell
./build.ps1
```

Output: `build/SonarSubHunt.pbw`

```powershell
./build.ps1 -Phone 192.168.1.23
./build.ps1 -Zip
```

CloudPebble import zip: `dist/SonarSubHunt-cloudpebble.zip`  
Store listing copy + screenshots: see [`STORE.md`](STORE.md)

```powershell
python tools/generate_store_screenshots.py
```

## Layout

```
src/c/     game, aim, sonar FX, settings, UI
src/pkjs/  Clay config (config.js) + phone JS
resources/ menu icon + sprites
store/     Rebble screenshots
```
