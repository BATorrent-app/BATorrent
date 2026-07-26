# Third-party assets

## Fonts

The interface typeface is **[IBM Plex Sans](https://github.com/IBM/plex)** (Regular,
Medium, SemiBold, Bold), © 2017 IBM Corp., licensed under the **SIL Open Font License
1.1** — full text in `src/fonts/IBMPlexSans-LICENSE.txt`. It is bundled and registered
at startup so the UI has the same metrics on macOS, Windows and Linux.

The brand wordmark uses **New Rocker** (SIL OFL 1.1) — `src/fonts/NewRocker-OFL.txt`.

**Inter** (SIL OFL 1.1) was the interface typeface until 2026-07-26 and its files are
still in the tree; remove them once the Plex switch has been through a full visual pass.

## Icons

The interface icons in `src/icons/` come from **[Tabler Icons](https://tabler.io/icons)**,
licensed **MIT**, © Paweł Kuna.

They are used with one modification: the stroke weight is normalized from Tabler's
default `2` to `1.5`, and the stroke colour is set to the app's neutral so the SVG
reads correctly when opened on its own. Colour at runtime comes from `IconImg`'s
tint, not from the file.

`turtle.svg` (the alternative-speed toggle) is adapted from
**[Lucide](https://lucide.dev)**, licensed **ISC**, © Lucide Contributors — Tabler has
no equivalent.

MIT and ISC both require the copyright notice to be retained; each SVG carries its
attribution in a comment at the top of the file, and the notices are reproduced here.

```
MIT License — Copyright (c) 2020-2024 Paweł Kuna (Tabler Icons)
ISC License — Copyright (c) 2022 Lucide Contributors
```

Full licence texts: https://github.com/tabler/tabler-icons/blob/master/LICENSE
and https://github.com/lucide-icons/lucide/blob/main/LICENSE
