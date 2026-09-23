# On using AI

People ask whether BATorrent is written with AI, so here is the straight answer.

Yes, I use AI coding tools. They help me write and refactor code, chase bugs and
review changes, the way other tools in my setup do. They don't decide anything.
What the app does, how it looks and what it refuses to do (ads, telemetry,
accounts) are my calls, and I test what goes into a release and answer for it,
bugs included. Every change also goes through the same CI as the rest of the
code: the Catch2 suite, the QML smoke test and the sanitizer builds.

The app itself has nothing to do with AI. There are no AI features, and it sends
nothing to any AI service. Everything it sends on its own is listed in
[PRIVACY.md](PRIVACY.md).

If you want to contribute, using AI is fine with me. What I ask is the same as
for any pull request: understand what you're sending, test it, and tell me how.
If you can't answer questions about a change in review, I can't merge it. Keep
pull requests focused; large rewrites or cleanups nobody asked for will be
closed. Bug and security reports should come from something you actually saw
happen, not from a tool's guess.
