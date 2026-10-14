from dataclasses import dataclass

import torch
import torch.nn as nn
import torch.nn.functional as F

from pocketlm.quantization.kv_cache_int8 import (
    QuantizedKV,
    append_quantized_kv,
    dequantize_kv,
    quantize_kv,
)

                                                              
               
                                                              

@dataclass
class PocketLMConfig:
    vocab_size: int = 6144
    max_seq_len: int = 160

    n_layers: int = 12
    d_model: int = 320

    n_heads: int = 8
    n_kv_heads: int = 2

    d_ff: int = 864

    rope_theta: float = 10000.0
    rms_eps: float = 1e-5

    @property
    def head_dim(self) -> int:
        assert self.d_model % self.n_heads == 0
        return self.d_model // self.n_heads


                                                              
         
                                                              

class RMSNorm(nn.Module):
    def __init__(self, dim: int, eps: float = 1e-5):
        super().__init__()

        self.weight = nn.Parameter(
            torch.ones(dim)
        )

        self.eps = eps

    def forward(self, x):
                                       
                                  
        x_float = x.float()

        rms = torch.rsqrt(
            x_float.pow(2).mean(
                dim=-1,
                keepdim=True,
            )
            + self.eps
        )

        normalized = (
            x_float * rms
        ).to(x.dtype)

        return normalized * self.weight


                                                              
      
                                                              

def build_rope_cache(
    head_dim: int,
    max_seq_len: int,
    theta: float = 10000.0,
):
    assert head_dim % 2 == 0

    inv_freq = 1.0 / (
        theta
        ** (
            torch.arange(
                0,
                head_dim,
                2,
                dtype=torch.float32,
            )
            / head_dim
        )
    )

    positions = torch.arange(
        max_seq_len,
        dtype=torch.float32,
    )

    frequencies = torch.outer(
        positions,
        inv_freq,
    )

    cos = frequencies.cos()
    sin = frequencies.sin()

    return cos, sin


def apply_rope(
    x,
    cos,
    sin,
    positions,
):
    """
    x:
        [batch, heads, time, head_dim]
    """

    cos_t = cos[positions].to(
        device=x.device,
        dtype=x.dtype,
    )[None, None, :, :]

    sin_t = sin[positions].to(
        device=x.device,
        dtype=x.dtype,
    )[None, None, :, :]

    x_even = x[..., 0::2]
    x_odd = x[..., 1::2]

    out_even = (
        x_even * cos_t
        - x_odd * sin_t
    )

    out_odd = (
        x_even * sin_t
        + x_odd * cos_t
    )

    return torch.stack(
        (out_even, out_odd),
        dim=-1,
    ).flatten(-2)


                                                              
                         
                                                              

