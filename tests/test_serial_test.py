import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('serial_test', ROOT / 'tools/serial_test.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

BOOT = b'Flash=33554432\r\n[TIED30M_PACKAGE] PASS\r\n[TIED30M_HEAD_CRC] PASS\r\n[READY]\r\nYou> '
SUCCESS = (b'\r\nYou: hello\r\nPocketGPT: thinking... [TIED30M Q4/FP16 dedup]\r\n'
           b'PocketGPT: Hello.\r\n[STATS] tokens=3 stopped=yes total_ms=4420.6\r\nYou> ')


class FakeSerial:
    def __init__(self, chunks):
        self.chunks = iter(chunks)
        self.writes = []
        self.closed = False

    def open(self):
        pass

    def read(self, size):
        return next(self.chunks)

    def write(self, data):
        self.writes.append(data)
        return len(data)

    def flush(self):
        pass

    def close(self):
        self.closed = True


class SerialTests(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.output = Path(folder.name) / 'results'

    def run_serial(self, chunks, hello_only=True):
        self.serial = FakeSerial(chunks)
        argv = ['serial_test.py', '--port', 'MOCK', '--output', str(self.output)]
        if hello_only:
            argv.append('--hello-only')
        with patch.dict(sys.modules, {'serial': types.SimpleNamespace(Serial=lambda: self.serial)}), \
             patch.object(sys, 'argv', argv), contextlib.redirect_stdout(io.StringIO()):
            module.main()

    def results(self):
        return json.loads((self.output / 'results.json').read_text())

    def test_success_saves_transcript_and_closes_port(self):
        self.run_serial([BOOT, SUCCESS])
        result, = self.results()
        self.assertTrue(result['ok'])
        self.assertIsNone(result['error'])
        self.assertEqual(result['serial'], SUCCESS.decode())
        self.assertEqual((self.output / 'boot.txt').read_bytes(), BOOT)
        self.assertEqual(self.serial.writes, [b'hello\n'])
        self.assertTrue(self.serial.closed)

    def test_fragmented_response_and_prompt(self):
        self.run_serial([BOOT[:20], BOOT[20:], SUCCESS[:-3], SUCCESS[-3:-1], SUCCESS[-1:]])
        self.assertTrue(self.results()[0]['ok'])

    def test_generation_failure_raises_after_saving_transcript(self):
        failure = b'\r\n[CHAT] predict FAIL step=0\r\nPocketGPT: [generation failed]\r\nYou> '
        with self.assertRaisesRegex(RuntimeError, 'Inference failed'):
            self.run_serial([BOOT, failure])
        result, = self.results()
        self.assertFalse(result['ok'])
        self.assertIn('FAIL', result['error'])
        self.assertEqual(result['serial'], failure.decode())
        self.assertEqual(self.serial.writes, [b'hello\n'])
        self.assertTrue(self.serial.closed)

    def test_later_failure_preserves_earlier_results(self):
        failure = b'PocketGPT: [generation failed]\r\nYou> '
        with self.assertRaisesRegex(RuntimeError, 'Inference failed'):
            self.run_serial([BOOT, SUCCESS, failure], hello_only=False)
        self.assertEqual([r['ok'] for r in self.results()], [True, False])
        self.assertEqual(self.serial.writes, [b'hello\n', b'who are you?\n'])
        self.assertTrue(self.serial.closed)

    def test_incomplete_or_failed_generations_are_rejected(self):
        responses = [
            'PocketGPT: [generation failed]\n',
            'PocketGPT: [empty generation: END before first token]\n',
            '[CHAT] decode FAIL step=1\n',
            '[CHAT] runtime is not ready\n',
            '[CHAT] prompt too long tokens=40 max=64\n',
            '[FAIL] context position out of range\n',
            'PocketGPT: thinking... [TIED30M Q4/FP16 dedup]\n',
            'PocketGPT: \n[STATS] tokens=3 stopped=yes\n',
            'PocketGPT: Hello.\n',
            'PocketGPT: Hello.\n[STATS] tokens=0 stopped=yes\n',
        ]
        for response in responses:
            with self.subTest(response=response):
                self.assertIsNotNone(module.response_error(response + 'You> '))

    def test_output_cap_and_failure_word_in_reply_are_valid(self):
        response = 'PocketGPT: A test can FAIL.\n[STATS] tokens=24 stopped=no\nYou> '
        self.assertIsNone(module.response_error(response))

    def test_wrong_boot_sends_no_questions(self):
        with self.assertRaisesRegex(RuntimeError, 'Expected DevKit boot'):
            self.run_serial([b'Flash=8388608\r\n[READY]\r\nYou> '])
        self.assertEqual(self.serial.writes, [])
        self.assertTrue(self.serial.closed)
        self.assertFalse((self.output / 'results.json').exists())


if __name__ == '__main__':
    unittest.main()
