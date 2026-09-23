# Changelog

## v4.8.0

### Added
- Point BATorrent at your own VPN. Set it up in Settings and only this app goes
  through it; if the tunnel drops, transfers stop instead of falling back to
  your normal connection. Split tunnel, auto-connect, and the binding rechecked
  over the tunnel's whole life instead of once at startup.
- Drop a WireGuard `.conf` on the VPN card to import it. Each profile shows the
  country it actually lands in, resolved from the endpoint rather than the name.
- The network interface picker names the tunnel BATorrent brought up, and marks
  the ones that look like tunnels, instead of listing identical-looking adapters.
- Direct downloads: any http link, segmented and resumable, with a fallback for
  servers that refuse range requests. File hosts included. Paste it, use Ctrl+D,
  or drop the link on the window.
- A setup wizard on first run: language and content language, theme, covers or
  list, and where the navigation and detail panel sit. Every answer applies as
  it is clicked. Reachable any time from Help, and shown once to everyone
  updating to 4.8, since most of these settings were never easy to find.
- Search warns about releases that want a password, are too small to be what
  they claim, or are cam rips, before the bandwidth is spent.
- More than one video plays at once, each in its own window.
- Start with the system, without the window opening minimised when it is
  launched by hand.
- macOS: double-clicking a `.torrent` opens BATorrent.
- Windows: a bigger tray icon with a status dot under the wing, green while
  bound to the VPN and red while not.
- Software replaces Apps as a category name, and Compressed joins the built-ins.

### Changed
- One progress bar for every state, same height and place: downloading in the
  accent, seeding amber, done green, paused grey. Missing files and storage
  errors get moving hazard stripes instead of a percentage, because a torrent
  that stopped at 62% still has a number and drawing it claims progress that is
  not happening.
- The detail panel keeps cover, title and progress on screen while the tabs
  change, in both the side inspector and the bottom deck. The inspector
  collapses to a rail, like the deck already could.
- New typeface, and one icon rule throughout: solid names a thing or a state,
  a stroke names an action. Every context-menu item has an icon now, and the
  toolbar's Copy wears a magnet, because it copies a magnet link.
- Free space moved next to the transfer figures; the navigation rail shows what
  is transferring in the space it used to leave empty.
- Notifications have a dismiss button. They always closed on click, but nothing
  said so.
- Find: "See all" opens that shelf as a grid instead of changing a filter chip.

### Fixed
- Brand-new magnets no longer appear as SEEDING. libtorrent reports "finished"
  for a torrent that has nothing to do yet, and that answer was trusted raw.
- Storage failures are their own state, separate from files that merely moved.
- Adding the same magnet twice no longer creates a second entry that fights the
  first for the same data.
- The category filter no longer hides everything when a category is selected.
- Expand details is disabled when nothing is selected, instead of looking broken.

---

## v4.8.0-beta7

Test build for the beta round. Still a beta, not a public release.

beta5 and beta6 never published. beta5 used a QML type that only exists in
Qt 6.8 while the build pins 6.7, so the packaged app failed to start; beta6
shipped the Windows bundle without the MSVC runtime, which would not launch on
a machine that has no redistributable installed. Both are fixed here, along
with three binding loops in the main window that ran on every boot.

### Changed
- The VPN is now a "light" model: instead of running its own tunnel, BATorrent
  binds to the VPN you already have connected, so your provider's app stays in
  charge and BATorrent simply refuses to move traffic outside it. The built-in
  WireGuard cockpit is hidden while that path is reworked.

### Added
- Language-aware content: the app's language now drives what surfaces first, so
  results and recommendations in your own language stop being buried.
- Games: "Get & Install" flow, plus fresher game sources.
- Search warns about a release that looks bad before you download it.
- Accessibility: text contrast now passes AA, and controls are reachable by a
  screen reader.

### Fixed
- Queue positions show each item's real number instead of the same label on all.
- Categories survive a restart.
- RSS: downloads that silently never started, and a list that stayed empty until
  the next check.
- Posters and covers that went missing.
- One consistent icon set and typeface across the interface.
- Transfer direction uses one red and one amber instead of two of each.
- Startup robustness on Windows, and a set of crash fixes under the hood.

---

## v4.8.0-beta3

Second 4.8 test build, from the beta tester round. Still a beta for testing.

### Fixed
- A torrent could flip between "seeding" and "downloading" forever, or show
  "SEEDING" at 0 bytes; the state now reflects what's actually verified on disk.
