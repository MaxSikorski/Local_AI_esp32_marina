import argparse
import hashlib
import json
import struct
from pathlib import Path

K10_SHA = 'c16d054088967821e7d8fa98954173cbc7a866faf624ef1974d615e15c093b07'
TAIL_SHA = '5091779b6a8b294903d8c332a2c081a77f6f15a864eb0e6c9d56134880f1fe0e'
MODEL_SHA = 'd219d5add0fcff686003116dca0f9b05eda4b2e1ca5f9b4b6345e5819ab8fb45'

def checked(path, size, sha):
    data = path.read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != sha:
        raise ValueError('Unexpected model component: ' + path.name)
    return data

def assemble_app(firmware, k10, tail):
    if not firmware or len(firmware) > 0x6f000:
        raise ValueError('Firmware overlaps K10 cache at physical 0x7F000')
    if len(tail) != 159936 or len(k10) != 26656:
        raise ValueError('Unexpected app-tail size')
    app = bytearray(b'\xff' * (0x76000+len(tail)))
    app[:len(firmware)] = firmware
    app[0x6f000:0x6f000+len(k10)] = k10
    app[0x76000:] = tail
    return bytes(app)

def check_partitions(data):
    entries = {}
    for off in range(0, len(data)-31, 32):
        magic, typ, sub, address, size, label, flags = struct.unpack_from('<HBBII16sI', data, off)
        if magic != 0x50aa: break
        entries[label.split(b'\0')[0].decode()] = (typ, sub, address, size)
    expected = {'nvs':(1,2,0x9000,0x5000), 'app0':(0,0,0x10000,0xa0000),
                'model':(1,0x40,0xb0000,0x740000), 'coredump':(1,3,0x7f0000,0x10000)}
    if entries != expected:
        raise ValueError('Wrong partition table; requires original XIAO app/cache layout')

def main():
    ap=argparse.ArgumentParser(description='Package the XIAO UI with both fixed-address caches offline. Never flashes a device.')
    ap.add_argument('--build-dir',type=Path,required=True)
    ap.add_argument('--assets-dir',type=Path,required=True,help='Reviewed xiao asset directory')
    ap.add_argument('--out-dir',type=Path,required=True)
    args=ap.parse_args()
    b,a=args.build_dir,args.assets_dir
    partitions=(b/'partitions.bin').read_bytes();check_partitions(partitions)
    k10=checked(a/'cache_components/k10_cache.bin',26656,K10_SHA)
    tail=checked(a/'cache_components/app_tail_cache.bin',159936,TAIL_SHA)
    model=checked(a/'flash/tied_model_partition_0xB0000.bin',7602176,MODEL_SHA)
    app=assemble_app((b/'firmware.bin').read_bytes(),k10,tail)
    blobs={'bootloader.bin':(0,(b/'bootloader.bin').read_bytes()),'partitions.bin':(0x8000,partitions),
           'app_with_tail.bin':(0x10000,app),'model.bin':(0xb0000,model)}
    args.out_dir.mkdir(parents=True,exist_ok=False)
    manifest={}
    for name,(address,data) in blobs.items():
        (args.out_dir/name).write_bytes(data)
        manifest[name]={'address':hex(address),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    (args.out_dir/'FLASH_LAYOUT.json').write_text(json.dumps(manifest,indent=2)+'\n')
    files=sorted(args.out_dir.iterdir())
    (args.out_dir/'SHA256SUMS.txt').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in files))
    print('Created app with K10 and app-tail caches at original addresses; no NVS image and no hardware access.')

if __name__=='__main__': main()
