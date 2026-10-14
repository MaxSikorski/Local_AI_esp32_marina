"""Tokenize the frozen Friend Base chat corpus v2 as full-LM streams.

Each row is serialized as ``EOS USER prompt ASSISTANT answer END``. Rows are
concatenated and one final EOS is appended, so shifted next-token CE includes
END->EOS and EOS->USER transitions. The first run must use ``--dry-run``.
No model or optimizer is constructed by this builder.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import statistics
from collections import Counter
from pathlib import Path

import numpy as np
import sentencepiece as spm


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data/pocketlm/friend_base_chat_v2"
AUDIT = ROOT / "checkpoints/_audit_friend_base_chat_tokenized_v2"
TRAIN_SOURCE = DATA / "friend_base_chat_v2_train.jsonl"
VAL_SOURCE = DATA / "friend_base_chat_v2_val.jsonl"
MANIFEST = DATA / "friend_base_chat_v2_manifest.json"
TOKENIZER = ROOT / "data/pocketlm/tokenizers/spm_6144/tokenizer.model"
TRAIN_BIN = DATA / "friend_base_chat_v2_train.bin"
VAL_BIN = DATA / "friend_base_chat_v2_val.bin"
META = DATA / "friend_base_chat_v2_tokenized_meta.json"

MAX_SEQ_LEN = 160
VOCAB_SIZE = 6144
PAD, UNK, BOS, EOS, USER, ASSISTANT, END = 0, 1, 2, 3, 4, 5, 9
EXPECTED_TOKENIZER_SHA = "aaa73c697ca35198fc621d6d33060c16991b7a5426b7719f3cffdad8ddb08799"
SAMPLE_COUNT = 24
PROGRESS_ROWS = 50_000


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError("v2 tokenization contract failure: " + message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def iter_jsonl(path: Path):
    with path.open(encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            if line.strip():
                try:
                    yield json.loads(line)
                except json.JSONDecodeError as exc:
                    raise RuntimeError(f"invalid JSON at {path}:{line_number}") from exc


def numeric_stats(values: np.ndarray) -> dict:
    return {
        "min": int(values.min()),
        "max": int(values.max()),
        "mean": float(values.mean()),
        "median": float(np.median(values)),
        "p90": float(np.percentile(values, 90)),
        "p99": float(np.percentile(values, 99)),
    }


def sample_indices(row_count: int) -> set[int]:
    return {
        round(position * (row_count - 1) / (SAMPLE_COUNT - 1))
        for position in range(SAMPLE_COUNT)
    }


def tokenize_split(
    path: Path,
    tokenizer,
    *,
    split_name: str,
    expected_rows: int,
    expected_document_tokens: int,
) -> tuple[np.ndarray, dict, list[dict]]:
                                                                              
                                                                             
    array = np.empty(expected_document_tokens + 1, dtype=np.uint16)
    document_lengths = np.empty(expected_rows, dtype=np.int32)
    prompt_lengths = np.empty(expected_rows, dtype=np.int32)
    answer_lengths = np.empty(expected_rows, dtype=np.int32)
    chosen_samples = sample_indices(expected_rows)
    samples: list[dict] = []
    sources: Counter[str] = Counter()
    ids: set[str] = set()
    cursor = 0
    rows_seen = 0

    for index, row in enumerate(iter_jsonl(path)):
        require(index < expected_rows, f"{split_name} has extra rows")
        for field in ("id", "source", "prompt", "answer", "prompt_tokens", "answer_tokens", "sequence_tokens"):
            require(field in row, f"{split_name} row {index} missing {field}")
        row_id = str(row["id"])
        require(row_id not in ids, f"duplicate {split_name} ID {row_id}")
        ids.add(row_id)

        prompt = tokenizer.encode(row["prompt"], out_type=int, add_bos=False, add_eos=False)
        answer = tokenizer.encode(row["answer"], out_type=int, add_bos=False, add_eos=False)
        sequence = [EOS, USER] + prompt + [ASSISTANT] + answer + [END]
        require(prompt and answer, f"empty tokenization {row_id}")
        require(UNK not in sequence, f"UNK in {row_id}")
        require(len(prompt) == int(row["prompt_tokens"]), f"prompt length binding {row_id}")
        require(len(answer) == int(row["answer_tokens"]), f"answer length binding {row_id}")
        require(len(sequence) == int(row["sequence_tokens"]), f"sequence length binding {row_id}")
        require(len(sequence) <= MAX_SEQ_LEN, f"sequence overflow {row_id}")
        require(cursor + len(sequence) <= expected_document_tokens, f"token budget overflow {row_id}")
        require(all(0 <= token < VOCAB_SIZE for token in sequence), f"token range {row_id}")

        end_cursor = cursor + len(sequence)
        array[cursor:end_cursor] = sequence
        cursor = end_cursor
        document_lengths[index] = len(sequence)
        prompt_lengths[index] = len(prompt)
        answer_lengths[index] = len(answer)
        sources[str(row["source"])] += 1
        rows_seen += 1

        if index in chosen_samples:
            decoded_prompt = tokenizer.decode(prompt)
            decoded_answer = tokenizer.decode(answer)
                                                                            
                                                                         
                                                                            
            require(tokenizer.encode(decoded_prompt, out_type=int, add_bos=False, add_eos=False) == prompt, f"prompt round-trip {row_id}")
            require(tokenizer.encode(decoded_answer, out_type=int, add_bos=False, add_eos=False) == answer, f"answer round-trip {row_id}")
            samples.append({
                "index": index,
                "id": row_id,
                "source": row["source"],
                "prompt": row["prompt"],
                "answer": row["answer"],
                "token_ids": sequence,
                "decoded_prompt": decoded_prompt,
                "decoded_answer": decoded_answer,
                "prompt_text_exact": decoded_prompt == row["prompt"],
                "answer_text_exact": decoded_answer == row["answer"],
                "END_position_in_document": len(sequence) - 1,
            })
        if rows_seen % PROGRESS_ROWS == 0:
            print(f"TOKENIZE {split_name}: {rows_seen:,}/{expected_rows:,} rows", flush=True)

    require(rows_seen == expected_rows, f"{split_name} row count {rows_seen} != {expected_rows}")
    require(cursor == expected_document_tokens, f"{split_name} document tokens {cursor} != {expected_document_tokens}")
    array[cursor] = EOS

    targets = array[1:]
    counts = np.bincount(targets.astype(np.int64), minlength=VOCAB_SIZE)
    require(int(counts[USER]) == expected_rows, f"{split_name} USER target count")
    require(int(counts[ASSISTANT]) == expected_rows, f"{split_name} ASSISTANT target count")
    require(int(counts[END]) == expected_rows, f"{split_name} END target count")
    require(int(counts[EOS]) == expected_rows, f"{split_name} EOS target count")
    require(int(array.min()) >= 0 and int(array.max()) < VOCAB_SIZE, f"{split_name} token range")
    require(len(samples) == len(chosen_samples), f"{split_name} sample coverage")

    return array, {
        "rows": rows_seen,
        "stream_tokens": len(array),
        "training_targets": len(array) - 1,
        "document_tokens": cursor,
        "document_length_stats": numeric_stats(document_lengths),
        "prompt_token_stats": numeric_stats(prompt_lengths),
        "answer_token_stats": numeric_stats(answer_lengths),
        "source_rows": dict(sorted(sources.items())),
        "special_target_counts": {
            "EOS": int(counts[EOS]),
            "USER": int(counts[USER]),
            "ASSISTANT": int(counts[ASSISTANT]),
            "END": int(counts[END]),
        },
        "truncated_rows": 0,
        "unknown_token_rows": 0,
    }, samples


def exclusive_binary_write(path: Path, array: np.ndarray) -> None:
    require(not path.exists() and not path.is_symlink(), "refusing overwrite " + str(path))
    temporary = Path(str(path) + ".tmp")
    require(not temporary.exists(), "fresh temporary path " + str(temporary))
    with temporary.open("xb") as stream:
        array.tofile(stream)
        stream.flush()
        os.fsync(stream.fileno())
    os.link(temporary, path)
    temporary.unlink()


def exclusive_text_write(path: Path, text: str) -> None:
    require(not path.exists() and not path.is_symlink(), "refusing overwrite " + str(path))
    temporary = Path(str(path) + ".tmp")
    require(not temporary.exists(), "fresh temporary path " + str(temporary))
    with temporary.open("x", encoding="utf-8") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.link(temporary, path)
    temporary.unlink()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--write", action="store_true")
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    require(manifest["version"] == "friend_base_chat_corpus_v2", "manifest version")
    require(manifest["tokenization_contract"] == "EOS USER prompt ASSISTANT answer END; max 160", "manifest serialization")
    require(sha256(TOKENIZER) == EXPECTED_TOKENIZER_SHA, "tokenizer SHA")
    require(sha256(TRAIN_SOURCE) == manifest["files"][str(TRAIN_SOURCE.relative_to(ROOT))], "train corpus SHA")
    require(sha256(VAL_SOURCE) == manifest["files"][str(VAL_SOURCE.relative_to(ROOT))], "validation corpus SHA")

    tokenizer = spm.SentencePieceProcessor(model_file=str(TOKENIZER))
    require(tokenizer.get_piece_size() == VOCAB_SIZE, "vocab size")
    require([tokenizer.pad_id(), tokenizer.unk_id(), tokenizer.bos_id(), tokenizer.eos_id()] == [PAD, UNK, BOS, EOS], "base IDs")
    require([tokenizer.id_to_piece(i) for i in (USER, ASSISTANT, END)] == ["<|user|>", "<|assistant|>", "<|end|>"], "chat IDs")

    train, train_stats, train_samples = tokenize_split(
        TRAIN_SOURCE,
        tokenizer,
        split_name="train",
        expected_rows=int(manifest["row_counts"]["train"]),
        expected_document_tokens=int(manifest["actual_document_tokens"]["train"]),
    )
    val, val_stats, val_samples = tokenize_split(
        VAL_SOURCE,
        tokenizer,
        split_name="validation",
        expected_rows=int(manifest["row_counts"]["validation"]),
        expected_document_tokens=int(manifest["actual_document_tokens"]["validation"]),
    )

    report = {
        "classification": "DRY_RUN_PASS" if args.dry_run else "TOKENIZATION_READY",
        "serialization": "EOS USER prompt ASSISTANT answer END; continuous full-LM stream; final EOS",
        "assistant_only": False,
        "padding": False,
        "dtype": "uint16",
        "max_sequence_length": MAX_SEQ_LEN,
        "vocab_size": VOCAB_SIZE,
        "special_ids": {"PAD": PAD, "UNK": UNK, "BOS": BOS, "EOS": EOS, "USER": USER, "ASSISTANT": ASSISTANT, "END": END},
        "train": train_stats,
        "validation": val_stats,
        "corpus_sha256": {"train": sha256(TRAIN_SOURCE), "validation": sha256(VAL_SOURCE), "manifest": sha256(MANIFEST)},
        "tokenizer_sha256": sha256(TOKENIZER),
        "roundtrip_samples": train_samples + val_samples,
        "intended_outputs": [str(path.relative_to(ROOT)) for path in (TRAIN_BIN, VAL_BIN, META)],
        "model_weights_loaded": False,
        "optimizer_constructed": False,
        "training_updates": 0,
    }

    if args.write:
        require(not AUDIT.exists() and not AUDIT.is_symlink(), "fresh tokenization audit directory")
        exclusive_binary_write(TRAIN_BIN, train)
        exclusive_binary_write(VAL_BIN, val)
        report["array_sha256"] = {"train": sha256(TRAIN_BIN), "validation": sha256(VAL_BIN)}
        exclusive_text_write(META, json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
        read_train = np.memmap(TRAIN_BIN, dtype=np.uint16, mode="r")
        read_val = np.memmap(VAL_BIN, dtype=np.uint16, mode="r")
        require(np.array_equal(read_train, train), "train exact read-back")
        require(np.array_equal(read_val, val), "validation exact read-back")
        require(sha256(TRAIN_SOURCE) == report["corpus_sha256"]["train"], "train source unchanged")
        require(sha256(VAL_SOURCE) == report["corpus_sha256"]["validation"], "validation source unchanged")
        require(sha256(MANIFEST) == report["corpus_sha256"]["manifest"], "manifest unchanged")
        require(sha256(TOKENIZER) == report["tokenizer_sha256"], "tokenizer unchanged")
        AUDIT.mkdir(parents=True)
        exclusive_text_write(AUDIT / "summary.json", json.dumps(report, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
        exclusive_text_write(AUDIT / "REPORT.md", (
            "# Friend Base chat corpus v2 tokenization\n\n"
            "**TOKENIZATION READY.** The frozen replacement chat corpus was serialized as a continuous uint16 full-LM stream.\n\n"
            f"- Train rows: {train_stats['rows']:,}\n- Train stream tokens: {train_stats['stream_tokens']:,}\n"
            f"- Validation rows: {val_stats['rows']:,}\n- Validation stream tokens: {val_stats['stream_tokens']:,}\n"
            "- Truncated rows: 0\n- Unknown-token rows: 0\n- Training updates: 0\n"
        ))
        paths = [TRAIN_BIN, VAL_BIN, META, AUDIT / "summary.json", AUDIT / "REPORT.md"]
        exclusive_text_write(AUDIT / "SHA256SUMS.txt", "".join(f"{sha256(path)}  {path.relative_to(ROOT)}\n" for path in paths))

    print(json.dumps({
        "classification": report["classification"],
        "train": train_stats,
        "validation": val_stats,
        "corpus_sha256": report["corpus_sha256"],
        "tokenizer_sha256": report["tokenizer_sha256"],
        "intended_outputs": report["intended_outputs"],
    }, indent=2, sort_keys=True))
    print("Tokenizer arrays generated: " + ("YES" if args.write else "NO"))
    print("Files written: " + ("YES" if args.write else "NONE"))
    print("Training updates: 0")
    print("Optimizer constructed: NO")
    print("Model weights loaded: NO")


if __name__ == "__main__":
    main()