- File-type labels (PDF/EXE/ZIP) no longer appear on movie/media posters.
- Integrated VPN robustness (Windows): the app no longer reports "protected"
  before the tunnel actually carries traffic; a tunnel that fails to connect is
  torn down automatically so it can't leave you offline; and a leftover tunnel
  from a previous run is cleared before connecting, instead of wedging the
  connection with "The object already exists".
- Several crash/robustness fixes under the hood.

### Added
- Rename imported VPN profiles (e.g. "DK, Copenhagen").

## v4.8.0-beta1

First 4.8 test build. The theme is the privacy layer (integrated VPN) plus the
tester-round polish. Not a public release, just a beta for testing.

### Added
- Integrated WireGuard VPN. Import your provider's `.conf` (IVPN, Mullvad,
  Proton…) and connect from inside BATorrent: a status pill at the top bar and
  a full cockpit in Settings (grey = off, amber = connecting, green = connected).
  The connected server's endpoint is shown next to the profile name.
- Split tunnel: route only BATorrent's traffic through the VPN, leaving the rest
  of the system on its normal connection.
- Connect on launch: reconnect the last-used profile automatically at startup.
- HTTP / file-host downloads, and a manual "Download from a link" (Ctrl+D).
  Link downloads carry a file-type label (PDF, EXE, ZIP, JPG…).
- "Block known bad peers" (Settings > IP filtering): keeps a reputable IP
  blocklist updated and drops flagged addresses before the handshake, aimed at
  the antivirus warnings some downloads trigger.
- Missing files are detected: a manually deleted download shows a clear red
  "Files missing" state with a guided recovery banner instead of sitting idle.
- The status filters (All, Active, … Queued, Completed) now scroll with a hover
  arrow when the window is too narrow to show them all, instead of clipping.

### Fixed
- The seeding badge is gold (matching the up-arrow); green now means only DONE.
- A queued item gets its own QUEUE badge; no more badge AND duplicate text
  under the poster; the space now shows the release year and genres from TMDB.
- Refresh repaints speed/ETA/status immediately, so you can tell it did something.
- Several crash and robustness fixes: guarded torrent-handle operations that
  could close the app, a subtitle-download memory cap, and a fix so a completed
  download with a temp folder actually moves to its final location.

### Changed
- "Watch in player" is now just "Watch".

## v4.7.0 "Cinema"

### Added
- The built-in player was reworked. Audio, subtitles and speed are in one panel
  instead of three menus, the seek bar shows a frame preview on hover, a
  next-episode card counts down near the end, intros and credits get a skip
  button when the file has chapters, and the video's colors spill into the
  black bars (can be turned off in Settings).
- Player controls are centered, and volume is a slider that slides out on hover.
- Downloads waiting for a queue slot have a "Queued" status and filter.
- Favorite folders: the add dialogs list your recent save locations with their
  free space, so a full default disk is one click from a better one.
- The disk gauge in the top bar cycles through every drive, not just the main one.
- Windows: separate toggles for .torrent, magnet: and bittorrent: associations,
  and a Refresh button in the toolbar.

### Fixed
- Linux: the AppImage crashed at launch with an "undefined symbol" error on every
  machine, because it shipped the system libtorrent instead of BATorrent's own.
  A build check now catches this (#32).
- Magnets: the Add dialog no longer opens twice after drag and drop, the paste
  dialog no longer closes before you can start the download, and a magnet from
  the browser no longer shrinks a maximized window.
- "Remove with files" finishes deleting even if you quit right after, and a
  removed torrent no longer comes back from the watched folder.
- Detail panel values no longer spill past the edge. The panel can sit at the
  bottom in grid view (Settings > Appearance).
- Windows: peer countries show again, and the window no longer opens wider than
  a scaled screen.

### Changed
- Settings save as you go and say so: a change flashes "Saved" and the button
  reads "Done".
- A finished download's card is quieter. The DONE badge is enough, so the long
  status text is gone.
- Torrents with no artwork (an Ubuntu ISO, a code archive) no longer show a
  placeholder cover in the details.

## v4.6.0 "Signal"

### Added
- Magnets get a set of well-known open trackers automatically, so fetching
  metadata doesn't depend on DHT alone (Settings > Network > Protocol turns it
  off). They are removed, along with DHT, PEX and local discovery, as soon as a
  torrent turns out to be private.
- Download cards show how much is actually downloaded ("107 MB of 6.4 GB") next
  to the percentage.
- With a download limit set, waiting torrents read "In queue (#3)" instead of
  "Paused", and start on their own when a slot frees up.
