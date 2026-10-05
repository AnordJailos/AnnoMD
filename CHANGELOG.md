# Changelog

All notable changes to AnnoMD. Versioning: [Semantic Versioning](https://semver.org).
How to publish a release: see [RELEASING.md](RELEASING.md).

## 1.3.1
- README: new **Verify your download** section (how to check a download against `SHA256SUMS.txt`).
- Dependabot now keeps the GitHub Actions used by the release workflows up to date (one pull request per month).
- RELEASING.md: Windows note for editing `VERSION`, and the rules for keeping updates working.
- No changes to how the app behaves — this release ships the documentation and exercises the release pipeline end to end.

## 1.3.0
- **Help menu:** Check for Updates, AnnoMD on GitHub, Report an Issue, About AnnoMD.
- **Check for Updates** asks GitHub for the latest release when you click it (never in the background).
- The version is now built into `AnnoMD.exe` (Properties → Details) and shown in *Help → About*.
- The installer detects a running AnnoMD and asks you to close it, instead of failing on a locked file.
- Settings → Apps now shows the publisher and links.
- Release workflow checks that the git tag matches `VERSION`, and attaches `SHA256SUMS.txt` to releases.
- Optional workflow to publish updates to winget (inactive until configured; see RELEASING.md).

## 1.2.1
- Right-click context menus in the editor, the preview and on tabs. New Edit → Delete command.

## 1.2.0
- Tabs (one editor per file), draggable splitter, code blocks rendered as cards.
- Windows installer, new "MD" icon, renamed to AnnoMD.

## 1.1.0 and earlier
- Initial Win32 reader/editor: File / Edit / Paragraph / Format / View / Theme menus, six themes, fonts, live preview.
