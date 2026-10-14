from pocketlm.pocketlm_v1 import PocketLM, PocketLMConfig


EXPECTED_PARAMETERS_30M = 30_460_864


def create_model_30m():
    cfg = PocketLMConfig(
        vocab_size=6144,
        max_seq_len=160,
        n_layers=12,
        d_model=448,
        n_heads=8,
        n_kv_heads=2,
        d_ff=1344,
        rope_theta=10000.0,
        rms_eps=1e-5,
    )

    model = PocketLM(cfg)

    params = sum(
        p.numel()
        for p in model.parameters()
    )

    if params != EXPECTED_PARAMETERS_30M:
        raise RuntimeError(
            f"PocketLM-30M parameter mismatch: "
            f"{params:,} != "
            f"{EXPECTED_PARAMETERS_30M:,}"
        )

    return model
