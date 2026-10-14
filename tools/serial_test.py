import argparse
import json
import re
import time
from pathlib import Path


def response_error(text):
    lines = text.splitlines()
    for line in lines:
        if line.startswith(('PocketGPT: [generation failed]', 'PocketGPT: [empty generation:', '[FAIL]')):
            return line
        if line.startswith('[CHAT] ') and (
            re.search(r'\bFAIL\b', line)
            or 'runtime is not ready' in line
            or 'prompt too long' in line
        ):
            return line
    replies = [line.removeprefix('PocketGPT: ').strip() for line in lines
               if line.startswith('PocketGPT: ') and not line.startswith('PocketGPT: thinking...')]
    if not any(replies):
        return 'No nonempty model reply received'
    stats = re.search(r'^\[STATS\] tokens=(\d+)\b', text, re.MULTILINE)
    if not stats or int(stats.group(1)) == 0:
        return 'Missing successful generation statistics (tokens must be greater than zero)'
    return None


def main():
    ap = argparse.ArgumentParser(description='DevKit serial inference test; explicitly opens the selected device.')
    ap.add_argument('--port', required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--baud', type=int, default=115200)
    ap.add_argument('--hello-only', action='store_true')
    args = ap.parse_args()
    import serial
    args.output.mkdir(parents=True, exist_ok=False)
    questions = ['hello', 'who are you?', 'what can you do?', 'what is 2+2?', '2+4*5']
    if args.hello_only:
        questions = questions[:1]
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = args.port, args.baud, .2
    s.dtr = s.rts = False
    def wait_prompt(timeout):
        data = bytearray()
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            data.extend(s.read(8192))
            if data.endswith(b'You> '):
                return bytes(data)
        raise RuntimeError('Timed out waiting for the DevKit prompt. Check board and baud rate.')
    s.open()
    try:
        boot = wait_prompt(60)
        (args.output / 'boot.txt').write_bytes(boot)
        if b'[READY]' not in boot or b'Flash=33554432' not in boot:
            raise RuntimeError('Expected DevKit boot was not observed; no questions sent. Reset board after opening.')
        results = []
        for q in questions:
            start = time.monotonic()
            s.write((q + '\n').encode())
            s.flush()
            data = wait_prompt(180)
            text = data.decode('utf-8', 'replace')
            error = response_error(text)
            results.append(dict(prompt=q, serial=text, host_elapsed_seconds=time.monotonic()-start,
                                ok=error is None, error=error))
            (args.output / 'results.json').write_text(json.dumps(results, indent=2))
            if error is not None:
                raise RuntimeError(f'Inference failed for {q!r}: {error}. Transcript saved to {args.output / "results.json"}')
            print(q, 'passed')
    finally:
        s.close()


if __name__ == '__main__':
    main()