class GQAAttention(nn.Module):
    def __init__(
        self,
        cfg: PocketLMConfig,
    ):
        super().__init__()

        assert (
            cfg.n_heads
            % cfg.n_kv_heads
            == 0
        )

        self.n_heads = cfg.n_heads
        self.n_kv_heads = cfg.n_kv_heads
        self.head_dim = cfg.head_dim

        self.kv_repeat = (
            cfg.n_heads
            // cfg.n_kv_heads
        )

        self.scale = (
            self.head_dim ** -0.5
        )

        self.max_seq_len = (
            cfg.max_seq_len
        )

                
                            
        self.q_proj = nn.Linear(
            cfg.d_model,
            cfg.n_heads
            * self.head_dim,
            bias=False,
        )

              
                                   
        self.k_proj = nn.Linear(
            cfg.d_model,
            cfg.n_kv_heads
            * self.head_dim,
            bias=False,
        )

                
                                   
        self.v_proj = nn.Linear(
            cfg.d_model,
            cfg.n_kv_heads
            * self.head_dim,
            bias=False,
        )

        self.o_proj = nn.Linear(
            cfg.n_heads
            * self.head_dim,
            cfg.d_model,
            bias=False,
        )

        cos, sin = build_rope_cache(
            cfg.head_dim,
            cfg.max_seq_len,
            cfg.rope_theta,
        )

        self.register_buffer(
            "rope_cos",
            cos,
            persistent=False,
        )

        self.register_buffer(
            "rope_sin",
            sin,
            persistent=False,
        )

    def forward(
        self,
        x,
        past_kv=None,
        use_cache=False,
        quantized_kv_cache=False,
    ):
        batch_size, seq_len, _ = (
            x.shape
        )

        if past_kv is None:
            past_len = 0
            past_k = None
            past_v = None

        elif isinstance(
            past_kv,
            QuantizedKV,
        ):
            past_len = (
                past_kv.seq_len
            )

            past_k, past_v = (
                dequantize_kv(
                    past_kv,
                    dtype=x.dtype,
                )
            )

        else:
            past_k, past_v = past_kv
            past_len = (
                past_k.size(2)
            )

        total_len = (
            past_len + seq_len
        )

        if (
            total_len
            > self.max_seq_len
        ):
            raise ValueError(
                f"Context length "
                f"{total_len} exceeds "
                f"max_seq_len="
                f"{self.max_seq_len}"
            )

                                                  
                           
                                                  

        q = self.q_proj(x)
        k_new = self.k_proj(x)
        v_new = self.v_proj(x)

            
                     
            
                       

        q = q.view(
            batch_size,
            seq_len,
            self.n_heads,
            self.head_dim,
        ).transpose(1, 2)

            
                    
            
                       

        k_new = k_new.view(
            batch_size,
            seq_len,
            self.n_kv_heads,
            self.head_dim,
        ).transpose(1, 2)

                            

        v_new = v_new.view(
            batch_size,
            seq_len,
            self.n_kv_heads,
            self.head_dim,
        ).transpose(1, 2)

                                                  
                        
                                                  

        positions = torch.arange(
            past_len,
            total_len,
            device=x.device,
        )

        q = apply_rope(
            q,
            self.rope_cos,
            self.rope_sin,
            positions,
        )

        k_new = apply_rope(
            k_new,
            self.rope_cos,
            self.rope_sin,
            positions,
        )

                                                  
                           
                                                  

        if past_k is not None:
            k = torch.cat(
                (
                    past_k,
                    k_new,
                ),
                dim=2,
            )

            v = torch.cat(
                (
                    past_v,
                    v_new,
                ),
                dim=2,
            )

        else:
            k = k_new
            v = v_new

        if use_cache:
            if quantized_kv_cache:
                if isinstance(
                    past_kv,
                    QuantizedKV,
                ):
                                             
                                                      
                    present_kv = (
                        append_quantized_kv(
                            past_kv,
                            k_new,
                            v_new,
                        )
                    )
                else:
                                                   
                                       
                    present_kv = quantize_kv(
                        k,
                        v,
                    )
            else:
                present_kv = (
                    k,
                    v,
                )
        else:
            present_kv = None

                                                  
             
         
                                    
                                          
                                                  

        k_attn = (
            k.repeat_interleave(
                self.kv_repeat,
                dim=1,
            )
        )

        v_attn = (
            v.repeat_interleave(
                self.kv_repeat,
                dim=1,
            )
        )

                                                  
                          
                                                  

        scores = torch.matmul(
            q,
            k_attn.transpose(
                -2,
                -1,
            ),
        )

        scores = (
            scores * self.scale
        )

                                                  
                     
         
                     
                                  
                             
                                                  

        query_positions = (
            positions[:, None]
        )

        key_positions = (
            torch.arange(
                total_len,
                device=x.device,
            )[None, :]
        )

        causal_mask = (
            key_positions
            <= query_positions
        )

        scores = (
            scores.masked_fill(
                ~causal_mask[
                    None,
                    None,
                    :,
                    :
                ],
                torch.finfo(
                    scores.dtype
                ).min,
            )
        )

                                   
                              
        probs = F.softmax(
            scores.float(),
            dim=-1,
        ).to(q.dtype)

        out = torch.matmul(
            probs,
            v_attn,
        )

        out = (
            out
            .transpose(1, 2)
            .contiguous()
            .view(
                batch_size,
                seq_len,
                -1,
            )
        )

        out = self.o_proj(out)

        return out, present_kv


                                                              
                     
                                                              

