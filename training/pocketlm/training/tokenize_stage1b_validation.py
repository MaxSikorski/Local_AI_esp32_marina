from pathlib import Path
import json
import time

import numpy as np
import sentencepiece as spm


                                                              
       
                                                              

ROOT = Path(__file__).resolve().parents[2]

TOKENIZER_PATH = (
    ROOT
    / "data"
    / "pocketlm"
    / "tokenizers"
    / "spm_6144"
    / "tokenizer.model"
)

DATA_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "pretrain_stage1b"
)

INPUT_PATH = (
    DATA_DIR
    / "stage1b_fresh_val.txt"
)

OUTPUT_PATH = (
    DATA_DIR
    / "stage1b_fresh_val.bin"
)

META_PATH = (
    DATA_DIR
    / "stage1b_fresh_val_tokenized_meta.json"
)


                                                              
               
                                                              

EXPECTED_VOCAB_SIZE = 6144
TOKEN_DTYPE = np.uint16


                                                              
              
                                                              

def tokenize_file(
    input_path,
    output_path,
    tokenizer,
):
    eos_id = tokenizer.eos_id()

    token_count = 0
    document_count = 0

    min_token_id = None
    max_token_id = None

    start_time = time.time()

    document_lines = []

    with (
        input_path.open(
            "r",
            encoding="utf-8",
            errors="ignore",
        ) as in_file,
        output_path.open(
            "wb"
        ) as out_file,
    ):

        def process_document():
            nonlocal document_lines
            nonlocal token_count
            nonlocal document_count
            nonlocal min_token_id
            nonlocal max_token_id

            if not document_lines:
                return

            text = "\n".join(
                document_lines
            ).strip()

            document_lines = []

            if not text:
                return

            ids = tokenizer.encode(
                text,
                out_type=int,
            )

            if not ids:
                return

            local_min = min(ids)
            local_max = max(ids)

            if min_token_id is None:
                min_token_id = local_min
            else:
                min_token_id = min(
                    min_token_id,
                    local_min,
                )

            if max_token_id is None:
                max_token_id = local_max
            else:
                max_token_id = max(
                    max_token_id,
                    local_max,
                )

                                        
            ids.append(
                eos_id
            )

            arr = np.asarray(
                ids,
                dtype=TOKEN_DTYPE,
            )

            arr.tofile(
                out_file
            )

            token_count += len(ids)
            document_count += 1

        for line in in_file:
            stripped = line.strip()

            if stripped:
                document_lines.append(
                    stripped
                )

            else:
                process_document()

                                       
                                 
        process_document()

    elapsed = (
        time.time()
        - start_time
    )

    return {
        "tokens":
            token_count,

        "documents":
            document_count,

        "min_token_id":
            min_token_id,

        "max_token_id":
            max_token_id,

        "seconds":
            elapsed,
    }


                                                              
              
                                                              

def verify_binary(
    path,
    expected_tokens,
    vocab_size,
):
    data = np.memmap(
        path,
        dtype=TOKEN_DTYPE,
        mode="r",
    )

    print()
    print(
        "Verifying fresh validation binary..."
    )

    print(
        f"Tokens in file: "
        f"{len(data):,}"
    )

    if (
        len(data)
        != expected_tokens
    ):
        raise RuntimeError(
            "Token count mismatch."
        )

    min_id = int(
        data.min()
    )

    max_id = int(
        data.max()
    )

    print(
        f"Token ID range: "
        f"{min_id} .. {max_id}"
    )

    if min_id < 0:
        raise RuntimeError(
            "Negative token ID."
        )

    if max_id >= vocab_size:
        raise RuntimeError(
            f"Token ID {max_id} "
            f"outside vocab size "
            f"{vocab_size}."
        )

    print(
        "Binary verification: PASSED ✅"
    )

    return (
        min_id,
        max_id,
    )


                                                              
      
                                                              

def main():
    print()
    print("=" * 72)
    print(
        "PocketLM-S3 Fresh Stage-1B Validation Tokenizer"
    )
    print("=" * 72)

    if not TOKENIZER_PATH.exists():
        raise FileNotFoundError(
            TOKENIZER_PATH
        )

    if not INPUT_PATH.exists():
        raise FileNotFoundError(
            INPUT_PATH
        )

    tokenizer = (
        spm.SentencePieceProcessor(
            model_file=str(
                TOKENIZER_PATH
            )
        )
    )

    vocab_size = (
        tokenizer.get_piece_size()
    )

    print()
    print(
        f"Tokenizer vocab: "
        f"{vocab_size:,}"
    )

    print(
        f"PAD={tokenizer.pad_id()} "
        f"UNK={tokenizer.unk_id()} "
        f"BOS={tokenizer.bos_id()} "
        f"EOS={tokenizer.eos_id()}"
    )

    if (
        vocab_size
        != EXPECTED_VOCAB_SIZE
    ):
        raise RuntimeError(
            f"Expected vocab 6144, "
            f"got {vocab_size}"
        )

    if tokenizer.eos_id() != 3:
        raise RuntimeError(
            "Expected EOS ID = 3"
        )

    stats = tokenize_file(
        INPUT_PATH,
        OUTPUT_PATH,
        tokenizer,
    )

    min_id, max_id = (
        verify_binary(
            OUTPUT_PATH,
            stats["tokens"],
            vocab_size,
        )
    )

    input_bytes = (
        INPUT_PATH.stat().st_size
    )

    output_bytes = (
        OUTPUT_PATH.stat().st_size
    )

    bytes_per_token = (
        input_bytes
        / stats["tokens"]
    )

    windows_160 = (
        stats["tokens"]
        // 160
    )

    metadata = {
        "input_file":
            str(INPUT_PATH),

        "output_file":
            str(OUTPUT_PATH),

        "tokenizer":
            str(TOKENIZER_PATH),

        "vocab_size":
            vocab_size,

        "dtype":
            "uint16",

        "documents":
            stats["documents"],

        "tokens":
            stats["tokens"],

        "input_bytes":
            input_bytes,

        "output_bytes":
            output_bytes,

        "bytes_per_token":
            bytes_per_token,

        "token_id_min":
            min_id,

        "token_id_max":
            max_id,

        "windows_160":
            windows_160,

        "seconds":
            stats["seconds"],
    }

    with META_PATH.open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            metadata,
            f,
            indent=2,
        )

    print()
    print("=" * 72)
    print(
        "FRESH VALIDATION TOKENIZATION COMPLETE"
    )
    print("=" * 72)

    print()
    print(
        f"Documents: "
        f"{stats['documents']:,}"
    )

    print(
        f"Tokens: "
        f"{stats['tokens']:,}"
    )

    print(
        f"Bytes/token: "
        f"{bytes_per_token:.3f}"
    )

    print(
        f"Binary size: "
        f"{output_bytes / 1024**2:.2f} MiB"
    )

    print(
        f"160-token windows: "
        f"{windows_160:,}"
    )

    print(
        f"Time: "
        f"{stats['seconds']:.2f} sec"
    )

    print()
    print(
        "Saved:"
    )

    print(
        f"  {OUTPUT_PATH}"
    )

    print(
        f"  {META_PATH}"
    )

    print()


if __name__ == "__main__":
    main()