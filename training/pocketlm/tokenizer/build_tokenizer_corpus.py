from pathlib import Path
import random
import re

from datasets import load_dataset


SEED = 42

ROOT = Path(__file__).resolve().parents[2]
OLD_CORPUS = ROOT / "data" / "tokenizer_corpus.txt"
OUTPUT = ROOT / "data" / "pocketlm" / "tokenizer_corpus.txt"

TARGETS = {
    "tinystories": 15 * 1024 * 1024,
    "fineweb":     35 * 1024 * 1024,
    "oasst":       20 * 1024 * 1024,
    "dolly":       10 * 1024 * 1024,
    "gsm8k":        5 * 1024 * 1024,
}


def clean_text(text):
    if not text:
        return ""

    text = str(text)

    text = text.replace("\x00", " ")
    text = text.replace("\r\n", "\n")
    text = text.replace("\r", "\n")

                                     
    text = re.sub(r"[ \t]+", " ", text)

                                        
    text = re.sub(r"\n{3,}", "\n\n", text)

    return text.strip()


def byte_len(text):
    return len(text.encode("utf-8"))


def write_piece(out, text):
    text = clean_text(text)

    if not text:
        return 0

    text += "\n\n"

    out.write(text)

    return byte_len(text)


                                                              
                                
                                                              

def add_local_tinystories(out, target_bytes):
    print("\n[1/5] Local TinyStories")

    if not OLD_CORPUS.exists():
        print("  old corpus missing -- skipped")
        return 0

    rng = random.Random(SEED)

                                                
                                
    text = OLD_CORPUS.read_text(
        encoding="utf-8",
        errors="ignore",
    )

    paragraphs = [
        clean_text(x)
        for x in text.split("\n\n")
        if clean_text(x)
    ]

    rng.shuffle(paragraphs)

    written = 0

    for paragraph in paragraphs:
        written += write_piece(
            out,
            paragraph,
        )

        if written >= target_bytes:
            break

    print(
        f"  wrote {written / 1024**2:.2f} MiB"
    )

    return written


                                                              
             
                                                              

def add_fineweb(out, target_bytes):
    print("\n[2/5] FineWeb-Edu")

    dataset = load_dataset(
        "HuggingFaceFW/fineweb-edu",
        split="train",
        streaming=True,
    )

    dataset = dataset.shuffle(
        seed=SEED,
        buffer_size=10_000,
    )

    written = 0
    examples = 0

    for row in dataset:
        text = clean_text(
            row.get("text", "")
        )

                                        
        if len(text) < 200:
            continue

                                        
                                        
        if len(text) > 12_000:
            text = text[:12_000]

        written += write_piece(
            out,
            text,
        )

        examples += 1

        if written >= target_bytes:
            break

    print(
        f"  examples: {examples:,}"
    )
    print(
        f"  wrote {written / 1024**2:.2f} MiB"
    )

    return written


                                                              
               
                                                              

def add_oasst(out, target_bytes):
    print("\n[3/5] OpenAssistant")

    dataset = load_dataset(
        "OpenAssistant/oasst1",
        split="train",
    )

    rows = []

    for row in dataset:
        if row.get("lang") != "en":
            continue

        text = clean_text(
            row.get("text", "")
        )

        if not text:
            continue

        role = row.get("role", "")

        if role == "prompter":
            formatted = (
                "<|user|>\n"
                + text
            )

        elif role == "assistant":
            formatted = (
                "<|assistant|>\n"
                + text
            )

        else:
            formatted = text

        rows.append(formatted)

    rng = random.Random(SEED)
    rng.shuffle(rows)

    written = 0

    for text in rows:
        written += write_piece(
            out,
            text,
        )

        if written >= target_bytes:
            break

    print(
        f"  English messages: {len(rows):,}"
    )
    print(
        f"  wrote {written / 1024**2:.2f} MiB"
    )

    return written


                                                              
       
                                                              

def add_dolly(out, target_bytes):
    print("\n[4/5] Dolly")

    dataset = load_dataset(
        "databricks/databricks-dolly-15k",
        split="train",
    )

    rows = []

    for row in dataset:
        instruction = clean_text(
            row.get("instruction", "")
        )

        context = clean_text(
            row.get("context", "")
        )

        response = clean_text(
            row.get("response", "")
        )

        if not instruction or not response:
            continue

        parts = [
            "<|user|>",
            instruction,
        ]

        if context:
            parts.extend([
                "",
                "Context:",
                context,
            ])

        parts.extend([
            "",
            "<|assistant|>",
            response,
        ])

        rows.append(
            "\n".join(parts)
        )

    rng = random.Random(SEED)
    rng.shuffle(rows)

    written = 0

    for text in rows:
        written += write_piece(
            out,
            text,
        )

        if written >= target_bytes:
            break

    print(
        f"  examples: {len(rows):,}"
    )
    print(
        f"  wrote {written / 1024**2:.2f} MiB"
    )

    return written


                                                              
       
                                                              

def add_gsm8k(out, target_bytes):
    print("\n[5/5] GSM8K")

    dataset = load_dataset(
        "openai/gsm8k",
        "main",
        split="train",
    )

    rows = []

    for row in dataset:
        question = clean_text(
            row.get("question", "")
        )

        answer = clean_text(
            row.get("answer", "")
        )

        if not question or not answer:
            continue

        text = (
            "<|user|>\n"
            + question
            + "\n\n"
            + "<|assistant|>\n"
            + answer
        )

        rows.append(text)

    rng = random.Random(SEED)
    rng.shuffle(rows)

    written = 0

                                          
                                         
    for text in rows:
        written += write_piece(
            out,
            text,
        )

        if written >= target_bytes:
            break

    print(
        f"  examples: {len(rows):,}"
    )
    print(
        f"  wrote {written / 1024**2:.2f} MiB"
    )

    return written


                                                              
      
                                                              

def main():
    random.seed(SEED)

    OUTPUT.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    print("=" * 60)
    print("PocketTokenizer Corpus Builder")
    print("=" * 60)

    totals = {}

    with OUTPUT.open(
        "w",
        encoding="utf-8",
    ) as out:

        totals["tinystories"] = (
            add_local_tinystories(
                out,
                TARGETS["tinystories"],
            )
        )

        totals["fineweb"] = (
            add_fineweb(
                out,
                TARGETS["fineweb"],
            )
        )

        totals["oasst"] = (
            add_oasst(
                out,
                TARGETS["oasst"],
            )
        )

        totals["dolly"] = (
            add_dolly(
                out,
                TARGETS["dolly"],
            )
        )

        totals["gsm8k"] = (
            add_gsm8k(
                out,
                TARGETS["gsm8k"],
            )
        )

    total = sum(
        totals.values()
    )

    print()
    print("=" * 60)
    print("CORPUS COMPLETE")
    print("=" * 60)

    for name, size in totals.items():
        print(
            f"{name:12s}: "
            f"{size / 1024**2:7.2f} MiB"
        )

    print("-" * 60)

    print(
        f"{'TOTAL':12s}: "
        f"{total / 1024**2:7.2f} MiB"
    )

    print()
    print(f"Saved to:")
    print(OUTPUT)


if __name__ == "__main__":
    main()