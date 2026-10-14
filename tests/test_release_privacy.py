import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('verify_release', Path(__file__).resolve().parents[1] / 'tools/verify_release.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class PrivacyTests(unittest.TestCase):
    def test_no_manifest_needed(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'README.md').write_text('Public source')
            module.verify(root)

    def test_secret_and_metadata_rejected(self):
        for name, value in [('key.txt', 'AIza' + 'x' * 35), ('.DS_Store', 'metadata'), ('secrets.h', '')]:
            with self.subTest(name=name), tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                (root / name).write_text(value)
                with self.assertRaises(ValueError):
                    module.verify(root)

    def test_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'link').symlink_to('/tmp')
            with self.assertRaises(ValueError):
                module.verify(root)


if __name__ == '__main__':
    unittest.main()
