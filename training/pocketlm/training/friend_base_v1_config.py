from pathlib import Path


                                                              
                         
                                                              

SEED = 42

ROOT = Path(__file__).resolve().parents[2]

DATA_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "friend_base_v1"
)

TRAIN_TEXT = DATA_DIR / "friend_base_train.txt"
VAL_TEXT = DATA_DIR / "friend_base_val.txt"

TRAIN_BIN = DATA_DIR / "friend_base_train.bin"
VAL_BIN = DATA_DIR / "friend_base_val.bin"

MANIFEST_PATH = DATA_DIR / "friend_base_manifest.json"
TOKEN_META_PATH = DATA_DIR / "friend_base_token_meta.json"

TOKENIZER_PATH = (
    ROOT
    / "data"
    / "pocketlm"
    / "tokenizers"
    / "spm_6144"
    / "tokenizer.model"
)

CHECKPOINT_DIR = (
    ROOT
    / "checkpoints"
    / "pocketlm_friend_base_30m_v1"
)

LATEST_PATH = CHECKPOINT_DIR / "friend_base_latest.pt"
BEST_PATH = CHECKPOINT_DIR / "friend_base_best.pt"
FINAL_PATH = CHECKPOINT_DIR / "friend_base_final.pt"
METRICS_PATH = CHECKPOINT_DIR / "training_metrics.json"


                                                              
                              
                                                              

VOCAB_SIZE = 6144
SEQ_LEN = 160

PAD_ID = 0
UNK_ID = 1
BOS_ID = 2
EOS_ID = 3

USER_ID = 4
ASSISTANT_ID = 5
SYSTEM_ID = 6
MEMORY_ID = 7
END_MEMORY_ID = 8
END_ID = 9


                                                              
                         
                                                              

TRAIN_TARGET_TOKENS = {
    "fineweb_edu": 40_000_000,
    "cosmopedia": 5_000_000,
    "soda_narrative": 5_000_000,
}

VAL_TARGET_TOKENS = {
    "fineweb_edu": 800_000,
    "cosmopedia": 100_000,
    "soda_narrative": 100_000,
}

TOTAL_TRAIN_TARGET_TOKENS = sum(
    TRAIN_TARGET_TOKENS.values()
)

TOTAL_VAL_TARGET_TOKENS = sum(
    VAL_TARGET_TOKENS.values()
)


                                                              
                     
                                                              

FINEWEB_DATASET = "HuggingFaceFW/fineweb-edu"
FINEWEB_CONFIG = "sample-10BT"
FINEWEB_REVISION = "v1.4.0"

COSMOPEDIA_DATASET = "HuggingFaceTB/smollm-corpus"
COSMOPEDIA_CONFIG = "cosmopedia-v2"

SODA_DATASET = "allenai/soda"

SODA_REVISION = (
    "fdc848ab0183208ea7808206c91c724414d0a071"
)


                                                              
                
                                                              

SODA_RELATIONS = {
    "xIntent",
    "xReact",
    "xWant",
    "xNeed",
    "xEffect",
}


                                                              
                      
                                                              

COSMOPEDIA_ALLOWED_PAIRS = {
    ("textbook_narrative", "stanford"),
    ("textbook_academic", "openstax"),
    ("textbook_narrative_tone", "khanacademy"),
}


                                                              
            
                                                              

assert TOTAL_TRAIN_TARGET_TOKENS == 50_000_000
assert TOTAL_VAL_TARGET_TOKENS == 1_000_000
assert VOCAB_SIZE < 65536
