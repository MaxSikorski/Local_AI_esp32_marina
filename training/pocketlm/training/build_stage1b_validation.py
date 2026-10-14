from pathlib import Path
import json
import re
import time

from datasets import load_dataset


                                                              
       
                                                              

ROOT = Path(__file__).resolve().parents[2]

OUTPUT_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "pretrain_stage1b"
)

VAL_PATH = (
    OUTPUT_DIR
    / "stage1b_fresh_val.txt"
)

META_PATH = (
    OUTPUT_DIR
    / "stage1b_fresh_val_meta.json"
)


                                                              
                 
                                                              

SEED = 42

SHUFFLE_BUFFER_SIZE = 20_000

                              
 
                                      
 
                                 
 
                                      
 
                                                
                          

STAGE1A_ACCEPTED_DOCS = 109_599
STAGE1B_ACCEPTED_DOCS = 117_085

TOTAL_ACCEPTED_DOCS_TO_SKIP = (
    STAGE1A_ACCEPTED_DOCS
    + STAGE1B_ACCEPTED_DOCS
)


                                                              
                   
                                                              

VAL_TARGET_MIB = 8

VAL_TARGET_BYTES = (
    VAL_TARGET_MIB
    * 1024
    * 1024
)

MAX_DOCUMENT_CHARS = 30_000


                                                              
          
                                                              

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


def looks_reasonable(text: str) -> bool:
    """
    Keep this filter identical to Stage 1A / 1B.
    """

    if not text:
        return False

    if len(text) < 300:
        return False

    if len(text) > MAX_DOCUMENT_CHARS:
        text = text[
            :MAX_DOCUMENT_CHARS
        ]

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


def utf8_size(text: str) -> int:
    return len(
        text.encode(
            "utf-8"
        )
    )


def write_document(
    out_file,
    text: str,
) -> int:
    text = clean_text(
        text
    )

    if not text:
        return 0

    if len(text) > MAX_DOCUMENT_CHARS:
        text = text[
            :MAX_DOCUMENT_CHARS
        ]

    text += "\n\n"

    out_file.write(
        text
    )

    return utf8_size(
        text
    )


                                                              
         
                                                              

def build_validation():
    print()
    print("=" * 72)
    print(
        "PocketLM-S3 Fresh Stage-1B Validation Builder"
    )
    print("=" * 72)

    print()
    print(
        "Source: FineWeb-Edu"
    )

    print(
        f"Validation target: "
        f"{VAL_TARGET_MIB} MiB"
    )

    print()
    print(
        "Skipping all FineWeb documents already "
        "reserved for Stage 1A + Stage 1B..."
    )

    print(
        f"Accepted docs to skip: "
        f"{TOTAL_ACCEPTED_DOCS_TO_SKIP:,}"
    )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    dataset = load_dataset(
        "HuggingFaceFW/fineweb-edu",
        split="train",
        streaming=True,
    )

    dataset = dataset.shuffle(
        seed=SEED,
        buffer_size=SHUFFLE_BUFFER_SIZE,
    )

    accepted_seen = 0
    rejected_seen = 0

    val_docs = 0
    val_bytes = 0

    start_time = time.time()

    with VAL_PATH.open(
        "w",
        encoding="utf-8",
    ) as out_file:

        for row in dataset:
            text = clean_text(
                row.get(
                    "text",
                    "",
                )
            )

            if not looks_reasonable(
                text
            ):
                rejected_seen += 1
                continue

            if len(text) > MAX_DOCUMENT_CHARS:
                text = text[
                    :MAX_DOCUMENT_CHARS
                ]

                                                             
                                                         
                                                             

            if (
                accepted_seen
                <
                TOTAL_ACCEPTED_DOCS_TO_SKIP
            ):
                accepted_seen += 1

                if (
                    accepted_seen
                    % 25_000
                    == 0
                ):
                    print(
                        f"  replayed "
                        f"{accepted_seen:,}"
                        "/"
                        f"{TOTAL_ACCEPTED_DOCS_TO_SKIP:,}"
                        " accepted docs"
                    )

                continue

                                                             
                                                           
                                                             

            written = write_document(
                out_file,
                text,
            )

            if written == 0:
                continue

            val_bytes += (
                written
            )

            val_docs += 1
            accepted_seen += 1

            if (
                val_docs
                % 500
                == 0
            ):
                print(
                    f"  fresh val docs="
                    f"{val_docs:,} | "
                    f"{val_bytes / 1024**2:.2f} MiB"
                )

            if (
                val_bytes
                >= VAL_TARGET_BYTES
            ):
                break

    elapsed = (
        time.time()
        - start_time
    )

    return {
        "source":
            "HuggingFaceFW/fineweb-edu",

        "seed":
            SEED,

        "shuffle_buffer_size":
            SHUFFLE_BUFFER_SIZE,

        "stage1a_accepted_docs":
            STAGE1A_ACCEPTED_DOCS,

        "stage1b_accepted_docs":
            STAGE1B_ACCEPTED_DOCS,

        "accepted_docs_skipped":
            TOTAL_ACCEPTED_DOCS_TO_SKIP,

        "fresh_validation_docs":
            val_docs,

        "fresh_validation_bytes":
            val_bytes,

        "fresh_validation_mib":
            val_bytes / 1024**2,

        "stream_position_after_val":
            accepted_seen,

        "rejected_documents_seen":
            rejected_seen,

        "seconds":
            elapsed,

        "output_file":
            str(VAL_PATH),
    }


                                                              
      
                                                              

def main():
    stats = build_validation()

    actual_bytes = (
        VAL_PATH.stat().st_size
    )

    if (
        actual_bytes
        != stats[
            "fresh_validation_bytes"
        ]
    ):
        raise RuntimeError(
            "Validation file size mismatch."
        )

                                           
                                    

    rough_tokens = (
        actual_bytes
        / 3.828
    )

    stats[
        "rough_token_estimate"
    ] = rough_tokens

    with META_PATH.open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            stats,
            f,
            indent=2,
        )

    print()
    print("=" * 72)
    print(
        "FRESH VALIDATION COMPLETE"
    )
    print("=" * 72)

    print()
    print(
        f"Docs skipped first: "
        f"{TOTAL_ACCEPTED_DOCS_TO_SKIP:,}"
    )

    print(
        f"Fresh validation docs: "
        f"{stats['fresh_validation_docs']:,}"
    )

    print(
        f"Validation size: "
        f"{actual_bytes / 1024**2:.2f} MiB"
    )

    print(
        f"Rough tokens: "
        f"{rough_tokens / 1e6:.2f}M"
    )

    print(
        f"FineWeb stream now at: "
        f"{stats['stream_position_after_val']:,}"
    )

    print()
    print(
        "This validation data was NOT used by:"
    )

    print(
        "  Stage 1A ✅"
    )

    print(
        "  Stage 1B training corpus ✅"
    )

    print()
    print(
        "Saved:"
    )

    print(
        f"  {VAL_PATH}"
    )

    print(
        f"  {META_PATH}"
    )

    print()


if __name__ == "__main__":
    main()