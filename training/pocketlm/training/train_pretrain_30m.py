from pathlib import Path
import argparse
import gc
import json
import math
import time

import numpy as np
import sentencepiece as spm
import torch
import torch.nn.functional as F

from pocketlm.training.pocketlm_30m import create_model_30m

from pocketlm.pocketlm_v1 import (
    PocketLM,
    PocketLMConfig,
)


                                                              
               
                                                              

ROOT = Path(__file__).resolve().parents[2]

DATA_DIR = (
    ROOT
    / "data"
    / "pocketlm"
    / "pretrain"
)

TRAIN_BIN = DATA_DIR / "pretrain_train.bin"
VAL_BIN = DATA_DIR / "pretrain_val.bin"

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
    / "pocketlm_pretrain_30m_wide_v1"
)

LATEST_PATH = (
    CHECKPOINT_DIR
    / "pretrain_latest.pt"
)

BEST_PATH = (
    CHECKPOINT_DIR
    / "pretrain_best.pt"
)

FINAL_PATH = (
    CHECKPOINT_DIR
    / "pretrain_final.pt"
)

METRICS_PATH = (
    CHECKPOINT_DIR
    / "training_metrics.json"
)


                        

SEED = 42

SEQ_LEN = 160
BATCH_SIZE = 32

PEAK_LR = 2e-4
MIN_LR = 2e-5

WARMUP_STEPS = 1000

WEIGHT_DECAY = 0.1
BETA1 = 0.9
BETA2 = 0.95

GRAD_CLIP = 1.0

LOG_EVERY = 100
EVAL_EVERY = 1000
SAVE_EVERY = 1000

                        
 
               
                             
 
                                      
                                       

VAL_BATCHES = 64


                                                              
        
                                                              

def get_device():
    if torch.backends.mps.is_available():
        return torch.device("mps")

    if torch.cuda.is_available():
        return torch.device("cuda")

    return torch.device("cpu")


def synchronize(device):
    if device.type == "mps":
        torch.mps.synchronize()

    elif device.type == "cuda":
        torch.cuda.synchronize()


                                                              
         
                                                              

def load_data():
    if not TRAIN_BIN.exists():
        raise FileNotFoundError(
            TRAIN_BIN
        )

    if not VAL_BIN.exists():
        raise FileNotFoundError(
            VAL_BIN
        )

    train = np.memmap(
        TRAIN_BIN,
        dtype=np.uint16,
        mode="r",
    )

    val = np.memmap(
        VAL_BIN,
        dtype=np.uint16,
        mode="r",
    )

    return train, val


def make_training_starts(
    num_tokens,
):
    """
    Divide the corpus into mostly
    non-overlapping 160-token windows.

    Then shuffle their order.

    This gives us approximately one
    complete pass over the corpus.
    """

    num_windows = (
        (num_tokens - 1)
        // SEQ_LEN
    )

    starts = (
        np.arange(
            num_windows,
            dtype=np.int64,
        )
        * SEQ_LEN
    )

    rng = np.random.default_rng(
        SEED
    )

    rng.shuffle(
        starts
    )

    return starts


def make_validation_starts(
    num_tokens,
):
    """
    Fixed deterministic positions
    spread across the validation corpus.
    """

    wanted = (
        VAL_BATCHES
        * BATCH_SIZE
    )

    max_start = (
        num_tokens
        - SEQ_LEN
        - 1
    )

    starts = np.linspace(
        0,
        max_start,
        num=wanted,
        dtype=np.int64,
    )

    return starts


def get_batch(
    data,
    starts,
    device,
):
    """
    For every start position we read:

        token 0 ... token 160

    Input:
        first 160 tokens

    Target:
        same sequence shifted by one.
    """

    batch = np.stack(
        [
            np.asarray(
                data[
                    start:
                    start + SEQ_LEN + 1
                ],
                dtype=np.int64,
            )
            for start in starts
        ],
        axis=0,
    )

    batch = torch.from_numpy(
        batch
    )

    x = (
        batch[:, :-1]
        .to(
            device,
            non_blocking=True,
        )
    )

    y = (
        batch[:, 1:]
        .to(
            device,
            non_blocking=True,
        )
    )

    return x, y


                                                              
           
                                                              

