# Third-party materials & outstanding checks

This inventory records dependencies and provenance clues identified during cleanup. It is not a completed rights audit. Project manifests and lockfiles are the reference for dependency versions.

| Project | External materials | Treatment / outstanding checks |
|---|---|---|
| Music Separation | Demucs, librosa, Basic Pitch, music21, pedalboard, Gemini, score tools | Dependency files retained. Audio, generated results, and model weights excluded. Check package and model terms before redistribution. |
| Radio | Broadcast streams, track metadata services, app icons | No broadcast recordings included. Stream usage terms need review. Icon images withheld pending provenance checks. |
| Dev Desk | Apple frameworks, local tool records | Personal data excluded; framework binaries not redistributed. |
| Crisis Indicators | Google GenAI, React, Recharts, external banner | Dependencies retained; historical README banner removed. Model output is not verified statistical data. |
| X Cleaner | X API, Python and web dependencies | Actual settings and job records excluded. Clearly fake test tokens retained to preserve test behavior. |
| Mobility Guide | OpenStreetMap / Geofabrik, Seoul terrain data | Existing ODbL/source references preserved in historical documentation. Large source datasets, transformed walking networks, and screenshots excluded. |
| My Inconveniences | Bubble Tea, Lip Gloss, Go dependencies | go.mod/go.sum retained; dependency source trees not copied. |
| Timetable Wizard | React, Lucide, Vite, other dependencies | Existing Kim Kangwoo MIT LICENSE preserved; dependency source trees not copied. |
| Game experiments | raylib, embedded fonts with NeoDunggeunmo provenance clues, system-font-derived bitmaps | Both font_data.h files withheld from the public tree. Verify provenance and permissions, or replace and regenerate before restoring. |
| Portfolio badges | Shields.io-hosted badge images and technology logos | Loaded remotely by the README. Logos identify the technologies; no ownership or endorsement is claimed. |

## Outstanding work

- **Game fonts:** The first generator references `neodgm.ttf`; the second uses macOS system fonts. Establish the provenance of the generated data and use fonts suitable for redistribution. Missing headers currently prevent building the game snapshots as-is.
- **Radio icons and Mobility Guide screenshots:** Withheld pending creation/source verification. Radio asset manifests remain, so icons need restoration or replacement before building.
- **First-party licensing:** Apart from the existing MIT-covered project, license selection remains open.

A service URL in source code does not establish permission to redistribute its content. This repository is a source portfolio with some materials excluded, not a complete runnable distribution.

## Codex activity graphic

The activity colors were sampled from a screenshot supplied by the repository owner. No underlying token totals, account credentials, or API data are included. The SVG layout, animation, and generator were created for this portfolio. It animates once and respects reduced-motion preferences; a separate static SVG is provided.

Design inspiration: [Avi Vashishta's profile](https://github.com/AVIVASHISHTA29/AVIVASHISHTA29) and [animated README article](https://www.avivashishta.com/blog/build-animated-github-profile-readme). The terminal/SVG presentation approach was referenced; their source code and artwork were not copied.