class SwiGLU(nn.Module):
    def __init__(
        self,
        cfg: PocketLMConfig,
    ):
        super().__init__()

        self.gate_proj = nn.Linear(
            cfg.d_model,
            cfg.d_ff,
            bias=False,
        )

        self.up_proj = nn.Linear(
            cfg.d_model,
            cfg.d_ff,
            bias=False,
        )

        self.down_proj = nn.Linear(
            cfg.d_ff,
            cfg.d_model,
            bias=False,
        )

    def forward(self, x):
        gate = F.silu(
            self.gate_proj(x)
        )

        up = self.up_proj(x)

        return self.down_proj(
            gate * up
        )


                                                              
                   
                                                              

class TransformerBlock(nn.Module):
    def __init__(
        self,
        cfg: PocketLMConfig,
    ):
        super().__init__()

        self.attn_norm = RMSNorm(
            cfg.d_model,
            cfg.rms_eps,
        )

        self.attn = GQAAttention(
            cfg
        )

        self.ffn_norm = RMSNorm(
            cfg.d_model,
            cfg.rms_eps,
        )

        self.ffn = SwiGLU(
            cfg
        )

    def forward(
        self,
        x,
        past_kv=None,
        use_cache=False,
        quantized_kv_cache=False,
    ):
        attn_out, present_kv = (
            self.attn(
                self.attn_norm(x),
                past_kv=past_kv,
                use_cache=use_cache,
                quantized_kv_cache=(
                    quantized_kv_cache
                ),
            )
        )

        x = x + attn_out

        ffn_out = self.ffn(
            self.ffn_norm(x)
        )

        x = x + ffn_out

        return x, present_kv


                                                              
          
                                                              

