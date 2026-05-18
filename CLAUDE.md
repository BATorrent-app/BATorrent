# BATorrent — project notes for Claude

## Code style

- Write far fewer comments. Default to none. Only add a comment when the *why* is non-obvious (a hidden constraint, a subtle libtorrent/Qt quirk, a workaround for a specific bug). Don't narrate *what* the code does — well-named identifiers already do that.
- One-line comments are almost always enough. Avoid multi-line comment blocks explaining context that belongs in a commit message or PR description.
- Don't re-explain the surrounding control flow, the obvious purpose of a function, or what an alert/signal is for.
