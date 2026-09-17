#!/usr/bin/env python3
"""Chip-style SFX for Arcade. 22050 Hz mono s16 WAV. No external samples."""
import math
import os
import random
import struct
import wave

SR = 22050
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "sfx")


def clamp(x):
    if x > 0.99:
        return 0.99
    if x < -0.99:
        return -0.99
    return x


def write_wav(name, samples):
    path = os.path.join(OUT, name)
    with wave.open(path, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        frames = b"".join(struct.pack("<h", int(clamp(s) * 32767)) for s in samples)
        w.writeframes(frames)
    print(path, "n=%d" % len(samples))


def sq(phase):
    return 1.0 if (phase % 1.0) < 0.5 else -1.0


def noise():
    return random.uniform(-1.0, 1.0)


def shot():
    n = int(SR * 0.07)
    out = []
    for i in range(n):
        t = i / SR
        f = 1480.0 - 720.0 * (i / n)
        amp = 0.28 * (1.0 - i / n) ** 0.45
        out.append(amp * sq(t * f))
    return out


def eshot():
    n = int(SR * 0.10)
    out = []
    for i in range(n):
        t = i / SR
        f = 310.0 + 40.0 * math.sin(t * 80.0)
        amp = 0.22 * (1.0 - i / n) ** 0.3
        out.append(amp * sq(t * f))
    return out


def boom():
    n = int(SR * 0.32)
    out = []
    random.seed(7)
    for i in range(n):
        t = i / SR
        amp = 0.42 * math.exp(-t * 9.0)
        thump = 0.35 * sq(t * (90.0 - 50.0 * t)) * math.exp(-t * 14.0)
        out.append(amp * noise() + thump)
    return out


def hit():
    n = int(SR * 0.045)
    out = []
    for i in range(n):
        t = i / SR
        amp = 0.26 * (1.0 - i / n)
        out.append(amp * (sq(t * 1760.0) + 0.5 * sq(t * 2340.0)))
    return out


def death():
    n = int(SR * 0.48)
    out = []
    for i in range(n):
        t = i / SR
        u = i / n
        f = 520.0 * (1.0 - 0.88 * u)
        amp = 0.30 * (1.0 - u) ** 0.6
        out.append(amp * sq(t * f))
    return out


def wave_sting():
    notes = [523.25, 659.25, 783.99]
    out = []
    for f in notes:
        n = int(SR * 0.07)
        for i in range(n):
            t = i / SR
            amp = 0.22 * (1.0 - i / n) ** 0.4
            out.append(amp * sq(t * f))
    return out


def start():
    out = []
    for f, dur, a in ((1046.5, 0.07, 0.24), (1568.0, 0.12, 0.28)):
        n = int(SR * dur)
        for i in range(n):
            t = i / SR
            amp = a * (1.0 - i / n) ** 0.35
            out.append(amp * sq(t * f))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    write_wav("shot.wav", shot())
    write_wav("eshot.wav", eshot())
    write_wav("boom.wav", boom())
    write_wav("hit.wav", hit())
    write_wav("death.wav", death())
    write_wav("wave.wav", wave_sting())
    write_wav("start.wav", start())


if __name__ == "__main__":
    main()
