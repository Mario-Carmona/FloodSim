# Building the DanaSim Viewer installers

`publish/` is gitignored — installers aren't committed to the repo. To produce
them from a clean checkout, run the steps below from `viewer_net/`.

## 1. Publish the app (single-file, self-contained)

```bash
dotnet publish src/DanaSim.Viewer.Web/DanaSim.Viewer.Web.csproj -c Release -r linux-x64 \
  --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true \
  -o publish/linux-x64

dotnet publish src/DanaSim.Viewer.Web/DanaSim.Viewer.Web.csproj -c Release -r win-x64 \
  --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true \
  -o publish/win-x64
```

**Windows gotcha:** `PublishSingleFile` does NOT copy `aspnetcorev2_inprocess.dll`
into `publish/win-x64/` — it gets bundled into the single-file exe instead, but
IIS needs it as a standalone DLL next to the exe to load the in-process hosting
module (`web.config` points `aspNetCore` at it directly). Copy it manually after
publishing:

```bash
cp src/DanaSim.Viewer.Web/bin/Release/net10.0/win-x64/aspnetcorev2_inprocess.dll publish/win-x64/
```

Without this step, `makensis` fails with `File: ... -> no files found` on the
line that packages `aspnetcorev2_inprocess.dll`, and an installer built without
it would produce a working dashboard but a broken IIS deployment.

## 2. Build the Linux .deb

Requires `dpkg-deb` and `fakeroot`.

```bash
cd packaging
./build-deb.sh
```

Produces `publish/danasim-viewer_<version>_amd64.deb`.

## 3. Build the Windows installer

Requires NSIS (`makensis`) — works fine cross-compiled from Linux, no Wine
needed.

```bash
cd packaging
makensis danasim-viewer.nsi
```

Produces `publish/DanaSimViewer-Setup-<version>-win64.exe`.

## Bumping the version

Edit `VERSION` in `build-deb.sh` and `APPVERSION` in `danasim-viewer.nsi` —
they're independent and must be kept in sync manually.

## When to rebuild

Rebuild and replace these artifacts whenever a change lands in
`DanaSim.Viewer.Web`, `DanaSim.Viewer.Infrastructure`, or
`DanaSim.Viewer.Application` — anything that affects the published binary,
`appsettings.json` defaults, or `web.config`/`run.sh`/`run.bat` (e.g. the
`AppPaths`/`Paths:UserDataDir` deployment-path handling).
