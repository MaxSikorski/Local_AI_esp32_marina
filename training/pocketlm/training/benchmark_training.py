import gc
import time

import torch
import torch.nn.functional as F

from pocketlm.pocketlm_v1 import (
    PocketLM,
    PocketLMConfig,
)


                                                              
               
                                                              

SEQ_LEN = 160

BATCH_SIZES = [
    4,
    8,
    16,
    24,
    32,
    48,
    64,
]

WARMUP_STEPS = 2
BENCHMARK_STEPS = 5


                                                              
        
                                                              

def get_device():
    if torch.backends.mps.is_available():
        return torch.device("mps")

    if torch.cuda.is_available():
        return torch.device("cuda")

    return torch.device("cpu")


                                                              
               
                                                              

def benchmark_batch(
    batch_size,
    device,
):
    print()
    print("=" * 65)
    print(
        f"BATCH SIZE = {batch_size}"
    )
    print("=" * 65)

    gc.collect()

    if device.type == "mps":
        torch.mps.empty_cache()

    cfg = PocketLMConfig()

    model = PocketLM(cfg).to(
        device
    )

    model.train()

    optimizer = torch.optim.AdamW(
        model.parameters(),
        lr=3e-4,
        betas=(0.9, 0.95),
        weight_decay=0.1,
    )

                                        
    tokens = torch.randint(
        low=0,
        high=cfg.vocab_size,
        size=(
            batch_size,
            SEQ_LEN + 1,
        ),
        device=device,
    )

    x = tokens[:, :-1]
    y = tokens[:, 1:]

    def step():
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

        optimizer.step()

        return loss

                                              
            
                                              

    try:
        for _ in range(
            WARMUP_STEPS
        ):
            step()

        if device.type == "mps":
            torch.mps.synchronize()

        elif device.type == "cuda":
            torch.cuda.synchronize()

                                                  
                              
                                                  

        start = time.perf_counter()

        last_loss = None

        for _ in range(
            BENCHMARK_STEPS
        ):
            last_loss = step()

        if device.type == "mps":
            torch.mps.synchronize()

        elif device.type == "cuda":
            torch.cuda.synchronize()

        elapsed = (
            time.perf_counter()
            - start
        )

        seconds_per_step = (
            elapsed
            / BENCHMARK_STEPS
        )

        tokens_per_step = (
            batch_size
            * SEQ_LEN
        )

        tokens_per_second = (
            tokens_per_step
            / seconds_per_step
        )

        print(
            f"Loss:              "
            f"{last_loss.item():.4f}"
        )

        print(
            f"Seconds/step:      "
            f"{seconds_per_step:.3f}"
        )

        print(
            f"Tokens/step:       "
            f"{tokens_per_step:,}"
        )

        print(
            f"Training tok/sec:  "
            f"{tokens_per_second:,.0f}"
        )

                                                  
                                
                                                  

        memory_info = {}

        if device.type == "mps":
            allocated = (
                torch.mps.current_allocated_memory()
            )

            driver = (
                torch.mps.driver_allocated_memory()
            )

            memory_info = {
                "allocated_gib":
                    allocated
                    / 1024**3,

                "driver_gib":
                    driver
                    / 1024**3,
            }

            print(
                f"MPS allocated:     "
                f"{memory_info['allocated_gib']:.2f} GiB"
            )

            print(
                f"MPS driver memory: "
                f"{memory_info['driver_gib']:.2f} GiB"
            )

        result = {
            "batch_size":
                batch_size,

            "tokens_per_step":
                tokens_per_step,

            "seconds_per_step":
                seconds_per_step,

            "tokens_per_second":
                tokens_per_second,

            **memory_info,
        }

        del optimizer
        del model
        del tokens
        del x
        del y

        gc.collect()

        if device.type == "mps":
            torch.mps.empty_cache()

        return result

    except RuntimeError as exc:
        print()
        print(
            "FAILED:"
        )

        print(exc)

        del optimizer
        del model
        del tokens
        del x
        del y

        gc.collect()

        if device.type == "mps":
            torch.mps.empty_cache()

        return None


                                                              
      
                                                              

def main():
    torch.manual_seed(42)

    device = get_device()

    print()
    print("=" * 65)
    print(
        "PocketLM-S3 Training Benchmark"
    )
    print("=" * 65)

    print(
        f"Device: {device}"
    )

    print(
        f"Sequence length: {SEQ_LEN}"
    )

    print(
        "Model: PocketLM-S3 ~15M"
    )

    results = []

    for batch_size in BATCH_SIZES:
        result = benchmark_batch(
            batch_size,
            device,
        )

        if result is None:
            print()
            print(
                "Stopping after first "
                "failed batch size."
            )
            break

        results.append(
            result
        )

    if not results:
        raise RuntimeError(
            "No batch size succeeded."
        )

                                    
                                     
                                        
    best = max(
        results,
        key=lambda x:
            x["tokens_per_second"],
    )

    print()
    print("=" * 65)
    print(
        "BENCHMARK SUMMARY"
    )
    print("=" * 65)

    print(
        f"{'Batch':>7} | "
        f"{'Tok/step':>10} | "
        f"{'Sec/step':>9} | "
        f"{'Tok/sec':>10}"
    )

    print("-" * 50)

    for result in results:
        print(
            f"{result['batch_size']:7d} | "
            f"{result['tokens_per_step']:10,d} | "
            f"{result['seconds_per_step']:9.3f} | "
            f"{result['tokens_per_second']:10,.0f}"
        )

    print()
    print(
        "Highest measured throughput:"
    )

    print(
        f"Batch {best['batch_size']} "
        f"→ "
        f"{best['tokens_per_second']:,.0f} tok/s"
    )

                                     
    seconds = (
        136_939_872
        / best["tokens_per_second"]
    )

    print()
    print(
        "Very rough time for "
        "136.94M tokens at this throughput:"
    )

    print(
        f"{seconds / 3600:.2f} hours"
    )

    print()
    print(
        "NOTE: real training can differ "
        "because of data loading, validation, "
        "checkpointing and learning-rate scheduling."
    )


if __name__ == "__main__":
    main()