def build_optimizer(model):
    """
    Weight decay only on matrix weights.

    RMSNorm weights are 1D and should
    not receive weight decay.
    """

    decay = []
    no_decay = []

    for parameter in model.parameters():
        if not parameter.requires_grad:
            continue

        if parameter.ndim >= 2:
            decay.append(
                parameter
            )
        else:
            no_decay.append(
                parameter
            )

    optimizer = torch.optim.AdamW(
        [
            {
                "params": decay,
                "weight_decay":
                    WEIGHT_DECAY,
            },
            {
                "params": no_decay,
                "weight_decay":
                    0.0,
            },
        ],
        lr=PEAK_LR,
        betas=(
            BETA1,
            BETA2,
        ),
        eps=1e-8,
    )

    print()
    print(
        "Optimizer parameter groups:"
    )

    print(
        f"  decay tensors:    "
        f"{len(decay)}"
    )

    print(
        f"  no-decay tensors: "
        f"{len(no_decay)}"
    )

    return optimizer


                                                              
                        
                                                              

def learning_rate(
    step,
    total_steps,
):
                    

    if step < WARMUP_STEPS:
        return (
            PEAK_LR
            * (step + 1)
            / WARMUP_STEPS
        )

                   

    denominator = max(
        1,
        total_steps
        - WARMUP_STEPS,
    )

    progress = (
        (step - WARMUP_STEPS)
        / denominator
    )

    progress = min(
        max(progress, 0.0),
        1.0,
    )

    cosine = (
        0.5
        * (
            1.0
            + math.cos(
                math.pi
                * progress
            )
        )
    )

    return (
        MIN_LR
        + (
            PEAK_LR
            - MIN_LR
        )
        * cosine
    )


def set_learning_rate(
    optimizer,
    lr,
):
    for group in (
        optimizer.param_groups
    ):
        group["lr"] = lr


                                                              
            
                                                              

@torch.no_grad()
def evaluate(
    model,
    val_data,
    val_starts,
    device,
):
    model.eval()

    losses = []

    for batch_index in range(
        VAL_BATCHES
    ):
        start = (
            batch_index
            * BATCH_SIZE
        )

        end = (
            start
            + BATCH_SIZE
        )

        starts = (
            val_starts[
                start:end
            ]
        )

        x, y = get_batch(
            val_data,
            starts,
            device,
        )

        logits, _ = model(
            x
        )

        loss = F.cross_entropy(
            logits.reshape(
                -1,
                model.cfg.vocab_size,
            ),
            y.reshape(-1),
        )

        losses.append(
            float(
                loss.detach().cpu()
            )
        )

    model.train()

    return (
        sum(losses)
        / len(losses)
    )


                                                              
                        
                                                              

@torch.no_grad()
def print_samples(
    model,
    tokenizer,
    device,
):
    model.eval()

    prompts = [
        "The human brain",
        "A computer is",
        "The reason people sleep is",
    ]

    print()
    print("SAMPLES")
    print("-" * 70)

    for prompt in prompts:
        ids = tokenizer.encode(
            prompt,
            out_type=int,
        )

        input_ids = torch.tensor(
            [ids],
            dtype=torch.long,
            device=device,
        )

        generated = model.generate(
            input_ids,
            max_new_tokens=30,
            temperature=0.8,
            top_k=40,
            eos_token_id=3,
        )

        text = tokenizer.decode(
            generated[0]
            .detach()
            .cpu()
            .tolist()
        )

        print()
        print(
            f"PROMPT: {prompt}"
        )

        print(
            f"OUTPUT: {text}"
        )

    print()
    print("-" * 70)

    model.train()


                                                              
             
                                                              

def save_checkpoint(
    path,
    model,
    optimizer,
    completed_steps,
    best_val_loss,
    total_steps,
):
    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    payload = {
        "model":
            model.state_dict(),

        "optimizer":
            optimizer.state_dict(),

        "completed_steps":
            completed_steps,

        "best_val_loss":
            best_val_loss,

        "total_steps":
            total_steps,

        "config":
            vars(model.cfg),
    }

    temp_path = Path(
        str(path) + ".tmp"
    )

    torch.save(
        payload,
        temp_path,
    )

    temp_path.replace(
        path
    )


