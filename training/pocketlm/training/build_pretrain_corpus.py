from pathlib import Path
import random
import re

from datasets import load_dataset


                                                              
               
                                                              

SEED = 42

ROOT = Path(__file__).resolve().parents[2]

OLD_TINYSTORIES = (
    ROOT
    / "data"
    / "tokenizer_corpus.txt"
)

OUTPUT_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "pretrain"
)

TRAIN_PATH = (
    OUTPUT_DIR
    / "pretrain_train.txt"
)

VAL_PATH = (
    OUTPUT_DIR
    / "pretrain_val.txt"
)


                              
 
                                            
 
               
                              
 
                                         
                                

TRAIN_TARGET_MIB = 500
VAL_TARGET_MIB = 8

TRAIN_TARGET_BYTES = (
    TRAIN_TARGET_MIB
    * 1024
    * 1024
)

VAL_TARGET_BYTES = (
    VAL_TARGET_MIB
    * 1024
    * 1024
)


                                              
                                          
TINYSTORIES_TRAIN_MIB = 25
TINYSTORIES_VAL_MIB = 1

TINYSTORIES_TRAIN_BYTES = (
    TINYSTORIES_TRAIN_MIB
    * 1024
    * 1024
)

TINYSTORIES_VAL_BYTES = (
    TINYSTORIES_VAL_MIB
    * 1024
    * 1024
)


                                                              
               
                                                              

def clean_text(text: str) -> str:
    if not text:
        return ""

    text = str(text)

    text = text.replace(
        "\x00",
        " ",
    )

    text = text.replace(
        "\r\n",
        "\n",
    )

    text = text.replace(
        "\r",
        "\n",
    )

                                      
    text = re.sub(
        r"[ \t]+",
        " ",
        text,
    )

                                  
    text = re.sub(
        r"\n{3,}",
        "\n\n",
        text,
    )

    return text.strip()


def byte_len(text: str) -> int:
    return len(
        text.encode(
            "utf-8"
        )
    )


def write_document(
    out,
    text: str,
) -> int:
    text = clean_text(text)

    if not text:
        return 0

    text += "\n\n"

    out.write(text)

    return byte_len(text)


                                                              
                             
                                                              

def looks_reasonable(text: str) -> bool:
    if not text:
        return False

                                               
    if len(text) < 300:
        return False

                                      
    if len(text) > 30_000:
        text = text[:30_000]

    chars = len(text)

    if chars == 0:
        return False

    alpha = sum(
        ch.isalpha()
        for ch in text
    )

    printable = sum(
        ch.isprintable()
        or ch in "\n\t"
        for ch in text
    )

    alpha_ratio = (
        alpha / chars
    )

    printable_ratio = (
        printable / chars
    )

                                           
                         
    if alpha_ratio < 0.45:
        return False

    if printable_ratio < 0.98:
        return False

                                              
    if re.search(
        r"(.)\1{40,}",
        text,
    ):
        return False

    return True


                                                              
             
                                                              

def load_tinystories():
    if not OLD_TINYSTORIES.exists():
        print(
            "TinyStories corpus missing."
        )

        return []

    print(
        "Loading local TinyStories..."
    )

    raw = OLD_TINYSTORIES.read_text(
        encoding="utf-8",
        errors="ignore",
    )

    paragraphs = [
        clean_text(x)
        for x in raw.split(
            "\n\n"
        )
    ]

    paragraphs = [
        x
        for x in paragraphs
        if len(x) >= 80
    ]

    rng = random.Random(
        SEED
    )

    rng.shuffle(
        paragraphs
    )

    print(
        f"  paragraphs: "
        f"{len(paragraphs):,}"
    )

    return paragraphs


def write_tinystories_split(
    paragraphs,
    train_out,
    val_out,
):
    train_written = 0
    val_written = 0

    index = 0

                               
                                     
    while (
        index < len(paragraphs)
        and val_written
        < TINYSTORIES_VAL_BYTES
    ):
        val_written += (
            write_document(
                val_out,
                paragraphs[index],
            )
        )

        index += 1

    while (
        index < len(paragraphs)
        and train_written
        < TINYSTORIES_TRAIN_BYTES
    ):
        train_written += (
            write_document(
                train_out,
                paragraphs[index],
            )
        )

        index += 1

    print()
    print("TinyStories")
    print("-" * 50)

    print(
        f"  train: "
        f"{train_written / 1024**2:.2f} MiB"
    )

    print(
        f"  val:   "
        f"{val_written / 1024**2:.2f} MiB"
    )

    return (
        train_written,
        val_written,
    )


                                                              
             
                                                              

