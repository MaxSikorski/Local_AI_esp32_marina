from pathlib import Path
import argparse
import json
import math
import time

import numpy as np
import sentencepiece as spm
import torch
import torch.nn.functional as F

from pocketlm.training.pocketlm_30m import (
    create_model_30m,
    EXPECTED_PARAMETERS_30M,
)

from pocketlm.training.friend_base_v1_config import (
    ROOT,
    TRAIN_BIN,
    VAL_BIN,
    TOKEN_META_PATH,
    TOKENIZER_PATH,
    CHECKPOINT_DIR as PRODUCTION_CHECKPOINT_DIR,
    VOCAB_SIZE,
    SEQ_LEN,
    SEED,
)


                                                              
                        
                                                              

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

SMOKE_STEPS = 5
SMOKE_VAL_BATCHES = 2

SMOKE_CHECKPOINT_DIR = (
    ROOT
    / "checkpoints"
    / "_smoke_pocketlm_friend_base_30m_v1"
)

FORBIDDEN_OLD_CHECKPOINT_DIR = (
    ROOT
    / "checkpoints"
    / "pocketlm_pretrain_30m_wide_v1"
)

RUN_KIND = "pocketlm_friend_base_30m_v1"


                                                              
               
                                                              

def runtime_paths(smoke_test: bool):
    checkpoint_dir = (
        SMOKE_CHECKPOINT_DIR
        if smoke_test
        else PRODUCTION_CHECKPOINT_DIR
    )

    checkpoint_dir = checkpoint_dir.resolve()

    if (
        checkpoint_dir
        == FORBIDDEN_OLD_CHECKPOINT_DIR.resolve()
    ):
        raise RuntimeError(
            "Refusing to use old Stage-1 checkpoint directory."
        )

    return {
        "dir": checkpoint_dir,
        "latest":
            checkpoint_dir
            / "friend_base_latest.pt",
        "best":
            checkpoint_dir
            / "friend_base_best.pt",
        "final":
            checkpoint_dir
            / "friend_base_final.pt",
        "metrics":
            checkpoint_dir
            / "training_metrics.json",
    }


                                                              
        
                                                              

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

    if not TOKEN_META_PATH.exists():
        raise FileNotFoundError(
            TOKEN_META_PATH
        )

    metadata = json.loads(
        TOKEN_META_PATH.read_text(
            encoding="utf-8"
        )
    )

    if metadata.get("version") != "friend_base_v1":
        raise RuntimeError(
            "Unexpected dataset metadata version."
        )

    if metadata.get("dtype") != "uint16":
        raise RuntimeError(
            "Expected uint16 dataset."
        )

    if int(metadata["vocab_size"]) != VOCAB_SIZE:
        raise RuntimeError(
            "Dataset/tokenizer vocab mismatch."
        )

    if int(metadata["seq_len"]) != SEQ_LEN:
        raise RuntimeError(
            "Dataset sequence-length mismatch."
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

    expected_train = int(
        metadata["train"]["tokens"]
    )

    expected_val = int(
        metadata["validation"]["tokens"]
    )

    if len(train) != expected_train:
        raise RuntimeError(
            "Train token-count mismatch: "
            f"{len(train):,} != "
            f"{expected_train:,}"
        )

    if len(val) != expected_val:
        raise RuntimeError(
            "Val token-count mismatch: "
            f"{len(val):,} != "
            f"{expected_val:,}"
        )

    signature = {
        "version":
            metadata["version"],
        "train_tokens":
            expected_train,
        "val_tokens":
            expected_val,
        "vocab_size":
            VOCAB_SIZE,
        "seq_len":
            SEQ_LEN,
    }

    return (
        train,
        val,
        signature,
    )


def make_training_starts(
    num_tokens,
):
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
    val_batches,
):
    wanted = (
        val_batches
        * BATCH_SIZE
    )

    max_start = (
        num_tokens
        - SEQ_LEN
        - 1
    )

    if max_start <= 0:
        raise RuntimeError(
            "Validation corpus is too small."
        )

    return np.linspace(
        0,
        max_start,
        num=wanted,
        dtype=np.int64,
    )