class PocketLM(nn.Module):
    def __init__(
        self,
        cfg: PocketLMConfig,
    ):
        super().__init__()

        self.cfg = cfg

        self.tok_embeddings = (
            nn.Embedding(
                cfg.vocab_size,
                cfg.d_model,
            )
        )

        self.layers = nn.ModuleList(
            [
                TransformerBlock(cfg)
                for _ in range(
                    cfg.n_layers
                )
            ]
        )

        self.final_norm = RMSNorm(
            cfg.d_model,
            cfg.rms_eps,
        )

        self.lm_head = nn.Linear(
            cfg.d_model,
            cfg.vocab_size,
            bias=False,
        )

                                                  
                         
         
                                      
         
                            
                                
         
                                  
                                                  

        self.lm_head.weight = (
            self.tok_embeddings.weight
        )

        self.apply(
            self._init_weights
        )

    def _init_weights(
        self,
        module,
    ):
        if isinstance(
            module,
            nn.Linear,
        ):
            nn.init.normal_(
                module.weight,
                mean=0.0,
                std=0.02,
            )

        elif isinstance(
            module,
            nn.Embedding,
        ):
            nn.init.normal_(
                module.weight,
                mean=0.0,
                std=0.02,
            )

    def forward(
        self,
        input_ids,
        past_kv=None,
        use_cache=False,
        quantized_kv_cache=False,
    ):
        if input_ids.dim() != 2:
            raise ValueError(
                "input_ids must have shape "
                "[batch, sequence]"
            )

        _, seq_len = (
            input_ids.shape
        )

        if past_kv is None:
            past_kv = [
                None
            ] * self.cfg.n_layers

        elif (
            len(past_kv)
            != self.cfg.n_layers
        ):
            raise ValueError(
                "past_kv must contain "
                "one entry per layer"
            )

        if (
            past_kv[0] is None
            and seq_len
            > self.cfg.max_seq_len
        ):
            raise ValueError(
                "Input sequence exceeds "
                "max_seq_len"
            )

        x = self.tok_embeddings(
            input_ids
        )

        new_cache = (
            []
            if use_cache
            else None
        )

        for (
            layer,
            layer_past,
        ) in zip(
            self.layers,
            past_kv,
        ):
            x, present = layer(
                x,
                past_kv=layer_past,
                use_cache=use_cache,
                quantized_kv_cache=(
                    quantized_kv_cache
                ),
            )

            if use_cache:
                new_cache.append(
                    present
                )

        x = self.final_norm(x)

        logits = self.lm_head(x)

        return logits, new_cache

                                                              
                               
                                                              

    @torch.no_grad()
    def generate(
        self,
        input_ids,
        max_new_tokens=16,
        temperature=0.0,
        top_k=20,
        eos_token_id=None,
    ):
        if (
            input_ids.size(0)
            != 1
        ):
            raise ValueError(
                "PocketLM v1 generation "
                "currently supports "
                "batch size 1"
            )

        self.eval()

        output_ids = (
            input_ids.clone()
        )

        if (
            output_ids.size(1)
            >= self.cfg.max_seq_len
        ):
            return output_ids

                              
                           
        logits, cache = self(
            output_ids,
            use_cache=True,
        )

        for _ in range(
            max_new_tokens
        ):
            next_logits = (
                logits[:, -1, :]
            )

                                                  
                             
                                                  

            if temperature <= 0:
                next_token = (
                    torch.argmax(
                        next_logits,
                        dim=-1,
                        keepdim=True,
                    )
                )

                                                  
                      
                                                  

            else:
                next_logits = (
                    next_logits
                    / temperature
                )

                if (
                    top_k is not None
                    and top_k > 0
                ):
                    k = min(
                        top_k,
                        next_logits.size(-1),
                    )

                    values, indices = (
                        torch.topk(
                            next_logits,
                            k=k,
                            dim=-1,
                        )
                    )

                    probabilities = (
                        F.softmax(
                            values,
                            dim=-1,
                        )
                    )

                    sampled_index = (
                        torch.multinomial(
                            probabilities,
                            num_samples=1,
                        )
                    )

                    next_token = (
                        indices.gather(
                            -1,
                            sampled_index,
                        )
                    )

                else:
                    probabilities = (
                        F.softmax(
                            next_logits,
                            dim=-1,
                        )
                    )

                    next_token = (
                        torch.multinomial(
                            probabilities,
                            num_samples=1,
                        )
                    )

            output_ids = torch.cat(
                (
                    output_ids,
                    next_token,
                ),
                dim=1,
            )

                          
            if (
                eos_token_id
                is not None
                and next_token.item()
                == eos_token_id
            ):
                break

                                    
            if (
                output_ids.size(1)
                >= self.cfg.max_seq_len
            ):
                break

                                
             
                                                
            logits, cache = self(
                next_token,
                past_kv=cache,
                use_cache=True,
            )

        return output_ids


                                                              
           
                                                              

def count_parameters(
    model: nn.Module,
):
    return sum(
        parameter.numel()
        for parameter
        in model.parameters()
    )


                                                              
                    
                                                              