def save_model_only(
    path,
    model,
    completed_steps,
    val_loss,
):
    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    payload = {
        "model":
            {
                name:
                    tensor
                    .detach()
                    .cpu()
                for name, tensor
                in model.state_dict().items()
            },

        "completed_steps":
            completed_steps,

        "val_loss":
            val_loss,

        "config":
            vars(model.cfg),
    }

    temp_path = Path(
        str(path) + ".tmp"
    )

    torch.save(
        payload,
        temp_path,
    )

    temp_path.replace(
        path
    )


def move_optimizer_to_device(
    optimizer,
    device,
):
    for state in (
        optimizer.state.values()
    ):
        for key, value in (
            state.items()
        ):
            if torch.is_tensor(
                value
            ):
                state[key] = (
                    value.to(
                        device
                    )
                )


def load_checkpoint(
    path,
    model,
    optimizer,
    device,
):
    print()
    print(
        f"Loading checkpoint:"
    )

    print(path)

    checkpoint = torch.load(
        path,
        map_location="cpu",
        weights_only=False,
    )

    model.load_state_dict(
        checkpoint["model"]
    )

    optimizer.load_state_dict(
        checkpoint["optimizer"]
    )

    move_optimizer_to_device(
        optimizer,
        device,
    )

    completed_steps = int(
        checkpoint[
            "completed_steps"
        ]
    )

    best_val_loss = float(
        checkpoint[
            "best_val_loss"
        ]
    )

    print(
        f"Resume step: "
        f"{completed_steps:,}"
    )

    print(
        f"Best val loss: "
        f"{min(best_val_loss, final_val):.4f}"
    )

    return (
        completed_steps,
        best_val_loss,
    )


                                                              
         
                                                              

