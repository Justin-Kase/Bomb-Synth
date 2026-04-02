#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "🔨 Building Bomb-Synth (Release)..."
mkdir -p "$PROJECT_DIR/build"
cd "$PROJECT_DIR/build"

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -j$(sysctl -n hw.ncpu)

echo ""
echo "📦 Build complete. Running install..."
bash "$SCRIPT_DIR/install.sh"
