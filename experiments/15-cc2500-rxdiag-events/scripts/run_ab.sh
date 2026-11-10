#!/bin/bash
# BLE ON/OFF A/B for rxdiag events. usage: run_ab.sh FREQ_KHZ AGC2 AGC1 REPS SECS TAG
# Needs the drone FC running rxdiag on USB and an ESP32 flashed with ESP-IDF's bleprph
# example; esp_ctl.py switches the ESP32 on and off through its reset line.
# PY: Python with pyserial (default: the repo's host/.venv). ESP_PORT: the ESP32's serial port.
set -u
F=$1; A2=$2; A1=$3; N=$4; S=$5; T=$6
D=$(cd "$(dirname "$0")" && pwd)
PY=${PY:-$D/../../../host/.venv/bin/python}
OUT=$D/../data/ab
mkdir -p "$OUT"
for i in $(seq "$N"); do
  for m in off on; do
    "$PY" "$D/esp_ctl.py" $m $((S+3)) > "$OUT/$T-$m$i.esp.txt" 2>&1 &
    E=$!
    sleep 1.5
    "$PY" "$D/rxdiag_events.py" run --freq-khz "$F" --agc2 "$A2" --agc1 "$A1" --seconds "$S" --note "$T $m$i" --out "$OUT/$T-$m$i.txt" >/dev/null
    wait $E
  done
done
