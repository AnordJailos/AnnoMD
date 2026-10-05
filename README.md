<p align="center">
  <img src="assets/logo.png" alt="AnnoMD logo" width="128">
</p>

<h1 align="center">AnnoMD</h1>

<p align="center">
  <b>A tiny, native Markdown reader and editor.</b><br>
  Open a <code>.md</code> file, read it, edit it, close it. No IDE required.
</p>

<p align="center">
  <img alt="Platform" src="https://img.shields.io/badge/Windows-x64-0078D6">
  <img alt="Linux" src="https://img.shields.io/badge/Linux-in%20progress-lightgrey">
  <img alt="macOS" src="https://img.shields.io/badge/macOS-planned-lightgrey">
  <img alt="License" src="https://img.shields.io/badge/license-MIT-green">
</p>

---

## The problem

You just want to **read or tweak a Markdown file** — a `README.md`, a changelog, a note, some docs.

Today that usually means launching **VS Code** or a **JetBrains IDE**. Those are excellent tools for software projects, but they are heavyweight: they ship a whole browser engine (Electron) or a JVM, spin up background services, indexers and extension hosts, take a while to start, and routinely occupy **hundreds of megabytes of RAM** (JetBrains IDEs often well over a gigabyte) — all to show you some text.

On a modest laptop, in a VM, or when you already have a real IDE session running, that is a lot of overhead for a text file.

## The solution

**AnnoMD** is a single, small native program built for exactly one job: reading and editing `.md` files.

- **One ~100 KB executable.** No Electron, no JVM, no embedded browser, no runtime or framework to install.
- **Native Win32** — it uses the operating system's own text controls instead of shipping its own, so there is very little to load and very little to keep in memory.
- **Quiet when idle.** Nothing runs in the background; work (word count, preview) only happens a moment after you stop typing, and the preview is only rendered while it is visible.
- **Memory-conscious by design:** capped undo history, preview disabled on very large files, nothing cached it doesn't need.
- **Everything you need, nothing you don't:** tabs, live preview, themes, fonts, find & replace.

> Measurements: I haven't published benchmark numbers yet. Open Task Manager next to your IDE and compare — and if you do, please share the results in an issue or PR.

## Features

| | |
|---|---|
| **Tabs** | Open as many files as you like. Each tab keeps its own undo history, caret and scroll position. New files are named `Untitled1`, `Untitled2`, … Drag & drop, multi-select open, `●` marks unsaved tabs. |
| **Views** | *Editor only*, *Split* (editor + live preview with a **draggable splitter**) and *Reading view*. |
| **Rendered preview** | Headings, **bold**, *italic*, ~~strikethrough~~, links, lists, task lists, quotes, rules, and **code blocks drawn as bordered cards** with a language label. Inline `code` is shown as a chip. |
| **Menus** | **File · Edit · Paragraph · Format · View · Theme** |
| **Paragraph** | Headings 1–6, quote, bullet / numbered / task lists, indent / outdent, horizontal rule — applied to every selected line, and toggled off if applied twice. |
| **Format** | Bold, italic, strikethrough, inline code, code block, link, image — toggles on the selection. |
| **Fonts** | Quick presets (Segoe UI, Calibri, Cambria, Georgia, Times New Roman, Arial, Verdana, Consolas, Courier New), sizes 9–24 pt, plus **every installed font** via *More fonts…* |
| **Themes** | Light, Sepia, Dark, Solarized Dark, Midnight (pure black, easy on OLED) and High Contrast. The title bar follows dark themes. |
| **Find & Replace** | Match case, whole word, replace all. |
| **Quality of life** | Right-click menus (editor, preview and tabs), **Help → Check for Updates**, word wrap, zoom, status bar (line, column, words, characters, encoding), always-on-top, settings remembered between runs, UTF-8 / UTF-16 files, original line endings preserved. |

## Install

### Windows (64-bit)

1. Go to the [**Releases**](../../releases) page.
2. Download **`AnnoMD-Setup.exe`** and run it — or grab the portable **`AnnoMD.exe`**, which needs no installation.

The installer adds Start Menu / Desktop shortcuts, an *"Edit with AnnoMD"* right-click entry for `.md` files, and a normal uninstaller (Settings → Apps). It can also be run silently with `/S`.

> **SmartScreen:** the installer is not code-signed yet, so Windows may show *"Unknown publisher"*. Click **More info → Run anyway**.
> Developed for Windows 10/11 (x64). It should work on Windows 7 and newer, but that is not regularly tested.

### Linux — *in progress*

AnnoMD is currently a Win32 program, so a **native Linux build is being written** (a lightweight GTK port that keeps the same feature set and the same small footprint). Packages (AppImage / `.deb`) will follow once it works.

### macOS — *planned*

