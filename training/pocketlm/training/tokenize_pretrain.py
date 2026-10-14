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

PRETRAIN_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "pretrain"
)

TRAIN_TEXT = (
    PRETRAIN_DIR
    / "pretrain_train.txt"
)

VAL_TEXT = (
    PRETRAIN_DIR
    / "pretrain_val.txt"
)

TRAIN_BIN = (
    PRETRAIN_DIR
    / "pretrain_train.bin"
)

VAL_BIN = (
    PRETRAIN_DIR
    / "pretrain_val.bin"
)

META_PATH = (
    PRETRAIN_DIR
    / "pretrain_meta.json"
)


                                                              
               
                                                              

EXPECTED_VOCAB_SIZE = 6144

                                      
             
TOKEN_DTYPE = np.uint16

                                          
                                          
FLUSH_TOKENS = 1_000_000

                                           
                           
PROGRESS_BYTES = 25 * 1024 * 1024


                                                              
              
                                                              

def tokenize_file(
    input_path: Path,
    output_path: Path,
    tokenizer,
    eos_id: int,
):
    """
    Stream a text file paragraph-by-paragraph.

    Each paragraph:
        text -> SentencePiece IDs -> EOS

    The complete token stream is written directly
    as uint16, so we never keep the entire dataset
    in memory.
    """

    total_file_bytes = (
        input_path.stat().st_size
    )

    bytes_read = 0
    next_progress = PROGRESS_BYTES

    token_count = 0
    paragraph_count = 0

    min_token_id = None
    max_token_id = None

    token_buffer = []
    paragraph_lines = []

    start_time = time.time()

    def flush_token_buffer(out_file):
        nonlocal token_buffer

        if not token_buffer:
            return

        array = np.asarray(
            token_buffer,
            dtype=TOKEN_DTYPE,
        )

        array.tofile(out_file)

        token_buffer = []

    def process_paragraph():
        nonlocal token_count
        nonlocal paragraph_count
        nonlocal min_token_id
        nonlocal max_token_id
        nonlocal paragraph_lines
        nonlocal token_buffer

        if not paragraph_lines:
            return

        text = "\n".join(
            paragraph_lines
        ).strip()

        paragraph_lines = []

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

        token_buffer.extend(ids)

                                               
        token_buffer.append(eos_id)

        token_count += (
            len(ids) + 1
        )

        paragraph_count += 1

    print()
    print("=" * 70)
    print(
        f"TOKENIZING: {input_path.name}"
    )
    print("=" * 70)

    print(
        f"Input size: "
        f"{total_file_bytes / 1024**2:.2f} MiB"
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
                line.encode("utf-8")
            )

            stripped = line.strip()

            if stripped:
                paragraph_lines.append(
                    stripped
                )

            else:
                process_paragraph()

            if (
                len(token_buffer)
                >= FLUSH_TOKENS
            ):
                flush_token_buffer(
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
                    / total_file_bytes
                )

                tokens_m = (
                    token_count / 1e6
                )

                speed = (
                    token_count
                    / elapsed
                    if elapsed > 0
                    else 0
                )

                print(
                    f"  {pct:6.1f}% | "
                    f"{bytes_read / 1024**2:7.1f} MiB | "
                    f"{tokens_m:7.2f}M tokens | "
                    f"{paragraph_count:,} paragraphs | "
                    f"{speed:,.0f} tok/s"
                )

                while (
                    next_progress
                    <= bytes_read
                ):
                    next_progress += (
                        PROGRESS_BYTES
                    )

                                        
                        
        process_paragraph()

        flush_token_buffer(
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
            f"Binary size mismatch: "
            f"{output_bytes:,} bytes "
            f"!= expected "
            f"{expected_bytes:,}"
        )

    print()
    print(
        f"Paragraphs: "
        f"{paragraph_count:,}"
    )

    print(
        f"Tokens:     "
        f"{token_count:,}"
    )

    print(
        f"Token IDs:  "
        f"{min_token_id} .. "
        f"{max_token_id}"
    )

    print(
        f"Output:     "
        f"{output_bytes / 1024**2:.2f} MiB"
    )

    print(
        f"Time:       "
        f"{elapsed / 60:.2f} min"
    )

    print(
        f"Speed:      "
        f"{token_count / elapsed:,.0f} tokens/sec"
    )

    return {
        "source_file": str(
            input_path
        ),
        "binary_file": str(
            output_path
        ),
        "source_bytes": (
            total_file_bytes
        ),
        "binary_bytes": (
            output_bytes
        ),
        "paragraphs": (
            paragraph_count
        ),
        "tokens": (
            token_count
        ),
        "min_token_id": (
            min_token_id
        ),
        "max_token_id": (
            max_token_id
        ),
        "seconds": (
            elapsed
        ),
    }


                                                              
                     
                                                              

