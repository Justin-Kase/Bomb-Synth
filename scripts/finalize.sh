#!/usr/bin/env bash
# Run this once to finalize the setup
set -euo pipefail
cd ~/Code/Bomb-Synth
chmod +x scripts/install.sh scripts/build.sh
echo "Scripts made executable."

# Build check
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -20
cmake --build . --config Release -j$(sysctl -n hw.ncpu) 2>&1 | tail -30

cd ..
git add -A
git commit -m "feat: sequencer UI improvements + build/install scripts + JUCE pin"
git push
echo "Done!"
