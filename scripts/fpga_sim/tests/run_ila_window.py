import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import time


def stop_process(process):
    if process.poll() is None:
        os.killpg(process.pid, signal.SIGKILL)
    process.wait()


def run_case(options):
    output = Path(options.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    occupied = list(Path('/dev/shm').glob('xdma_sim_*'))
    if occupied:
        raise RuntimeError(f'FPGA_SIM shared memory already exists: {occupied}')
    image = output / 'nops.bin'
    image.write_bytes(bytes.fromhex('13000000') + bytes.fromhex('93801000') * 255)
    environment = os.environ.copy()
    for name in ('FPGA_ILA_ARM_CMD', 'FPGA_ILA_UPLOAD_CMD', 'FPGA_DDR_LOAD_CMD'):
        environment.pop(name, None)
    host_arguments = [str(Path(options.host).resolve()), str(options.base)] if options.control else [
        str(Path(options.host).resolve()), '-i', str(image), '--diff', str(Path(options.ref).resolve()),
        '--ram-size=128MB', f'--ila-window-base={options.base}',
        f'--ila-window-size={options.size}',
    ]
    if not options.control and not options.slow_build:
        host_arguments += [f'--ref-mode={options.mode}', '--ref-fork-interval=0', '--packet-pool-log2=11']
    simulator_arguments = [
        str(Path(options.simulator).resolve()), f'+start={options.start}',
        f'+finish={options.finish}', f'+gap={options.gap}',
    ]
    if options.corrupt:
        simulator_arguments.append('+corrupt')
    if options.debug:
        simulator_arguments.append('+debug')
    if options.no_squash and not options.control:
        host_arguments.append('--no-squash')
    with (output / 'host.log').open('w') as host_log, (output / 'simulator.log').open('w') as simulator_log:
        host = subprocess.Popen(['stdbuf', '-oL', '-eL', *host_arguments], stdout=host_log, stderr=subprocess.STDOUT,
                                env=environment, start_new_session=True)
        simulator = None
        owned_files = {}
        try:
            deadline = time.monotonic() + 10
            while not Path('/dev/shm/xdma_sim_axilite').exists():
                if host.poll() is not None or time.monotonic() >= deadline:
                    raise RuntimeError('host did not initialize shared memory')
                time.sleep(0.02)
            owned_files = {path: path.stat().st_ino for path in Path('/dev/shm').glob('xdma_sim_*')}
            simulator = subprocess.Popen(['stdbuf', '-oL', '-eL', *simulator_arguments], stdout=simulator_log, stderr=subprocess.STDOUT,
                                         env=environment, start_new_session=True)
            host_result = host.wait(timeout=options.timeout)
            if simulator.poll() is not None:
                raise RuntimeError(f'simulator failed with code {simulator.returncode}')
            host_text = (output / 'host.log').read_text()
            expected = 'PASS atomic BAR' if options.control else (
                ('RefForkSummary MISMATCH' if options.mode == 'fork' else 'FAILED')
                if options.corrupt else 'HIT GOOD TRAP')
            if expected not in host_text or bool(host_result) != options.corrupt:
                raise RuntimeError(f'unexpected host result {host_result}; inspect {output}')
            if not options.corrupt and not options.control:
                expected_instructions = options.finish // options.gap - options.start // options.gap
                counts = re.findall(r'instrCnt = ([0-9,]+), cycleCnt = ([0-9,]+)', host_text)
                if not counts or any((int(instructions.replace(',', '')), int(cycle.replace(',', ''))) !=
                                     (expected_instructions, options.finish) for instructions, cycle in counts):
                    raise RuntimeError(f'incorrect retirement or cycle counts: {counts}')
                if options.mode == 'fork' and 'RefForkSummary MATCH' not in host_text:
                    raise RuntimeError('fork did not accept the complete trusted prefix')
            if options.control:
                print(f'PASS control base={options.base} logs={output}')
            else:
                print(f'PASS mode={options.mode} base={options.base} size={options.size} gap={options.gap} '
                      f'corrupt={options.corrupt} logs={output}')
        finally:
            stop_process(host)
            if simulator is not None:
                stop_process(simulator)
            for owned_file, inode in owned_files.items():
                if owned_file.exists() and owned_file.stat().st_ino == inode:
                    owned_file.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', required=True)
    parser.add_argument('--simulator', required=True)
    parser.add_argument('--ref', default='')
    parser.add_argument('--output', required=True)
    parser.add_argument('--mode', choices=('slow', 'fork'), default='fork')
    parser.add_argument('--size', type=int, default=4)
    parser.add_argument('--base', type=int, default=0)
    parser.add_argument('--start', type=int, default=0)
    parser.add_argument('--finish', type=int, default=40)
    parser.add_argument('--gap', type=int, default=1)
    parser.add_argument('--timeout', type=int, default=30)
    parser.add_argument('--corrupt', action='store_true')
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--no-squash', action='store_true')
    parser.add_argument('--control', action='store_true')
    parser.add_argument('--slow-build', action='store_true')
    run_case(parser.parse_args())
