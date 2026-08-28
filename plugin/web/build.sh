#!/usr/bin/env bash
# Builds the browser engine. Needs emsdk on PATH:  source ~/emsdk/emsdk_env.sh
#
# Standalone wasm with no imports, so an AudioWorklet instantiates it with a
# bare WebAssembly.instantiate() - no Emscripten JS runtime. Memory growth is
# off so the worklet's views over the heap stay valid; 64 MB covers the two
# 12 MB keyframe rings plus working space.
set -euo pipefail
cd "$(dirname "$0")/../.."

em++ -O3 -std=c++17 -I lib -I plugin/Source \
  plugin/web/wasm_bridge.cpp plugin/Source/CapicolaEngine.cpp \
  -sSTANDALONE_WASM=1 --no-entry \
  -sEXPORTED_FUNCTIONS='["_colacut_init","_colacut_buf_l","_colacut_buf_r","_colacut_set_params","_colacut_slice","_colacut_process","_colacut_env_in","_colacut_env_out","_colacut_in_peak","_colacut_in_gate","_colacut_latency"]' \
  -sALLOW_MEMORY_GROWTH=0 -sINITIAL_MEMORY=64MB \
  -o plugin/web/colacut.wasm

echo "built plugin/web/colacut.wasm"