def add_fineweb(
    train_out,
    val_out,
    train_written,
    val_written,
):
    print()
    print(
        "Streaming FineWeb-Edu..."
    )

    dataset = load_dataset(
        "HuggingFaceFW/fineweb-edu",
        split="train",
        streaming=True,
    )

                                          
                                          
    dataset = dataset.shuffle(
        seed=SEED,
        buffer_size=20_000,
    )

    accepted = 0
    rejected = 0

                                               
                                         
    rng = random.Random(
        SEED + 100
    )

    for row in dataset:
        if (
            train_written
            >= TRAIN_TARGET_BYTES
            and val_written
            >= VAL_TARGET_BYTES
        ):
            break

        text = clean_text(
            row.get(
                "text",
                "",
            )
        )

        if not looks_reasonable(
            text
        ):
            rejected += 1
            continue

                                         
        if len(text) > 30_000:
            text = text[:30_000]

                                     
                                      
                                   
        use_for_val = (
            val_written
            < VAL_TARGET_BYTES
            and rng.random()
            < 0.02
        )

        if use_for_val:
            val_written += (
                write_document(
                    val_out,
                    text,
                )
            )

        elif (
            train_written
            < TRAIN_TARGET_BYTES
        ):
            train_written += (
                write_document(
                    train_out,
                    text,
                )
            )

        accepted += 1

        if (
            accepted % 10_000
            == 0
        ):
            print(
                f"  accepted={accepted:,} "
                f"rejected={rejected:,} "
                f"train="
                f"{train_written / 1024**2:.1f} MiB "
                f"val="
                f"{val_written / 1024**2:.1f} MiB"
            )

    print()
    print("FineWeb-Edu complete")
    print("-" * 50)

    print(
        f"  accepted docs: "
        f"{accepted:,}"
    )

    print(
        f"  rejected docs: "
        f"{rejected:,}"
    )

    return (
        train_written,
        val_written,
    )


                                                              
      
                                                              

def main():
    random.seed(
        SEED
    )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    print()
    print("=" * 65)
    print(
        "PocketLM-S3 Stage-1 "
        "Pretraining Corpus Builder"
    )
    print("=" * 65)

    print()
    print(
        f"Train target: "
        f"{TRAIN_TARGET_MIB} MiB"
    )

    print(
        f"Validation target: "
        f"{VAL_TARGET_MIB} MiB"
    )

    print()
    print(
        "Stage 1 contains GENERAL LANGUAGE only."
    )

    print(
        "OpenAssistant / Dolly / GSM8K "
        "are reserved for later stages."
    )

                                                              
                      
                                                              

    tinystories = (
        load_tinystories()
    )

                                                              
                   
                                                              

    with (
        TRAIN_PATH.open(
            "w",
            encoding="utf-8",
        )
        as train_out,
        VAL_PATH.open(
            "w",
            encoding="utf-8",
        )
        as val_out,
    ):

        (
            train_written,
            val_written,
        ) = (
            write_tinystories_split(
                tinystories,
                train_out,
                val_out,
            )
        )

        (
            train_written,
            val_written,
        ) = add_fineweb(
            train_out,
            val_out,
            train_written,
            val_written,
        )

                                                              
                 
                                                              

    actual_train = (
        TRAIN_PATH.stat().st_size
    )

    actual_val = (
        VAL_PATH.stat().st_size
    )

    print()
    print("=" * 65)
    print(
        "PRETRAIN CORPUS COMPLETE"
    )
    print("=" * 65)

    print(
        f"Train: "
        f"{actual_train / 1024**2:.2f} MiB"
    )

    print(
        f"Validation: "
        f"{actual_val / 1024**2:.2f} MiB"
    )

    print()

                                             
                        
    estimated_train_tokens = (
        actual_train / 3.54
    )

    estimated_val_tokens = (
        actual_val / 3.54
    )

    print(
        "ROUGH token estimate:"
    )

    print(
        f"  train ≈ "
        f"{estimated_train_tokens / 1e6:.1f}M tokens"
    )

    print(
        f"  val   ≈ "
        f"{estimated_val_tokens / 1e6:.1f}M tokens"
    )

    print()
    print(
        "Saved:"
    )

    print(
        f"  {TRAIN_PATH}"
    )

    print(
        f"  {VAL_PATH}"
    )

    print()


if __name__ == "__main__":
    main()