def verify_binary(
    path: Path,
    expected_tokens: int,
    vocab_size: int,
):
    data = np.memmap(
        path,
        dtype=TOKEN_DTYPE,
        mode="r",
    )

    print()
    print(
        f"Verifying {path.name}..."
    )

    print(
        f"  memmap tokens: "
        f"{len(data):,}"
    )

    if (
        len(data)
        != expected_tokens
    ):
        raise RuntimeError(
            f"Expected "
            f"{expected_tokens:,} tokens, "
            f"found {len(data):,}"
        )

                                         
                                        
    chunk_size = 10_000_000

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
            global_min = local_min
        else:
            global_min = min(
                global_min,
                local_min,
            )

        if global_max is None:
            global_max = local_max
        else:
            global_max = max(
                global_max,
                local_max,
            )

    print(
        f"  ID range: "
        f"{global_min} .. "
        f"{global_max}"
    )

    if global_min < 0:
        raise RuntimeError(
            "Negative token ID found"
        )

    if global_max >= vocab_size:
        raise RuntimeError(
            f"Token ID {global_max} "
            f"exceeds vocab size "
            f"{vocab_size}"
        )

    print(
        "  binary verification: PASSED"
    )


                                                              
      
                                                              

def main():
    print()
    print("=" * 70)
    print(
        "PocketLM-S3 Pretraining Tokenizer"
    )
    print("=" * 70)

                                                              
                 
                                                              

    for path in [
        TOKENIZER_PATH,
        TRAIN_TEXT,
        VAL_TEXT,
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

    if (
        vocab_size
        != EXPECTED_VOCAB_SIZE
    ):
        raise RuntimeError(
            f"Expected vocab "
            f"{EXPECTED_VOCAB_SIZE}, "
            f"got {vocab_size}"
        )

    pad_id = (
        tokenizer.pad_id()
    )

    unk_id = (
        tokenizer.unk_id()
    )

    bos_id = (
        tokenizer.bos_id()
    )

    eos_id = (
        tokenizer.eos_id()
    )

    print(
        f"PAD={pad_id} "
        f"UNK={unk_id} "
        f"BOS={bos_id} "
        f"EOS={eos_id}"
    )

    if eos_id != 3:
        raise RuntimeError(
            f"Expected EOS ID 3, "
            f"got {eos_id}"
        )

                                                              
                    
                                                              

    train_stats = tokenize_file(
        TRAIN_TEXT,
        TRAIN_BIN,
        tokenizer,
        eos_id,
    )

                                                              
                         
                                                              

    val_stats = tokenize_file(
        VAL_TEXT,
        VAL_BIN,
        tokenizer,
        eos_id,
    )

                                                              
                            
                                                              

    verify_binary(
        TRAIN_BIN,
        train_stats["tokens"],
        vocab_size,
    )

    verify_binary(
        VAL_BIN,
        val_stats["tokens"],
        vocab_size,
    )

                                                              
                               
                                                              

    train_tokens = (
        train_stats["tokens"]
    )

    val_tokens = (
        val_stats["tokens"]
    )

    chars_train = (
        train_stats[
            "source_bytes"
        ]
    )

    chars_val = (
        val_stats[
            "source_bytes"
        ]
    )

                                      
                                          
                                        
    train_bytes_per_token = (
        chars_train
        / train_tokens
    )

    val_bytes_per_token = (
        chars_val
        / val_tokens
    )

    context_len = 160

                                         
                                                  
    train_windows_160 = (
        train_tokens
        // context_len
    )

    val_windows_160 = (
        val_tokens
        // context_len
    )

    metadata = {
        "tokenizer": str(
            TOKENIZER_PATH
        ),
        "vocab_size": (
            vocab_size
        ),
        "dtype": "uint16",
        "dtype_bytes": 2,
        "pad_id": pad_id,
        "unk_id": unk_id,
        "bos_id": bos_id,
        "eos_id": eos_id,
        "context_length": (
            context_len
        ),
        "train": (
            train_stats
        ),
        "validation": (
            val_stats
        ),
        "train_bytes_per_token": (
            train_bytes_per_token
        ),
        "val_bytes_per_token": (
            val_bytes_per_token
        ),
        "train_windows_160": (
            train_windows_160
        ),
        "val_windows_160": (
            val_windows_160
        ),
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
    print("=" * 70)
    print(
        "PRETRAIN TOKENIZATION COMPLETE"
    )
    print("=" * 70)

    print()
    print(
        f"TRAIN TOKENS: "
        f"{train_tokens:,}"
    )

    print(
        f"VAL TOKENS:   "
        f"{val_tokens:,}"
    )

    print()

    print(
        f"Train bytes/token: "
        f"{train_bytes_per_token:.3f}"
    )

    print(
        f"Val bytes/token:   "
        f"{val_bytes_per_token:.3f}"
    )

    print()

    print(
        f"160-token train windows: "
        f"{train_windows_160:,}"
    )

    print(
        f"160-token val windows:   "
        f"{val_windows_160:,}"
    )

    print()

    print(
        "Saved:"
    )

    print(
        f"  {TRAIN_BIN}"
    )

    print(
        f"  {VAL_BIN}"
    )

    print(
        f"  {META_PATH}"
    )

    print()


if __name__ == "__main__":
    main()