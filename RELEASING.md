# Releasing AnnoMD

Versions follow [Semantic Versioning](https://semver.org): `MAJOR.MINOR.PATCH`

| Change | Bump | Example |
|---|---|---|
| Bug fix | PATCH | 1.3.0 → 1.3.1 |
| New feature, still compatible | MINOR | 1.3.1 → 1.4.0 |
| Breaking change | MAJOR | 1.4.0 → 2.0.0 |

The **`VERSION` file is the single source of truth.** The Makefile reads it and feeds the same number into
`AnnoMD.exe` (shown in *Help → About* and in the file's Properties → Details), the installer, and Settings → Apps.

## Publish a new version

1. **Edit `VERSION`** (e.g. `1.4.0`) and add an entry to `CHANGELOG.md`.
2. **Commit and push** to `main`:
   ```sh
   git add .
   git commit -m "Release 1.4.0"
   git push
   ```
3. **Tag it** — the tag must be `v` + the contents of `VERSION`:
   ```sh
   git tag v1.4.0
   git push --tags
   ```
4. GitHub Actions then:
   - **fails early** if the tag and `VERSION` disagree,
   - builds `AnnoMD.exe` and `AnnoMD-Setup.exe`,
   - publishes a GitHub Release with both files plus `SHA256SUMS.txt` and auto-generated release notes.
5. Done. Users get it via the channels below.

**Tagged the wrong thing?** Delete the tag and release, fix, and re-tag:
```sh
git tag -d v1.4.0 && git push origin :refs/tags/v1.4.0
```
(and delete the Release on GitHub's Releases page).

## How users update

| Who | How |
|---|---|
| Any user | **Help → Check for Updates…** asks GitHub for the latest release and offers to open the download page. It only runs when clicked; AnnoMD never checks in the background. |
| Installer users | Run the new `AnnoMD-Setup.exe`. It installs over the old version and keeps settings. It asks you to close AnnoMD first if it is still running. |
| Portable users | Replace `AnnoMD.exe` with the new file. |
| winget users | `winget upgrade AnordJailos.AnnoMD` (once the package is accepted, see below). |

Check a download against `SHA256SUMS.txt`:
```powershell
Get-FileHash .\AnnoMD-Setup.exe -Algorithm SHA256
```

## winget (Windows Package Manager)

**One-time first submission (manual).** winget only automates *updates* to a package that already exists.

1. On Windows: `winget install wingetcreate`
2. Publish a release first (steps above), then run:
   ```
   wingetcreate new https://github.com/AnordJailos/AnnoMD/releases/download/v1.3.0/AnnoMD-Setup.exe
   ```
3. When prompted use: **PackageIdentifier** `AnordJailos.AnnoMD` · **Publisher** `AnordJailos` · **PackageName** `AnnoMD` ·
   **License** `MIT` · **PackageUrl** `https://github.com/AnordJailos/AnnoMD` · installer type **nullsoft**, scope **machine**,
   silent switch **/S**.
4. Let it submit the pull request to `microsoft/winget-pkgs` (it needs a GitHub token; the tool walks you through it) and
   wait for review. Note: the installer is **not code-signed**, so reviewers/automated checks may take longer or ask questions.

**After that is merged — automatic updates.** The workflow `.github/workflows/winget.yml` opens the update pull request for
every new release. To switch it on, add a repository secret **`WINGET_TOKEN`** (Settings → Secrets and variables → Actions):
a classic personal access token with the `public_repo` scope. Until the secret exists the workflow does nothing.

## Why there is no silent self-updater (yet)

An app that downloads and runs new executables by itself must be able to prove the download is genuine. The installer is
not code-signed yet, and it installs to Program Files (needs administrator rights), so a self-updater would be a security
risk. Plan: sign the installer first, then reconsider. Until then updates are opt-in and visible: the in-app check, the
installer, or winget.