def get_batch(
    data,
    starts,
    device,
):
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
    print("Optimizer parameter groups:")
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
    val_batches,
):
    model.eval()

    losses = []

    for batch_index in range(
        val_batches
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

        del x
        del y
        del logits
        del loss

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
        "A good friend",
        "When someone has a bad day",
        "People usually feel proud when",
    ]

    print()
    print("BASE-LANGUAGE SAMPLES")
    print("-" * 72)

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
            max_new_tokens=24,
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
    print("-" * 72)

    model.train()


                                                              
             
                                                              

def save_checkpoint(
    path,
    model,
    optimizer,
    completed_steps,
    best_val_loss,
    total_steps,
    data_signature,
):
    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    payload = {
        "run_kind":
            RUN_KIND,
        "initialization":
            "random",
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
        "data_signature":
            data_signature,
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
    data_signature,
):
    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    payload = {
        "run_kind":
            RUN_KIND,
        "initialization":
            "random",
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
        "data_signature":
            data_signature,
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
            if not torch.is_tensor(
                value
            ):
                continue

                                                        
                                                        
                                                     
                                                       
            if key == "step":
                state[key] = (
                    value.cpu()
                )
                continue

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
    expected_total_steps,
    expected_data_signature,
):
    print()
    print("Loading FRIEND checkpoint:")
    print(path)

    checkpoint = torch.load(
        path,
        map_location="cpu",
        weights_only=False,
    )

    if (
        checkpoint.get("run_kind")
        != RUN_KIND
    ):
        raise RuntimeError(
            "Checkpoint run_kind mismatch. "
            "Refusing to load."
        )

    if (
        checkpoint.get("initialization")
        != "random"
    ):
        raise RuntimeError(
            "Checkpoint initialization "
            "metadata mismatch."
        )

    if (
        checkpoint.get("data_signature")
        != expected_data_signature
    ):
        raise RuntimeError(
            "Checkpoint data signature mismatch."
        )

    if (
        int(checkpoint["total_steps"])
        != expected_total_steps
    ):
        raise RuntimeError(
            "Checkpoint total_steps mismatch."
        )

    if (
        checkpoint.get("config")
        != vars(model.cfg)
    ):
        raise RuntimeError(
            "Checkpoint model config mismatch."
        )

    model.load_state_dict(
        checkpoint["model"],
        strict=True,
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
        f"{best_val_loss:.4f}"
    )

    return (
        completed_steps,
        best_val_loss,
    )


                                                              
         
                                                              

def save_metrics(
    path,
    metrics,
):
    path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    temp_path = Path(
        str(path) + ".tmp"
    )

    with temp_path.open(
        "w",
        encoding="utf-8",
    ) as f:
        json.dump(
            metrics,
            f,
            indent=2,
        )

    temp_path.replace(
        path
    )


                                                              
        
                                                              