- A "Peers found, connecting…" status between searching and downloading.
- An amber pulse along the poster's bottom edge while a torrent is seeding.
- Every action in the right-click menu has an icon, the labels are clearer
  ("Pause download", "Resume download"), and the menu animates open.

### Fixed
- Magnets that never fetched or downloaded slowly. The DHT now bootstraps from
  several routers (one blocked host used to mean no DHT at all), and every
  tracker tier is announced to at once, like other clients do.
- A magnet added moments before a crash no longer disappears; it is saved as
  soon as it is added.
- "Remove with files" sticks. Quitting right after removing used to leave the
  data on disk; pending deletions now finish on the next launch.
- Removed torrents could come back if a .torrent with the same name had ever
  gone through the watched folder.
- Pausing a torrent whose engine handle had expired could close the app.
- The Browse button in the add-torrent dialog did nothing.
- Windows: peer countries show again (as country codes, since Windows has no
  emoji flags), and the window no longer opens wider than a scaled screen.
- Game covers resolve for long subtitled titles ("Garfield Kart 2 All You Can
  Drift") by retrying with a shorter search.
- Listening on IPv6 works again with a custom port.

### Changed
- First run picks a random high listen port instead of 6881, which ISPs often
  throttle and which collided with other torrent clients on the same machine.
  Existing installs that never chose a port migrate once.
- Less color: the DONE badge and hover play buttons are dark glass, a finished
  download's progress bar is neutral, and transfer numbers are only red or amber
  while data is moving.

## v4.5.1 "Find"

### Added
- Navigation moved to a bar at the top: Downloads, Find and HUB as tabs, with the
  disk gauge and the active-download chip next to them. The old left sidebar is
  still in Settings > Appearance > "Classic side navigation".
- Find: Search and Discover are one page. Browse the featured banner and the
  poster shelves (movies, series, games, your list), or start typing to search.
  Clearing the search takes you back to where you were browsing.
- "Mark as completed" is in the main right-click menu once a download finishes,
  and reaching a seeding limit (ratio or time) marks the torrent completed
  instead of leaving it paused.
- Setting a game executable shows a confirmation, and the game can be played
  right away, even while the torrent is still finishing other files.

### Fixed
- Games: Play always responds. A finished game that was still seeding counted as
  downloading, so Play did nothing. Launch failures show a message and open the
  folder, executables that need admin rights get the UAC prompt on Windows, and
  .app bundles open correctly on macOS.
- Windows: mouse-wheel scrolling was very slow; it now moves a normal amount.
- Windows: the tray menu opens where you click.
- An interrupted update that leaves a mismatched engine DLL (the most common
  crash we saw) shows a re-download dialog instead of crashing in a loop, and
  after any crashed start the next launch begins with a fresh engine state.
- Light theme: the game card button label is readable, the HUB continue rails
  no longer render as black boxes, and the grid hover shadow no longer smudges.
- Grid cards: a long status no longer overlaps the size column. Empty states in
  Peers, Files and Trackers are centered.

### Changed
- The top bar shows just the bat glyph (the full wordmark stays on the splash,
  About and the classic layout), and the grid selection ring is red.

## v4.4.1

Hotfix for v4.4.0, which didn't launch on Windows.

### Fixed
- Windows: the app launches again. v4.4.0 used a QML shadow effect
  (`RectangularShadow`) that didn't resolve on Windows, so the UI failed to load
  and the app quit without an error. It now uses the same shadow as the rest of
  the app.

## v4.4.0 "Dublado"

### Added
- With "Prefer my language" on (the default), the bundled stream source is asked
  for releases in your language, dubbed ones included, so they are listed first.
  With the app in Portuguese, dual-audio releases now show up at the top instead
  of around position 19.
- Source manager: a "Sources" button in search lists every torrent source with a
  switch, plus a catalog grouped by region: Russian/CIS trackers (RuTor, Kinozal,
  NNM-Club via TorAPI), a Brazilian preset (Comando, BluDV and others via
  torrent-indexer), and Jackett for everything else.
- BitSearch is on by default. It aggregates TPB, 1337x, YTS and nyaa in many
  languages, so non-English releases show up without setup.
- Series are grouped by season, with an episode picker and tabs for complete
  and season packs, instead of one flat list of 400 rows. Stremio series resolve
  per episode.
- The detail drawer shows a strip of screenshots: backdrops for movies and
  series, in-game stills for games.
- A Dubbed / Subtitled / Original filter at the top of search results.
- Free up space from inside the app: clicking the disk bar opens a list of every
  torrent, sortable by size or age, with delete buttons. When a download won't
  fit, the warning links straight to it with the amount you're short. Adding a
  `.torrent` that won't fit now warns even with "always use default path" on.

### Changed
- Two view modes instead of three: Grid and Classic. Classic replaces List and is
  now a proper table with seeds, peers, ratio (red under 1.0, green above), ETA,
  release names in monospace, and sorting on every column.

### Fixed
- Progress no longer shows 100% early. A torrent at 99.95% shows 99.9% until it
  finishes, because every progress display rounds down now.
- The what's-new screen kept losing its note when a hotfix replaced a release
  (4.3.1 replacing 4.3.0). Notes now belong to the release line.
- The player shows a spinner when it runs out of buffer. Qt's own stall detection
  rarely fires for a file that is still growing, so playback used to just freeze.
- Two crashes: one when a torrent's selection went stale (removed while
  selected), and one at shutdown when a late log message reached the already
  closed log file.

