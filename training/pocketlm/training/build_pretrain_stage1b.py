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

TRAIN_PATH = (
    OUTPUT_DIR
    / "stage1b_train.txt"
)

META_PATH = (
    OUTPUT_DIR
    / "stage1b_meta.json"
)


                                                              
           
 
                
 
               
                     
                           
 
                       
 
                                      
 
                                              
                                
                                 
 
                                               
                             
                                                              

SEED = 42
SHUFFLE_BUFFER_SIZE = 20_000

STAGE1A_ACCEPTED_FINEWEB_DOCS = 109_599


                                            
 
                                              
                                               
                       
 
                                        

TARGET_MIB = 520

TARGET_BYTES = (
    TARGET_MIB
    * 1024
    * 1024
)

MAX_DOCUMENT_CHARS = 30_000

PROGRESS_EVERY_DOCS = 10_000


                                                              
          
                                                              

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
    IMPORTANT:
    This intentionally matches the Stage-1A
    FineWeb filtering logic.
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


                                                              
                
                                                              

def build_stage1b():
    print()
    print("=" * 72)
    print(
        "PocketLM-S3 Stage-1B Fresh Pretraining Corpus"
    )
    print("=" * 72)

    print()
    print(
        "Source: FineWeb-Edu"
    )

    print(
        f"Target: {TARGET_MIB} MiB"
    )

    print()
    print(
        "Replaying Stage-1A FineWeb stream..."
    )

    print(
        "Will skip first "
        f"{STAGE1A_ACCEPTED_FINEWEB_DOCS:,} "
        "accepted documents."
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
        buffer_size=(
            SHUFFLE_BUFFER_SIZE
        ),
    )

    accepted_seen = 0
    rejected_seen = 0

    stage1b_docs = 0
    stage1b_bytes = 0

    start_time = time.time()

    with TRAIN_PATH.open(
        "w",
        encoding="utf-8",
    ) as out_file:

        for row in dataset:
            raw_text = row.get(
                "text",
                "",
            )

            text = clean_text(
                raw_text
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
                STAGE1A_ACCEPTED_FINEWEB_DOCS
            ):
                accepted_seen += 1

                if (
                    accepted_seen
                    % 10_000
                    == 0
                ):
                    print(
                        "  replayed "
                        f"{accepted_seen:,}"
                        "/"
                        f"{STAGE1A_ACCEPTED_FINEWEB_DOCS:,}"
                        " accepted docs"
                    )

                continue

                                                              
                                                    
                                          
                                                              

            written = write_document(
                out_file,
                text,
            )

            if written == 0:
                continue

            stage1b_bytes += (
                written
            )

            stage1b_docs += 1
            accepted_seen += 1

            if (
                stage1b_docs
                % PROGRESS_EVERY_DOCS
                == 0
            ):
                elapsed = (
                    time.time()
                    - start_time
                )

                mib = (
                    stage1b_bytes
                    / 1024**2
                )

                pct = (
                    100.0
                    * stage1b_bytes
                    / TARGET_BYTES
                )

                print(
                    f"  Stage1B docs="
                    f"{stage1b_docs:,} | "
                    f"{mib:.1f} MiB | "
                    f"{pct:.1f}% | "
                    f"rejected total="
                    f"{rejected_seen:,} | "
                    f"time="
                    f"{elapsed / 60:.1f} min"
                )

            if (
                stage1b_bytes
                >= TARGET_BYTES
            ):
                break

    return {
        "source":
            "HuggingFaceFW/fineweb-edu",

        "seed":
            SEED,

        "shuffle_buffer_size":
            SHUFFLE_BUFFER_SIZE,

        "stage1a_accepted_docs_skipped":
            STAGE1A_ACCEPTED_FINEWEB_DOCS,

        "stage1b_documents":
            stage1b_docs,

        "stage1b_bytes":
            stage1b_bytes,

        "stage1b_mib":
            stage1b_bytes
            / 1024**2,

        "rejected_documents_seen":
            rejected_seen,

        "total_accepted_stream_position":
            accepted_seen,

        "output_file":
            str(TRAIN_PATH),
    }


                                                              
      
                                                              

def main():
    stats = build_stage1b()

    actual_bytes = (
        TRAIN_PATH.stat().st_size
    )

    if (
        actual_bytes
        != stats["stage1b_bytes"]
    ):
        raise RuntimeError(
            "Output size does not match "
            "recorded byte count."
        )

                          
     
                        
                         
     
                                    
                             

    rough_tokens = (
        actual_bytes
        / 3.829
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
        "STAGE-1B CORPUS COMPLETE"
    )
    print("=" * 72)

    print()
    print(
        f"Fresh documents: "
        f"{stats['stage1b_documents']:,}"
    )

    print(
        f"Train size: "
        f"{actual_bytes / 1024**2:.2f} MiB"
    )

    print(
        f"Rough tokens: "
        f"{rough_tokens / 1e6:.1f}M"
    )

    print()
    print(
        "FineWeb stream:"
    )

    print(
        f"  Stage-1A docs skipped: "
        f"{STAGE1A_ACCEPTED_FINEWEB_DOCS:,}"
    )

    print(
        f"  Current accepted position: "
        f"{stats['total_accepted_stream_position']:,}"
    )

    print()
    print(
        "Saved:"
    )

    print(
        f"  {TRAIN_PATH}"
    )

    print(
        f"  {META_PATH}"
    )

    print()


if __name__ == "__main__":
    main()