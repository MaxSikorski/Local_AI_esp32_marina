import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
LOCAL = ROOT / 'xiao_ai/keyboard_interfaces/xiao_local'

class LayoutTests(unittest.TestCase):
    def check_binary(self, size):
        with tempfile.TemporaryDirectory() as d:
            class Env:
                def subst(self, key):
                    return {'$BUILD_DIR': d, '$PROGNAME': 'firmware'}[key]
                def AddPostAction(self, *args): pass
                def AddPreAction(self, *args): pass
            env = Env()
            namespace = {'Import': lambda *args: None, 'env': env}
            exec(compile((LOCAL/'check_firmware_size.py').read_text(), 'check_firmware_size.py', 'exec'), namespace)
            pathlib.Path(d, 'firmware.bin').write_bytes(bytes(size))
            namespace['check_layout'](None, None, env)

    def test_fresh_build_fits_before_k10(self):
        self.check_binary(450944)

    def test_k10_overlap_rejected(self):
        with self.assertRaises(RuntimeError):
            self.check_binary(0x6f001)

    def test_original_partitions_preserved(self):
        rows=[]
        for line in (LOCAL/'partitions_xiao_8mb.csv').read_text().splitlines():
            if not line.strip() or line.lstrip().startswith('#'): continue
            fields=[v.strip() for v in line.split(',')]
            rows.append((fields[0],int(fields[3],0),int(fields[4],0)))
        cache=next((r for r in rows if r[0]=='coredump'),None)
        self.assertEqual(cache,('coredump',0x7f0000,0x10000))
        for left,right in zip(sorted(rows,key=lambda r:r[1]),sorted(rows,key=lambda r:r[1])[1:]):
            self.assertLessEqual(left[1]+left[2],right[1])
        self.assertLessEqual(max(o+n for _,o,n in rows),0x800000)


class PackagingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import importlib.util
        spec=importlib.util.spec_from_file_location('package_xiao',ROOT/'tools/package_xiao.py')
        cls.mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(cls.mod)

    def test_firmware_and_tail_bytes_preserved(self):
        firmware=b'F'*450944;tail=b'T'*159936;k10=b'K'*26656
        out=self.mod.assemble_app(firmware,k10,tail)
        self.assertEqual(out[:len(firmware)],firmware)
        self.assertEqual(out[0x76000:],tail)
        self.assertEqual(out[len(firmware):0x6f000],b'\xff'*(0x6f000-len(firmware)))
        self.assertEqual(out[0x6f000:0x6f000+26656],k10)
        self.assertLessEqual(len(out),0xa0000)

    def test_oversize_firmware_rejected(self):
        with self.assertRaises(ValueError): self.mod.assemble_app(bytes(468752),bytes(26656),bytes(159936))

    def test_incomplete_partition_layout_rejected(self):
        import struct
        old=struct.pack('<HBBII16sI',0x50aa,1,3,0x7f0000,0x10000,b'coredump',0)
        with self.assertRaises(ValueError): self.mod.check_partitions(old)

if __name__=='__main__': unittest.main()
