#!/usr/bin/env python3
"""Convert binary RAM segments to xeDebug pd_raw2 files and a load Tcl script."""

import argparse
import array
from pathlib import Path


def number(value):
    return int(value, 0)


def tcl_word(value):
    for char in ('\\', '"', '$', '[', ']'):
        value = value.replace(char, '\\' + char)
    return '"' + value.replace('\n', '\\n').replace('\r', '\\r') + '"'


def convert(segments, memories, ram_bytes, output):
    if not 0 < ram_bytes <= 16 << 30 or not memories or any(not name for name in memories):
        raise ValueError('provide RAM capacity up to 16 GiB and all RAM helper hierarchies')
    bank_bytes = min(ram_bytes, 8 << 30)
    banks = (ram_bytes + bank_bytes - 1) // bank_bytes
    ranges = []
    for offset, path in segments:
        size = path.stat().st_size
        end = offset + (size + 7) // 8 * 8
        if offset < 0 or offset % 8 or not size or end > ram_bytes:
            raise ValueError(f'{path}: segment must be nonempty, 8-byte aligned and fit in RAM')
        ranges.append((offset, end))
    ordered = sorted(ranges)
    if any(a[1] > b[0] for a, b in zip(ordered, ordered[1:])):
        raise ValueError('segments overlap after padding the final 64-bit word')

    output.mkdir(parents=True, exist_ok=False)
    commands = []
    for segment, (offset, path) in enumerate(segments):
        remaining = path.stat().st_size
        with path.open('rb') as source:
            while remaining:
                bank = offset // bank_bytes
                count = min(remaining, bank_bytes - offset % bank_bytes)
                raw = output / f'segment-{segment}-bank-{bank}.raw2'
                with raw.open('wb') as target:
                    left = count
                    while left:
                        data = source.read(min(left, 1 << 20))
                        if not data:
                            raise ValueError(f'{path}: unexpected EOF')
                        left -= len(data)
                        words = array.array('Q')
                        words.frombytes(data.ljust((len(data) + 7) // 8 * 8, b'\0'))
                        words.byteswap()
                        words.tofile(target)
                start = offset % bank_bytes // 8
                end = start + (count + 7) // 8 - 1
                if banks == 1:
                    name = 'single_bank.memory'
                elif banks == 2:
                    name = 'banked.' + ('memory_lo', 'memory_hi')[bank]
                # Each lane helper uses the full word index, including the lane bits.
                for memory in memories:
                    commands.append(
                        f'memory -load %pd_raw2 {tcl_word(memory + "." + name)} '
                        f'-file {tcl_word(str(raw.resolve()))} '
                        f'-start {start} -end {end} -nofill'
                    )
                offset += count
                remaining -= count
    (output / 'load.tcl').write_text('\n'.join(commands) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('segments', nargs='+', metavar='OFFSET:FILE',
                        help='byte offset from RAM base (0x80000000), and binary segment')
    parser.add_argument('--memory', action='append', required=True,
                        help='RAM helper hierarchy; repeat for every lane helper')
    parser.add_argument('--ram-size', type=number, required=True, help='generated RAM size in GiB')
    parser.add_argument('--output', type=Path, required=True, help='new output directory')
    args = parser.parse_args()
    try:
        segments = [(number(offset), Path(path))
                    for offset, path in (value.split(':', 1) for value in args.segments)]
        convert(segments, args.memory, args.ram_size << 30, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f'{error}\n')
    print(args.output / 'load.tcl')


if __name__ == '__main__':
    main()
