import argparse
import hashlib
import json
import struct
from pathlib import Path


def verify(root):
    m = json.loads((root / 'quant_pack_manifest.json').read_text())
    blobs = {}
    for stem in ('weights_q4', 'scales_f16', 'kept_tensors'):
        data = (root / (stem + '.bin')).read_bytes()
        if hashlib.sha256(data).hexdigest() != m['files'][stem + '_sha256']:
            raise ValueError('SHA256 mismatch: ' + stem)
        if len(data) != m['bytes'][stem]:
            raise ValueError('Size mismatch: ' + stem)
        blobs[stem] = data
    kept = blobs['kept_tensors']
    magic, version, size, off, n, vocab, dim, digest = struct.unpack('<8s6I32s', kept[-64:])
    if (magic, version, size, off, n, vocab, dim) != (b'PGTIEDv1', 1, 5527424, 0, 5505024, 6144, 448):
        raise ValueError('Unexpected tied layout footer')
    if size + 64 != len(kept) or hashlib.sha256(kept[off:off+n]).digest() != digest:
        raise ValueError('Invalid shared-matrix footer digest')
    records = {r['name']: r for r in m['records']}
    head, emb = records['lm_head.weight'], records['tok_embeddings.weight']
    if head['storage'] != 'fp16_alias' or head['alias_of'] != emb['name'] or head['kept_nbytes'] != 0 or head['kept_offset'] != emb['kept_offset']:
        raise ValueError('Head is not a zero-byte embedding alias')
    total = sum(map(len, blobs.values()))
    if total != 20241616 or total != m['bytes']['total_pack']:
        raise ValueError('Unexpected total package size')
    print('PASS: hashes, sizes, footer and shared-matrix alias;', total, 'bytes')


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description='Verify the deployed DevKit deduplicated pack. No hardware access.')
    ap.add_argument('pack', type=Path)
    verify(ap.parse_args().pack)
