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
    / "stage1b_train.txt"
)

OUTPUT_PATH = (
    DATA_DIR
    / "stage1b_train.bin"
)

META_PATH = (
    DATA_DIR
    / "stage1b_tokenized_meta.json"
)


                                                              
               
                                                              

EXPECTED_VOCAB_SIZE = 6144

TOKEN_DTYPE = np.uint16

FLUSH_TOKENS = 1_000_000

PROGRESS_BYTES = (
    25 * 1024 * 1024
)


                                                              
              
                                                              

def tokenize_file(
    input_path,
    output_path,
    tokenizer,
):
    eos_id = tokenizer.eos_id()

    total_bytes = (
        input_path.stat().st_size
    )

    bytes_read = 0
    next_progress = (
        PROGRESS_BYTES
    )

    token_count = 0
    document_count = 0

    min_token_id = None
    max_token_id = None

    token_buffer = []
    document_lines = []

    start_time = time.time()

    def flush_buffer(out_file):
        nonlocal token_buffer

        if not token_buffer:
            return

        arr = np.asarray(
            token_buffer,
            dtype=TOKEN_DTYPE,
        )

        arr.tofile(
            out_file
        )

        token_buffer = []

    def process_document():
        nonlocal document_lines
        nonlocal document_count
        nonlocal token_count
        nonlocal min_token_id
        nonlocal max_token_id
        nonlocal token_buffer

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
            min_token_id = (
                local_min
            )
        else:
            min_token_id = min(
                min_token_id,
                local_min,
            )

        if max_token_id is None:
            max_token_id = (
                local_max
            )
        else:
            max_token_id = max(
                max_token_id,
                local_max,
            )

        token_buffer.extend(
            ids
        )

                                          
        token_buffer.append(
            eos_id
        )

        token_count += (
            len(ids) + 1
        )

        document_count += 1

    print()
    print("=" * 72)
    print(
        "PocketLM-S3 Stage-1B Tokenization"
    )
    print("=" * 72)

    print()
    print(
        f"Input: {input_path}"
    )

    print(
        f"Input size: "
        f"{total_bytes / 1024**2:.2f} MiB"
    )

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

        for line in in_file:
            bytes_read += len(
                line.encode(
                    "utf-8"
                )
            )

            stripped = (
                line.strip()
            )

            if stripped:
                document_lines.append(
                    stripped
                )

            else:
                process_document()

            if (
                len(token_buffer)
                >= FLUSH_TOKENS
            ):
                flush_buffer(
                    out_file
                )

            if (
                bytes_read
                >= next_progress
            ):
                elapsed = (
                    time.time()
                    - start_time
                )

                pct = (
                    100.0
                    * bytes_read
                    / total_bytes
                )

                speed = (
                    token_count
                    / elapsed
                    if elapsed > 0
                    else 0
                )

                print(
                    f"{pct:6.1f}% | "
                    f"{bytes_read / 1024**2:7.1f} MiB | "
                    f"{token_count / 1e6:7.2f}M tokens | "
                    f"{document_count:,} docs | "
                    f"{speed:,.0f} tok/s"
                )

                while (
                    next_progress
                    <= bytes_read
                ):
                    next_progress += (
                        PROGRESS_BYTES
                    )

                        
        process_document()

        flush_buffer(
            out_file
        )

    elapsed = (
        time.time()
        - start_time
    )

    output_bytes = (
        output_path.stat().st_size
    )

    expected_bytes = (
        token_count
        * np.dtype(
            TOKEN_DTYPE
        ).itemsize
    )

    if (
        output_bytes
        != expected_bytes
    ):
        raise RuntimeError(
            "Binary file size mismatch: "
            f"{output_bytes:,} != "
            f"{expected_bytes:,}"
        )

    return {
        "tokens":
            token_count,

        "documents":
            document_count,

        "input_bytes":
            total_bytes,

        "output_bytes":
            output_bytes,

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
    print()
    print(
        "Verifying binary..."
    )

    data = np.memmap(
        path,
        dtype=TOKEN_DTYPE,
        mode="r",
    )

    if (
        len(data)
        != expected_tokens
    ):
        raise RuntimeError(
            "Token count mismatch."
        )

    chunk_size = (
        10_000_000
    )

    global_min = None
    global_max = None

    for start in range(
        0,
        len(data),
        chunk_size,
    ):
        chunk = data[
            start:
            start + chunk_size
        ]

        local_min = int(
            chunk.min()
        )

        local_max = int(
            chunk.max()
        )

        if global_min is None:
            global_min = (
                local_min
            )
        else:
            global_min = min(
                global_min,
                local_min,
            )

        if global_max is None:
            global_max = (
                local_max
            )
        else:
            global_max = max(
                global_max,
                local_max,
            )

    print(
        f"Tokens in file: "
        f"{len(data):,}"
    )

    print(
        f"Token ID range: "
        f"{global_min} .. "
        f"{global_max}"
    )

    if global_min < 0:
        raise RuntimeError(
            "Negative token ID found."
        )

    if (
        global_max
        >= vocab_size
    ):
        raise RuntimeError(
            f"Token {global_max} "
            f"is outside vocabulary."
        )

    print(
        "Binary verification: PASSED ✅"
    )

    return (
        global_min,
        global_max,
    )


                                                              
      
                                                              

def main():
    for path in [
        TOKENIZER_PATH,
        INPUT_PATH,
    ]:
        if not path.exists():
            raise FileNotFoundError(
                path
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
        f"Tokenizer: "
        f"{TOKENIZER_PATH}"
    )

    print(
        f"Vocabulary: "
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
            f"Expected "
            f"{EXPECTED_VOCAB_SIZE}, "
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

    bytes_per_token = (
        stats["input_bytes"]
        / stats["tokens"]
    )

    windows_160 = (
        stats["tokens"]
        // 160
    )

                           
    stage1a_tokens = (
        136_939_520
    )

    total_tokens_seen_after_1b = (
        stage1a_tokens
        + stats["tokens"]
    )

    tokens_per_parameter = (
        total_tokens_seen_after_1b
        / 14_999_360
    )

    metadata = {
        "tokenizer":
            str(TOKENIZER_PATH),

        "vocab_size":
            vocab_size,

        "dtype":
            "uint16",

        "eos_id":
            tokenizer.eos_id(),

        "documents":
            stats["documents"],

        "tokens":
            stats["tokens"],

        "input_bytes":
            stats["input_bytes"],

        "output_bytes":
            stats["output_bytes"],

        "bytes_per_token":
            bytes_per_token,

        "token_id_min":
            min_id,

        "token_id_max":
            max_id,

        "windows_160":
            windows_160,

        "stage1a_tokens_seen":
            stage1a_tokens,

        "combined_tokens_after_stage1b":
            total_tokens_seen_after_1b,

        "combined_tokens_per_parameter":
            tokens_per_parameter,

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
        "STAGE-1B TOKENIZATION COMPLETE"
    )
    print("=" * 72)

    print()
    print(
        f"Documents: "
        f"{stats['documents']:,}"
    )

    print(
        f"Stage-1B tokens: "
        f"{stats['tokens']:,}"
    )

    print(
        f"Bytes/token: "
        f"{bytes_per_token:.3f}"
    )

    print(
        f"Binary size: "
        f"{stats['output_bytes'] / 1024**2:.2f} MiB"
    )

    print(
        f"160-token windows: "
        f"{windows_160:,}"
    )

    print()
    print(
        "Combined pretraining:"
    )

    print(
        f"  Stage 1A: "
        f"{stage1a_tokens:,}"
    )

    print(
        f"  Stage 1B: "
        f"{stats['tokens']:,}"
    )

    print(
        f"  TOTAL:    "
        f"{total_tokens_seen_after_1b:,}"
    )

    print(
        f"  Tokens / parameter: "
        f"{tokens_per_parameter:.2f}"
    )

    print()
    print(
        f"Time: "
        f"{stats['seconds'] / 60:.2f} min"
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