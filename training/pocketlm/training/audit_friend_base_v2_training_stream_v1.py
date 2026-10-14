"""Audit the deterministic 300M-token Friend Base v2 training stream.

This module builds only an in-memory schedule over immutable uint16 streams.
It never loads a model, constructs an optimizer, or performs training.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path

import numpy as np


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "checkpoints/_audit_friend_base_v2_training_stream_v1"

STAGE1A_TRAIN = ROOT / "data/pocketlm/pretrain/pretrain_train.bin"
STAGE1A_VAL = ROOT / "data/pocketlm/pretrain/pretrain_val.bin"
STAGE1A_META = ROOT / "data/pocketlm/pretrain/pretrain_meta.json"
STAGE1B_TRAIN = ROOT / "data/pocketlm/pretrain_stage1b/stage1b_train.bin"
STAGE1B_VAL = ROOT / "data/pocketlm/pretrain_stage1b/stage1b_fresh_val.bin"
STAGE1B_META = ROOT / "data/pocketlm/pretrain_stage1b/stage1b_tokenized_meta.json"
STAGE1B_VAL_META = ROOT / "data/pocketlm/pretrain_stage1b/stage1b_fresh_val_tokenized_meta.json"
CHAT_TRAIN = ROOT / "data/pocketlm/friend_base_chat_v2/friend_base_chat_v2_train.bin"
CHAT_VAL = ROOT / "data/pocketlm/friend_base_chat_v2/friend_base_chat_v2_val.bin"
CHAT_META = ROOT / "data/pocketlm/friend_base_chat_v2/friend_base_chat_v2_tokenized_meta.json"
CHAT_MANIFEST = ROOT / "data/pocketlm/friend_base_chat_v2/friend_base_chat_v2_manifest.json"
TOKENIZER = ROOT / "data/pocketlm/tokenizers/spm_6144/tokenizer.model"

EXPECTED_SHA = {
    "stage1a_train": "d842e03bf782e6db3f92b1418af623916beb071e1a6c75c74ec9c79db2c10514",
    "stage1a_val": "d3b87aa187655ce9591bdfd90ed3a4e05c2497dca8e1dffbc3cd532160578c5e",
    "stage1a_meta": "39100b38f6dc8662a0619a1b0160474ae98de4652c9a857b0c81736bff8fa25d",
    "stage1b_train": "1325957154b567840bd1dce4214eb0a28df12536b83dbdf90e224016619b7461",
    "stage1b_val": "23f5ac1b593e431fb1662ce034ef13e2dc25c34544752aa885494be22ceffaac",
    "stage1b_meta": "9a27f208f508b73c90252d66720a0382630a7beaac61eeb3143ea1c368058244",
    "stage1b_val_meta": "c1c302473d8535d68e0de32e5106b64b0940d002347f8df03c15ea6d57d7b7b4",
    "chat_train": "100591e97667cc7a54ebc1cd6916e492be5e3088114fb38c54f10c84344ac08d",
    "chat_val": "137464c35c73f0e5eb9934a5830d493e2732474f54127d2fd72f0e1d6ccef06a",
    "chat_meta": "4fd6b0801049167bd79c1800e9847c429418cc5eb3d9b76b45f44df0a2e88506",
    "chat_manifest": "1de239bf626d95a5482f554434c3711c1d830b1958a44b9510c4b9b44525151a",
    "tokenizer": "aaa73c697ca35198fc621d6d33060c16991b7a5426b7719f3cffdad8ddb08799",
}

SEED = 42
GENERAL_SHUFFLE_SEED = SEED + 2202
CHAT_SHUFFLE_SEED = SEED + 2203
SEQ_LEN = 160
BATCH_SIZE = 32
TOTAL_UPDATES = 58_594
TOTAL_WINDOWS = TOTAL_UPDATES * BATCH_SIZE
CHAT_WINDOWS = 187_500
GENERAL_WINDOWS = TOTAL_WINDOWS - CHAT_WINDOWS
EVAL_TOKEN_TARGETS = (50_000_000, 100_000_000, 200_000_000, 300_000_000)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError("training-stream contract failure: " + message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def array_sha(array: np.ndarray) -> str:
    return hashlib.sha256(np.ascontiguousarray(array).view(np.uint8)).hexdigest()


def write_new(path: Path, text: str) -> None:
    require(not path.exists() and not path.is_symlink(), "refusing overwrite " + str(path))
    temporary = Path(str(path) + ".tmp")
    require(not temporary.exists(), "fresh temporary path")
    with temporary.open("x", encoding="utf-8") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.link(temporary, path)
    temporary.unlink()


def source_paths() -> dict[str, Path]:
    return {
        "stage1a_train": STAGE1A_TRAIN,
        "stage1a_val": STAGE1A_VAL,
        "stage1a_meta": STAGE1A_META,
        "stage1b_train": STAGE1B_TRAIN,
        "stage1b_val": STAGE1B_VAL,
        "stage1b_meta": STAGE1B_META,
        "stage1b_val_meta": STAGE1B_VAL_META,
        "chat_train": CHAT_TRAIN,
        "chat_val": CHAT_VAL,
        "chat_meta": CHAT_META,
        "chat_manifest": CHAT_MANIFEST,
        "tokenizer": TOKENIZER,
    }


def checkpoint_updates() -> list[dict]:
    tokens_per_update = BATCH_SIZE * SEQ_LEN
    result = []
    for target in EVAL_TOKEN_TARGETS:
        update = min(TOTAL_UPDATES, (target + tokens_per_update - 1) // tokens_per_update)
        result.append({
            "nominal_target_tokens": target,
            "update": update,
            "actual_processed_tokens": update * tokens_per_update,
        })
    return result


def build_schedule() -> tuple[dict, np.ndarray, np.ndarray]:
    paths = source_paths()
    actual_sha = {}
    for name, path in paths.items():
        require(path.is_file(), "missing " + name)
        actual_sha[name] = sha256(path)
        require(actual_sha[name] == EXPECTED_SHA[name], name + " SHA")

    stage1a_meta = json.loads(STAGE1A_META.read_text(encoding="utf-8"))
    stage1b_meta = json.loads(STAGE1B_META.read_text(encoding="utf-8"))
    stage1b_val_meta = json.loads(STAGE1B_VAL_META.read_text(encoding="utf-8"))
    chat_meta = json.loads(CHAT_META.read_text(encoding="utf-8"))
    require(stage1a_meta["context_length"] == SEQ_LEN, "Stage-1A sequence length")
    require(stage1b_meta["vocab_size"] == stage1a_meta["vocab_size"] == 6144, "general vocab")
    require(stage1b_val_meta["vocab_size"] == 6144, "Stage-1B validation vocab")
    require(chat_meta["max_sequence_length"] == SEQ_LEN and chat_meta["vocab_size"] == 6144, "chat format")
    require(chat_meta["serialization"].startswith("EOS USER prompt ASSISTANT answer END"), "chat serialization")

    arrays = {
        "stage1a_train": np.memmap(STAGE1A_TRAIN, dtype=np.uint16, mode="r"),
        "stage1a_val": np.memmap(STAGE1A_VAL, dtype=np.uint16, mode="r"),
        "stage1b_train": np.memmap(STAGE1B_TRAIN, dtype=np.uint16, mode="r"),
        "stage1b_val": np.memmap(STAGE1B_VAL, dtype=np.uint16, mode="r"),
        "chat_train": np.memmap(CHAT_TRAIN, dtype=np.uint16, mode="r"),
        "chat_val": np.memmap(CHAT_VAL, dtype=np.uint16, mode="r"),
    }
    expected_lengths = {
        "stage1a_train": int(stage1a_meta["train"]["tokens"]),
        "stage1a_val": int(stage1a_meta["validation"]["tokens"]),
        "stage1b_train": int(stage1b_meta["tokens"]),
        "stage1b_val": int(stage1b_val_meta["tokens"]),
        "chat_train": int(chat_meta["train"]["stream_tokens"]),
        "chat_val": int(chat_meta["validation"]["stream_tokens"]),
    }
    for name, array in arrays.items():
        require(len(array) == expected_lengths[name], name + " length")
        require(int(array.min()) >= 0 and int(array.max()) < 6144, name + " token range")

    stage1a_available = (len(arrays["stage1a_train"]) - 1) // SEQ_LEN
    stage1b_available = (len(arrays["stage1b_train"]) - 1) // SEQ_LEN
    chat_available = (len(arrays["chat_train"]) - 1) // SEQ_LEN
    require(chat_available == CHAT_WINDOWS, "exact one-pass chat window count")
    require(stage1a_available + stage1b_available >= GENERAL_WINDOWS, "general window budget")

    general_ids = np.arange(stage1a_available + stage1b_available, dtype=np.int64)
    np.random.default_rng(GENERAL_SHUFFLE_SEED).shuffle(general_ids)
    general_ids = general_ids[:GENERAL_WINDOWS].copy()
    chat_ids = np.arange(chat_available, dtype=np.int64)
    np.random.default_rng(CHAT_SHUFFLE_SEED).shuffle(chat_ids)

                                                                             
                                                                            
                                          
    edges = np.floor(np.arange(TOTAL_UPDATES + 1, dtype=np.float64) * CHAT_WINDOWS / TOTAL_UPDATES).astype(np.int64)
    chat_per_update = np.diff(edges)
    require(set(chat_per_update.tolist()) == {3, 4}, "3/4 chat windows per update")
    require(int(chat_per_update.sum()) == CHAT_WINDOWS, "chat allocation sum")

    selected_stage1a = int(np.count_nonzero(general_ids < stage1a_available))
    selected_stage1b = GENERAL_WINDOWS - selected_stage1a
    total_tokens = TOTAL_WINDOWS * SEQ_LEN
    chat_tokens = CHAT_WINDOWS * SEQ_LEN
    general_tokens = GENERAL_WINDOWS * SEQ_LEN
    require(total_tokens == 300_001_280, "total token budget")
    require(chat_tokens == 30_000_000, "chat token budget")
    require(general_tokens == 270_001_280, "general token budget")

    summary = {
        "classification": "TRAINING_STREAM_READY_FOR_RUNNER_IMPLEMENTATION",
        "materialized_mixed_stream": False,
        "reason": "schedule references immutable non-overlapping windows in existing memmaps",
        "seed": SEED,
        "sequence_length": SEQ_LEN,
        "batch_size": BATCH_SIZE,
        "updates": TOTAL_UPDATES,
        "total_windows": TOTAL_WINDOWS,
        "total_processed_tokens": total_tokens,
        "general": {
            "target_tokens": general_tokens,
            "selected_windows": GENERAL_WINDOWS,
            "available_windows": stage1a_available + stage1b_available,
            "stage1a_selected_windows": selected_stage1a,
            "stage1a_selected_tokens": selected_stage1a * SEQ_LEN,
            "stage1b_selected_windows": selected_stage1b,
            "stage1b_selected_tokens": selected_stage1b * SEQ_LEN,
            "repeated_windows": 0,
        },
        "chat": {
            "target_tokens": chat_tokens,
            "selected_windows": CHAT_WINDOWS,
            "available_windows": chat_available,
            "fraction": chat_tokens / total_tokens,
            "three_chat_window_updates": int(np.count_nonzero(chat_per_update == 3)),
            "four_chat_window_updates": int(np.count_nonzero(chat_per_update == 4)),
            "repeated_windows": 0,
            "unused_trailing_stream_tokens": len(arrays["chat_train"]) - 1 - chat_tokens,
        },
        "evaluation_checkpoints": checkpoint_updates(),
        "schedule_sha256": {
            "selected_general_global_window_ids": array_sha(general_ids),
            "shuffled_chat_window_ids": array_sha(chat_ids),
            "chat_windows_per_update": array_sha(chat_per_update),
        },
        "source_sha256": actual_sha,
        "validation_stream_tokens": {
            "stage1a": len(arrays["stage1a_val"]),
            "stage1b": len(arrays["stage1b_val"]),
            "chat_v2": len(arrays["chat_val"]),
        },
        "optimizer_constructed": False,
        "model_weights_loaded": False,
        "training_updates": 0,
        "checkpoints_created": 0,
    }
    return summary, general_ids, chat_per_update


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--write-plan", action="store_true")
    args = parser.parse_args()

    summary, _, _ = build_schedule()
    if args.write_plan:
        require(not OUT.exists() and not OUT.is_symlink(), "fresh audit directory")
        OUT.mkdir(parents=True)
        write_new(OUT / "summary.json", json.dumps(summary, indent=2, sort_keys=True) + "\n")
        write_new(OUT / "schedule_spec.json", json.dumps({
            key: summary[key] for key in (
                "seed", "sequence_length", "batch_size", "updates", "total_windows",
                "total_processed_tokens", "general", "chat", "evaluation_checkpoints",
                "schedule_sha256", "source_sha256",
            )
        }, indent=2, sort_keys=True) + "\n")
        write_new(OUT / "REPORT.md", (
            "# Friend Base v2 300M training-stream audit\n\n"
            "**TRAINING STREAM READY FOR RUNNER IMPLEMENTATION.**\n\n"
            f"- Total: {summary['total_processed_tokens']:,} targets in {summary['updates']:,} updates\n"
            f"- General: {summary['general']['target_tokens']:,} targets, no repeated windows\n"
            f"- Chat: {summary['chat']['target_tokens']:,} targets, exact one-pass use, no repeated windows\n"
            "- Mixed stream is a deterministic schedule over immutable memmaps; no redundant binary was created.\n"
            "- Model weights loaded: NO\n- Optimizer constructed: NO\n- Training updates: 0\n"
        ))
        paths = [OUT / "summary.json", OUT / "schedule_spec.json", OUT / "REPORT.md"]
        write_new(OUT / "SHA256SUMS.txt", "".join(f"{sha256(path)}  {path.relative_to(ROOT)}\n" for path in paths))

    print(json.dumps(summary, indent=2, sort_keys=True))
    print("Files written: " + ("PLAN ONLY" if args.write_plan else "NONE"))
    print("Model weights loaded: NO")
    print("Optimizer constructed: NO")
    print("Training updates: 0")
    print("Checkpoints created: 0")


if __name__ == "__main__":
    main()
