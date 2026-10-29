#!/usr/bin/env python3
"""Footswitch + LED test for the pedalboard HAT — run ON THE PI only.

Same pins as CounterTune PedalboardControls:
  Footswitch → BCM GPIO17 (to GND when pressed; pull-up)
  LED        → BCM GPIO27 (HIGH = lit)

No LED wired yet is fine: the pin is still driven; the printout shows state.

Needs: sudo apt install -y python3-gpiozero
Ctrl+C to stop.
"""
from __future__ import annotations

import sys

try:
    from gpiozero import Button, LED
except ImportError:
    print("Install gpiozero: sudo apt install -y python3-gpiozero")
    sys.exit(1)

FSW_PIN = 17
LED_PIN = 27

effect_on = False  # start bypassed; LED off


def main() -> None:
    global effect_on

    led = LED(LED_PIN)
    led.off()

    # pull_up=True: open = high, pressed to GND = low (matches HAT)
    fsw = Button(FSW_PIN, pull_up=True, bounce_time=0.05)

    def on_press() -> None:
        global effect_on
        effect_on = not effect_on
        if effect_on:
            led.on()
        else:
            led.off()
        print(
            f"TOGGLE → effect={'ON' if effect_on else 'OFF'}  "
            f"LED GPIO{LED_PIN}={'HIGH' if effect_on else 'LOW'}",
            flush=True,
        )

    fsw.when_pressed = on_press

    print(
        f"Footswitch GPIO{FSW_PIN} + LED GPIO{LED_PIN}. "
        "Start: effect OFF, LED pin LOW.\n"
        "Press the stomp to toggle. No LED wired is OK — watch this printout.\n"
        "Ctrl+C to stop."
    )

    try:
        # Keep process alive while gpiozero handles edges in the background.
        from signal import pause

        pause()
    except KeyboardInterrupt:
        print("\nStopped. LED pin → LOW.")
    finally:
        led.off()
        fsw.close()
        led.close()


if __name__ == "__main__":
    main()