def assert_fresh_directory(
    checkpoint_dir,
):
    if (
        checkpoint_dir.exists()
        and any(
            checkpoint_dir.iterdir()
        )
    ):
        raise RuntimeError(
            "Checkpoint directory is not empty:\n"
            f"{checkpoint_dir}\n"
            "Refusing to overwrite an existing run. "
            "Use --resume for a real existing run."
        )


                                                              
      
                                                              

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--resume",
        action="store_true",
        help=(
            "Resume only from this Friend run's "
            "friend_base_latest.pt."
        ),
    )

    parser.add_argument(
        "--max-steps",
        type=int,
        default=None,
        help=(
            "Absolute stopping step. "
            "Useful for controlled tests."
        ),
    )

    parser.add_argument(
        "--smoke-test",
        action="store_true",
        help=(
            "Use isolated smoke checkpoint directory, "
            "5 steps, and tiny validation."
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

    paths = runtime_paths(
        args.smoke_test
    )

    if not args.resume:
        assert_fresh_directory(
            paths["dir"]
        )

    print()
    print("=" * 72)
    print(
        "PocketLM-30M Friend Base v1"
    )
    print("=" * 72)

    print(
        f"Mode:        "
        f"{'SMOKE TEST' if args.smoke_test else 'PRODUCTION'}"
    )

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

    print(
        f"Checkpoint:  "
        f"{paths['dir']}"
    )

    if args.resume:
        print(
            "Initialization: RESUME FRIEND CHECKPOINT"
        )
    else:
        print(
            f"Initialization: RANDOM seed={SEED}"
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
        != VOCAB_SIZE
    ):
        raise RuntimeError(
            "Tokenizer vocabulary mismatch."
        )

    if tokenizer.eos_id() != 3:
        raise RuntimeError(
            "Tokenizer EOS mismatch."
        )

                                                              
          
                                                              

    (
        train_data,
        val_data,
        data_signature,
    ) = load_data()

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

    val_batches = (
        SMOKE_VAL_BATCHES
        if args.smoke_test
        else VAL_BATCHES
    )

    val_starts = (
        make_validation_starts(
            len(val_data),
            val_batches,
        )
    )

    total_steps = (
        len(train_starts)
        // BATCH_SIZE
    )

    full_pass_tokens = (
        total_steps
        * BATCH_SIZE
        * SEQ_LEN
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
        f"Full-pass tokens: "
        f"{full_pass_tokens:,}"
    )

    if total_steps != 9_765:
        raise RuntimeError(
            "Unexpected Friend Base full-pass "
            f"step count: {total_steps:,}"
        )

                                                              
           
                                                              

    model = (
        create_model_30m()
        .to(device)
    )

    parameter_count = sum(
        p.numel()
        for p in model.parameters()
    )

    print()
    print(
        f"Model parameters: "
        f"{parameter_count:,}"
    )

    if (
        parameter_count
        != EXPECTED_PARAMETERS_30M
    ):
        raise RuntimeError(
            "Unexpected parameter count."
        )

    optimizer = build_optimizer(
        model
    )

    completed_steps = 0
    best_val_loss = float(
        "inf"
    )

    if args.resume:
        if not paths["latest"].exists():
            raise FileNotFoundError(
                paths["latest"]
            )

        (
            completed_steps,
            best_val_loss,
        ) = load_checkpoint(
            paths["latest"],
            model,
            optimizer,
            device,
            total_steps,
            data_signature,
        )

                                                              
                
                                                              

    if args.smoke_test:
        if args.max_steps is None:
            stop_step = SMOKE_STEPS
        else:
            stop_step = min(
                total_steps,
                args.max_steps,
            )

                                                       
                                             
        if stop_step > 20:
            raise RuntimeError(
                "Smoke tests are limited to 20 steps."
            )

    else:
        stop_step = total_steps

        if args.max_steps is not None:
            stop_step = min(
                total_steps,
                args.max_steps,
            )

    if stop_step <= 0:
        raise RuntimeError(
            "stop_step must be positive."
        )

    if completed_steps >= stop_step:
        print(
            "Nothing to train: checkpoint is "
            "already at or beyond stop step."
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
        if not args.resume
        else "Running resume validation..."
    )

    initial_val = evaluate(
        model,
        val_data,
        val_starts,
        device,
        val_batches,
    )

    print(
        f"Validation loss: "
        f"{initial_val:.4f}"
    )

    print(
        f"Perplexity: "
        f"{math.exp(min(initial_val, 20)):.2f}"
    )

    if not args.resume:
        best_val_loss = initial_val

                                                        
                                                           
                                                     
        save_model_only(
            paths["best"],
            model,
            completed_steps,
            initial_val,
            data_signature,
        )

                                                              
             
                                                              

    if (
        args.resume
        and paths["metrics"].exists()
    ):
        metrics = json.loads(
            paths["metrics"].read_text(
                encoding="utf-8"
            )
        )

        metrics.setdefault(
            "resume_validations",
            [],
        ).append({
            "step":
                completed_steps,
            "val_loss":
                initial_val,
        })

    else:
        metrics = {
            "run_kind":
                RUN_KIND,
            "mode":
                (
                    "smoke"
                    if args.smoke_test
                    else "production"
                ),
            "initialization":
                "random",
            "seed":
                SEED,
            "data_signature":
                data_signature,
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
                "full_pass_tokens":
                    full_pass_tokens,
            },
            "initial_val_loss":
                initial_val,
            "history": [],
            "validations": [],
        }

    save_metrics(
        paths["metrics"],
        metrics,
    )

                                                              
           
                                                              

    model.train()

    log_every = (
        1
        if args.smoke_test
        else LOG_EVERY
    )

    eval_every = (
        stop_step + 1
        if args.smoke_test
        else EVAL_EVERY
    )

    save_every = (
        stop_step + 1
        if args.smoke_test
        else SAVE_EVERY
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

        if len(starts) != BATCH_SIZE:
            raise RuntimeError(
                "Incomplete training batch."
            )

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
                model.cfg.vocab_size,
            ),
            y.reshape(-1),
        )

        if not torch.isfinite(loss):
            raise RuntimeError(
                f"Non-finite loss at step {step + 1}"
            )

        loss.backward()

        grad_norm = (
            torch.nn.utils.clip_grad_norm_(
                model.parameters(),
                GRAD_CLIP,
            )
        )

        if not torch.isfinite(
            grad_norm
        ):
            raise RuntimeError(
                f"Non-finite gradient norm "
                f"at step {step + 1}"
            )

        optimizer.step()

        synchronize(
            device
        )

        interval_loss += (
            loss.detach()
        )

        interval_steps += 1

        completed = (
            step + 1
        )

        del x
        del y
        del logits
        del loss

                                                              
                 
                                                              

        if (
            completed % log_every == 0
            or completed == stop_step
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

            tokens_processed = (
                interval_steps
                * BATCH_SIZE
                * SEQ_LEN
            )

            tok_per_sec = (
                tokens_processed
                / max(elapsed, 1e-9)
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
            ].append({
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
            })

            save_metrics(
                paths["metrics"],
                metrics,
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
            % eval_every
            == 0
        ):
            val_loss = evaluate(
                model,
                val_data,
                val_starts,
                device,
                val_batches,
            )

            perplexity = math.exp(
                min(
                    val_loss,
                    20,
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

            metrics[
                "validations"
            ].append({
                "step":
                    completed,
                "val_loss":
                    val_loss,
                "perplexity":
                    perplexity,
            })

            if (
                val_loss
                < best_val_loss
            ):
                best_val_loss = (
                    val_loss
                )

                save_model_only(
                    paths["best"],
                    model,
                    completed,
                    val_loss,
                    data_signature,
                )

                print(
                    "  NEW BEST"
                )

            save_metrics(
                paths["metrics"],
                metrics,
            )

            print_samples(
                model,
                tokenizer,
                device,
            )

                                                              
                           
                                                              

        if (
            completed
            % save_every
            == 0
        ):
            save_checkpoint(
                paths["latest"],
                model,
                optimizer,
                completed,
                best_val_loss,
                total_steps,
                data_signature,
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
        val_batches,
    )

    if final_val < best_val_loss:
        best_val_loss = (
            final_val
        )

        save_model_only(
            paths["best"],
            model,
            stop_step,
            final_val,
            data_signature,
        )

                                                  
                                                
                                                    
    if not paths["best"].exists():
        save_model_only(
            paths["best"],
            model,
            stop_step,
            final_val,
            data_signature,
        )

    tokens_seen = (
        stop_step
        * BATCH_SIZE
        * SEQ_LEN
    )

    total_elapsed = (
        time.perf_counter()
        - training_start
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

    print(
        f"Training time: "
        f"{total_elapsed / 3600:.3f} h"
    )

    save_model_only(
        paths["final"],
        model,
        stop_step,
        final_val,
        data_signature,
    )

    save_checkpoint(
        paths["latest"],
        model,
        optimizer,
        stop_step,
        best_val_loss,
        total_steps,
        data_signature,
    )

    metrics["final"] = {
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
        paths["metrics"],
        metrics,
    )

    print()
    print("Saved:")
    print(
        f"  best:    "
        f"{paths['best']}"
    )
    print(
        f"  latest:  "
        f"{paths['latest']}"
    )
    print(
        f"  final:   "
        f"{paths['final']}"
    )
    print(
        f"  metrics: "
        f"{paths['metrics']}"
    )

    print_samples(
        model,
        tokenizer,
        device,
    )


if __name__ == "__main__":
    main()