### Security
- The Web UI password is stored as salted PBKDF2 (100k iterations) instead of
  unsalted SHA-256. Existing setups upgrade on the next login.

## v4.3.1

Hotfix for v4.3.0, which didn't launch on Windows.

### Fixed
- Windows: the app launches again. v4.3.0 shipped a libtorrent DLL that didn't
  match the one the app was built against, so Windows refused to start it with
  "procedure entry point not found". (#20)

### Internal
- Review of the IPC layer, network services, Web UI and QML views: the IPC frame
  parser rejects malformed input, every network client has a timeout,
  release-name parsing is faster, and a few QML property bugs are fixed.

## v4.3.0 "Continue"

The first release since v4.1.0. Continue watching, a command palette, automatic
subtitles, swarm health in search, and a lot of fixes.

### Added
- The HUB opens on your last movie and last game as large cards with progress,
  time left, hours played and a Resume button.
- Command palette: Ctrl/⌘+K anywhere to jump to a torrent by name, pause or
  resume everything, toggle alt speed, or open any page, window or Settings
  section.
- The player has a title bar with the file's quality and audio, a two-row
  control bar, a buffer indicator, and resume that works for streamed torrents
  (it used to restart them from zero).
- Automatic subtitles for movies and series, no account needed, with a language
  picker. Manual loading and ±0.5 s sync are still there, and you can add your
  own OpenSubtitles account for a bigger quota.
- A language filter on movie and series results, with a language badge and a
  `DUB` marker on each result.
- A "Play now" button when a movie finishes downloading.
- Discover and Search show real source health: the featured title shows
  "N sources, best X GB, seeds", and the torrent list has a green/amber/red seed
  bar.
- Discover filter for All / Games / Movies / Series. The banner always has art,
  even for games with no backdrop.
- Search shows the best match as its own banner instead of the first grid item.
- Hovering a stalled torrent says why: no peers yet, no seeds connected, peers
  not uploading, or the actual error.
- macOS: the native title bar is gone and the window controls sit inside the app.
  Windows and Linux keep native title bars for now.
- "Remove with files" moves data to the system trash. "Delete permanently" is
  there for when you're low on disk.
- Free disk space in the status bar.

### Fixed
- The window reopens at the size you left it.
- The listen port field no longer flips back to the old value after a change (it
  read the socket before libtorrent had re-bound).
- Paused torrents stay paused across restarts.
- The hidden `.parts` file is cleaned up when files are removed.
- The Downloads search box no longer stays red after one click, and Ctrl/⌘+K
  opens ready to type.
- The repack filter opens on the first click.
- A movie still downloading (files ending in `.!bt`) no longer offers "Install"
  as if it were a game.
- Games with the `.exe` in a subfolder are detected again, and Play opens the
  game instead of a 30-second intro clip it found first.
- Mouse-wheel scrolling moves a sensible amount on Discover, HUB, Search and
  Settings.
- Windows: the tray menu is anchored to the tray icon again.
- The nav rail's activity card no longer disappears when downloads are paused.
- Source and category names, result counts, status text, port diagnostics and
  the nav rail no longer show Portuguese in other languages.
- Duplicate search results are merged by infohash across providers.
- HTML entities in result titles (`&ndash;` and others) render as characters.
- Settings labels no longer show `&` accelerator marks ("&Export Settings").
- The Peers tab's country column no longer overlaps the IP header.
- Update check errors no longer show a dialog during the silent startup check.
- The new subtitle, language search and disk strings are translated in all eight
  languages, the HUB empty state shows its text again, and a diagnostics line
  that showed a raw `detail_seeds` key reads "Seeds".

### Changed
- Search warns when results won't fit in your free space, and adding a torrent
  that's too big asks first ("needs X, Y free, add anyway?").
- "Get best" is no longer red, so it doesn't compete with the Search button.
- The Discover banner rotates, with clickable dots.
- Esc closes every dialog and window and Enter confirms, including stacked
  dialogs.
- Focus rings on every control and press feedback on every button.
- Completed torrents are green and seeding ones amber, so a finished torrent no
  longer looks like an error.
- The anime accent art is a faint watermark in list view so row text stays
  readable. The grid keeps it bold.
- Search results without covers get generated placeholders instead of the logo.
- The Statistics window has a live speed graph, counts per state, and all-time
  and session columns.
- RSS and search have proper empty states.
- Discover placeholders follow the theme while loading, and search shows a
  spinner.
- The About dialog links to GitHub, releases and the privacy policy.

### Internal
- Crash reports with symbolicated stacks, and after a crash the app offers a
  pre-filled GitHub issue with the end of the log.
- A small local history of daily usage (bytes, torrents added and completed per
  category) for future statistics. It never leaves the machine.
- Shared menu and dialog components, and tests for the stats history.

## v4.1.0 "Parity"

Mostly fixes from user reports, plus the qBittorrent features people asked for.

### Fixed
- Settings save. Speed limits (normal and alternative), max active downloads,
  seed ratio, listen port, max connections, DHT, uTP, encryption, VPN interface,
  kill switch and proxy were reset to defaults on every launch. Thanks to
  everyone who reported "the upload limit always goes back to 0".
- Windows: turning off the splash or "close to tray" no longer reverts. The
  registry stores these as integers and the UI was reading them wrong.
- The first time the window hides to the tray, a notification says so, since
  Windows 11 puts new tray icons in the overflow.
- macOS: the Dock icon is the same size as other apps' icons.
- The "Torrent file / All files" filter in the open dialog follows the app
  language.

### Added
- A turtle button in the toolbar toggles the alternative speed limits,
  independent of the scheduler.
- Follow the system light/dark theme (Settings > Appearance).
- Pre-allocate disk space to reduce fragmentation (Settings > Downloads).
- Optionally force a hash check when adding a torrent, so existing or partial
  files on disk are picked up.
- A green/amber/red dot in the status bar shows whether the listen port looks
  reachable (UPnP/NAT-PMP and listen state, checked locally).
- File > Add torrent from URL (Ctrl+U) downloads a remote `.torrent` and opens the
  normal add dialog.
- Right-click > Export .torrent.
- The watch folder (Settings > Files) already existed; it's easier to find now.

### Internal
- Tests for the settings persistence and Windows boolean bugs.
- A Qt Quick Test harness for the splash and the shared widgets.

## v4.0.0 "Hub"

BATorrent becomes a media hub: find something, download it, then watch or play
it, all built around cover art. A collapsible left nav rail switches pages.

### Added
- A welcome / what's new screen on first install and after each update, with a
  note from me, that version's highlights and a link to the full release notes.
  A broken auto-update once left no way to reach users between releases.
- A guided tour of the nav rail, adding a torrent, Discover, Search, HUB and
  Settings. It runs once after the first welcome screen, can be skipped, and is
  in Help > Interactive tutorial.
- App icon picker (Settings > Appearance): pick the Dock/taskbar icon separately
  from the UI theme. The file manager icon comes from the signed bundle and
  doesn't change. Icon pack by @dkindratyuk-web (#15).
- Discover: trending and popular movies, series and games from TMDB and IGDB.
  Clicking one searches for it.
- Search finds the title first (*God of War Ragnarök*) from a poster grid, one
  cover per title, and then lists its downloads with filters for quality,
  source, repacker, provider and seeders. Relevance sorting matches whole words.
- Each result shows where it came from (RuTracker, Torrents, …). A "raw results"
  view is always available.
- A built-in player with resume and a watched bar on each poster. You can stream
  while the torrent is still downloading.
- HUB: Continue watching and Continue playing rails, then your movie and game
  libraries.
- Games launch from the HUB. The executable is detected (or you set it once),
  with Install and Open folder actions.

### Fixed
- The auto-updater checks the installer's size before running it, so a truncated
  download can't break the install. If startup fails twice, safe mode offers to
  reset settings or get the latest version, and the update dialog always has a
  "Download manually" link.
- The Peers tab no longer lags on large swarms: failed GeoIP lookups are cached
  and the list only refreshes while the tab is open.
- A torrent you had streamed no longer announces "download complete" on every
  launch.
- Translated into all 8 languages.

## v3.0.4

### Fixed
- The app launches on a clean Windows install. The Visual C++ runtime was
  missing; the MSVC runtime DLLs now ship with the installer, the portable build
  and winget.
- macOS: the Dock icon no longer looks transparent and can be customized. The
  app stopped overriding the Dock tile at runtime. (#14)
- The bat is bigger inside the macOS app icon.
- Smart Paste decodes `thunder://` links again: paste a Xunlei link and the
  magnet or torrent behind it is added.
- Game search matches across words and labels the repacker. "god of war fitgirl"
  finds FitGirl's repack, "fitgirl" lists all of theirs, and results show FitGirl,
  DODI, RUNE, TENOKE and so on.
- Streaming while downloading opens the right file even with the `.!bt` suffix,
  prefers VLC, mpv or IINA (which can play a file that's still growing) and falls
  back to the default player, and stops waiting if the torrent has no seeders.

## v3.0.3

### Added
- An "All" search that queries every source at once, game catalogs and torrent
  indexers, and merges the results. Game search reads Hydra-format community
  catalogs (a default is added on first run and can be removed), with cover art,
  clean titles and cached catalogs.
- Ukrainian, for eight UI languages.
- README with demo GIFs and screenshots in each language.

### Fixed
- Adding a game shows the right name and cover right away. The matcher uses the
  file list, strips editions (`Early Access`, `Complete Edition`, `GOTY`),
  handles apostrophes (`Baldur's` = `Baldurs`), roman numerals (`GTA V` =
  `GTA 5`) and Cyrillic titles, and checks the API result instead of guessing.
- A completed torrent could start downloading again when the `.!bt` mapping got
  out of sync with the disk. It now checks what's on disk and repairs itself on
  launch.
- Added torrents disappeared on restart unless they had downloaded something.
  They are saved as soon as they are added.
- The Peers tab no longer freezes on large swarms (9k+ peers).
- Windows: the welcome dialog no longer comes back after "don't show again".
- About: the Ukrainian flag renders, and Donate opens GitHub Sponsors.

## v3.0.2

### Added
- The WebUI was reskinned to match the desktop app: same palette, font, flat
  surfaces, the real logo, and a proper magnet icon.
- Phone pairing without typing: the generated WebUI password can be copied, and
  the QR code carries the credentials, so scanning it logs straight in. The
  credentials are removed from the address bar afterwards.
- Two search providers: RuTor (through a public TorAPI relay, no login) and
  Torrents-CSV.
- Per-file priority again: right-click a file in the detail panel to set Skip,
  Low, Normal or High.
- Rename a single file inside a torrent (double-click or the file menu).
- Remove a tracker with the ✕ on its row.
- Smart Paste on Ctrl+V: a magnet, a 40-character info hash or a `.torrent` URL
  is added right away. Text fields still paste text.

### Changed
- Search results are sorted by seeders, and each provider times out after 15 s
  so a dead one can't hang the UI.
- Anime fansub names (`[Group] Title - NN`) resolve to the right show, and audio
  layouts in titles (`DDP5.1`, `7.1`) no longer affect cover matching.

### Internal
- The old QWidget interface is gone. QML had been the only UI since 3.0.0 (the
  old code was behind a hidden `--legacy` flag), so the whole QWidget layer was
  removed, about 13,400 lines. The per-file priority, file rename, tracker
  removal and Smart Paste items above already existed in the backend but had
  never been wired to QML.
- macOS: the WebUI password hash moved from the keychain to app settings, so
  unsigned builds no longer ask for the login keychain password at launch. The
  password itself stays in the keychain.
- About 400 unused translation strings and some dead code removed, and an
  `ARCHITECTURE.md` added for contributors.

---

## v3.0.1

### Fixed
- Windows and Linux: the menu bar is back (File, Torrent, Settings, Help). It only
  rendered as a macOS global menu; it now draws inside the window, and macOS keeps
  the native menu.
- Title parsing strips release-site prefixes (`www.foo.com - `,
  `[ tracker.net ] - `) and uses only the show name before SxxExx, so
  `www.UIndex.org - Euphoria US S03E08 in God We trust` resolves to Euphoria.
- Tiles no longer show a blank label before the cover resolves; they use the
  parsed title, then the raw name. List mode matches grid.
- Episode tiles show SxxExx.
- Finished, error, kill-switch and RSS events show as OS notifications again,
  not only in-app toasts.

### Added
- Fix a wrong cover from the right-click menu: link the torrent to the right
  movie, series or game, or choose "No cover". Auto-matching never overwrites it.

### Changed
- The portable Windows download is named
  `BATorrent-windows-x86_64-portable.zip`, so the installer is the obvious choice.

---

## v3.0.0

### Changed
- The UI was rewritten in Qt Quick / QML, replacing QWidget. Every screen was
  ported: main window, settings, add and create torrent, search, RSS, statistics,
  diagnostics, inspector, log viewer, pairing, shortcuts, removed history,
  welcome, about and release notes.
- Live speed graph, working detail tabs (general, peers, files, trackers,
  pieces), drag and drop, native menu bar and right-click menus.
- Multi-select, column sorting, animated grid reordering, full-row hover and
  arrow-key navigation.
- A poster grid with TMDB and IGDB metadata and localized synopses.

### Added
- Custom themes: create, rename and delete palettes (background, panel, text and
  three accents), each with an optional background image and opacity.
- Five built-in themes. Midnight is blue instead of purple.
- The logo follows the OS color scheme, so it stays visible on light Windows
  taskbars.
- A startup animation that draws the bat outline, fills it and fades in the
  wordmark, with a toggle in Settings.
- System tray with click to restore, a menu (speed, pause/resume all, quit) and a
  popup with live counts and speeds.
- Desktop notifications for finished, error, kill-switch and RSS events.
- Discord Rich Presence.
- Peer country flags from GeoIP.
- Every window translated into 7 languages, with live switching.

### Fixed
- The auto-updater works again.
- The welcome dialog shows on first launch again.
- About and Release Notes read the real app version, library versions and
  changelog instead of hardcoded text.
- The "Active" filter and its count agree: both mean actually transferring, so
  idle seeders are no longer active.
- Dropping several .torrent files opens the add dialog for each in turn.
- Duplicate torrents are rejected on add, and clicking empty space deselects.
- Windows: "Open containing folder" opens the parent and selects the file, using
  the shell API like qBittorrent, instead of opening Documents with nothing
  selected.
- Windows: cover art loads (fixed `file:` URL handling).
- Windows: selecting a torrent no longer freezes the UI.
- Windows: fonts render like on macOS.
- Windows: faster startup (windows load lazily) and a sharper splash.
- Status colors: completed green, seeding amber, paused grey.

---

## v2.6.1

### Critical fix
- **Auto-updater broken since v2.5.0**: the "Check for updates" button and silent startup check were both failing silently due to accumulated signal connections. Fixed by disconnecting stale handlers before each check.
- Added redirect policy and 15-second timeout to update API requests
- Users on v2.5.0 through v2.6.0 must update manually this one time: the updater will work correctly from v2.6.1 onward

---

## v2.6.0

### Search plugin system
- **Multiple search providers** with configurable URL templates and JSON response mapping
- **Built-in providers:** The Pirate Bay (apibay), Nyaa.si: ready to use out of the box
- **Custom providers:** define your own URL template, JSON array path, and field mappings (name, hash, size, seeders, leechers)
- Provider selector in the search dialog alongside the existing Stremio source

### Translation system rewrite
- Migrated 683+ translation keys × 7 languages from hardcoded C++ to JSON files
- `translator.cpp` reduced from 5,615 lines to 62 lines
- JSON files in `translations/` directory, loaded via Qt resources at runtime
- Translators can now contribute by editing JSON: no C++ knowledge required
- `tr_()` shortcut and English fallback work exactly as before

### Category temp paths
- Assigning a category with a save path to a downloading torrent automatically updates the download destination
- With temp path active: updates the intended final path (auto-moves on completion)
- Without temp path: calls `move_storage` immediately to the category's save path

### Release workflow
- `CHANGELOG.md` as the source of truth for GitHub Release descriptions
- Release job automatically extracts the version-specific section on tag push
- All existing releases (v1.3 through v2.5.3) retroactively received proper descriptions

---

## v2.5.3

### New features
- **Temp download path**: download to a staging folder first, auto-move to the save path on completion. Keeps media servers (Plex, Jellyfin, Sonarr) from scanning partial files.
- **Content layout options**: Original / Create subfolder / No subfolder controls how multi-file torrents are laid out on disk.
- **Excluded file patterns**: regex rules (semicolon-separated) to auto-skip files when adding a torrent. Common patterns: `\.nfo$`, `\.txt$`, `sample`.

### Improvements
- Advanced settings tab fully translated (42 keys × 7 languages)
- Run on Complete and Watched Folder labels/tooltips translated
- User-agent now uses dynamic APP_VERSION instead of hardcoded string
- Peer fingerprint updated to match current version

### Fixes
- CI: release job now has `actions/checkout` + `actions:write` permission for post-release triggers
- Post-release MSIX and Homebrew triggers use `continue-on-error` to prevent marking the release as failed

---

## v2.5.2

### Stability (from qBittorrent code analysis)
- Try-catch around the entire `processAlerts` loop body: a single bad alert no longer crashes the app
- `active_checking=1`: only one torrent rechecks at a time (prevents OOM on 96GB+ torrents)
- `checking_mem_usage=512`: explicit memory budget for piece checking (8MB)
- Cache invalidation in `forceRecheck()`: root cause of the 96GB recheck crash
- `alert_queue_size=1000000`: generous queue so disk-full storms don't silently drop alerts
- Crash loop guard: `startupInProgress` flag in QSettings; skips resume data on crash-during-boot
- Rate-limited `file_error_alert` emissions (1 per 30s): disk-full no longer generates hundreds of notifications per second
- Auto-pause all downloads on disk-full detection
- Per-torrent error deduplication
- Handlers for `fastresume_rejected_alert`, `torrent_checked_alert`, `alerts_dropped_alert`, `storage_moved_failed_alert`

### Advanced settings (18 libtorrent tunables)
- Disk I/O: async threads, hashing threads, file pool size, checking memory, send buffer watermark
- Connections: global limit, connection speed, unchoke slots, per-torrent max uploads/connections
- Algorithms: choking (fixed slots / rate-based), seed choking (round robin / fastest upload / anti-leech)
- Toggles: rate-limit IP overhead, exempt LAN peers from speed limits

### Automation
- **Run on complete**: external command with template variables (%N=name, %D=path, %H=hash, %Z=size, %F=file)
- **Watched folder**: auto-add `.torrent` files every 10s, move to `.processed/` after adding
- **Torrent export directory**: auto-copy `.torrent` files to a backup folder on add
- **Download queue** with stalled-torrent detection (10KB/s for 60s = frees the queue slot)

### Power user features
- **Super seeding** mode for initial distribution
- **Force start**: bypass active-downloads queue cap for a single torrent
- **Per-torrent rate limits** (download + upload, persisted by info-hash)
- **Per-torrent stop-after-download and max seed time** (overrides global defaults)
- **Bandwidth scheduler**: alternative speed profile with hour-of-day + day-of-week schedule
- **Auto-complete**: mark torrent as Completed after configurable seeding window

### Polish
- Undo remove with toast + recently removed history (last 50 torrents, one-click restore)
- `diagnoseSlow()` diagnostic for stuck torrents
- Low disk space warning (<1GB remaining)
- Pause finished torrents on file errors (external drive unplugged)
- Opportunistic resume data saves on `piece_finished_alert` (rate-limited 1/min/torrent)
- File logging with rotation (5MB/file, keep 3) + log viewer dialog with level filter and export

---

## v2.5.0

### Privacy & private trackers
- **PT Mode**: one-toggle compliance: disables DHT/PEX/LSD, forces anonymous handshake, announces to every tier
- **Tor proxy preset**: one-click fill SOCKS5 127.0.0.1:9050
- **Anti-leecher blocking**: auto-detects and bans Xunlei, QQDownload, Baidu Netdisk P2P by peer_id prefix

### Notifications & integrations
- **Telegram webhook**: download complete, kill switch, RSS auto-download, errors pushed to any chat via bot token
- **Discord Rich Presence**: shows download progress in Discord profile with action buttons
- **Native OS notifications** via QSystemTrayIcon::showMessage

### Discovery & content
- **Smart Paste (Ctrl+V)**: magnet links, info hashes, and thunder:// links from clipboard
- **Torrent Inspector**: preview .torrent metadata before adding
- **RSS feed presets**: one-click add Nyaa, Sukebei, Linux Tracker
- **Thunder:// link decoding**: automatic decode of Xunlei's proprietary format

### WebUI & remote
- **QR code pairing**: scan to open WebUI on phone, no IP typing needed
- **Gitee update mirror**: alternative update source for users in China

### Interface
- Multi-tag system (free-form, multiple per torrent)
- Force Start queue bypass
- Recently removed history (last 50, one-click restore)
- Full backup/restore of settings + resume data
- Inline rename (F2)
- Resume on network change via QNetworkInformation
- Changelog popup after version bump
- Speed display in bytes or bits (togglable)
- Locale-aware number formatting
- 7 UI languages: EN, PT, ZH, JA, RU, ES, DE
