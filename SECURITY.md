# Security Policy

## Supported versions

Only the latest release of BATorrent receives security fixes. Please make sure
you are on the most recent version before reporting an issue.

## Reporting a vulnerability

Please report security vulnerabilities **privately**, not in a public issue:

- Preferred: open a private advisory via GitHub →
  **Security → Report a vulnerability**
  (https://github.com/Mateuscruz19/BATorrent/security/advisories/new)
- Or email **mateuscruz2077@gmail.com** with the details.

Include the affected version, the platform, reproduction steps, and the impact.
You can expect an initial response within a few days. Valid reports will be
fixed in a patch release and credited (unless you prefer to stay anonymous).

## Scope

BATorrent is a desktop BitTorrent client. Areas of particular interest:

- the embedded WebUI / remote-control server (`src/webui/`),
- handling of untrusted input: `.torrent` files, magnet links, RSS feeds,
- the auto-updater and any process the app launches,
- secret storage (tokens are kept in the OS keychain, never in plaintext).