Comes after Linux. Follow the [roadmap](#roadmap).

| Platform | Status |
|---|---|
| Windows x64 | ✅ Available — installer + portable |
| Linux | 🚧 Native port in progress |
| macOS | 🗓 Planned |

## Updating

- **From the app:** *Help → Check for Updates…* looks at the latest GitHub Release and offers to open the download page. It only runs when you click it — AnnoMD never contacts the internet in the background.
- **Installer users:** run the new `AnnoMD-Setup.exe`. It installs over the old version and keeps your settings. If AnnoMD is still open, the installer asks you to close it.
- **Portable users:** replace `AnnoMD.exe` with the new file.
- **winget:** `winget upgrade AnordJailos.AnnoMD` — *once the package has been accepted into the winget repository (not yet).*
- **Which version am I running?** *Help → About AnnoMD*, or right-click `AnnoMD.exe` → Properties → Details.
- Each release lists a SHA-256 checksum (`SHA256SUMS.txt`) — see [Verify your download](#verify-your-download).

Versions follow [Semantic Versioning](https://semver.org). See the [changelog](CHANGELOG.md); maintainers: [RELEASING.md](RELEASING.md).

## Verify your download

Every release includes a `SHA256SUMS.txt` file containing the SHA-256 fingerprint of each download. Checking it confirms that the file you have is exactly the one that was published — not corrupted or altered on the way.

**Windows (PowerShell)** — open it in the folder that contains the download and `SHA256SUMS.txt`:

```powershell
$expected = ((Get-Content .\SHA256SUMS.txt | Where-Object { $_ -match 'AnnoMD-Setup.exe' }) -split '\s+')[0]
$actual   = (Get-FileHash .\AnnoMD-Setup.exe -Algorithm SHA256).Hash
if ($actual -eq $expected) { "OK - file matches the release" } else { "MISMATCH - do not run it" }
```

Use `AnnoMD.exe` instead of `AnnoMD-Setup.exe` for the portable build. To see the fingerprint and compare it by eye, run `Get-FileHash .\AnnoMD-Setup.exe -Algorithm SHA256` (or `certutil -hashfile AnnoMD-Setup.exe SHA256` in Command Prompt); capital vs. lowercase letters don't matter.

**Installed copy:** the installer places the same `AnnoMD.exe` that is attached to the release, so `Get-FileHash "C:\Program Files\AnnoMD\AnnoMD.exe" -Algorithm SHA256` should equal the `AnnoMD.exe` line in `SHA256SUMS.txt`.

**Linux / macOS:** `sha256sum -c SHA256SUMS.txt --ignore-missing` (macOS: `shasum -a 256 AnnoMD-Setup.exe`, then compare).

If it doesn't match, don't run it: download it again from the [Releases](../../releases) page, and open an issue if it still differs.

> A checksum catches corrupted or tampered downloads, but it is published in the same place as the file, so it cannot protect against a compromised release. Code signing (planned) covers that.

## Keyboard shortcuts

| Action | Shortcut | Action | Shortcut |
|---|---|---|---|
| New tab | `Ctrl+N` | Bold | `Ctrl+B` |
| Open | `Ctrl+O` | Italic | `Ctrl+I` |
| Close tab | `Ctrl+W` | Inline code | ``Ctrl+` `` |
| Save / Save As | `Ctrl+S` / `Ctrl+Shift+S` | Link | `Ctrl+K` |
| Next / previous tab | `Ctrl+Tab` / `Ctrl+Shift+Tab` | Heading 1 – 6 | `Ctrl+1` … `Ctrl+6` |
| Find / Find next / Replace | `Ctrl+F` / `F3` / `Ctrl+H` | Editor / Split / Reading | `F5` / `F6` / `F7` |
| Zoom in / out / reset | `Ctrl++` / `Ctrl+-` / `Ctrl+0` | | |

## Markdown support in the preview

Supported: headings, bold, italic, strikethrough, inline code, fenced code blocks (``` or `~~~`, with language label), links, images (shown as `[image: alt]`), block quotes, bullet / numbered / task lists (with nesting), horizontal rules, backslash escapes.

Known limitations (contributions welcome): tables are shown as plain monospace text, images are not rendered, HTML blocks and footnotes are not interpreted, emphasis does not span line breaks.

## Build from source

AnnoMD is a single C file (`src/annomd.c`) with no dependencies beyond the Windows SDK / MinGW-w64 headers.

**On Linux (cross-compile) — this is what CI does**

```sh
sudo apt install gcc-mingw-w64-x86-64 nsis make python3
make            # -> build/AnnoMD.exe and build/AnnoMD-Setup.exe
```

**On Windows (MSYS2, MinGW-w64 shell)**

```sh
pacman -S mingw-w64-x86_64-gcc make mingw-w64-x86_64-nsis
make CROSS=
```

`make exe` builds only the program (no NSIS needed). `python3 tools/make_icon.py` regenerates the icon.

## Project layout

```
src/annomd.c            the whole application
src/app.rc, app.manifest  icon, version info + Windows manifest (visual styles, DPI)
assets/                 icon (and README logo)
installer/annomd.nsi    NSIS installer script
tools/make_icon.py      draws the icon (pure Python, no dependencies)
.github/workflows/      CI: builds + publishes releases on tags; optional winget updates
VERSION                 the single source of truth for the version number
CHANGELOG.md            what changed in each version
RELEASING.md            how to publish a release and how updates reach users
```

## Roadmap

- [x] Windows app: tabs, split/reading view, themes, fonts, find & replace
- [x] Windows installer + portable build
- [x] Help menu: About + manual "Check for Updates"
- [ ] winget package (manifest submission — see [RELEASING.md](RELEASING.md))
- [ ] Silent self-update (only after the installer is code-signed)
- [ ] **Native Linux build** (GTK) + AppImage / `.deb`
- [ ] **macOS build**
- [ ] Real table rendering in the preview
- [ ] Auto-continue lists when pressing Enter
- [ ] Export to HTML / PDF, printing
- [ ] Published memory / startup benchmarks vs. VS Code and JetBrains IDEs
- [ ] Code-signed Windows installer

## Contributing

Issues and pull requests are welcome especially benchmark numbers, Linux/macOS help, and Markdown-rendering fixes. Keep the project's goal in mind: **small and light first**.

## License

[MIT](LICENSE)
