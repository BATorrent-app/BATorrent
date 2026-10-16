# Third-party assets

## Fonts

The interface typeface is **[IBM Plex Sans](https://github.com/IBM/plex)** (Regular,
Medium, SemiBold, Bold), © 2017 IBM Corp., licensed under the **SIL Open Font License
1.1** — full text in `src/fonts/IBMPlexSans-LICENSE.txt`. It is bundled and registered
at startup so the UI has the same metrics on macOS, Windows and Linux.

The brand wordmark uses **New Rocker** (SIL OFL 1.1) — `src/fonts/NewRocker-OFL.txt`.

The themes bring their own title and interface faces, all from Google Fonts under the
**SIL Open Font License 1.1** (licence text next to each in `src/fonts/`):
**Barlow Condensed** (Cinema), **Newsreader** (Light), **Instrument Serif** (Midnight),
**Shippori Mincho** and **Zen Kaku Gothic New** (Sakura), **Space Grotesk** (Dark Star) and
**IBM Plex Mono** (Matrix). Newsreader and Space Grotesk are variable fonts instanced to
fixed weights; Shippori Mincho and Zen Kaku Gothic New are subset to Latin plus the few
kanji the Sakura theme draws. The OFL allows both as long as the fonts are not sold on
their own.

## Theme art

The background art of the themes (`src/images/themes/`) is public domain or was drawn
for the project:

- Cinema: Georges Méliès, drawing for *Le Voyage dans la Lune* (1902), via Wikimedia
  Commons, converted to black and white.
- Light: Ernst Haeckel, *Kunstformen der Natur* (1904), plate Discomedusae, via
  Wikimedia Commons, converted to sepia.
- Midnight: Vincent van Gogh, *Starry Night Over the Rhône* (1888), via Wikimedia Commons.
- Dark Star: NASA, ESA, N. Smith (UC Berkeley) and the Hubble Heritage Team, Carina Nebula
  (NGC 3372), public domain, recoloured as a violet duotone.
- Sakura (the cherry branch), Matrix (the character rain), the paper grain and the
  scanlines were made for the project.

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

## The in-app VPN

`batwg` (`tools/batwg`) is built from **[wireguard-go](https://git.zx2c4.com/wireguard-go)**
(MIT, © Jason A. Donenfeld and WireGuard LLC), whose userspace network stack is
**[gVisor](https://github.com/google/gvisor)**'s netstack (Apache License 2.0, © The gVisor
Authors), with the Go extended libraries `golang.org/x/crypto`, `x/net`, `x/sys` and `x/time`
(BSD 3-Clause, © The Go Authors) and `github.com/google/btree` (Apache License 2.0). The exact
versions are pinned in `tools/batwg/go.sum`. "WireGuard" is a registered trademark of Jason A.
Donenfeld; BATorrent is not affiliated with or endorsed by the WireGuard project.
