#!/usr/bin/env bash
# Builds a .deb package from the linux-x64 publish output.
# Run from viewer_net/packaging/  →  ../publish/DanaSimViewer_1.0.0_amd64.deb
set -e

VERSION="1.0.0"
ARCH="amd64"
PKG="danasim-viewer_${VERSION}_${ARCH}"
SRC="$(cd "$(dirname "$0")/../publish/linux-x64" && pwd)"
OUT="$(cd "$(dirname "$0")/../publish" && pwd)"
WORK="/tmp/${PKG}"

echo "Building from: $SRC"
echo "Output:        $OUT/${PKG}.deb"

# ── Directory structure ────────────────────────────────────────────────────────
rm -rf "$WORK"
mkdir -p \
  "$WORK/DEBIAN" \
  "$WORK/opt/danasim-viewer" \
  "$WORK/usr/bin" \
  "$WORK/usr/share/applications"

# ── Copy application files ─────────────────────────────────────────────────────
cp -r "$SRC"/. "$WORK/opt/danasim-viewer/"
chmod +x "$WORK/opt/danasim-viewer/DanaSim.Viewer.Web"
chmod +x "$WORK/opt/danasim-viewer/run.sh"

# ── Symlink for terminal use ───────────────────────────────────────────────────
ln -s /opt/danasim-viewer/run.sh "$WORK/usr/bin/danasim-viewer"

# ── .desktop launcher ─────────────────────────────────────────────────────────
cat > "$WORK/usr/share/applications/danasim-viewer.desktop" << 'EOF'
[Desktop Entry]
Name=DanaSim Viewer
Comment=Flood Simulation 3D Viewer
Exec=/opt/danasim-viewer/run.sh
Terminal=false
Type=Application
Categories=Science;Education;
EOF

# ── DEBIAN/control ────────────────────────────────────────────────────────────
cat > "$WORK/DEBIAN/control" << EOF
Package: danasim-viewer
Version: ${VERSION}
Architecture: ${ARCH}
Maintainer: DanaSim <danasim@example.com>
Description: DanaSim Flood Simulation 3D Viewer
 Web-based viewer for the DanaSim flood simulation system.
 Connects to an MQTT broker, renders live flood data in 3D,
 and serves the viewer dashboard at http://localhost:5027.
EOF

# ── DEBIAN/postinst ───────────────────────────────────────────────────────────
cat > "$WORK/DEBIAN/postinst" << 'EOF'
#!/bin/bash
chmod +x /opt/danasim-viewer/DanaSim.Viewer.Web
chmod +x /opt/danasim-viewer/run.sh
EOF
chmod 755 "$WORK/DEBIAN/postinst"

# ── Build .deb ────────────────────────────────────────────────────────────────
fakeroot dpkg-deb --build "$WORK" "$OUT/${PKG}.deb"
echo "Done: $OUT/${PKG}.deb"
