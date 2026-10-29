#!/usr/bin/env python3
"""Minimal MCP3008 pot reader for /dev/spidev0.0 — run ON THE PI only.
   J8/CH0 Chaos, J9/CH1 Bloom, J10/CH2 TimeMs, J11/CH3 Mix.
   Needs: sudo apt install python3-spidev  (or pip install spidev)
   Also: dtparam=spi=on, user in group 'spi', then: python3 pot_test.py
"""
import time
import sys

try:
    import spidev
except ImportError:
    print("Install python3-spidev first: sudo apt install python3-spidev")
    sys.exit(1)

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 1_000_000
spi.mode = 0

def read_ch(ch: int) -> int:
    # MCP3008 single-ended: start bit, (SGL/DIFF=1 | ch), don't care
    r = spi.xfer2([0x01, (0x08 | ch) << 4, 0x00])
    return ((r[1] & 0x03) << 8) | r[2]

NAMES = ["Chaos(CH0)", "Bloom(CH1)", "TimeMs(CH2)", "Mix(CH3)"]

print("Reading CH0–CH3 forever. Ctrl+C to stop. Twiddle each pot.")
try:
    while True:
        raw = [read_ch(i) for i in range(4)]
        chaos = raw[0] / 1023.0
        bloom = raw[1] / 1023.0
        time_ms = int(80 + (raw[2] / 1023.0) * (800 - 80))
        mix = raw[3] / 1023.0
        print(
            f"raw={raw}  Chaos={chaos:.3f} Bloom={bloom:.3f} "
            f"TimeMs={time_ms} Mix={mix:.3f}",
            flush=True,
        )
        time.sleep(0.1)
except KeyboardInterrupt:
    pass
finally:
    spi.close()
