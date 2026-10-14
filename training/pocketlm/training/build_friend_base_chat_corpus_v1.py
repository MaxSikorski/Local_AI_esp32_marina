"""Build a frozen, source-pinned conversational corpus for Friend Base research.

The first invocation must use ``--dry-run``.  Dry-run performs the full source
selection, filtering, deduplication, tokenization, and audit in memory and writes
nothing.  ``--write`` is deliberately separate and refuses to overwrite files.

This fixes the material limitation in ``build_friend_base_v1.py``: that builder
selected SODA's prose ``narrative`` field.  Here SODA contributes actual adjacent
turns from its ``dialogue`` and ``speakers`` fields.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import statistics
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

import sentencepiece as spm
from datasets import load_dataset


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data/pocketlm/friend_base_chat_v1"
AUDIT = ROOT / "checkpoints/_audit_friend_base_chat_corpus_v1"
TRAIN_OUT = OUT / "friend_base_chat_v1_train.jsonl"
VAL_OUT = OUT / "friend_base_chat_v1_val.jsonl"
MANIFEST_OUT = OUT / "friend_base_chat_v1_manifest.json"

TOKENIZER = ROOT / "data/pocketlm/tokenizers/spm_6144/tokenizer.model"
PRECONDITION = ROOT / "data/pocketlm/friend_precondition_v1/friend_precondition_v1_frozen_v1.jsonl"
PRECONDITION_MANIFEST = ROOT / "data/pocketlm/friend_precondition_v1/friend_precondition_v1_frozen_v1_manifest.json"
V4_DIR = ROOT / "data/pocketlm/friend_distill_pilot_v4"
V4_META = V4_DIR / "friend_v4_tokenized_meta.json"
V4_VALIDATION = V4_DIR / "friend_v4_validation_frozen_v1.jsonl"

SEED = 42
MAX_SEQ_LEN = 160
VOCAB_SIZE = 6144
PAD, UNK, BOS, EOS, USER, ASSISTANT, END = 0, 1, 2, 3, 4, 5, 9

SODA_DATASET = "allenai/soda"
SODA_REVISION = "fdc848ab0183208ea7808206c91c724414d0a071"
OASST_DATASET = "OpenAssistant/oasst1"
OASST_REVISION = "fdf72ae0827c1cda404aff25b6603abec9e3399b"

                                                                              
                                                                           
TRAIN_QUOTAS = {"local_friend": 400, "oasst1": 800, "soda": 58_800}
VAL_QUOTAS = {"oasst1": 50, "soda": 1_950}
TRAIN_ROWS = sum(TRAIN_QUOTAS.values())
VAL_ROWS = sum(VAL_QUOTAS.values())

EXPECTED_SHA = {
    "tokenizer": "aaa73c697ca35198fc621d6d33060c16991b7a5426b7719f3cffdad8ddb08799",
    "precondition": "97134b48118e257908b3165e2adb61f1f1978e2f603fe04ffdaf969b5bc5085a",
    "precondition_manifest": "b0742b26f39e40fca9091f78b4b0fba6be8dafd78fa4dddd0b92459bf5d30ee2",
    "v4_validation": "237f335a6a60684ea9ca9a765c6e06c134baa517d164be11cc7d865f31bbed04",
}

RESERVED = ("<|user|>", "<|assistant|>", "<|system|>", "<|memory|>", "<|end_memory|>", "<|end|>")
URL_RE = re.compile(r"(?:https?://|www\.)", re.I)
EMAIL_RE = re.compile(r"\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b", re.I)
PHONE_RE = re.compile(r"(?<!\d)(?:\+?\d[\s().-]*){8,}\d(?!\d)")
LIST_RE = re.compile(r"(?:^|\n)\s*(?:[-*]|\d+[.)])\s+", re.M)


@dataclass(frozen=True)
class Example:
    id: str
    source: str
    source_split: str
    source_id: str
    prompt: str
    answer: str
    prompt_tokens: int
    answer_tokens: int
    sequence_tokens: int
    license: str


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError("contract failure: " + message)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def stable_hash(text: str) -> str:
    return hashlib.sha256((str(SEED) + "\0" + text).encode("utf-8")).hexdigest()


def clean(text: object) -> str:
    value = str(text or "").replace("\x00", " ").replace("\r", "\n")
    return re.sub(r"\s+", " ", value).strip()


def normalized(text: str) -> str:
    text = text.lower().replace("’", "'").replace("“", '"').replace("”", '"')
    text = re.sub(r"[^a-z0-9']+", " ", text)
    return re.sub(r"\s+", " ", text).strip()


def word_ngrams(text: str, n: int = 3) -> set[tuple[str, ...]]:
    words = normalized(text).split()
    if len(words) < n:
        return {tuple(words)} if words else set()
    return {tuple(words[i:i + n]) for i in range(len(words) - n + 1)}


def jaccard(a: set, b: set) -> float:
    return len(a & b) / len(a | b) if a and b else 0.0


def text_rejection(prompt: str, answer: str) -> str | None:
    if not prompt or not answer:
        return "empty"
                                                                          
                                                                              
    combined = prompt + " " + answer
    lowered = combined.lower()
    if any(marker in lowered for marker in RESERVED):
        return "reserved_marker"
    if "```" in combined or "~~~" in combined:
        return "code_fence"
    if URL_RE.search(combined):
        return "url"
    if EMAIL_RE.search(combined):
        return "email"
    if PHONE_RE.search(combined):
        return "phone_like"
    if LIST_RE.search(combined):
        return "list_format"
    if re.search(r"(.)\1{12,}", combined):
        return "repeated_character"
    if sum(ch.isalpha() for ch in combined) / max(1, len(combined)) < 0.55:
        return "low_alpha"
    if sum(ch.isprintable() for ch in combined) / max(1, len(combined)) < 0.995:
        return "non_printable"
                                                                           
                                                                     
    if re.search(r"\b(?:suicide instructions|make a bomb|child porn(?:ography)?|rape fantasy)\b", lowered):
        return "unsafe_phrase"
    return None


def tokenize_candidate(tokenizer, prompt: str, answer: str, *, min_answer_tokens: int = 3) -> tuple[int, int, int] | None:
    p = tokenizer.encode(prompt, out_type=int, add_bos=False, add_eos=False)
    a = tokenizer.encode(answer, out_type=int, add_bos=False, add_eos=False)
    if not p or not a or UNK in p or UNK in a:
        return None
                                                             
    total = 4 + len(p) + len(a)
    if not (2 <= len(p) <= 90 and min_answer_tokens <= len(a) <= 100 and total <= MAX_SEQ_LEN):
        return None
    return len(p), len(a), total


def load_jsonl(path: Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def load_v4_reference() -> tuple[list[dict], dict]:
    meta = json.loads(V4_META.read_text(encoding="utf-8"))
    require(meta["tokenizer_sha256"] == EXPECTED_SHA["tokenizer"], "V4 tokenizer binding")
    require(meta["source_binding"]["friend_v4_validation_frozen_v1_sha256"] == EXPECTED_SHA["v4_validation"], "V4 validation metadata binding")
    train = []
    actual = {}
    for relative, expected in sorted(meta["source_binding"]["train_source_files_sha256"].items()):
        path = ROOT / relative
        actual[relative] = sha256(path)
        require(actual[relative] == expected, "V4 train source SHA " + relative)
        train.extend(load_jsonl(path))
    validation = load_jsonl(V4_VALIDATION)
    require(len(train) == 1100 and len(validation) == 180, "V4 reference row counts")
    return train + validation, {
        "v4_train_rows": len(train),
        "v4_validation_rows": len(validation),
        "v4_train_source_sha256": actual,
        "v4_validation_sha256": sha256(V4_VALIDATION),
        "v4_tokenized_meta_sha256": sha256(V4_META),
    }


class Collector:
    def __init__(self, tokenizer, references: list[dict]):
        self.tokenizer = tokenizer
        self.references = references
        self.reference_norm_prompt = {normalized(r["prompt"]) for r in references}
        self.reference_norm_answer = {normalized(r["answer"]) for r in references}
        self.reference_ngrams = [
            (r["id"], word_ngrams(r["prompt"]), word_ngrams(r["answer"]))
            for r in references
        ]
        self.ids: set[str] = set()
        self.prompts: set[str] = set()
        self.answers: set[str] = set()
        self.train: list[Example] = []
        self.val: list[Example] = []
        self.rejections: Counter[str] = Counter()
        self.near_reference_examples: list[dict] = []

    def add(self, *, split: str, id: str, source: str, source_split: str,
            source_id: str, prompt: object, answer: object, license: str,
            min_answer_tokens: int = 3) -> bool:
        prompt = clean(prompt)
        answer = clean(answer)
        reason = text_rejection(prompt, answer)
        if reason:
            self.rejections[source + ":" + reason] += 1
            return False
        lengths = tokenize_candidate(
            self.tokenizer, prompt, answer, min_answer_tokens=min_answer_tokens
        )
        if lengths is None:
            self.rejections[source + ":token_length_or_unk"] += 1
            return False
        nprompt, nanswer = normalized(prompt), normalized(answer)
        if id in self.ids or nprompt in self.prompts or nanswer in self.answers:
            self.rejections[source + ":global_exact_or_normalized_duplicate"] += 1
            return False
        if nprompt in self.reference_norm_prompt or nanswer in self.reference_norm_answer:
            self.rejections[source + ":v4_exact_or_normalized_overlap"] += 1
            return False
        pg, ag = word_ngrams(prompt), word_ngrams(answer)
        near = None
        for ref_id, rpg, rag in self.reference_ngrams:
                                                                               
                                                                                
            ps = jaccard(pg, rpg) if len(pg) >= 4 and len(rpg) >= 4 else 0.0
            ans = jaccard(ag, rag) if len(ag) >= 4 and len(rag) >= 4 else 0.0
            if max(ps, ans) >= 0.82:
                near = {"reference_id": ref_id, "prompt_jaccard": ps, "answer_jaccard": ans}
                break
        if near:
            self.rejections[source + ":v4_near_overlap"] += 1
            if len(self.near_reference_examples) < 20:
                self.near_reference_examples.append({"candidate_id": id, **near})
            return False
        ex = Example(id=id, source=source, source_split=source_split,
                     source_id=source_id, prompt=prompt, answer=answer,
                     prompt_tokens=lengths[0], answer_tokens=lengths[1],
                     sequence_tokens=lengths[2], license=license)
        (self.train if split == "train" else self.val).append(ex)
        self.ids.add(id)
        self.prompts.add(nprompt)
        self.answers.add(nanswer)
        return True


def add_local(collector: Collector) -> None:
    require(sha256(PRECONDITION) == EXPECTED_SHA["precondition"], "precondition SHA")
    require(sha256(PRECONDITION_MANIFEST) == EXPECTED_SHA["precondition_manifest"], "precondition manifest SHA")
    manifest = json.loads(PRECONDITION_MANIFEST.read_text(encoding="utf-8"))
    require(manifest["combined_sha256"] == EXPECTED_SHA["precondition"], "precondition manifest binding")
    rows = load_jsonl(PRECONDITION)
    require(len(rows) == TRAIN_QUOTAS["local_friend"] == 400, "local Friend quota")
    for row in rows:
        ok = collector.add(split="train", id="local_" + row["id"], source="local_friend",
                           source_split="frozen_train", source_id=row["id"], prompt=row["prompt"],
                           answer=row["answer"], license="repository-authored",
                           min_answer_tokens=1)
        require(ok, "frozen local row rejected: " + row["id"])


def label_map(row: dict) -> dict[str, float]:
    labels = row.get("labels") or {}
    return {str(k): float(v) for k, v in zip(labels.get("name") or [], labels.get("value") or [])}


def oasst_metadata_ok(row: dict) -> bool:
    if row.get("role") != "assistant" or row.get("lang") != "en":
        return False
    if row.get("deleted") or not row.get("review_result") or row.get("synthetic"):
        return False
    if row.get("rank") not in (0, None):
        return False
    toxic = row.get("detoxify") or {}
    if max((float(toxic.get(k) or 0.0) for k in (
        "toxicity", "severe_toxicity", "obscene", "identity_attack",
        "insult", "threat", "sexual_explicit")), default=0.0) > 0.20:
        return False
    labels = label_map(row)
    return labels.get("quality", 0.0) >= 0.50 and labels.get("helpfulness", 0.0) >= 0.50


def add_oasst_split(collector: Collector, split: str, quota: int) -> dict:
    dataset = load_dataset(OASST_DATASET, split=split, revision=OASST_REVISION)
    by_id = {row["message_id"]: row for row in dataset}
    scanned = 0
    accepted = 0
    for row in dataset:
        scanned += 1
        if not oasst_metadata_ok(row):
            collector.rejections["oasst1:metadata"] += 1
            continue
        parent = by_id.get(row.get("parent_id"))
        if not parent or parent.get("role") != "prompter" or parent.get("lang") != "en" or parent.get("deleted"):
            collector.rejections["oasst1:parent"] += 1
            continue
        target = "train" if split == "train" else "val"
        if collector.add(split=target, id="oasst1_" + row["message_id"], source="oasst1",
                         source_split=split, source_id=row["message_id"], prompt=parent["text"],
                         answer=row["text"], license="apache-2.0"):
            accepted += 1
            if accepted == quota:
                break
    require(accepted == quota, f"OASST {split} quota {accepted}/{quota}")
    return {"scanned": scanned, "accepted": accepted, "available_rows": len(dataset)}


def soda_pair(row: dict, tokenizer) -> tuple[int, str, str] | None:
    dialogue = row.get("dialogue") or []
    speakers = row.get("speakers") or []
    if len(dialogue) < 2 or len(dialogue) != len(speakers):
        return None
    candidates = [i for i in range(len(dialogue) - 1) if speakers[i] != speakers[i + 1]]
    if not candidates:
        return None
    source_id = str(row.get("original_index"))
    offset = int(stable_hash("soda-turn:" + source_id)[:16], 16) % len(candidates)
    for turn in candidates[offset:] + candidates[:offset]:
        prompt, answer = clean(dialogue[turn]), clean(dialogue[turn + 1])
        if text_rejection(prompt, answer) is None and tokenize_candidate(tokenizer, prompt, answer) is not None:
            return turn, prompt, answer
    return None


def add_soda(collector: Collector) -> dict:
    dataset = load_dataset(SODA_DATASET, split="train", streaming=True, revision=SODA_REVISION)
    wanted_train, wanted_val = TRAIN_QUOTAS["soda"], VAL_QUOTAS["soda"]
    scanned = accepted_train = accepted_val = 0
    for row in dataset:
        scanned += 1
        source_id = str(row.get("original_index"))
                                                                          
                                                                      
        bucket = int(stable_hash("soda-split:" + source_id)[:16], 16) % 1000
        split = "val" if bucket < 40 else "train"
        if split == "train" and accepted_train >= wanted_train:
            continue
        if split == "val" and accepted_val >= wanted_val:
            continue
        pair = soda_pair(row, collector.tokenizer)
        if pair is None:
            collector.rejections["soda:no_eligible_adjacent_pair"] += 1
            continue
        turn, prompt, answer = pair
        accepted = collector.add(
            split=split, id=f"soda_{source_id}_{turn:02d}", source="soda",
            source_split="stable_hash_holdout" if split == "val" else "stable_hash_train",
            source_id=source_id, prompt=prompt, answer=answer, license="cc-by-4.0")
        if accepted:
            if split == "train":
                accepted_train += 1
            else:
                accepted_val += 1
        if accepted_train == wanted_train and accepted_val == wanted_val:
            break
    require(accepted_train == wanted_train and accepted_val == wanted_val,
            f"SODA quotas train {accepted_train}/{wanted_train}, val {accepted_val}/{wanted_val}")
    return {"scanned": scanned, "accepted_train": accepted_train, "accepted_val": accepted_val}


def percentile(values: list[int], p: float) -> float:
    ordered = sorted(values)
    if not ordered:
        return 0.0
    at = (len(ordered) - 1) * p
    lo, hi = int(at), min(int(at) + 1, len(ordered) - 1)
    return float(ordered[lo] + (ordered[hi] - ordered[lo]) * (at - lo))


def stats(values: Iterable[int]) -> dict:
    values = list(values)
    return {
        "min": min(values), "max": max(values), "mean": statistics.mean(values),
        "median": statistics.median(values), "p90": percentile(values, 0.90),
        "p99": percentile(values, 0.99),
    }


def opening(answer: str) -> str:
    words = re.findall(r"[A-Za-z']+", answer)
    return " ".join(words[:2]).lower() if words else "<none>"


def canonical_line(example: Example) -> str:
    return json.dumps(asdict(example), sort_keys=True, ensure_ascii=False, separators=(",", ":")) + "\n"


def stream_digest(rows: list[Example]) -> str:
    digest = hashlib.sha256()
    for row in rows:
        digest.update(canonical_line(row).encode("utf-8"))
    return digest.hexdigest()


def audit_rows(rows: list[Example], name: str) -> dict:
    ids = [r.id for r in rows]
    prompts = [normalized(r.prompt) for r in rows]
    answers = [normalized(r.answer) for r in rows]
    require(len(ids) == len(set(ids)), name + " unique IDs")
    require(len(prompts) == len(set(prompts)), name + " unique normalized prompts")
    require(len(answers) == len(set(answers)), name + " unique normalized answers")
    return {
        "rows": len(rows),
        "source_counts": dict(sorted(Counter(r.source for r in rows).items())),
        "unique_ids": len(set(ids)), "unique_normalized_prompts": len(set(prompts)),
        "unique_normalized_answers": len(set(answers)),
        "prompt_token_stats": stats(r.prompt_tokens for r in rows),
        "answer_token_stats": stats(r.answer_tokens for r in rows),
        "sequence_token_stats": stats(r.sequence_tokens for r in rows),
        "question_prompts": sum(r.prompt.rstrip().endswith("?") for r in rows),
        "question_answers": sum(r.answer.rstrip().endswith("?") for r in rows),
        "top_openings": Counter(opening(r.answer) for r in rows).most_common(30),
        "canonical_sha256": stream_digest(rows),
    }


def deterministic_order(rows: list[Example], split: str) -> list[Example]:
    return sorted(rows, key=lambda row: (stable_hash(split + ":" + row.id), row.id))


def atomic_write(path: Path, text: str) -> None:
    require(not path.exists() and not path.is_symlink(), "refusing overwrite " + str(path))
    temporary = Path(str(path) + ".tmp")
    require(not temporary.exists(), "fresh temporary path " + str(temporary))
    with temporary.open("x", encoding="utf-8") as stream:
        stream.write(text)
        stream.flush()
        os.fsync(stream.fileno())
    os.link(temporary, path)
    temporary.unlink()


def write_outputs(train: list[Example], val: list[Example], manifest: dict, audit: dict) -> None:
    require(not OUT.exists() and not OUT.is_symlink(), "fresh corpus output directory")
    require(not AUDIT.exists() and not AUDIT.is_symlink(), "fresh audit directory")
    OUT.mkdir(parents=True)
    AUDIT.mkdir(parents=True)
    atomic_write(TRAIN_OUT, "".join(canonical_line(row) for row in train))
    atomic_write(VAL_OUT, "".join(canonical_line(row) for row in val))
    manifest = {**manifest, "files": {
        str(TRAIN_OUT.relative_to(ROOT)): sha256(TRAIN_OUT),
        str(VAL_OUT.relative_to(ROOT)): sha256(VAL_OUT),
    }}
    atomic_write(MANIFEST_OUT, json.dumps(manifest, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
    summary_path = AUDIT / "summary.json"
    atomic_write(summary_path, json.dumps(audit, indent=2, sort_keys=True, ensure_ascii=False) + "\n")
    report = (
        "# Friend Base conversational corpus v1\n\n"
        "**CORPUS READY.** The corpus contains real adjacent dialogue turns from the pinned SODA revision, "
        "high-quality English OASST1 parent/assistant pairs, and the frozen repository-authored Friend set. "
        "All examples fit the established 160-token `EOS USER prompt ASSISTANT answer END` full-LM document.\n\n"
        f"- Train rows: {len(train):,}\n- Validation rows: {len(val):,}\n"
        "- V4 reference exact/normalized/near overlaps admitted: 0\n"
        "- Training updates: 0\n- Model weights loaded: no\n"
    )
    atomic_write(AUDIT / "REPORT.md", report)
    paths = [TRAIN_OUT, VAL_OUT, MANIFEST_OUT, summary_path, AUDIT / "REPORT.md"]
    sums = "".join(f"{sha256(path)}  {path.relative_to(ROOT)}\n" for path in paths)
    atomic_write(AUDIT / "SHA256SUMS.txt", sums)
                                                               
    read_train, read_val = load_jsonl(TRAIN_OUT), load_jsonl(VAL_OUT)
    require(len(read_train) == TRAIN_ROWS and len(read_val) == VAL_ROWS, "read-back row counts")
    require(sha256(TRAIN_OUT) == manifest["files"][str(TRAIN_OUT.relative_to(ROOT))], "train read-back SHA")
    require(sha256(VAL_OUT) == manifest["files"][str(VAL_OUT.relative_to(ROOT))], "val read-back SHA")


def main() -> None:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--write", action="store_true")
    args = parser.parse_args()

    require(sha256(TOKENIZER) == EXPECTED_SHA["tokenizer"], "tokenizer SHA")
    tokenizer = spm.SentencePieceProcessor(model_file=str(TOKENIZER))
    require(tokenizer.get_piece_size() == VOCAB_SIZE, "vocab size")
    require([tokenizer.pad_id(), tokenizer.unk_id(), tokenizer.bos_id(), tokenizer.eos_id()] == [PAD, UNK, BOS, EOS], "base IDs")
    require([tokenizer.id_to_piece(i) for i in (USER, ASSISTANT, END)] == ["<|user|>", "<|assistant|>", "<|end|>"], "chat IDs")

    references, v4_binding = load_v4_reference()
    collector = Collector(tokenizer, references)
    add_local(collector)
    oasst_train = add_oasst_split(collector, "train", TRAIN_QUOTAS["oasst1"])
    oasst_val = add_oasst_split(collector, "validation", VAL_QUOTAS["oasst1"])
    soda = add_soda(collector)

    train = deterministic_order(collector.train, "train")
    val = deterministic_order(collector.val, "val")
    require(len(train) == TRAIN_ROWS and len(val) == VAL_ROWS, "final row counts")
    require(not ({r.source_id for r in train if r.source == "soda"} & {r.source_id for r in val if r.source == "soda"}), "SODA source-disjoint holdout")
    require(not ({normalized(r.prompt) for r in train} & {normalized(r.prompt) for r in val}), "train/val prompt separation")
    require(not ({normalized(r.answer) for r in train} & {normalized(r.answer) for r in val}), "train/val answer separation")

    train_audit, val_audit = audit_rows(train, "train"), audit_rows(val, "val")
    source_hashes = {
        "tokenizer": sha256(TOKENIZER), "precondition": sha256(PRECONDITION),
        "precondition_manifest": sha256(PRECONDITION_MANIFEST), **v4_binding,
    }
    manifest = {
        "version": "friend_base_chat_corpus_v1", "seed": SEED,
        "row_counts": {"train": len(train), "validation": len(val)},
        "quotas": {"train": TRAIN_QUOTAS, "validation": VAL_QUOTAS},
        "sources": {
            "soda": {"dataset": SODA_DATASET, "revision": SODA_REVISION, "license": "cc-by-4.0", "field": "adjacent dialogue turns", "selection": soda},
            "oasst1": {"dataset": OASST_DATASET, "revision": OASST_REVISION, "license": "apache-2.0", "train_selection": oasst_train, "validation_selection": oasst_val},
            "local_friend": {"license": "repository-authored", "rows": TRAIN_QUOTAS["local_friend"]},
        },
        "tokenization_contract": {"serialization": "EOS USER prompt ASSISTANT answer END", "max_sequence_length": MAX_SEQ_LEN, "vocab_size": VOCAB_SIZE, "special_ids": {"PAD": PAD, "UNK": UNK, "BOS": BOS, "EOS": EOS, "USER": USER, "ASSISTANT": ASSISTANT, "END": END}},
        "source_sha256": source_hashes,
        "canonical_content_sha256": {"train": train_audit["canonical_sha256"], "validation": val_audit["canonical_sha256"]},
        "validation_policy": "OASST official validation plus stable 4% dialogue-level SODA hash holdout; V4 validation excluded and used only as a leakage reference.",
        "training_updates": 0,
    }
    audit = {
        "classification": "DRY_RUN_PASS" if args.dry_run else "CORPUS_READY",
        "train": train_audit, "validation": val_audit,
        "rejections": dict(sorted(collector.rejections.items())),
        "v4_near_overlap_rejections": collector.near_reference_examples,
        "global_unique_ids": len(collector.ids) == TRAIN_ROWS + VAL_ROWS,
        "global_unique_normalized_prompts": len(collector.prompts) == TRAIN_ROWS + VAL_ROWS,
        "global_unique_normalized_answers": len(collector.answers) == TRAIN_ROWS + VAL_ROWS,
        "truncated_rows": 0, "unknown_token_rows": 0,
        "source_sha256": source_hashes,
        "intended_outputs": [str(p.relative_to(ROOT)) for p in (TRAIN_OUT, VAL_OUT, MANIFEST_OUT)],
        "samples": [asdict(r) for r in train[:10] + val[:5]],
        "training_updates": 0, "optimizer_constructed": False, "model_weights_loaded": False,
    }

    print(json.dumps({
        "mode": "DRY_RUN" if args.dry_run else "WRITE",
        "train": train_audit, "validation": val_audit,
        "rejections": dict(sorted(collector.rejections.items())),
        "source_sha256": source_hashes,
        "intended_outputs": audit["intended_outputs"],
        "serialization_examples": audit["samples"][:5],
    }, indent=2, ensure_ascii=False))
    if args.write:
        write_outputs(train, val, manifest, audit)
        print("Files written: YES")
    else:
        print("Files written: NONE")
    print("Models loaded: NONE")
    print("Optimizer constructed: NO")
    print("Training: NOT RUN")


if __name__ == "__main__":
    main()
