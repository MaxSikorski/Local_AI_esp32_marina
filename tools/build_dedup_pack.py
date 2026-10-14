#!/usr/bin/env python3
"""
Derive a deduplicated mixed Q4/FP16 pack for the tied 30.46M PocketGPT.

Source: the verified tied mixed pack that was flashed to the DevKit (25,746,576 bytes).
It stores the shared 6144x448 FP16 matrix twice in kept_tensors.bin:
  tok_embeddings.weight at kept offset 0, lm_head.weight at kept offset 5,527,424 (last record).

This tool does NOT re-quantize. It copies weights_q4.bin and scales_f16.bin unchanged,
truncates kept_tensors.bin before the duplicate head (no other offset moves), appends a
64-byte layout footer, and writes a manifest in which lm_head.weight aliases the embedding.

Footer (little-endian, 64 bytes, at kept offset 5,527,424):
  8s  magic "PGTIEDv1"
  6I  layout_version, payload_bytes, head_alias_offset, head_bytes, vocab, d_model
  32s sha256 of kept[head_alias_offset : head_alias_offset + head_bytes]

The output directory must not exist. Source files are hash-checked before and after.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import shutil
import struct
from pathlib import Path


                                                                                       
SOURCE_SHA256 = {
    "weights_q4.bin": "3f747bb82a16ae6b4f72873d10a4fd175a4d6782b9f108101668e2f03a0ef802",
    "scales_f16.bin": "9fec62a157d17c78b0074c6a685d65c9e10d187f33a691000c53aa214f4b4435",
    "kept_tensors.bin": "eb90975ef97bdff6952fb221375bfa611e6340e9eab49ee09a2844556ce9e624",
}

FOOTER_MAGIC = b"PGTIEDv1"
FOOTER_FMT = "<8s6I32s"
LAYOUT_VERSION = 1
UNIQUE_PARAMS = 30_460_864


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def check_sources(src: Path) -> None:
    for name, expected in SOURCE_SHA256.items():
        actual = sha256(src / name)
        if actual != expected:
            raise SystemExit(f"source hash mismatch {name}: {actual} != {expected}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--out-dir", required=True)
    args = ap.parse_args()
    src = Path(args.source).expanduser().resolve()
    out = Path(args.out_dir).expanduser().resolve()

    check_sources(src)
    manifest = json.loads((src / "quant_pack_manifest.json").read_text())
    records = {r["name"]: r for r in manifest["records"]}
    emb, head = records["tok_embeddings.weight"], records["lm_head.weight"]
    vocab, d_model = emb["shape"]

    if emb["shape"] != head["shape"] or emb["storage"] != head["storage"] or emb["kept_nbytes"] != head["kept_nbytes"]:
        raise SystemExit("embedding and head records are not layout-compatible")
    kept = (src / "kept_tensors.bin").read_bytes()
    head_off, n = head["kept_offset"], head["kept_nbytes"]
    if head_off + n != len(kept):
        raise SystemExit("lm_head.weight is not the last kept record; truncation would move other offsets")
    if max(r["kept_offset"] + r["kept_nbytes"] for r in manifest["records"] if r["kept_offset"] is not None and r["name"] != head["name"]) > head_off:
        raise SystemExit("another kept record overlaps the head region")
    emb_bytes = kept[emb["kept_offset"]:emb["kept_offset"] + n]
    if emb_bytes != kept[head_off:head_off + n]:
        raise SystemExit("embedding and head FP16 bytes differ: model is not tied, refusing to deduplicate")

    payload = kept[:head_off]
    footer = struct.pack(FOOTER_FMT, FOOTER_MAGIC, LAYOUT_VERSION, len(payload), emb["kept_offset"], n,
                         vocab, d_model, hashlib.sha256(emb_bytes).digest())
    assert len(footer) == 64

    out.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(src / "weights_q4.bin", out / "weights_q4.bin")
    shutil.copyfile(src / "scales_f16.bin", out / "scales_f16.bin")
    (out / "kept_tensors.bin").write_bytes(payload + footer)

    new_records = []
    for r in manifest["records"]:
        r = dict(r)
        if r["name"] == head["name"]:
            r.update(storage="fp16_alias", alias_of=emb["name"], kept_offset=emb["kept_offset"], kept_nbytes=0)
        new_records.append(r)

    files = {name: sha256(out / name) for name in ("weights_q4.bin", "scales_f16.bin", "kept_tensors.bin")}
    sizes = {name: (out / name).stat().st_size for name in files}
    new_manifest = {
        "format": "ESP32_LLM_V5E_MIXED_Q4_FP16_PACK_TIED",
        "format_version": manifest["format_version"],
        "tied_layout_version": LAYOUT_VERSION,
        "derived_from": {
            "directory": "source-mixed-pack",
            "total_pack_bytes": manifest["bytes"]["total_pack"],
            "sha256": SOURCE_SHA256,
            "checkpoint": "tied_plus050.pt",
        },
        "group_size": manifest["group_size"],
        "unique_params": UNIQUE_PARAMS,
        "serialized_elements_original": manifest["total_params"],
        "quantized_params": manifest["quantized_params"],
        "kept_unique_params": manifest["kept_params"] - head["numel"],
        "tied": {"head": head["name"], "alias_of": emb["name"], "kept_offset": emb["kept_offset"], "bytes": n},
        "kept_footer": {"offset": len(payload), "bytes": len(footer), "format": FOOTER_FMT, "magic": FOOTER_MAGIC.decode()},
        "files": {**{k.split(".")[0]: k for k in files}, **{k.split(".")[0] + "_sha256": v for k, v in files.items()}},
        "bytes": {
            "weights_q4": sizes["weights_q4.bin"],
            "scales_f16": sizes["scales_f16.bin"],
            "kept_tensors_payload": len(payload),
            "kept_footer": len(footer),
            "kept_tensors": sizes["kept_tensors.bin"],
            "weights_payload": sizes["weights_q4.bin"] + sizes["scales_f16.bin"] + len(payload),
            "total_pack": sum(sizes.values()),
        },
        "records": new_records,
    }
    (out / "quant_pack_manifest.json").write_text(json.dumps(new_manifest, indent=2))

    with (out / "quant_pack_inventory.csv").open("w", newline="") as f:
        cols = list(new_records[0].keys()) + ["alias_of"]
        w = csv.DictWriter(f, fieldnames=list(dict.fromkeys(cols)))
        w.writeheader()
        w.writerows(new_records)

    check_sources(src)
    (out / "SHA256SUMS.txt").write_text(
        "".join(f"{sha256(p)}  {p.name}\n" for p in sorted(out.iterdir()) if p.is_file() and p.name != "SHA256SUMS.txt"))
    print(json.dumps(new_manifest["bytes"], indent=2))


if __name__ == "__main__":
    main()
