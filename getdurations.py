#!/usr/bin/env python3
import os
import sys

from mutagen.mp3 import MP3


def get_mp3_durations_ms(directory):
    """
    Scans `directory` for .mp3 files and returns a dict
    mapping filename → duration in milliseconds.
    """
    durations = {}
    for fname in sorted(os.listdir(directory)):
        if not fname.lower().endswith(".mp3"):
            continue
        path = os.path.join(directory, fname)
        try:
            audio = MP3(path)
            # `audio.info.length` is in seconds (float)
            durations[fname] = int(audio.info.length * 1000)
        except Exception as e:
            print(f"Error reading {fname}: {e}", file=sys.stderr)
    return durations


if __name__ == "__main__":
    # optionally pass a directory; defaults to current
    directory = sys.argv[1] if len(sys.argv) > 1 else "."
    if not os.path.isdir(directory):
        print(f"Error: {directory} is not a directory", file=sys.stderr)
        sys.exit(1)

    durations = get_mp3_durations_ms(directory)
    for fname, ms in durations.items():
        print(f"{fname}: {ms} ms")
