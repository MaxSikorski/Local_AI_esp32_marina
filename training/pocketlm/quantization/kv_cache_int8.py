from __future__ import annotations

from dataclasses import dataclass

import torch


INT8_QMAX = 127


@dataclass
class QuantizedKV:
    k_q: torch.Tensor
    k_scale: torch.Tensor
    v_q: torch.Tensor
    v_scale: torch.Tensor

    @property
    def seq_len(self) -> int:
        return int(self.k_q.shape[2])

    @property
    def nbytes(self) -> int:
        tensors = (
            self.k_q,
            self.k_scale,
            self.v_q,
            self.v_scale,
        )

        return sum(
            t.numel() * t.element_size()
            for t in tensors
        )


def quantize_tensor_int8(
    x: torch.Tensor,
) -> tuple[
    torch.Tensor,
    torch.Tensor,
]:
    """
    Quantize [B, H, T, D] using one scale
    per [B, H, T] vector across head_dim D.

    Stored:
      values -> INT8
      scale  -> FP16, shape [B, H, T, 1]
    """

    if x.ndim != 4:
        raise ValueError(
            "Expected tensor shape [B, H, T, D]"
        )

    x_fp32 = x.float()

    scale = (
        x_fp32.abs()
        .amax(
            dim=-1,
            keepdim=True,
        )
        / INT8_QMAX
    )

    scale = scale.clamp_min(
        1e-8
    )

                                            
    scale_fp16 = scale.to(
        torch.float16
    )

                                               
                              
    scale_for_q = scale_fp16.float()

    q = torch.round(
        x_fp32 / scale_for_q
    ).clamp(
        -INT8_QMAX,
        INT8_QMAX,
    ).to(
        torch.int8
    )

    return q, scale_fp16


def dequantize_tensor_int8(
    q: torch.Tensor,
    scale: torch.Tensor,
    dtype: torch.dtype,
) -> torch.Tensor:
    if q.dtype != torch.int8:
        raise ValueError(
            "Quantized tensor must be int8"
        )

    if scale.dtype != torch.float16:
        raise ValueError(
            "Scale tensor must be float16"
        )

    return (
        q.float()
        * scale.float()
    ).to(dtype)


def quantize_kv(
    k: torch.Tensor,
    v: torch.Tensor,
) -> QuantizedKV:
    if k.shape != v.shape:
        raise ValueError(
            "K and V shapes must match"
        )

    k_q, k_scale = (
        quantize_tensor_int8(k)
    )

    v_q, v_scale = (
        quantize_tensor_int8(v)
    )

    return QuantizedKV(
        k_q=k_q,
        k_scale=k_scale,
        v_q=v_q,
        v_scale=v_scale,
    )


def dequantize_kv(
    cache: QuantizedKV,
    dtype: torch.dtype,
) -> tuple[
    torch.Tensor,
    torch.Tensor,
]:
    k = dequantize_tensor_int8(
        cache.k_q,
        cache.k_scale,
        dtype,
    )

    v = dequantize_tensor_int8(
        cache.v_q,
        cache.v_scale,
        dtype,
    )

    return k, v


def append_quantized_kv(
    cache: QuantizedKV,
    k_new: torch.Tensor,
    v_new: torch.Tensor,
) -> QuantizedKV:
    """
    Append newly-created float K/V values to an existing
    INT8 KV cache.

    Existing cached tokens are NOT requantized.
    Only k_new / v_new are quantized once.
    """

    if k_new.shape != v_new.shape:
        raise ValueError(
            "New K and V shapes must match"
        )

    if k_new.ndim != 4:
        raise ValueError(
            "Expected new K/V shape [B, H, T, D]"
        )

    newest = quantize_kv(
        k_new,
        v_new,
    )

    if (
        cache.k_q.shape[0]
        != newest.k_q.shape[0]
        or cache.k_q.shape[1]
        != newest.k_q.shape[1]
        or cache.k_q.shape[3]
        != newest.k_q.shape[3]
    ):
        raise ValueError(
            "Existing and new KV cache dimensions "
            "are incompatible"
        )

    return QuantizedKV(
        k_q=torch.cat(
            (
                cache.k_q,
                newest.k_q,
            ),
            dim=2,
        ),
        k_scale=torch.cat(
            (
                cache.k_scale,
                newest.k_scale,
            ),
            dim=2,
        ),
        v_q=torch.cat(
            (
                cache.v_q,
                newest.v_q,
            ),
            dim=2,
        ),
        v_scale=torch.cat(
            (
                cache.v_scale,
                newest.v_scale,
            ),
            dim=2,
        ),
    )
