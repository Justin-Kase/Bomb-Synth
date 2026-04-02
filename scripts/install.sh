#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"

VST3_SRC=$(find "$BUILD_DIR" -name "*.vst3" -maxdepth 6 | head -1)
AU_SRC=$(find "$BUILD_DIR" -name "*.component" -maxdepth 6 | head -1)

VST3_DEST=~/Library/Audio/Plug-Ins/VST3
AU_DEST=~/Library/Audio/Plug-Ins/Components

mkdir -p "$VST3_DEST" "$AU_DEST"

if [ -n "$VST3_SRC" ]; then
    echo "→ Installing VST3: $VST3_SRC → $VST3_DEST"
    cp -R "$VST3_SRC" "$VST3_DEST/"
    VST3_INSTALLED="$VST3_DEST/$(basename "$VST3_SRC")"
    xattr -rd com.apple.quarantine "$VST3_INSTALLED" 2>/dev/null || true
    codesign --force --deep --sign - "$VST3_INSTALLED"
    echo "✓ VST3 installed and signed: $VST3_INSTALLED"
else
    echo "⚠ No VST3 found in $BUILD_DIR — skipping"
fi

if [ -n "$AU_SRC" ]; then
    echo "→ Installing AU: $AU_SRC → $AU_DEST"
    cp -R "$AU_SRC" "$AU_DEST/"
    AU_INSTALLED="$AU_DEST/$(basename "$AU_SRC")"
    xattr -rd com.apple.quarantine "$AU_INSTALLED" 2>/dev/null || true
    codesign --force --deep --sign - "$AU_INSTALLED"
    echo "✓ AU installed and signed: $AU_INSTALLED"
else
    echo "⚠ No AU component found in $BUILD_DIR — skipping"
fi

echo ""
echo "🎛  Bomb-Synth installation complete."