def save_metrics(
    metrics,
):
    CHECKPOINT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    with METRICS_PATH.open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            metrics,
            f,
            indent=2,
        )


                                                              
               
                                                              

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--resume",
        action="store_true",
        help=(
            "Resume from "
            "pretrain_latest.pt"
        ),
    )

    parser.add_argument(
        "--max-steps",
        type=int,
        default=None,
        help=(
            "Optional early stop. "
            "Useful for smoke tests."
        ),
    )

    args = parser.parse_args()

    torch.manual_seed(
        SEED
    )

    np.random.seed(
        SEED
    )

    device = get_device()

    print()
    print("=" * 72)
    print(
        "PocketLM-30M Wide Stage-1 Pretraining"
    )
    print("=" * 72)

    print(
        f"Device:      {device}"
    )

    print(
        f"Batch size:  {BATCH_SIZE}"
    )

    print(
        f"Sequence:    {SEQ_LEN}"
    )

    print(
        f"Tokens/step: "
        f"{BATCH_SIZE * SEQ_LEN:,}"
    )

                                                              
               
                                                              

    tokenizer = (
        spm.SentencePieceProcessor(
            model_file=str(
                TOKENIZER_PATH
            )
        )
    )

    if (
        tokenizer.get_piece_size()
        != 6144
    ):
        raise RuntimeError(
            "Expected 6144-token tokenizer"
        )

                                                              
          
                                                              

    train_data, val_data = (
        load_data()
    )

    print()
    print(
        f"Train tokens: "
        f"{len(train_data):,}"
    )

    print(
        f"Val tokens:   "
        f"{len(val_data):,}"
    )

    train_starts = (
        make_training_starts(
            len(train_data)
        )
    )

    val_starts = (
        make_validation_starts(
            len(val_data)
        )
    )

    total_steps = (
        len(train_starts)
        // BATCH_SIZE
    )

    print(
        f"Train windows: "
        f"{len(train_starts):,}"
    )

    print(
        f"Full-pass steps: "
        f"{total_steps:,}"
    )

    print(
        f"Tokens seen: "
        f"{total_steps * BATCH_SIZE * SEQ_LEN:,}"
    )

                                                              
           
                                                              

    model = create_model_30m().to(device)
    cfg = model.cfg

    parameter_count = sum(
        p.numel()
        for p in model.parameters()
    )

    print()
    print(
        f"Model parameters: "
        f"{parameter_count:,}"
    )

    if parameter_count != 30_460_864:
        raise RuntimeError(
            "Unexpected parameter count"
        )

    optimizer = (
        build_optimizer(
            model
        )
    )

    completed_steps = 0
    best_val_loss = float(
        "inf"
    )

                                                              
                     
                                                              

    if args.resume:
        if not LATEST_PATH.exists():
            raise FileNotFoundError(
                LATEST_PATH
            )

        (
            completed_steps,
            best_val_loss,
        ) = load_checkpoint(
            LATEST_PATH,
            model,
            optimizer,
            device,
        )

                                                              
                              
                                                              

    stop_step = total_steps

    if args.max_steps is not None:
        stop_step = min(
            total_steps,
            args.max_steps,
        )

    if completed_steps >= stop_step:
        print(
            "Nothing to train: "
            "checkpoint is already "
            "at or beyond stop step."
        )

        return

    print()
    print(
        f"Starting at step: "
        f"{completed_steps:,}"
    )

    print(
        f"Stopping at step: "
        f"{stop_step:,}"
    )

                                                              
                        
                                                              

    print()
    print(
        "Running initial validation..."
    )

    initial_val = evaluate(
        model,
        val_data,
        val_starts,
        device,
    )

    print(
        f"Initial val loss: "
        f"{initial_val:.4f}"
    )

    print(
        f"Initial perplexity: "
        f"{math.exp(min(initial_val, 20)):.2f}"
    )

    if (
        best_val_loss
        == float("inf")
    ):
        best_val_loss = (
            initial_val
        )

                                                              
           
                                                              

    model.train()

    metrics = {
        "architecture": {
            "parameters":
                parameter_count,

            "batch_size":
                BATCH_SIZE,

            "sequence_length":
                SEQ_LEN,

            "tokens_per_step":
                BATCH_SIZE
                * SEQ_LEN,

            "total_steps":
                total_steps,
        },

        "initial_val_loss":
            initial_val,

        "history": [],
    }

    interval_start = (
        time.perf_counter()
    )

    interval_loss = (
        torch.zeros(
            (),
            device=device,
        )
    )

    interval_steps = 0

    training_start = (
        time.perf_counter()
    )

    for step in range(
        completed_steps,
        stop_step,
    ):
                                                  
            
                                                  

        lr = learning_rate(
            step,
            total_steps,
        )

        set_learning_rate(
            optimizer,
            lr,
        )

                                                  
               
                                                  

        batch_start = (
            step
            * BATCH_SIZE
        )

        batch_end = (
            batch_start
            + BATCH_SIZE
        )

        starts = train_starts[
            batch_start:
            batch_end
        ]

        x, y = get_batch(
            train_data,
            starts,
            device,
        )

                                                  
                 
                                                  

        optimizer.zero_grad(
            set_to_none=True
        )

        logits, _ = model(
            x
        )

        loss = F.cross_entropy(
            logits.reshape(
                -1,
                cfg.vocab_size,
            ),
            y.reshape(-1),
        )

                                                  
                  
                                                  

        loss.backward()

        grad_norm = (
            torch.nn.utils
            .clip_grad_norm_(
                model.parameters(),
                GRAD_CLIP,
            )
        )

        optimizer.step()

        interval_loss = (
            interval_loss
            + loss.detach()
        )

        interval_steps += 1

        completed = (
            step + 1
        )

                                                  
                 
                                                  

        if (
            completed % LOG_EVERY
            == 0
            or completed
            == stop_step
        ):
            synchronize(
                device
            )

            elapsed = (
                time.perf_counter()
                - interval_start
            )

            avg_loss = float(
                (
                    interval_loss
                    / interval_steps
                )
                .detach()
                .cpu()
            )

            tok_per_sec = (
                interval_steps
                * BATCH_SIZE
                * SEQ_LEN
                / elapsed
            )

            remaining = (
                stop_step
                - completed
            )

            eta_seconds = (
                remaining
                * BATCH_SIZE
                * SEQ_LEN
                / max(
                    tok_per_sec,
                    1.0,
                )
            )

            grad_value = float(
                grad_norm
                .detach()
                .cpu()
            )

            print(
                f"step "
                f"{completed:6,d}"
                f"/{total_steps:,} | "
                f"loss {avg_loss:.4f} | "
                f"lr {lr:.2e} | "
                f"grad {grad_value:.3f} | "
                f"{tok_per_sec:,.0f} tok/s | "
                f"ETA "
                f"{eta_seconds / 3600:.2f}h"
            )

            metrics[
                "history"
            ].append(
                {
                    "step":
                        completed,

                    "train_loss":
                        avg_loss,

                    "lr":
                        lr,

                    "grad_norm":
                        grad_value,

                    "tokens_per_second":
                        tok_per_sec,
                }
            )

            save_metrics(
                metrics
            )

            interval_start = (
                time.perf_counter()
            )

            interval_loss = (
                torch.zeros(
                    (),
                    device=device,
                )
            )

            interval_steps = 0

                                                  
                    
                                                  

        if (
            completed
            % EVAL_EVERY
            == 0
        ):
            val_loss = evaluate(
                model,
                val_data,
                val_starts,
                device,
            )

            perplexity = (
                math.exp(
                    min(
                        val_loss,
                        20,
                    )
                )
            )

            print()
            print(
                f"VALIDATION @ "
                f"{completed:,}"
            )

            print(
                f"  val loss:   "
                f"{val_loss:.4f}"
            )

            print(
                f"  perplexity: "
                f"{perplexity:.2f}"
            )

            if (
                val_loss
                < best_val_loss
            ):
                best_val_loss = (
                    val_loss
                )

                save_model_only(
                    BEST_PATH,
                    model,
                    completed,
                    val_loss,
                )

                print(
                    "  NEW BEST ✅"
                )

            print_samples(
                model,
                tokenizer,
                device,
            )

                                                  
                           
                                                  

        if (
            completed
            % SAVE_EVERY
            == 0
        ):
            save_checkpoint(
                LATEST_PATH,
                model,
                optimizer,
                completed,
                best_val_loss,
                total_steps,
            )

            print(
                f"Saved resume checkpoint "
                f"@ step {completed:,}"
            )

                                                              
                      
                                                              

    print()
    print(
        "Running final validation..."
    )

    final_val = evaluate(
        model,
        val_data,
        val_starts,
        device,
    )

    print()
    print("=" * 72)
    print(
        "TRAINING RUN COMPLETE"
    )
    print("=" * 72)

    print(
        f"Completed steps: "
        f"{stop_step:,}"
    )

    tokens_seen = (
        stop_step
        * BATCH_SIZE
        * SEQ_LEN
    )

    print(
        f"Tokens seen: "
        f"{tokens_seen:,}"
    )

    print(
        f"Final val loss: "
        f"{final_val:.4f}"
    )

    print(
        f"Final perplexity: "
        f"{math.exp(min(final_val, 20)):.2f}"
    )

    print(
        f"Best val loss: "
        f"{best_val_loss:.4f}"
    )

    total_elapsed = (
        time.perf_counter()
        - training_start
    )

    print(
        f"Training time: "
        f"{total_elapsed / 3600:.2f} h"
    )

                                                              
                 
                                                              

    save_model_only(
        FINAL_PATH,
        model,
        stop_step,
        final_val,
    )

                                         
                     

    save_checkpoint(
        LATEST_PATH,
        model,
        optimizer,
        stop_step,
        min(
            best_val_loss,
            final_val,
        ),
        total_steps,
    )

    if (
        final_val
        < best_val_loss
    ):
        best_val_loss = (
            final_val
        )

        save_model_only(
            BEST_PATH,
            model,
            stop_step,
            final_val,
        )

    metrics[
        "final"
    ] = {
        "step":
            stop_step,

        "tokens_seen":
            tokens_seen,

        "val_loss":
            final_val,

        "best_val_loss":
            best_val_loss,

        "seconds":
            total_elapsed,
    }

    save_metrics(
        metrics
    )

    print()
    print(
        "Saved:"
    )

    print(
        f"  best:   {BEST_PATH}"
    )

    print(
        f"  latest: {LATEST_PATH}"
    )

    print(
        f"  final:  {FINAL_PATH}"
    )

    print(
        f"  metrics:{METRICS_PATH}"
    )

    print_samples(
        model,
        tokenizer,
        device,
    )

    print()


if __name__ == "__main__":
    main()