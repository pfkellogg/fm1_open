# FM-1 Big Screen (screen mirror)

This branch (`screen-out`) adds one thing to M-VAVE's FM1 Open V14: while a computer asks for it,
the FM-1 sends its screen over its USB MIDI connection, so it can be shown big by
[FM-1 Mirror](https://github.com/pfkellogg/fm1-mirror) (Mac, Windows, Linux) or
[FM-1 Mirror + Spyglass](https://github.com/pfkellogg/fm1-mirror-spyglass-android) (Android), for players who can't read a 1.54-inch display.
It is not an official M-VAVE release.

- **Protocol** (app/screen.c, "Screen mirror"; app/transfer.c): the computer sends `F0 7D 'F' 'S' 00 F7`
  about once a second (`01` instead of `00` also asks for the whole screen again). The FM-1 sends every
  row it paints as `F0 7D 'F' 'S' 01 row col <runs> F7`, row and col as two 7-bit bytes each, each run
  four bytes: count 1–127, then the RGB565 colour as bits 0–6, 7–13, 14–15. It stops three seconds
  after the last hello; with nobody asking it sends nothing.
- **Room:** the app image has a fixed size limit, so the boot logo lost its growing bars and the
  "M-VAVE" line, and says "BIG SCREEN" instead of "OFFICIAL OPEN".
- **Building on macOS/Linux:** sdk/Build.ps1 runs JieLi's Windows tools through Wine when `FM1_WINE`
  points at a wine binary (PowerShell 7 runs the script). Because Build.ps1 changed, its pinned hash in
  sdk/manifest.json was updated.

- **MIT licensed screen part:** the screen mirror (this "Screen mirror" section, the hello check, and a
  firmware-neutral copy in [screen_mirror/](screen_mirror/)) is MIT licensed, so other FM-1
  firmwares can use it whatever their licence. FM-1 Mirror shows any firmware that sends these messages.

Contact: pfkellogg@gmail.com
