#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Build a single-file .torrent for a release asset, web-seeded from the GitHub
# release URL — the torrent works with zero seeders (HTTP seed serves it), and
# when github.com is unreachable (e.g. degraded RU routes) any peer keeps the
# download alive. Pure stdlib: bencode + SHA-1 pieces.

import hashlib
import sys
import time
from pathlib import Path

PIECE = 256 * 1024
TRACKERS = [
    "udp://tracker.opentrackr.org:1337/announce",
    "udp://open.demonii.com:1337/announce",
    "udp://tracker.torrent.eu.org:451/announce",
]


def bencode(obj):
    if isinstance(obj, int):
        return b"i%de" % obj
    if isinstance(obj, bytes):
        return b"%d:%s" % (len(obj), obj)
    if isinstance(obj, str):
        return bencode(obj.encode())
    if isinstance(obj, list):
        return b"l" + b"".join(bencode(x) for x in obj) + b"e"
    if isinstance(obj, dict):
        out = b"d"
        for k in sorted(obj):
            out += bencode(k) + bencode(obj[k])
        return out + b"e"
    raise TypeError(type(obj))


def main():
    if len(sys.argv) < 3:
        print("usage: make_release_torrent.py <file> <webseed-url> [out.torrent]")
        return 2
    path = Path(sys.argv[1])
    webseed = sys.argv[2]
    out = Path(sys.argv[3]) if len(sys.argv) > 3 else path.with_suffix(path.suffix + ".torrent")

    pieces = bytearray()
    size = 0
    with path.open("rb") as f:
        while chunk := f.read(PIECE):
            pieces += hashlib.sha1(chunk).digest()
            size += len(chunk)

    meta = {
        "announce": TRACKERS[0],
        "announce-list": [[t] for t in TRACKERS],
        "comment": "Official BATorrent release — https://github.com/BATorrent-app/BATorrent",
        "created by": "BATorrent release CI",
        "creation date": int(time.time()),
        "url-list": [webseed],
        "info": {
            "name": path.name,
            "length": size,
            "piece length": PIECE,
            "pieces": bytes(pieces),
        },
    }
    out.write_bytes(bencode(meta))
    info_hash = hashlib.sha1(bencode(meta["info"])).hexdigest()
    print(f"{out}  ({size} bytes, infohash {info_hash})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