def main():
    torch.manual_seed(42)

    cfg = PocketLMConfig()

                        
                                    
                       
    model = (
        PocketLM(cfg)
        .cpu()
        .eval()
    )

    params = count_parameters(
        model
    )

    expected_params = (
        14_999_360
    )

    print()
    print("PocketLM-S3 v1")
    print("=" * 55)

    print(
        f"vocab_size:      "
        f"{cfg.vocab_size}"
    )

    print(
        f"max_seq_len:     "
        f"{cfg.max_seq_len}"
    )

    print(
        f"layers:          "
        f"{cfg.n_layers}"
    )

    print(
        f"d_model:         "
        f"{cfg.d_model}"
    )

    print(
        f"Q heads:         "
        f"{cfg.n_heads}"
    )

    print(
        f"KV heads:        "
        f"{cfg.n_kv_heads}"
    )

    print(
        f"head_dim:        "
        f"{cfg.head_dim}"
    )

    print(
        f"d_ff:            "
        f"{cfg.d_ff}"
    )

    print()
    print(
        f"Parameters:      "
        f"{params:,}"
    )

    print(
        f"Expected:        "
        f"{expected_params:,}"
    )

    assert (
        params
        == expected_params
    ), (
        f"Parameter mismatch: "
        f"{params:,} "
        f"!= "
        f"{expected_params:,}"
    )

    print()
    print(
        f"FP32 weights:    "
        f"{params * 4 / 1024**2:.2f} MiB"
    )

    print(
        f"FP16 weights:    "
        f"{params * 2 / 1024**2:.2f} MiB"
    )

    print(
        f"INT8 weights:    "
        f"{params / 1024**2:.2f} MiB"
    )

    print(
        f"Raw Q4 weights:  "
        f"{params * 0.5 / 1024**2:.2f} MiB"
    )

                                                              
             
                     
                                                              

    tied = (
        model.lm_head.weight.data_ptr()
        ==
        model.tok_embeddings.weight.data_ptr()
    )

    print()
    print(
        f"Tied embeddings: "
        f"{tied}"
    )

    assert tied

                                                              
             
                          
                                                              

    test_input = torch.randint(
        0,
        cfg.vocab_size,
        (2, 12),
    )

    with torch.no_grad():
        logits, _ = model(
            test_input
        )

    print(
        f"Forward logits:  "
        f"{tuple(logits.shape)}"
    )

    assert logits.shape == (
        2,
        12,
        cfg.vocab_size,
    )

                                                              
             
                    
     
                                     
                                
                                                              

    sequence_a = torch.randint(
        0,
        cfg.vocab_size,
        (1, 10),
    )

    sequence_b = (
        sequence_a.clone()
    )

    sequence_b[:, 6:] = (
        torch.randint(
            0,
            cfg.vocab_size,
            (1, 4),
        )
    )

    with torch.no_grad():
        logits_a, _ = model(
            sequence_a
        )

        logits_b, _ = model(
            sequence_b
        )

    causal_diff = (
        logits_a[:, 5, :]
        - logits_b[:, 5, :]
    ).abs().max().item()

    print(
        "Causal-mask diff: "
        f"{causal_diff:.8f}"
    )

    assert (
        causal_diff < 1e-5
    )

                                                              
             
                                     
                                       
                                                              

    sequence = torch.randint(
        0,
        cfg.vocab_size,
        (1, 8),
    )

    with torch.no_grad():
        full_logits, _ = model(
            sequence
        )

        cache = None
        incremental_outputs = []

        for index in range(
            sequence.size(1)
        ):
            token = sequence[
                :,
                index:index + 1,
            ]

            step_logits, cache = (
                model(
                    token,
                    past_kv=cache,
                    use_cache=True,
                )
            )

            incremental_outputs.append(
                step_logits
            )

        incremental_logits = (
            torch.cat(
                incremental_outputs,
                dim=1,
            )
        )

    cache_diff = (
        full_logits
        - incremental_logits
    ).abs().max().item()

    print(
        f"KV-cache diff:   "
        f"{cache_diff:.8f}"
    )

    assert (
        cache_diff < 1e-4
    )

                                

    k_cache, v_cache = (
        cache[0]
    )

    print(
        "Layer-0 K cache: "
        f"{tuple(k_cache.shape)}"
    )

    print(
        "Layer-0 V cache: "
        f"{tuple(v_cache.shape)}"
    )

    expected_cache_shape = (
        1,
        cfg.n_kv_heads,
        8,
        cfg.head_dim,
    )

    assert (
        k_cache.shape
        == expected_cache_shape
    )

    assert (
        v_cache.shape
        == expected_cache_shape
    )

                                                              
             
                                    
     
                                     
                            
                                                              

    prompt = torch.randint(
        0,
        cfg.vocab_size,
        (1, 4),
    )

    generated = model.generate(
        prompt,
        max_new_tokens=6,
        temperature=0.0,
    )

    print(
        "Generation shape:"
        f" {tuple(generated.shape)}"
    )

    print(
        "Generated IDs:   "
        f"{generated.tolist()[0]}"
    )

    assert generated.shape == (
        1,
        10,
    )

    print()
    print(
        "ALL POCKETLM-S3 v1 "
        "ARCHITECTURE TESTS PASSED"
    )
    print()


if __name__ == "__main__":
    main()