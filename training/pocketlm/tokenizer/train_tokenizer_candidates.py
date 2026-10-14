from pathlib import Path
import json

import sentencepiece as spm


ROOT = Path(__file__).resolve().parents[2]

CORPUS = (
    ROOT
    / "data"
    / "pocketlm"
    / "tokenizer_corpus.txt"
)

OUTPUT_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "tokenizers"
)

VOCAB_SIZES = [
    2048,
    4096,
    6144,
    8192,
]

SPECIAL_TOKENS = [
    "<|user|>",
    "<|assistant|>",
    "<|system|>",
    "<|memory|>",
    "<|end_memory|>",
    "<|end|>",
]


def train_candidate(vocab_size: int):
    candidate_dir = (
        OUTPUT_DIR
        / f"spm_{vocab_size}"
    )

    candidate_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    prefix = (
        candidate_dir
        / "tokenizer"
    )

    model_path = Path(
        str(prefix) + ".model"
    )

    vocab_path = Path(
        str(prefix) + ".vocab"
    )

    print()
    print("=" * 65)
    print(
        f"TRAINING TOKENIZER: "
        f"{vocab_size:,}"
    )
    print("=" * 65)

    spm.SentencePieceTrainer.train(
        input=str(CORPUS),

        model_prefix=str(prefix),

                                              
        model_type="bpe",

        vocab_size=vocab_size,

                                                              
                        
                                                              

        pad_id=0,
        unk_id=1,
        bos_id=2,
        eos_id=3,

        pad_piece="<pad>",
        unk_piece="<unk>",
        bos_piece="<s>",
        eos_piece="</s>",

                                                              
                                 
         
                                          
                                          
                                                              

        user_defined_symbols=(
            SPECIAL_TOKENS
        ),

                                                              
                       
         
                                                  
                                               
                                                              

        byte_fallback=True,

                                                              
                                  
         
                                                
                                         
         
                                                 
                                                              

        character_coverage=0.999,

                                                              
                                                     
                                
                                                              

        max_sentence_length=20000,

                                                      
        input_sentence_size=0,

        shuffle_input_sentence=True,

                                              
        hard_vocab_limit=True,

                                                   
        minloglevel=1,
    )

                                                              
                            
                                                              

    if not model_path.exists():
        raise RuntimeError(
            f"Missing model: {model_path}"
        )

    if not vocab_path.exists():
        raise RuntimeError(
            f"Missing vocab: {vocab_path}"
        )

    tokenizer = (
        spm.SentencePieceProcessor(
            model_file=str(
                model_path
            )
        )
    )

    actual_vocab = (
        tokenizer.get_piece_size()
    )

    print()
    print(
        f"Requested vocab: "
        f"{vocab_size:,}"
    )

    print(
        f"Actual vocab:    "
        f"{actual_vocab:,}"
    )

    if actual_vocab != vocab_size:
        raise RuntimeError(
            f"Expected {vocab_size}, "
            f"got {actual_vocab}"
        )

                                                              
                           
                                                              

    expected_base = {
        "<pad>": 0,
        "<unk>": 1,
        "<s>": 2,
        "</s>": 3,
    }

    print()
    print("Base tokens")
    print("-" * 40)

    for piece, expected_id in (
        expected_base.items()
    ):
        actual_id = (
            tokenizer.piece_to_id(
                piece
            )
        )

        print(
            f"{piece:16s} "
            f"ID = {actual_id}"
        )

        if actual_id != expected_id:
            raise RuntimeError(
                f"{piece} expected "
                f"ID {expected_id}, "
                f"got {actual_id}"
            )

                                                              
                                    
                                                              

    print()
    print("PocketLM special tokens")
    print("-" * 40)

    special_ids = {}

    for token in SPECIAL_TOKENS:
        token_id = (
            tokenizer.piece_to_id(
                token
            )
        )

        encoded_ids = (
            tokenizer.encode(
                token,
                out_type=int,
            )
        )

        pieces = (
            tokenizer.encode(
                token,
                out_type=str,
            )
        )

        special_ids[token] = (
            token_id
        )

        print(
            f"{token:16s} "
            f"ID = {token_id:4d} "
            f"pieces={pieces}"
        )

                                              
                                               
                         
         
                                             
                                                  
                                      

        if token not in pieces:
            raise RuntimeError(
                f"{token} was split: "
                f"{pieces}"
            )

                                                   
        if (
            tokenizer.id_to_piece(
                token_id
            )
            != token
        ):
            raise RuntimeError(
                f"ID {token_id} "
                f"does not map back "
                f"to {token}"
            )

                                                 
                           
        if token_id not in encoded_ids:
            raise RuntimeError(
                f"Token ID {token_id} "
                f"missing from encoded "
                f"sequence {encoded_ids}"
            )

                                                              
                   
                                                              

    metadata = {
        "vocab_size": (
            vocab_size
        ),
        "model_type": "bpe",
        "byte_fallback": True,
        "character_coverage": 0.999,
        "max_sentence_length": 20000,
        "pad_id": 0,
        "unk_id": 1,
        "bos_id": 2,
        "eos_id": 3,
        "special_tokens": (
            special_ids
        ),
        "corpus": str(
            CORPUS
        ),
    }

    meta_path = (
        candidate_dir
        / "meta.json"
    )

    with meta_path.open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            metadata,
            f,
            indent=2,
            ensure_ascii=False,
        )

                                                              
                   
                                                              

    sample = (
        "<|user|>\n"
        "Why do people dream?\n"
        "<|assistant|>\n"
        "Dreaming is related to activity "
        "in the brain during sleep."
        "<|end|>"
    )

    pieces = tokenizer.encode(
        sample,
        out_type=str,
    )

    ids = tokenizer.encode(
        sample,
        out_type=int,
    )

    decoded = tokenizer.decode(
        ids
    )

    print()
    print("Sanity sample")
    print("-" * 40)

    print(
        "Sample token count:",
        len(ids),
    )

    print(
        "First pieces:",
        pieces[:30],
    )

    print(
        "First IDs:",
        ids[:30],
    )

    print(
        "Decoded sample:"
    )

    print(decoded)

                                            
                                                  
                    

    if decoded != sample:
        print()
        print(
            "NOTE: decoded text differs "
            "slightly because of "
            "SentencePiece normalization."
        )

    print()
    print(
        f"Saved model: "
        f"{model_path}"
    )

    print(
        f"Saved vocab: "
        f"{vocab_path}"
    )

    print(
        f"Saved metadata: "
        f"{meta_path}"
    )


def main():
    if not CORPUS.exists():
        raise FileNotFoundError(
            CORPUS
        )

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    print()
    print(
        "PocketTokenizer Candidate Training"
    )

    print("=" * 65)

    corpus_size = (
        CORPUS.stat().st_size
        / 1024**2
    )

    print(
        f"Corpus: "
        f"{corpus_size:.2f} MiB"
    )

    print(
        "Candidates:",
        VOCAB_SIZES,
    )

    for vocab_size in (
        VOCAB_SIZES
    ):
        train_candidate(
            vocab_size
        )

    print()
    print("=" * 65)

    print(
        "ALL TOKENIZER "
        "CANDIDATES TRAINED"
    )

    print("=" * 65)

    print()

    print(
        "Saved under:"
    )

    print(
        OUTPUT_DIR
    )


if __name__ == "__main__":
    main()