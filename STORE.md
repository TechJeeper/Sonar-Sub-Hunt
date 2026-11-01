# Rebble Appstore listing — Sonar Sub Hunt

Copy these fields into [dev-portal.rebble.io](https://dev-portal.rebble.io/submit).

| Field | Value |
| --- | --- |
| App name | Sonar Sub Hunt |
| Type | Watch app |
| Category | Games |
| Website | *(optional)* |
| Source code | https://github.com/TechJeeper/Sonar-Sub-Hunt |
| `.pbw` | `build/SonarSubHunt.pbw` |

## Description

Paste into each platform asset collection (max 1600 characters):

```
Sonar Sub Hunt turns your wrist into a sonar scope.

Sweep your arm around the room — the watch uses the compass and accelerometer to aim an invisible sonar cone. Auditory pings (speaker or vibration) get faster and higher as you lock onto hidden enemy subs. When the HUD says LOCK — FIRE!, tap the screen or press Select to launch a torpedo.

You have a limited rack of torpedoes. Clear every sub to advance waves. Misses cost ammo; an empty rack ends the run.

• Aim with compass heading + wrist tilt
• Ping feedback accelerates as you near a contact
• Tight lock cone before a hit registers
• Wave progression with score and torpedo count
• Tap to fire on touch watches (Select / Up / Down always work)
• Feedback mode: Both, Sound, or Vibrate (Clay settings)

Works on Pebble Time 2, Pebble Time, Pebble 2 / Time Round-class, and Pebble 2 Duo. Watches without a speaker still vibrate when Both is selected.
```

*(~820 characters)*

## Release notes (v1.0.0)

```
Initial release — sweep, lock, fire. Hunt hidden subs with your wrist.
```

## Screenshots

Exact display sizes (no bezel). Regenerate with:

```powershell
python tools/generate_store_screenshots.py
```

| Platform | Size | Folder |
| --- | --- | --- |
| basalt (Pebble Time) | 144×168 | `store/screenshots/basalt/` |
| diorite (Pebble 2) | 144×168 | `store/screenshots/diorite/` |
| flint (Pebble 2 Duo) | 144×168 | `store/screenshots/flint/` |
| emery (Pebble Time 2) | 200×228 | `store/screenshots/emery/` |

Scenes (5 each): title, hunting ping, LOCK — FIRE!, hit, wave clear.

## Listing checklist

- [ ] Large + small app icons
- [x] Screenshots (5 per platform under `store/screenshots/`)
- [ ] Marketing banner (required for watch apps)
- [ ] Description above on each asset collection
- [ ] Publish `build/SonarSubHunt.pbw` as release 1.0.0
