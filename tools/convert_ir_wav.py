#!/usr/bin/env python3
"""Decode a mono WAV impulse response into a generated C++ header.

Hand-rolls the RIFF chunk walk (stdlib `wave` can't represent IEEE-float
PCM and isn't needed here) so unrecognized chunks such as a Broadcast Wave
`bext` chunk are skipped generically instead of assumed absent. The only
hard requirement is mono; bit depth / sample format is decoded from
whatever the `fmt ` chunk actually reports (8/16/24/32-bit integer PCM or
32/64-bit IEEE float), not assumed.

Usage:
    convert_ir_wav.py <input.wav> <output.h> --identifier NAME
"""
import argparse
import struct
from pathlib import Path

WAVE_FORMAT_PCM = 1
WAVE_FORMAT_IEEE_FLOAT = 3
WAVE_FORMAT_EXTENSIBLE = 0xFFFE


def read_chunks(data: bytes) -> dict[bytes, bytes]:
    if data[0:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a RIFF/WAVE file")

    chunks: dict[bytes, bytes] = {}
    pos = 12
    while pos + 8 <= len(data):
        chunk_id = data[pos : pos + 4]
        chunk_size = struct.unpack_from("<I", data, pos + 4)[0]
        chunk_data = data[pos + 8 : pos + 8 + chunk_size]
        chunks.setdefault(chunk_id, chunk_data)
        pos += 8 + chunk_size + (chunk_size & 1)  # chunks are word-aligned

    return chunks


def parse_fmt(fmt: bytes) -> tuple[int, int, int, int]:
    audio_format, num_channels, sample_rate, _byte_rate, _block_align, bits_per_sample = (
        struct.unpack_from("<HHIIHH", fmt, 0)
    )

    if audio_format == WAVE_FORMAT_EXTENSIBLE:
        if len(fmt) < 40:
            raise ValueError("WAVE_FORMAT_EXTENSIBLE fmt chunk is too short")
        # SubFormat GUID's first two bytes carry the real format tag (PCM/float).
        audio_format = struct.unpack_from("<H", fmt, 24)[0]

    return audio_format, num_channels, sample_rate, bits_per_sample


def decode_samples(data: bytes, audio_format: int, bits_per_sample: int) -> list[float]:
    if audio_format == WAVE_FORMAT_PCM:
        if bits_per_sample == 8:
            return [(b - 128) / 128.0 for b in data]

        if bits_per_sample == 16:
            count = len(data) // 2
            return [v / 32768.0 for v in struct.unpack_from(f"<{count}h", data, 0)]

        if bits_per_sample == 24:
            count = len(data) // 3
            out = []
            for i in range(count):
                b0, b1, b2 = data[i * 3 : i * 3 + 3]
                value = b0 | (b1 << 8) | (b2 << 16)
                if value & 0x800000:
                    value -= 0x1000000
                out.append(value / 8388608.0)
            return out

        if bits_per_sample == 32:
            count = len(data) // 4
            return [v / 2147483648.0 for v in struct.unpack_from(f"<{count}i", data, 0)]

        raise ValueError(f"unsupported PCM bit depth: {bits_per_sample}")

    if audio_format == WAVE_FORMAT_IEEE_FLOAT:
        if bits_per_sample == 32:
            count = len(data) // 4
            return list(struct.unpack_from(f"<{count}f", data, 0))

        if bits_per_sample == 64:
            count = len(data) // 8
            return [float(v) for v in struct.unpack_from(f"<{count}d", data, 0)]

        raise ValueError(f"unsupported float bit depth: {bits_per_sample}")

    raise ValueError(f"unsupported audio format tag: {audio_format}")


def format_float(v: float) -> str:
    s = f"{v:.9g}"
    if "." not in s and "e" not in s and "E" not in s:
        s += ".0"
    return s + "f"


def write_header(out_path: Path, identifier: str, sample_rate: int, samples: list[float]) -> None:
    lines = [
        "// Generated file, do not edit",
        f"// Regenerate with: tools/convert_ir_wav.py <source.wav> {out_path.as_posix()} --identifier {identifier}",
        "#pragma once",
        "",
        '#include "IRData.h"',
        "#include <array>",
        "",
        f"inline constexpr std::array<float, {len(samples)}> {identifier}_samples = {{",
    ]

    values_per_line = 10
    for i in range(0, len(samples), values_per_line):
        row = samples[i : i + values_per_line]
        lines.append("    " + ", ".join(format_float(v) for v in row) + ",")

    lines += [
        "};",
        "",
        f"inline constexpr SpeakerIRData {identifier} {{ {sample_rate}, {identifier}_samples }};",
        "",
    ]

    out_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input_wav", type=Path)
    parser.add_argument("output_header", type=Path)
    parser.add_argument("--identifier", required=True, help="C++ identifier for the generated struct instance")
    args = parser.parse_args()

    data = args.input_wav.read_bytes()
    chunks = read_chunks(data)

    if b"fmt " not in chunks:
        raise ValueError(f"{args.input_wav}: missing fmt chunk")
    if b"data" not in chunks:
        raise ValueError(f"{args.input_wav}: missing data chunk")

    audio_format, num_channels, sample_rate, bits_per_sample = parse_fmt(chunks[b"fmt "])

    if num_channels != 1:
        raise ValueError(f"{args.input_wav}: expected mono, found {num_channels} channels")

    samples = decode_samples(chunks[b"data"], audio_format, bits_per_sample)

    args.output_header.parent.mkdir(parents=True, exist_ok=True)
    write_header(args.output_header, args.identifier, sample_rate, samples)

    print(
        f"{args.input_wav.name}: {len(samples)} samples @ {sample_rate} Hz "
        f"(format={audio_format}, bits={bits_per_sample}) -> {args.output_header}"
    )


if __name__ == "__main__":
    main()
