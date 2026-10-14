
#include "tokenizer_runtime.h"
#include <Arduino.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <soc/soc_memory_types.h>


 
 
 
 
 
 

#define setup resolver_probe_setup_unused
#define loop  resolver_probe_loop_unused

#include "model_storage.inc"

#undef setup
#undef loop


 
 
 
 
 
 
 
 
 
 
 
 
 

#define setup lmhead_bench_setup_unused
#define loop  lmhead_bench_loop_unused

#include "output_projection.inc"

#undef setup
#undef loop


namespace {

constexpr float RMS_EPS =
    1.0e-5f;

constexpr std::size_t TOP_K =
    8;


 
 
 

constexpr std::uint32_t
    MODEL_LM_HEAD_OFFSET =
        0x001000;

constexpr std::uint32_t
    MODEL_NORMS_OFFSET =
        0x167000;

constexpr std::uint32_t
    MODEL_NORMS_BYTES =
        22400;

constexpr std::uint32_t
    NORM_ROW_BYTES =
        896;

constexpr std::uint32_t
    FINAL_NORM_RELATIVE =
        21504;


 
 
 

constexpr std::uint32_t
    EMBEDDING_BYTES =
        5505024;

constexpr std::uint32_t
    EMBEDDING_ROW_BYTES =
        896;

const char* EMBEDDING_PATH =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "tok_embeddings_fp16.bin";


 
 
 

alignas(16)
float hidden_rt[
    cfg::kDModel
];

alignas(16)
float normed_rt[
    cfg::kDModel
];

alignas(16)
float q_rt[
    cfg::kDModel
];

alignas(16)
float k_rt[
    cfg::kKvDim
];

alignas(16)
float v_rt[
    cfg::kKvDim
];

alignas(16)
float attention_rt[
    cfg::kDModel
];

alignas(16)
float o_rt[
    cfg::kDModel
];

alignas(16)
float gate_rt[
    cfg::kDff
];

alignas(16)
float up_rt[
    cfg::kDff
];

alignas(16)
float down_rt[
    cfg::kDModel
];

alignas(16)
float attn_norm_weight_rt[
    cfg::kDModel
];

alignas(16)
float ffn_norm_weight_rt[
    cfg::kDModel
];

alignas(16)
float final_norm_weight_rt[
    cfg::kDModel
];

alignas(4)
std::uint16_t fp16_row_rt[
    cfg::kDModel
];


 
 
 
 
 
 
 
 
 
 
 

static inline float
runtime_q4_lut_load_addx4(
    const float* base,
    std::uint32_t index
) {

#if defined(__XTENSA__)

    std::uintptr_t addr;

    const std::uintptr_t base_addr =
        reinterpret_cast<
            std::uintptr_t
        >(base);

    __asm__ __volatile__(
        "addx4 %0, %1, %2"
        : "=r"(addr)
        : "r"(index),
          "r"(base_addr)
    );

    return *reinterpret_cast<
        const float*
    >(addr);

#else

    return base[index];

#endif
}


#define RUNTIME_STORE4_F32_AES3(P,X0,X1,X2,X3) \
    do { \
        __asm__ volatile ( \
            "EE.STF.128.IP " \
            "%4, %3, %2, %1, %0, 16" \
            : "+r"(P) \
            : "f"(X0), \
              "f"(X1), \
              "f"(X2), \
              "f"(X3) \
            : "memory" \
        ); \
    } while (0)


#define RUNTIME_BUILD_LUT(LUT, SCALE) \
    do { \
        float* rt_store_p_ = (LUT); \
        const float rt_s1_ = \
            (SCALE); \
        const float rt_s2_ = \
            rt_s1_ + rt_s1_; \
        const float rt_s3_ = \
            rt_s2_ + rt_s1_; \
        RUNTIME_STORE4_F32_AES3( \
            rt_store_p_, \
            0.0f, \
            rt_s1_, \
            rt_s2_, \
            rt_s3_ \
        ); \
        const float rt_s4_ = \
            rt_s2_ + rt_s2_; \
        const float rt_s5_ = \
            rt_s4_ + rt_s1_; \
        const float rt_s6_ = \
            rt_s4_ + rt_s2_; \
        const float rt_s7_ = \
            rt_s6_ + rt_s1_; \
        RUNTIME_STORE4_F32_AES3( \
            rt_store_p_, \
            rt_s4_, \
            rt_s5_, \
            rt_s6_, \
            rt_s7_ \
        ); \
        const float rt_s8_ = \
            rt_s4_ + rt_s4_; \
        const float rt_n8_ = \
            -rt_s8_; \
        const float rt_n7_ = \
            -rt_s7_; \
        const float rt_n6_ = \
            -rt_s6_; \
        const float rt_n5_ = \
            -rt_s5_; \
        RUNTIME_STORE4_F32_AES3( \
            rt_store_p_, \
            rt_n8_, \
            rt_n7_, \
            rt_n6_, \
            rt_n5_ \
        ); \
        const float rt_n4_ = \
            -rt_s4_; \
        const float rt_n3_ = \
            -rt_s3_; \
        const float rt_n2_ = \
            -rt_s2_; \
        const float rt_n1_ = \
            -rt_s1_; \
        RUNTIME_STORE4_F32_AES3( \
            rt_store_p_, \
            rt_n4_, \
            rt_n3_, \
            rt_n2_, \
            rt_n1_ \
        ); \
    } while (0)

#define RUNTIME_BUILD_LUT_F10_STF(LUT, SCALE_BITS, ZERO_F1) \
    do { \
        __asm__ __volatile__( \
            "wfr f10, %[bits]\n\t" \
            \
            "add.s f5, f10, f10\n\t" \
            "add.s f6, f5, f10\n\t" \
            "ee.stf.128.ip f6, f5, f10, %[zero], %[ptr], 16\n\t" \
            \
            "add.s f3, f5, f5\n\t" \
            "add.s f8, f3, f10\n\t" \
            "add.s f4, f5, f3\n\t" \
            "add.s f7, f4, f10\n\t" \
            "ee.stf.128.ip f7, f4, f8, f3, %[ptr], 16\n\t" \
            \
            "add.s f9, f3, f3\n\t" \
            "neg.s f7, f7\n\t" \
            "neg.s f4, f4\n\t" \
            "neg.s f8, f8\n\t" \
            "neg.s f9, f9\n\t" \
            "ee.stf.128.ip f8, f4, f7, f9, %[ptr], 16\n\t" \
            \
            "neg.s f4, f3\n\t" \
            "neg.s f6, f6\n\t" \
            "neg.s f5, f5\n\t" \
            "neg.s f10, f10\n\t" \
            "ee.stf.128.ip f10, f5, f6, f4, %[ptr], -48\n\t" \
            \
            : \
            : [ptr] "r"(LUT), \
              [bits] "r"(SCALE_BITS), \
              [zero] "f"(ZERO_F1) \
            : \
              "f3", "f4", "f5", "f6", \
              "f7", "f8", "f9", "f10", \
              "memory" \
        ); \
    } while (0)



__attribute__((always_inline)) static inline float runtime_fp16_to_float_inline(std::uint16_t h) {
    const std::uint32_t bits =
        (
            static_cast<std::uint32_t>(h)
            << 13
        )
        + 0x38000000u;

    return __builtin_bit_cast(
        float,
        bits
    );
}


__attribute__((noinline))
IRAM_ATTR
void runtime_q4_rowpair2_fp16_reference(
    const std::uint8_t* packed_q,
    const std::uint16_t* scales_fp16,
    std::size_t rows,
    std::size_t cols,
    const float* input,
    float* output
) {

    const std::size_t groups_per_row =
        cols / 64;

    const std::size_t bytes_per_row =
        cols / 2;

    constexpr std::size_t
        packed_per_group =
            32;


    alignas(16)
    float lut0[16];

    alignas(16)
    float lut1[16];
    register float rt_zero_f1 asm("f1") = 0.0f;
    __asm__ __volatile__("" : "+f"(rt_zero_f1));


    for (
        std::size_t row = 0;
        row < rows;
        row += 2
    ) {

        const std::size_t row0 =
            row;

        const std::size_t row1 =
            row + 1;


        const std::uint8_t* row_q0 =
            packed_q
            + row0
                * bytes_per_row;

        const std::uint8_t* row_q1 =
            packed_q
            + row1
                * bytes_per_row;


        const std::uint16_t* row_s0 =
            scales_fp16
            + row0
                * groups_per_row;

        const std::uint16_t* row_s1 =
            scales_fp16
            + row1
                * groups_per_row;


        float acc0 =
            0.0f;

        float acc1 =
            0.0f;


        const float* xp_group =
            input;


        for (
            std::size_t group = 0;
            group < groups_per_row;
            ++group
        ) {

            const std::uint32_t scale0_bits =
                (
                    static_cast<std::uint32_t>(
                        row_s0[group]
                    )
                    << 13
                )
                + 0x38000000u;

            const std::uint32_t scale1_bits =
                (
                    static_cast<std::uint32_t>(
                        row_s1[group]
                    )
                    << 13
                )
                + 0x38000000u;


            RUNTIME_BUILD_LUT_F10_STF(
                lut0,
                scale0_bits,
                rt_zero_f1
            );

            RUNTIME_BUILD_LUT_F10_STF(
                lut1,
                scale1_bits,
                rt_zero_f1
            );


            const auto* q0 =
                static_cast<
                    const std::uint32_t*
                >(
                    __builtin_assume_aligned(
                        row_q0
                        + group
                            * packed_per_group,
                        4
                    )
                );


            const auto* q1 =
                static_cast<
                    const std::uint32_t*
                >(
                    __builtin_assume_aligned(
                        row_q1
                        + group
                            * packed_per_group,
                        4
                    )
                );


#define RT_ACC_ONE(W0,W1,SHIFT,XV) \
            do { \
                const float xv__ = \
                    (XV); \
                const std::uint32_t i0__ = \
                    ((W0) >> (SHIFT)) \
                    & 0x0Fu; \
                const std::uint32_t i1__ = \
                    ((W1) >> (SHIFT)) \
                    & 0x0Fu; \
                acc0 += \
                    runtime_q4_lut_load_addx4( \
                        lut0, \
                        i0__ \
                    ) * xv__; \
                acc1 += \
                    runtime_q4_lut_load_addx4( \
                        lut1, \
                        i1__ \
                    ) * xv__; \
            } while (0)


#define RT_LOAD4_F32_AES3(P,X0,X1,X2,X3) \
            do { \
                __asm__ volatile ( \
                    "EE.LDF.128.IP " \
                    "%3, %2, %1, %0, %4, 16" \
                    : "=&f"(X0), \
                      "=&f"(X1), \
                      "=&f"(X2), \
                      "=&f"(X3), \
                      "+r"(P) \
                    : \
                    : "memory" \
                ); \
            } while (0)


#define RT_ACC_WORD(W0,W1) \
            do { \
                float x0__; \
                float x1__; \
                float x2__; \
                float x3__; \
                RT_LOAD4_F32_AES3( \
                    xp_group, \
                    x0__, \
                    x1__, \
                    x2__, \
                    x3__ \
                ); \
                RT_ACC_ONE(W0,W1, 0,x0__); \
                RT_ACC_ONE(W0,W1, 4,x1__); \
                RT_ACC_ONE(W0,W1, 8,x2__); \
                RT_ACC_ONE(W0,W1,12,x3__); \
                RT_LOAD4_F32_AES3( \
                    xp_group, \
                    x0__, \
                    x1__, \
                    x2__, \
                    x3__ \
                ); \
                RT_ACC_ONE(W0,W1,16,x0__); \
                RT_ACC_ONE(W0,W1,20,x1__); \
                RT_ACC_ONE(W0,W1,24,x2__); \
                RT_ACC_ONE(W0,W1,28,x3__); \
            } while (0)

            {
                const std::uint32_t w0 =
                    q0[0];

                const std::uint32_t w1 =
                    q1[0];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[1];

                const std::uint32_t w1 =
                    q1[1];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[2];

                const std::uint32_t w1 =
                    q1[2];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[3];

                const std::uint32_t w1 =
                    q1[3];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[4];

                const std::uint32_t w1 =
                    q1[4];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[5];

                const std::uint32_t w1 =
                    q1[5];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[6];

                const std::uint32_t w1 =
                    q1[6];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }

            {
                const std::uint32_t w0 =
                    q0[7];

                const std::uint32_t w1 =
                    q1[7];

                RT_ACC_WORD(
                    w0,
                    w1
                );
            }


#undef RT_ACC_WORD
#undef RT_LOAD4_F32_AES3
#undef RT_ACC_ONE
        }


        output[row0] =
            acc0;

        output[row1] =
            acc1;
    }
}


#include "q4_two_token_kernel.inc"
#include "q4_feed_forward_kernel.inc"

static bool rego_enabled=true;
#ifdef REGO_PROBE
static unsigned rego_coverage[2];
#endif
#include "q4_register_offset_kernel.inc"
#include "q4_four_row_kernel.inc"
#include "q4_kernel_dispatch.inc"

#undef RUNTIME_BUILD_LUT
#undef RUNTIME_STORE4_F32_AES3


 
 
 

bool runMatrix(
    std::uint32_t layer,
    xp::MatrixFamily family,
    const float* input,
    float* output
) {

    ResolvedMatrix r{};


    if (
        !resolveMatrix(
            layer,
            family,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &r
        )
    ) {
        Serial.printf(
            "MATRIX_RESOLVE: FAIL|"
            "L%u|F%u\n",
            static_cast<unsigned>(
                layer
            ),
            static_cast<unsigned>(
                family
            )
        );

        return false;
    }


    if (
        !r.spec
        ||
        r.spec->rows % 2u
            != 0u
        ||
        r.spec->cols % 64u
            != 0u
    ) {
        Serial.printf(
            "MATRIX_SHAPE: FAIL|"
            "L%u|F%u\n",
            static_cast<unsigned>(
                layer
            ),
            static_cast<unsigned>(
                family
            )
        );

        return false;
    }


    runtime_q4_rowpair2_fp16(
        r.q4,
        r.scales,
        r.spec->rows,
        r.spec->cols,
        input,
        output
    );


    return true;
}


 
 
 
 
 
 

void runtimeRmsNorm(
    const float* input,
    const float* weight,
    float* output,
    std::size_t count
) {

    double sum_sq =
        0.0;


    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {
        sum_sq +=
            static_cast<double>(
                input[i]
            )
            *
            static_cast<double>(
                input[i]
            );
    }


    const float mean_sq =
        static_cast<float>(
            sum_sq
            /
            static_cast<double>(
                count
            )
        );


    const float inv_rms =
        1.0f
        /
        sqrtf(
            mean_sq
            + RMS_EPS
        );


    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {
        output[i] =
            input[i]
            * inv_rms
            * weight[i];
    }
}


 
 
 

bool loadNormRow(
    std::uint32_t relative_offset,
    float* output
) {

    if (
        !g_model_base
        ||
        !output
        ||
        relative_offset
            + NORM_ROW_BYTES
            > MODEL_NORMS_BYTES
    ) {
        return false;
    }


    const auto* src =
        reinterpret_cast<
            const std::uint16_t*
        >(
            g_model_base
            + MODEL_NORMS_OFFSET
            + relative_offset
        );


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        output[i] =
            fp16_to_float(
                src[i]
            );
    }


    return true;
}


 
 
 

 
File g_embedding_file;


bool loadEmbeddingRow(
    std::uint32_t token_id,
    float* output
) {

    if (
        token_id
            >= cfg::kVocabSize
    ) {
        return false;
    }


    File& f = g_embedding_file;

    if (!f) {
        f =
            SD.open(
                EMBEDDING_PATH,
                FILE_READ
            );
    }


    if (!f) {
        return false;
    }


    if (
        static_cast<
            std::uint32_t
        >(f.size())
        != EMBEDDING_BYTES
    ) {
        f.close();

        return false;
    }


    const std::uint32_t offset =
        token_id
        * EMBEDDING_ROW_BYTES;


    const bool read_ok =
        readFileExactAt(
            f,
            offset,
            reinterpret_cast<
                std::uint8_t*
            >(fp16_row_rt),
            EMBEDDING_ROW_BYTES
        );


    if (!read_ok) {
        f.close();

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        output[i] =
            fp16_to_float(
                fp16_row_rt[i]
            );
    }


    return true;
}


 
 
 

bool finiteVector(
    const float* values,
    std::size_t count
) {

    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {
        if (!isfinite(values[i])) {
            return false;
        }
    }


    return true;
}


double vectorChecksum(
    const float* values,
    std::size_t count
) {

    double sum =
        0.0;


    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {
        sum +=
            static_cast<double>(
                values[i]
            );
    }


    return sum;
}


double vectorL2(
    const float* values,
    std::size_t count
) {

    double sum =
        0.0;


    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {

        const double v =
            static_cast<double>(
                values[i]
            );

        sum +=
            v * v;
    }


    return sqrt(sum);
}


float vectorMaxAbs(
    const float* values,
    std::size_t count
) {

    float result =
        0.0f;


    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {

        const float a =
            fabsf(
                values[i]
            );


        if (a > result) {
            result = a;
        }
    }


    return result;
}


void printVectorStats(
    const char* label,
    const float* values,
    std::size_t count
) {

    Serial.printf(
        "%s|"
        "l2=%.9f|"
        "checksum=%.9f|"
        "max_abs=%.9f|"
        "finite=%s\n",

        label,

        vectorL2(
            values,
            count
        ),

        vectorChecksum(
            values,
            count
        ),

        static_cast<double>(
            vectorMaxAbs(
                values,
                count
            )
        ),

        finiteVector(
            values,
            count
        )
            ? "yes"
            : "NO"
    );
}


 
 
 
 
 
 
 
 
 
 
 
 

bool runLayerPos0(
    std::uint32_t layer
) {

    const std::uint32_t
        attn_norm_relative =
            layer
            * 2u
            * NORM_ROW_BYTES;


    const std::uint32_t
        ffn_norm_relative =
            attn_norm_relative
            + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {
        Serial.printf(
            "NORM_LOAD: FAIL|L%u\n",
            static_cast<unsigned>(
                layer
            )
        );

        return false;
    }


    const std::uint32_t t0 =
        millis();


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::QProj,
            normed_rt,
            q_rt
        )
    ) {
        return false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::KProj,
            normed_rt,
            k_rt
        )
    ) {
        return false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::VProj,
            normed_rt,
            v_rt
        )
    ) {
        return false;
    }


     
     
    for (
        std::size_t h = 0;
        h < cfg::kNHeads;
        ++h
    ) {

        const std::size_t kv_h =
            h / cfg::kKvRepeat;


        for (
            std::size_t d = 0;
            d < cfg::kHeadDim;
            ++d
        ) {

            attention_rt[
                h
                * cfg::kHeadDim
                + d
            ] =
                v_rt[
                    kv_h
                    * cfg::kHeadDim
                    + d
                ];
        }
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::OProj,
            attention_rt,
            o_rt
        )
    ) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
    ) {
        return false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        const float silu =
            g
            /
            (
                1.0f
                + expf(-g)
            );


        gate_rt[i] =
            silu
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        hidden_rt[i] +=
            down_rt[i];
    }


    const std::uint32_t elapsed =
        millis() - t0;


    Serial.printf(
        "LAYER|%02u|ms=%lu|"
        "l2=%.9f|"
        "checksum=%.9f|"
        "max_abs=%.9f|"
        "finite=%s\n",

        static_cast<unsigned>(
            layer
        ),

        static_cast<unsigned long>(
            elapsed
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        ),

        static_cast<double>(
            vectorMaxAbs(
                hidden_rt,
                cfg::kDModel
            )
        ),

        finiteVector(
            hidden_rt,
            cfg::kDModel
        )
            ? "yes"
            : "NO"
    );


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 

void insertTop8(
    std::uint32_t token,
    float score,
    std::uint32_t top_tokens[TOP_K],
    float top_scores[TOP_K]
) {

    for (
        std::size_t i = 0;
        i < TOP_K;
        ++i
    ) {

        if (
            score
            > top_scores[i]
        ) {

            for (
                std::size_t j =
                    TOP_K - 1;
                j > i;
                --j
            ) {

                top_scores[j] =
                    top_scores[j - 1];

                top_tokens[j] =
                    top_tokens[j - 1];
            }


            top_scores[i] =
                score;

            top_tokens[i] =
                token;

            return;
        }
    }
}


bool runMixedLmHead(
    const float* final_normed
) {

    const std::uint8_t* lm_base =
        g_model_base
        + MODEL_LM_HEAD_OFFSET;


    const std::uint8_t* q4 =
        lm_base
        + OFF_Q4;


    const auto* scales_f16 =
        reinterpret_cast<
            const std::uint16_t*
        >(
            lm_base
            + OFF_SCALES_F16
        );


    const auto* repair_ids =
        reinterpret_cast<
            const std::uint32_t*
        >(
            lm_base
            + OFF_REPAIR_IDS
        );


    const auto* repair_weights =
        reinterpret_cast<
            const std::int8_t*
        >(
            lm_base
            + OFF_REPAIR_WEIGHTS
        );


    const auto* repair_scales =
        reinterpret_cast<
            const float*
        >(
            lm_base
            + OFF_REPAIR_SCALES
        );


    const std::uint32_t expected_ids[
        REPAIR_COUNT
    ] = {
        330,
        582,
        1411,
        1631,
        2458
    };


    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {

        if (
            repair_ids[r]
            != expected_ids[r]
        ) {

            Serial.println(
                "REPAIR_IDS: FAIL"
            );

            return false;
        }
    }


    Serial.println(
        "REPAIR_IDS: PASS"
    );


     
    std::memcpy(
        x,
        final_normed,
        cfg::kDModel
            * sizeof(float)
    );


    const std::uint32_t q4_t0 =
        micros();


    q4_head_rowpair2(
        q4,
        scales_f16,
        x,
        logits
    );


    const std::uint32_t q4_us =
        micros() - q4_t0;


    const std::uint32_t repair_t0 =
        micros();


    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {

        const std::uint32_t token =
            repair_ids[r];


        logits[token] =
            int8_repair_score(
                repair_weights
                    + r * D,
                repair_scales[r],
                x
            );
    }


    const std::uint32_t repair_us =
        micros() - repair_t0;


    if (
        !finiteVector(
            logits,
            VOCAB
        )
    ) {

        Serial.println(
            "LM_LOGITS_FINITE: FAIL"
        );

        return false;
    }


    std::uint32_t top_tokens[
        TOP_K
    ] = {};


    float top_scores[
        TOP_K
    ];


    for (
        std::size_t i = 0;
        i < TOP_K;
        ++i
    ) {
        top_scores[i] =
            -INFINITY;
    }


    double checksum =
        0.0;


    for (
        std::uint32_t token = 0;
        token < VOCAB;
        ++token
    ) {

        const float score =
            logits[token];


        checksum +=
            static_cast<double>(
                score
            );


        insertTop8(
            token,
            score,
            top_tokens,
            top_scores
        );
    }


    Serial.printf(
        "LM_HEAD_TIMING|"
        "q4_ms=%.3f|"
        "repair_us=%u\n",

        static_cast<double>(
            q4_us
        ) / 1000.0,

        static_cast<unsigned>(
            repair_us
        )
    );


    Serial.println(
        "=== DEVICE TOP8 ==="
    );


    for (
        std::size_t i = 0;
        i < TOP_K;
        ++i
    ) {

        Serial.printf(
            "TOP|%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                i
            ),

            static_cast<unsigned>(
                top_tokens[i]
            ),

            static_cast<double>(
                top_scores[i]
            )
        );
    }


    const std::uint32_t
        expected_top8[
            TOP_K
        ] = {
            3168,
            6070,
            3745,
            6098,
            454,
            703,
            759,
            4427
        };


    bool top8_match =
        true;


    for (
        std::size_t i = 0;
        i < TOP_K;
        ++i
    ) {

        if (
            top_tokens[i]
            != expected_top8[i]
        ) {
            top8_match =
                false;
        }
    }


    const bool top1_match =
        top_tokens[0]
            == 3168u;


    const double margin =
        static_cast<double>(
            top_scores[0]
        )
        -
        static_cast<double>(
            top_scores[1]
        );


    Serial.printf(
        "TOP1|"
        "token=%u|"
        "score=%.9f|"
        "margin=%.9f\n",

        static_cast<unsigned>(
            top_tokens[0]
        ),

        static_cast<double>(
            top_scores[0]
        ),

        margin
    );


    Serial.printf(
        "LOGITS_CHECKSUM|%.9f\n",
        checksum
    );


    Serial.printf(
        "HOST_TOP1_3168_MATCH: %s\n",
        top1_match
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "HOST_TOP8_IDS_MATCH: %s\n",
        top8_match
            ? "PASS"
            : "FAIL"
    );


    return
        top1_match
        &&
        top8_match;
}

}  


void token0_all12_setup_unused() {

     
     
     
     
     
     
     
     
     
     
     
     

    resolver_probe_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO TOKEN-0 ALL12 "
        "INFERENCE PROBE ==="
    );


    if (
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        Serial.println(
            "RUNTIME_STATE: FAIL"
        );

        return;
    }


    Serial.println(
        "RUNTIME_STATE: PASS"
    );


    static_assert(
        cfg::kNumLayers == 12,
        "expected 12 layers"
    );

    static_assert(
        cfg::kDModel == 448,
        "expected d_model 448"
    );

    static_assert(
        cfg::kDff == 1344,
        "expected d_ff 1344"
    );

    static_assert(
        cfg::kKvDim == 112,
        "expected kv_dim 112"
    );


     
     
     

    constexpr std::uint32_t
        TOKEN_ID =
            0;


    if (
        !loadEmbeddingRow(
            TOKEN_ID,
            hidden_rt
        )
    ) {

        Serial.println(
            "EMBEDDING_LOAD: FAIL"
        );

        return;
    }


    Serial.println(
        "EMBEDDING_LOAD: PASS"
    );


    printVectorStats(
        "EMBEDDING",
        hidden_rt,
        cfg::kDModel
    );


    Serial.printf(
        "EMBEDDING_FIRST8|"
        "%.9f,%.9f,%.9f,%.9f,"
        "%.9f,%.9f,%.9f,%.9f\n",

        static_cast<double>(
            hidden_rt[0]
        ),

        static_cast<double>(
            hidden_rt[1]
        ),

        static_cast<double>(
            hidden_rt[2]
        ),

        static_cast<double>(
            hidden_rt[3]
        ),

        static_cast<double>(
            hidden_rt[4]
        ),

        static_cast<double>(
            hidden_rt[5]
        ),

        static_cast<double>(
            hidden_rt[6]
        ),

        static_cast<double>(
            hidden_rt[7]
        )
    );


    Serial.println(
        "POS0_ROPE_IDENTITY: PASS"
    );


     
     
     

    const std::uint32_t layers_t0 =
        millis();


    for (
        std::uint32_t layer = 0;
        layer < cfg::kNumLayers;
        ++layer
    ) {

        if (
            !runLayerPos0(
                layer
            )
        ) {

            Serial.printf(
                "ALL12_FORWARD: FAIL|"
                "layer=%u\n",
                static_cast<unsigned>(
                    layer
                )
            );

            return;
        }
    }


    const std::uint32_t layers_ms =
        millis() - layers_t0;


    Serial.printf(
        "ALL12_FORWARD: PASS|"
        "ms=%lu\n",
        static_cast<unsigned long>(
            layers_ms
        )
    );


    printVectorStats(
        "HIDDEN_AFTER_L12",
        hidden_rt,
        cfg::kDModel
    );


    Serial.printf(
        "HIDDEN_AFTER_L12_FIRST8|"
        "%.9f,%.9f,%.9f,%.9f,"
        "%.9f,%.9f,%.9f,%.9f\n",

        static_cast<double>(
            hidden_rt[0]
        ),

        static_cast<double>(
            hidden_rt[1]
        ),

        static_cast<double>(
            hidden_rt[2]
        ),

        static_cast<double>(
            hidden_rt[3]
        ),

        static_cast<double>(
            hidden_rt[4]
        ),

        static_cast<double>(
            hidden_rt[5]
        ),

        static_cast<double>(
            hidden_rt[6]
        ),

        static_cast<double>(
            hidden_rt[7]
        )
    );


     
     
     

    if (
        !loadNormRow(
            FINAL_NORM_RELATIVE,
            final_norm_weight_rt
        )
    ) {

        Serial.println(
            "FINAL_NORM_LOAD: FAIL"
        );

        return;
    }


    runtimeRmsNorm(
        hidden_rt,
        final_norm_weight_rt,
        x,
        cfg::kDModel
    );


    Serial.println(
        "FINAL_NORM_LOAD: PASS"
    );


    printVectorStats(
        "FINAL_NORMED",
        x,
        cfg::kDModel
    );


    Serial.printf(
        "FINAL_NORMED_FIRST8|"
        "%.9f,%.9f,%.9f,%.9f,"
        "%.9f,%.9f,%.9f,%.9f\n",

        static_cast<double>(
            x[0]
        ),

        static_cast<double>(
            x[1]
        ),

        static_cast<double>(
            x[2]
        ),

        static_cast<double>(
            x[3]
        ),

        static_cast<double>(
            x[4]
        ),

        static_cast<double>(
            x[5]
        ),

        static_cast<double>(
            x[6]
        ),

        static_cast<double>(
            x[7]
        )
    );


     
     
     

    const bool lm_ok =
        runMixedLmHead(
            x
        );


    Serial.println();


    Serial.printf(
        "TOKEN0_MIXED_LMHEAD: %s\n",
        lm_ok
            ? "PASS"
            : "FAIL"
    );


    const bool final_ok =
        lm_ok
        &&
        finiteVector(
            hidden_rt,
            cfg::kDModel
        )
        &&
        finiteVector(
            x,
            cfg::kDModel
        );


    Serial.printf(
        "TOKEN0_ALL12_INFERENCE_GATE: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void token0_all12_loop_unused() {
    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 

namespace {

constexpr std::size_t
    HELLO_MAX_CONTEXT =
        64;

constexpr std::size_t
    HELLO_PROMPT_COUNT =
        4;

constexpr std::size_t
    HELLO_TOP_K =
        8;

constexpr float
    HELLO_ROPE_THETA =
        10000.0f;


const char* HELLO_TOK_NORMALIZER =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "tokenizer_normalizer.bin";

const char* HELLO_TOK_ENCODER =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "tokenizer_encoder.bin";

const char* HELLO_TOK_DECODER =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "tokenizer_decoder.bin";


pocketlm::TokenizerRuntime
    hello_tokenizer{};


 
 
 
 
 
 

constexpr std::size_t
    HELLO_KV_VALUES_ONE =
        cfg::kNumLayers
        * HELLO_MAX_CONTEXT
        * cfg::kKvDim;

constexpr std::size_t
    HELLO_KV_BYTES_ONE =
        HELLO_KV_VALUES_ONE
        * sizeof(float);

constexpr std::size_t
    HELLO_KV_BYTES_TOTAL =
        HELLO_KV_BYTES_ONE
        * 2;


float* hello_kv_block =
    nullptr;

float* hello_kv_k =
    nullptr;

float* hello_kv_v =
    nullptr;


alignas(16)
float hello_attn_scores[
    HELLO_MAX_CONTEXT
];


alignas(16)
float hello_rope_cos[
    cfg::kHeadDim / 2
];

alignas(16)
float hello_rope_sin[
    cfg::kHeadDim / 2
];


std::size_t
    hello_rope_cached_position =
        static_cast<std::size_t>(-1);


 
 
 

bool helloInitTokenizer() {

    File normalizer =
        SD.open(
            HELLO_TOK_NORMALIZER,
            FILE_READ
        );

    File encoder =
        SD.open(
            HELLO_TOK_ENCODER,
            FILE_READ
        );

    File decoder =
        SD.open(
            HELLO_TOK_DECODER,
            FILE_READ
        );


    if (
        !normalizer
        ||
        !encoder
        ||
        !decoder
    ) {

        Serial.println(
            "TOKENIZER_SD_FILES: FAIL"
        );

        return false;
    }


    Serial.printf(
        "TOKENIZER_FILES|"
        "normalizer=%lu|"
        "encoder=%lu|"
        "decoder=%lu\n",

        static_cast<unsigned long>(
            normalizer.size()
        ),

        static_cast<unsigned long>(
            encoder.size()
        ),

        static_cast<unsigned long>(
            decoder.size()
        )
    );


    const uint32_t t0 =
        millis();


    const bool ok =
        pocketlm::tokenizer_runtime_init(
            &hello_tokenizer,
            &normalizer,
            &encoder,
            &decoder
        );


    Serial.printf(
        "TOKENIZER_SD_INIT: %s|"
        "ms=%lu|"
        "free_psram=%lu\n",

        ok ? "PASS" : "FAIL",

        static_cast<unsigned long>(
            millis() - t0
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        )
    );


    return ok;
}


 
 
 

bool helloAllocateKv() {

    hello_kv_block =
        static_cast<float*>(
            heap_caps_malloc(
                HELLO_KV_BYTES_TOTAL,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!hello_kv_block) {

        Serial.printf(
            "KV_CACHE_ALLOC: FAIL|"
            "bytes=%lu|"
            "free_psram=%lu|"
            "largest=%lu\n",

            static_cast<unsigned long>(
                HELLO_KV_BYTES_TOTAL
            ),

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return false;
    }


    hello_kv_k =
        hello_kv_block;

    hello_kv_v =
        hello_kv_block
        + HELLO_KV_VALUES_ONE;


    std::memset(
        hello_kv_block,
        0,
        HELLO_KV_BYTES_TOTAL
    );


    Serial.printf(
        "KV_CACHE_ALLOC: PASS|"
        "bytes=%lu|"
        "one=%lu|"
        "free_psram=%lu|"
        "largest=%lu\n",

        static_cast<unsigned long>(
            HELLO_KV_BYTES_TOTAL
        ),

        static_cast<unsigned long>(
            HELLO_KV_BYTES_ONE
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


    return true;
}


std::size_t helloKvOffset(
    std::size_t layer,
    std::size_t position,
    std::size_t kv_head
) {

    return (
        (
            layer
            * HELLO_MAX_CONTEXT
            + position
        )
        * cfg::kKvDim
    )
    + kv_head
        * cfg::kHeadDim;
}


void helloStoreCurrentKv(
    std::size_t layer,
    std::size_t position
) {

    for (
        std::size_t kv_head = 0;
        kv_head < cfg::kNKvHeads;
        ++kv_head
    ) {

        const std::size_t off =
            helloKvOffset(
                layer,
                position,
                kv_head
            );


        std::memcpy(
            hello_kv_k + off,
            k_rt
                + kv_head
                    * cfg::kHeadDim,
            cfg::kHeadDim
                * sizeof(float)
        );


        std::memcpy(
            hello_kv_v + off,
            v_rt
                + kv_head
                    * cfg::kHeadDim,
            cfg::kHeadDim
                * sizeof(float)
        );
    }
}


 
 
 

void helloPrepareRope(
    std::size_t position
) {

    if (
        hello_rope_cached_position
            == position
    ) {
        return;
    }


    std::size_t pair =
        0;


    for (
        std::size_t i = 0;
        i + 1 < cfg::kHeadDim;
        i += 2,
        ++pair
    ) {

        const float inv_freq =
            1.0f
            /
            powf(
                HELLO_ROPE_THETA,
                static_cast<float>(i)
                /
                static_cast<float>(
                    cfg::kHeadDim
                )
            );


        const float angle =
            static_cast<float>(
                position
            )
            * inv_freq;


        hello_rope_cos[pair] =
            cosf(angle);

        hello_rope_sin[pair] =
            sinf(angle);
    }


    hello_rope_cached_position =
        position;
}


void helloApplyRopeOne(
    float* vec
) {

    std::size_t pair =
        0;


    for (
        std::size_t i = 0;
        i + 1 < cfg::kHeadDim;
        i += 2,
        ++pair
    ) {

        const float x0 =
            vec[i];

        const float x1 =
            vec[i + 1];

        const float c =
            hello_rope_cos[pair];

        const float s =
            hello_rope_sin[pair];


        vec[i] =
            x0 * c
            - x1 * s;

        vec[i + 1] =
            x0 * s
            + x1 * c;
    }
}


void helloApplyRopeQk(
    std::size_t position
) {

    helloPrepareRope(
        position
    );


    for (
        std::size_t h = 0;
        h < cfg::kNHeads;
        ++h
    ) {

        helloApplyRopeOne(
            q_rt
            + h
                * cfg::kHeadDim
        );
    }


    for (
        std::size_t h = 0;
        h < cfg::kNKvHeads;
        ++h
    ) {

        helloApplyRopeOne(
            k_rt
            + h
                * cfg::kHeadDim
        );
    }
}


 
 
 

void helloAttentionFromCache(
    std::size_t layer,
    std::size_t position
) {

    const float score_scale =
        1.0f
        /
        sqrtf(
            static_cast<float>(
                cfg::kHeadDim
            )
        );


    for (
        std::size_t head = 0;
        head < cfg::kNHeads;
        ++head
    ) {

        const std::size_t kv_head =
            head
            / cfg::kKvRepeat;


        const float* q =
            q_rt
            + head
                * cfg::kHeadDim;


        float max_score =
            -INFINITY;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float* k =
                hello_kv_k
                + helloKvOffset(
                    layer,
                    t,
                    kv_head
                );


            float score =
                0.0f;


            for (
                std::size_t d = 0;
                d < cfg::kHeadDim;
                ++d
            ) {

                score +=
                    q[d]
                    * k[d];
            }


            score *=
                score_scale;


            hello_attn_scores[t] =
                score;


            if (
                score
                > max_score
            ) {

                max_score =
                    score;
            }
        }


        float denom =
            0.0f;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float e =
                expf(
                    hello_attn_scores[t]
                    - max_score
                );


            hello_attn_scores[t] =
                e;

            denom +=
                e;
        }


        float* dst =
            attention_rt
            + head
                * cfg::kHeadDim;


        for (
            std::size_t d = 0;
            d < cfg::kHeadDim;
            ++d
        ) {

            dst[d] =
                0.0f;
        }


        const float inv_denom =
            1.0f
            / denom;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float prob =
                hello_attn_scores[t]
                * inv_denom;


            const float* v =
                hello_kv_v
                + helloKvOffset(
                    layer,
                    t,
                    kv_head
                );


            for (
                std::size_t d = 0;
                d < cfg::kHeadDim;
                ++d
            ) {

                dst[d] +=
                    prob
                    * v[d];
            }
        }
    }
}


 
 
 

bool helloRunLayerPosition(
    std::uint32_t layer,
    std::size_t position,
    bool print_reference_stats
) {

    const std::uint32_t
        attn_norm_relative =
            layer
            * 2u
            * NORM_ROW_BYTES;


    const std::uint32_t
        ffn_norm_relative =
            attn_norm_relative
            + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        Serial.printf(
            "NORM_LOAD: FAIL|"
            "L%u|pos=%u\n",
            static_cast<unsigned>(
                layer
            ),
            static_cast<unsigned>(
                position
            )
        );

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::QProj,
            normed_rt,
            q_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::KProj,
            normed_rt,
            k_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::VProj,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::OProj,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    const bool finite =
        finiteVector(
            hidden_rt,
            cfg::kDModel
        );


    if (print_reference_stats) {

        Serial.printf(
            "PROMPT_LAST_LAYER|%02u|"
            "l2=%.9f|"
            "checksum=%.9f|"
            "max_abs=%.9f|"
            "finite=%s\n",

            static_cast<unsigned>(
                layer
            ),

            vectorL2(
                hidden_rt,
                cfg::kDModel
            ),

            vectorChecksum(
                hidden_rt,
                cfg::kDModel
            ),

            static_cast<double>(
                vectorMaxAbs(
                    hidden_rt,
                    cfg::kDModel
                )
            ),

            finite
                ? "yes"
                : "NO"
        );
    }


    return finite;
}


 
 
 

bool helloRunToken(
    std::uint32_t token,
    std::size_t position,
    bool print_last_layers
) {

    if (
        position
        >= HELLO_MAX_CONTEXT
    ) {

        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {

        Serial.printf(
            "EMBEDDING: FAIL|"
            "pos=%u|token=%u\n",

            static_cast<unsigned>(
                position
            ),

            static_cast<unsigned>(
                token
            )
        );

        return false;
    }


    const uint32_t t0 =
        millis();


    for (
        std::uint32_t layer = 0;
        layer < cfg::kNumLayers;
        ++layer
    ) {

        if (
            !helloRunLayerPosition(
                layer,
                position,
                print_last_layers
            )
        ) {

            Serial.printf(
                "TOKEN_FORWARD: FAIL|"
                "pos=%u|"
                "token=%u|"
                "layer=%u\n",

                static_cast<unsigned>(
                    position
                ),

                static_cast<unsigned>(
                    token
                ),

                static_cast<unsigned>(
                    layer
                )
            );

            return false;
        }
    }


    Serial.printf(
        "TOKEN_FORWARD: PASS|"
        "pos=%u|"
        "token=%u|"
        "ms=%lu|"
        "hidden_l2=%.9f|"
        "checksum=%.9f\n",

        static_cast<unsigned>(
            position
        ),

        static_cast<unsigned>(
            token
        ),

        static_cast<unsigned long>(
            millis() - t0
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


    return true;
}


 
 
 

bool helloPredictFirst(
    std::uint32_t* predicted_token
) {

    if (
        !loadNormRow(
            FINAL_NORM_RELATIVE,
            final_norm_weight_rt
        )
    ) {

        Serial.println(
            "FINAL_NORM_LOAD: FAIL"
        );

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        final_norm_weight_rt,
        x,
        cfg::kDModel
    );


    printVectorStats(
        "HELLO_FINAL_PROMPT_NORM",
        x,
        cfg::kDModel
    );


    const std::uint8_t* lm_base =
        g_model_base
        + MODEL_LM_HEAD_OFFSET;


    const std::uint8_t* q4 =
        lm_base
        + OFF_Q4;


    const auto* scales_f16 =
        reinterpret_cast<
            const std::uint16_t*
        >(
            lm_base
            + OFF_SCALES_F16
        );


    const auto* repair_ids =
        reinterpret_cast<
            const std::uint32_t*
        >(
            lm_base
            + OFF_REPAIR_IDS
        );


    const auto* repair_weights =
        reinterpret_cast<
            const std::int8_t*
        >(
            lm_base
            + OFF_REPAIR_WEIGHTS
        );


    const auto* repair_scales =
        reinterpret_cast<
            const float*
        >(
            lm_base
            + OFF_REPAIR_SCALES
        );


    q4_head_rowpair2(
        q4,
        scales_f16,
        x,
        logits
    );


    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {

        const std::uint32_t token =
            repair_ids[r];


        logits[token] =
            int8_repair_score(
                repair_weights
                    + r * D,
                repair_scales[r],
                x
            );
    }


    if (
        !finiteVector(
            logits,
            VOCAB
        )
    ) {

        return false;
    }


    std::uint32_t top_tokens[
        HELLO_TOP_K
    ] = {};


    float top_scores[
        HELLO_TOP_K
    ];


    for (
        std::size_t i = 0;
        i < HELLO_TOP_K;
        ++i
    ) {

        top_scores[i] =
            -INFINITY;
    }


    for (
        std::uint32_t token = 0;
        token < VOCAB;
        ++token
    ) {

        insertTop8(
            token,
            logits[token],
            top_tokens,
            top_scores
        );
    }


    Serial.println(
        "=== HELLO DEVICE FIRST TOP8 ==="
    );


    for (
        std::size_t i = 0;
        i < HELLO_TOP_K;
        ++i
    ) {

        Serial.printf(
            "HELLO_TOP|%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                i
            ),

            static_cast<unsigned>(
                top_tokens[i]
            ),

            static_cast<double>(
                top_scores[i]
            )
        );
    }


    const std::uint32_t
        expected_top8[
            HELLO_TOP_K
        ] = {
            355,
            415,
            330,
            1746,
            382,
            324,
            359,
            450
        };


    bool top8_same =
        true;


    for (
        std::size_t i = 0;
        i < HELLO_TOP_K;
        ++i
    ) {

        if (
            top_tokens[i]
            != expected_top8[i]
        ) {

            top8_same =
                false;
        }
    }


    const float margin =
        top_scores[0]
        - top_scores[1];


    Serial.printf(
        "HELLO_TOP1|"
        "token=%u|"
        "score=%.9f|"
        "margin=%.9f\n",

        static_cast<unsigned>(
            top_tokens[0]
        ),

        static_cast<double>(
            top_scores[0]
        ),

        static_cast<double>(
            margin
        )
    );


    Serial.printf(
        "HELLO_HOST_TOP8_IDS_MATCH: %s\n",
        top8_same
            ? "PASS"
            : "INFO_DIFFERENT"
    );


    if (predicted_token) {

        *predicted_token =
            top_tokens[0];
    }


    return
        top_tokens[0]
        == 355u;
}


 
 
 

bool helloDecodeFirst(
    std::uint32_t token
) {

    char text[64] = {};

    std::size_t bytes =
        0;


    const bool ok =
        pocketlm::tokenizer_runtime_decode(
            &hello_tokenizer,
            &token,
            1,
            text,
            sizeof(text) - 1,
            &bytes
        );


    if (
        ok
        &&
        bytes
            < sizeof(text)
    ) {

        text[bytes] =
            '\0';
    }


    Serial.printf(
        "HELLO_FIRST_DECODE|"
        "token=%u|"
        "ok=%s|"
        "bytes=%u|"
        "text='%s'\n",

        static_cast<unsigned>(
            token
        ),

        ok ? "PASS" : "FAIL",

        static_cast<unsigned>(
            bytes
        ),

        ok ? text : ""
    );


    return
        ok
        &&
        std::strcmp(
            text,
            "H"
        )
        == 0;
}

}  


void hello_prefill_setup_unused() {

     
    resolver_probe_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO HELLO PREFILL "
        "FIRST-TOKEN PROBE ==="
    );


    if (
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        Serial.println(
            "HELLO_RUNTIME_STATE: FAIL"
        );

        return;
    }


    Serial.println(
        "HELLO_RUNTIME_STATE: PASS"
    );


     
     
    if (!helloAllocateKv()) {

        return;
    }


    if (!helloInitTokenizer()) {

        return;
    }


     
     
     

    std::uint32_t encoded[
        16
    ] = {};

    std::size_t encoded_count =
        0;


    const char* prompt =
        "hello";


    const bool enc_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            prompt,
            std::strlen(prompt),
            encoded,
            16,
            &encoded_count
        );


    Serial.printf(
        "TOKENIZER_HELLO_ENCODE|"
        "ok=%s|"
        "count=%u|",

        enc_ok ? "PASS" : "FAIL",

        static_cast<unsigned>(
            encoded_count
        )
    );


    for (
        std::size_t i = 0;
        i < encoded_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0 ? "ids=" : ",",
            static_cast<unsigned>(
                encoded[i]
            )
        );
    }


    Serial.println();


    const bool encode_exact =
        enc_ok
        &&
        encoded_count == 2
        &&
        encoded[0] == 576u
        &&
        encoded[1] == 625u;


    Serial.printf(
        "TOKENIZER_HELLO_IDS_576_625: %s\n",
        encode_exact
            ? "PASS"
            : "FAIL"
    );


    if (!encode_exact) {

        return;
    }


    const std::uint32_t prompt_ids[
        HELLO_PROMPT_COUNT
    ] = {
        4u,
        encoded[0],
        encoded[1],
        5u
    };


    Serial.printf(
        "SERIALIZED_PROMPT|"
        "%u,%u,%u,%u\n",

        static_cast<unsigned>(
            prompt_ids[0]
        ),

        static_cast<unsigned>(
            prompt_ids[1]
        ),

        static_cast<unsigned>(
            prompt_ids[2]
        ),

        static_cast<unsigned>(
            prompt_ids[3]
        )
    );


    const bool serialization_ok =
        prompt_ids[0] == 4u
        &&
        prompt_ids[1] == 576u
        &&
        prompt_ids[2] == 625u
        &&
        prompt_ids[3] == 5u;


    Serial.printf(
        "HELLO_SERIALIZATION_4_576_625_5: %s\n",
        serialization_ok
            ? "PASS"
            : "FAIL"
    );


    if (!serialization_ok) {

        return;
    }


     
     
     
     
     
     

    const uint32_t prefill_t0 =
        millis();


    bool forward_ok =
        true;


    for (
        std::size_t pos = 0;
        pos < HELLO_PROMPT_COUNT;
        ++pos
    ) {

        const bool ok =
            helloRunToken(
                prompt_ids[pos],
                pos,
                pos
                    == HELLO_PROMPT_COUNT
                        - 1
            );


        if (!ok) {

            forward_ok =
                false;

            break;
        }
    }


    Serial.printf(
        "HELLO_PREFILL_FORWARD: %s|"
        "tokens=%u|"
        "ms=%lu\n",

        forward_ok
            ? "PASS"
            : "FAIL",

        static_cast<unsigned>(
            HELLO_PROMPT_COUNT
        ),

        static_cast<unsigned long>(
            millis() - prefill_t0
        )
    );


    if (!forward_ok) {

        return;
    }


     
     
     

    std::uint32_t next =
        0;


    const bool top1_ok =
        helloPredictFirst(
            &next
        );


    Serial.printf(
        "HELLO_FIRST_NEXT_355: %s\n",
        (
            top1_ok
            &&
            next == 355u
        )
            ? "PASS"
            : "FAIL"
    );


    const bool decode_ok =
        helloDecodeFirst(
            next
        );


    Serial.printf(
        "HELLO_FIRST_DECODE_H: %s\n",
        decode_ok
            ? "PASS"
            : "FAIL"
    );


    const bool final_ok =
        encode_exact
        &&
        serialization_ok
        &&
        forward_ok
        &&
        top1_ok
        &&
        next == 355u
        &&
        decode_ok;


    Serial.println();


    Serial.printf(
        "HELLO_PREFILL_FIRST_NEXT_GATE: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void hello_prefill_loop_unused() {

    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

namespace {

bool autoregPredictTop1(
    std::size_t step,
    std::uint32_t expected_token,
    std::uint32_t* predicted_token
) {

    if (
        !loadNormRow(
            FINAL_NORM_RELATIVE,
            final_norm_weight_rt
        )
    ) {

        Serial.printf(
            "AUTOREG_FINAL_NORM: FAIL|"
            "step=%u\n",
            static_cast<unsigned>(
                step
            )
        );

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        final_norm_weight_rt,
        x,
        cfg::kDModel
    );


    const std::uint8_t* lm_base =
        g_model_base
        + MODEL_LM_HEAD_OFFSET;


    const std::uint8_t* q4 =
        lm_base
        + OFF_Q4;


    const auto* scales_f16 =
        reinterpret_cast<
            const std::uint16_t*
        >(
            lm_base
            + OFF_SCALES_F16
        );


    const auto* repair_ids =
        reinterpret_cast<
            const std::uint32_t*
        >(
            lm_base
            + OFF_REPAIR_IDS
        );


    const auto* repair_weights =
        reinterpret_cast<
            const std::int8_t*
        >(
            lm_base
            + OFF_REPAIR_WEIGHTS
        );


    const auto* repair_scales =
        reinterpret_cast<
            const float*
        >(
            lm_base
            + OFF_REPAIR_SCALES
        );


    const uint32_t lm_t0 =
        micros();


    q4_head_rowpair2(
        q4,
        scales_f16,
        x,
        logits
    );


    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {

        const std::uint32_t token =
            repair_ids[r];


        logits[token] =
            int8_repair_score(
                repair_weights
                    + r * D,
                repair_scales[r],
                x
            );
    }


    const uint32_t lm_us =
        micros() - lm_t0;


    if (
        !finiteVector(
            logits,
            VOCAB
        )
    ) {

        Serial.printf(
            "AUTOREG_LOGITS_FINITE: FAIL|"
            "step=%u\n",
            static_cast<unsigned>(
                step
            )
        );

        return false;
    }


    std::uint32_t best_token =
        0;

    std::uint32_t second_token =
        0;


    float best_score =
        -INFINITY;

    float second_score =
        -INFINITY;


     
    for (
        std::uint32_t token = 0;
        token < VOCAB;
        ++token
    ) {

        const float score =
            logits[token];


        if (
            score
            > best_score
        ) {

            second_score =
                best_score;

            second_token =
                best_token;


            best_score =
                score;

            best_token =
                token;

        } else if (
            score
            > second_score
        ) {

            second_score =
                score;

            second_token =
                token;
        }
    }


    const float margin =
        best_score
        - second_score;


    const bool match =
        best_token
        == expected_token;


    Serial.printf(
        "AUTOREG_PREDICT|"
        "step=%u|"
        "token=%u|"
        "score=%.9f|"
        "second=%u|"
        "second_score=%.9f|"
        "margin=%.9f|"
        "expected=%u|"
        "lm_us=%lu|"
        "%s\n",

        static_cast<unsigned>(
            step
        ),

        static_cast<unsigned>(
            best_token
        ),

        static_cast<double>(
            best_score
        ),

        static_cast<unsigned>(
            second_token
        ),

        static_cast<double>(
            second_score
        ),

        static_cast<double>(
            margin
        ),

        static_cast<unsigned>(
            expected_token
        ),

        static_cast<unsigned long>(
            lm_us
        ),

        match
            ? "PASS"
            : "FAIL"
    );


    if (
        predicted_token
    ) {

        *predicted_token =
            best_token;
    }


    return match;
}


bool autoregDecodeGenerated(
    const std::uint32_t* tokens,
    std::size_t count
) {

    char text[128] = {};

    std::size_t bytes =
        0;


    const bool ok =
        pocketlm::tokenizer_runtime_decode(
            &hello_tokenizer,
            tokens,
            count,
            text,
            sizeof(text) - 1,
            &bytes
        );


    if (
        ok
        &&
        bytes
            < sizeof(text)
    ) {

        text[bytes] =
            '\0';
    }


    Serial.printf(
        "AUTOREG_DECODE|"
        "ok=%s|"
        "tokens=%u|"
        "bytes=%u|"
        "text='%s'\n",

        ok
            ? "PASS"
            : "FAIL",

        static_cast<unsigned>(
            count
        ),

        static_cast<unsigned>(
            bytes
        ),

        ok
            ? text
            : ""
    );


    return
        ok
        &&
        std::strcmp(
            text,
            "Hello."
        )
        == 0;
}

}  


void xiao_autoreg_correctness_setup_unused() {

     
     
     
     
     
     
     
    hello_prefill_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO HELLO FULL "
        "AUTOREGRESSIVE PROBE ==="
    );


    const bool state_ok =
        g_model_base
        &&
        g_transformer_cache
        &&
        g_sd_scratch
        &&
        g_sd_residual
        &&
        hello_kv_block
        &&
        hello_kv_k
        &&
        hello_kv_v;


    Serial.printf(
        "AUTOREG_RUNTIME_STATE: %s\n",
        state_ok
            ? "PASS"
            : "FAIL"
    );


    if (!state_ok) {
        return;
    }


     
    std::uint32_t generated[3] = {
        355u,
        0u,
        0u
    };


     
     
     
     

    const bool feed_h_ok =
        helloRunToken(
            355u,
            4u,
            false
        );


    Serial.printf(
        "AUTOREG_FEED_355_POS4: %s\n",
        feed_h_ok
            ? "PASS"
            : "FAIL"
    );


    if (!feed_h_ok) {
        return;
    }


    std::uint32_t second =
        0;


    const bool second_ok =
        autoregPredictTop1(
            1u,
            3048u,
            &second
        );


    generated[1] =
        second;


    Serial.printf(
        "AUTOREG_NEXT_3048: %s\n",
        (
            second_ok
            &&
            second == 3048u
        )
            ? "PASS"
            : "FAIL"
    );


    if (
        !second_ok
        ||
        second != 3048u
    ) {

        return;
    }


     
     
     
     

    const bool feed_ello_ok =
        helloRunToken(
            3048u,
            5u,
            false
        );


    Serial.printf(
        "AUTOREG_FEED_3048_POS5: %s\n",
        feed_ello_ok
            ? "PASS"
            : "FAIL"
    );


    if (!feed_ello_ok) {
        return;
    }


    std::uint32_t third =
        0;


    const bool third_ok =
        autoregPredictTop1(
            2u,
            6070u,
            &third
        );


    generated[2] =
        third;


    Serial.printf(
        "AUTOREG_NEXT_6070: %s\n",
        (
            third_ok
            &&
            third == 6070u
        )
            ? "PASS"
            : "FAIL"
    );


    if (
        !third_ok
        ||
        third != 6070u
    ) {

        return;
    }


    Serial.printf(
        "AUTOREG_GENERATED_IDS|"
        "%u,%u,%u\n",

        static_cast<unsigned>(
            generated[0]
        ),

        static_cast<unsigned>(
            generated[1]
        ),

        static_cast<unsigned>(
            generated[2]
        )
    );


    const bool ids_ok =
        generated[0] == 355u
        &&
        generated[1] == 3048u
        &&
        generated[2] == 6070u;


    Serial.printf(
        "AUTOREG_IDS_355_3048_6070: %s\n",
        ids_ok
            ? "PASS"
            : "FAIL"
    );


    const bool decode_ok =
        autoregDecodeGenerated(
            generated,
            3
        );


    Serial.printf(
        "AUTOREG_TEXT_HELLO_DOT: %s\n",
        decode_ok
            ? "PASS"
            : "FAIL"
    );


    const bool final_ok =
        state_ok
        &&
        feed_h_ok
        &&
        second_ok
        &&
        second == 3048u
        &&
        feed_ello_ok
        &&
        third_ok
        &&
        third == 6070u
        &&
        ids_ok
        &&
        decode_ok;


    Serial.println();


    Serial.printf(
        "HELLO_FULL_AUTOREGRESSIVE_GATE: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void xiao_autoreg_correctness_loop_unused() {

    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

namespace {

constexpr std::size_t
    PROD_CONTEXT =
        64;

constexpr std::size_t
    PROD_MAX_GENERATE =
        24;

constexpr std::uint32_t
    PROD_USER_ID =
        4u;

constexpr std::uint32_t
    PROD_ASSISTANT_ID =
        5u;

constexpr std::uint32_t
    PROD_END_ID =
        9u;

constexpr bool
    PROD_STOP_AT_FIRST_SENTENCE =
        false;

constexpr std::size_t
    PROD_INPUT_BYTES =
        160;

constexpr std::size_t
    PROD_DECODE_BYTES =
        512;


static_assert(
    HELLO_MAX_CONTEXT
        == PROD_CONTEXT,
    "KV context must remain 64"
);


char prod_input[
    PROD_INPUT_BYTES
] = {};

std::size_t prod_input_len =
    0;

bool prod_runtime_ready =
    false;


 
 
 

void trim_ascii_in_place(char* text) {
  if (!text) return;
  std::size_t n = std::strlen(text);
  while (n > 0 && (text[n - 1] == ' ' || text[n - 1] == '\t' || text[n - 1] == '\r' || text[n - 1] == '\n')) {
    text[n - 1] = '\0';
    --n;
  }
  char* start = text;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') ++start;
  if (start != text) std::memmove(text, start, std::strlen(start) + 1);
}


 
 
 
 
 
 
 
 
 
 
 
 
 

bool final_is_hspace(char c) {
  return c == ' ' || c == '\t';
}

bool final_is_digit(char c) {
  return c >= '0' && c <= '9';
}

bool final_is_math_op(char c) {
  return c == '+' || c == '-' || c == '*' || c == '/';
}

bool final_is_math_span_char(char c) {
  return final_is_digit(c)
      || final_is_hspace(c)
      || c == '.'
      || final_is_math_op(c)
      || c == '('
      || c == ')';
}

bool final_append_char(
    char c,
    char* out,
    std::size_t cap,
    std::size_t* n) {
  if (!out || !n || cap == 0 || *n + 1 >= cap) return false;
  out[(*n)++] = c;
  out[*n] = '\0';
  return true;
}

bool final_replace_math_glyphs(
    const char* input,
    char* out,
    std::size_t cap) {
  if (!input || !out || cap == 0) return false;
  std::size_t n = 0;
  out[0] = '\0';

  const unsigned char* p =
      reinterpret_cast<const unsigned char*>(input);

  while (*p != 0) {
     
    if (p[0] == 0xC3 && p[1] == 0x97) {
      if (!final_append_char('*', out, cap, &n)) return false;
      p += 2;
      continue;
    }
     
    if (p[0] == 0xC3 && p[1] == 0xB7) {
      if (!final_append_char('/', out, cap, &n)) return false;
      p += 2;
      continue;
    }
     
    if (p[0] == 0xC2 && p[1] == 0xB7) {
      if (!final_append_char('*', out, cap, &n)) return false;
      p += 2;
      continue;
    }
     
    if (p[0] == 0xE2 && p[1] == 0x8B && p[2] == 0x85) {
      if (!final_append_char('*', out, cap, &n)) return false;
      p += 3;
      continue;
    }
     
    if (p[0] == 0xE2 && p[1] == 0x88 && p[2] == 0x92) {
      if (!final_append_char('-', out, cap, &n)) return false;
      p += 3;
      continue;
    }

    if (!final_append_char(
            static_cast<char>(*p),
            out,
            cap,
            &n)) {
      return false;
    }
    ++p;
  }

  return true;
}

bool final_canonicalize_input(
    const char* input,
    char* out,
    std::size_t cap) {
  if (!input || !out || cap == 0) return false;

  char glyphs[256] = {};
  char collapsed[256] = {};

  if (!final_replace_math_glyphs(
          input,
          glyphs,
          sizeof(glyphs))) {
    return false;
  }

  std::size_t cn = 0;
  bool pending_space = false;

  for (std::size_t i = 0; glyphs[i] != '\0'; ++i) {
    const char c = glyphs[i];

    if (final_is_hspace(c)) {
      if (cn > 0) pending_space = true;
      continue;
    }

    if (pending_space) {
      if (!final_append_char(
              ' ',
              collapsed,
              sizeof(collapsed),
              &cn)) {
        return false;
      }
      pending_space = false;
    }

    if (!final_append_char(
            c,
            collapsed,
            sizeof(collapsed),
            &cn)) {
      return false;
    }
  }

  std::size_t on = 0;
  out[0] = '\0';
  const std::size_t len = std::strlen(collapsed);

  std::size_t i = 0;
  while (i < len) {
    if (final_is_digit(collapsed[i])) {
      std::size_t j = i;
      std::size_t last_digit_plus_one = i + 1;
      bool has_operator = false;

      while (
          j < len
          && final_is_math_span_char(collapsed[j])) {
        if (final_is_digit(collapsed[j])) {
          last_digit_plus_one = j + 1;
        }
        if (final_is_math_op(collapsed[j])) {
          has_operator = true;
        }
        ++j;
      }

      if (
          has_operator
          && last_digit_plus_one > i + 1) {
        for (
            std::size_t k = i;
            k < last_digit_plus_one;
            ++k) {
          if (final_is_hspace(collapsed[k])) continue;

          if (!final_append_char(
                  collapsed[k],
                  out,
                  cap,
                  &on)) {
            return false;
          }
        }

        i = last_digit_plus_one;
        continue;
      }
    }

    if (!final_append_char(
            collapsed[i],
            out,
            cap,
            &on)) {
      return false;
    }

    ++i;
  }

  return true;
}




 
 
 
 
 
 

bool prodHasSentenceEnd(
    const char* text
) {

    if (!text) {
        return false;
    }


    for (
        const char* p = text;
        *p;
        ++p
    ) {

        if (
            *p == '.'
            ||
            *p == '!'
            ||
            *p == '?'
        ) {

            return true;
        }
    }


    return false;
}


 
 
 

void prodResetTurn() {

    if (
        hello_kv_block
    ) {

        std::memset(
            hello_kv_block,
            0,
            HELLO_KV_BYTES_TOTAL
        );
    }


    hello_rope_cached_position =
        static_cast<std::size_t>(
            -1
        );
}


 
 
 
 
 
 
 


 
 
__attribute__((noinline))
void q4_head_rowpair2_rows(
    const std::uint8_t* packed_q,
    const std::uint16_t* scales_fp16,
    const float* input,
    float* output,
    std::size_t row_begin,
    std::size_t row_end
) {

    constexpr std::size_t PACKED_PER_GROUP = 32;

    alignas(16) float lut0[16];
    alignas(16) float lut1[16];


#define BUILD_LUT(LUT, SCALE) \
    do { \
        (LUT)[0] = 0.0f; \
        (LUT)[1] = (SCALE); \
        (LUT)[2] = (SCALE) + (SCALE); \
        (LUT)[3] = (LUT)[2] + (SCALE); \
        (LUT)[4] = (LUT)[2] + (LUT)[2]; \
        (LUT)[5] = (LUT)[4] + (SCALE); \
        (LUT)[6] = (LUT)[4] + (LUT)[2]; \
        (LUT)[7] = (LUT)[6] + (SCALE); \
        const float eight_scale__ = \
            (LUT)[4] + (LUT)[4]; \
        (LUT)[8]  = -eight_scale__; \
        (LUT)[9]  = -(LUT)[7]; \
        (LUT)[10] = -(LUT)[6]; \
        (LUT)[11] = -(LUT)[5]; \
        (LUT)[12] = -(LUT)[4]; \
        (LUT)[13] = -(LUT)[3]; \
        (LUT)[14] = -(LUT)[2]; \
        (LUT)[15] = -(SCALE); \
    } while (0)


    for (
        std::size_t row = row_begin;
        row < row_end;
        row += 2
    ) {

        const std::size_t row0 = row;
        const std::size_t row1 = row + 1;

        const std::uint8_t* row_q0 =
            packed_q
            + row0 * BYTES_PER_ROW;

        const std::uint8_t* row_q1 =
            packed_q
            + row1 * BYTES_PER_ROW;

        const std::uint16_t* row_scales0 =
            scales_fp16
            + row0 * GROUPS_PER_ROW;

        const std::uint16_t* row_scales1 =
            scales_fp16
            + row1 * GROUPS_PER_ROW;

        float acc0 = 0.0f;
        float acc1 = 0.0f;


        for (
            std::size_t group = 0;
            group < GROUPS_PER_ROW;
            ++group
        ) {

            const float scale0 =
                fp16_to_float(
                    row_scales0[group]
                );

            const float scale1 =
                fp16_to_float(
                    row_scales1[group]
                );

            BUILD_LUT(
                lut0,
                scale0
            );

            BUILD_LUT(
                lut1,
                scale1
            );


            const auto* q0 =
                static_cast<
                    const std::uint32_t*
                >(
                    __builtin_assume_aligned(
                        row_q0
                        + group
                          * PACKED_PER_GROUP,
                        4
                    )
                );

            const auto* q1 =
                static_cast<
                    const std::uint32_t*
                >(
                    __builtin_assume_aligned(
                        row_q1
                        + group
                          * PACKED_PER_GROUP,
                        4
                    )
                );

            const std::size_t col_base =
                group * GROUP;


#define ACC_PAIR_ONE(W0, W1, SHIFT, COL) \
            do { \
                const float xv__ = \
                    input[(COL)]; \
                const std::uint32_t i0__ = \
                    ((W0) >> (SHIFT)) \
                    & 0x0Fu; \
                const std::uint32_t i1__ = \
                    ((W1) >> (SHIFT)) \
                    & 0x0Fu; \
                acc0 += \
                    q4_lut_load_addx4( \
                        lut0, i0__ \
                    ) * xv__; \
                acc1 += \
                    q4_lut_load_addx4( \
                        lut1, i1__ \
                    ) * xv__; \
            } while (0)


#define ACC_PAIR_WORD(W0, W1, BASECOL) \
            ACC_PAIR_ONE(W0,W1, 0,(BASECOL)+0); \
            ACC_PAIR_ONE(W0,W1, 4,(BASECOL)+1); \
            ACC_PAIR_ONE(W0,W1, 8,(BASECOL)+2); \
            ACC_PAIR_ONE(W0,W1,12,(BASECOL)+3); \
            ACC_PAIR_ONE(W0,W1,16,(BASECOL)+4); \
            ACC_PAIR_ONE(W0,W1,20,(BASECOL)+5); \
            ACC_PAIR_ONE(W0,W1,24,(BASECOL)+6); \
            ACC_PAIR_ONE(W0,W1,28,(BASECOL)+7)


            {
                const std::uint32_t w0 = q0[0];
                const std::uint32_t w1 = q1[0];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 0
                );
            }

            {
                const std::uint32_t w0 = q0[1];
                const std::uint32_t w1 = q1[1];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 8
                );
            }

            {
                const std::uint32_t w0 = q0[2];
                const std::uint32_t w1 = q1[2];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 16
                );
            }

            {
                const std::uint32_t w0 = q0[3];
                const std::uint32_t w1 = q1[3];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 24
                );
            }

            {
                const std::uint32_t w0 = q0[4];
                const std::uint32_t w1 = q1[4];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 32
                );
            }

            {
                const std::uint32_t w0 = q0[5];
                const std::uint32_t w1 = q1[5];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 40
                );
            }

            {
                const std::uint32_t w0 = q0[6];
                const std::uint32_t w1 = q1[6];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 48
                );
            }

            {
                const std::uint32_t w0 = q0[7];
                const std::uint32_t w1 = q1[7];
                ACC_PAIR_WORD(
                    w0, w1,
                    col_base + 56
                );
            }


#undef ACC_PAIR_WORD
#undef ACC_PAIR_ONE
        }

        output[row0] = acc0;
        output[row1] = acc1;
    }


#undef BUILD_LUT
}

bool parallelQ4Head(
    const std::uint8_t* q4,
    const std::uint16_t* scales_f16
);

bool prodPredictNext(
    std::uint32_t* next_token,
    float* next_score,
    std::uint32_t* elapsed_us
) {

    if (
        !loadNormRow(
            FINAL_NORM_RELATIVE,
            final_norm_weight_rt
        )
    ) {

        Serial.println(
            "[CHAT] final norm load FAIL"
        );

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        final_norm_weight_rt,
        x,
        cfg::kDModel
    );


    const std::uint8_t* lm_base =
        g_model_base
        + MODEL_LM_HEAD_OFFSET;


    const std::uint8_t* q4 =
        lm_base
        + OFF_Q4;


    const auto* scales_f16 =
        reinterpret_cast<
            const std::uint16_t*
        >(
            lm_base
            + OFF_SCALES_F16
        );


    const auto* repair_ids =
        reinterpret_cast<
            const std::uint32_t*
        >(
            lm_base
            + OFF_REPAIR_IDS
        );


    const auto* repair_weights =
        reinterpret_cast<
            const std::int8_t*
        >(
            lm_base
            + OFF_REPAIR_WEIGHTS
        );


    const auto* repair_scales =
        reinterpret_cast<
            const float*
        >(
            lm_base
            + OFF_REPAIR_SCALES
        );


    const std::uint32_t t0 =
        micros();


    if (!parallelQ4Head(q4, scales_f16)) {
        return false;
    }


    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {

        const std::uint32_t token =
            repair_ids[r];


        logits[token] =
            int8_repair_score(
                repair_weights
                    + r * D,
                repair_scales[r],
                x
            );
    }


    const std::uint32_t us =
        micros() - t0;


    if (
        !finiteVector(
            logits,
            VOCAB
        )
    ) {

        Serial.println(
            "[CHAT] logits finite FAIL"
        );

        return false;
    }


    std::uint32_t best_token =
        0;

    float best_score =
        -INFINITY;


     
    for (
        std::uint32_t token = 0;
        token < VOCAB;
        ++token
    ) {

        const float score =
            logits[token];


        if (
            score
            > best_score
        ) {

            best_score =
                score;

            best_token =
                token;
        }
    }


    if (next_token) {
        *next_token =
            best_token;
    }

    if (next_score) {
        *next_score =
            best_score;
    }

    if (elapsed_us) {
        *elapsed_us =
            us;
    }


    return true;
}


 
 
 

bool prodDecode(
    const std::uint32_t* ids,
    std::size_t count,
    char* out,
    std::size_t cap
) {

    if (
        !ids
        ||
        !out
        ||
        cap == 0
    ) {

        return false;
    }


    out[0] =
        '\0';


    std::size_t bytes =
        0;


    const bool ok =
        pocketlm::tokenizer_runtime_decode(
            &hello_tokenizer,
            ids,
            count,
            out,
            cap - 1,
            &bytes
        );


    if (
        bytes
        >= cap
    ) {

        bytes =
            cap - 1;
    }


    out[bytes] =
        '\0';


    return ok;
}


 
 
 

bool prodGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    for (
        std::size_t pos = 0;
        pos < prompt_count;
        ++pos
    ) {

        const std::uint32_t token_t0 =
            micros();


        if (
            !helloRunToken(
                prompt_ids[pos],
                pos,
                false
            )
        ) {

            Serial.printf(
                "[CHAT] prompt token FAIL|"
                "pos=%u|"
                "id=%u\n",

                static_cast<unsigned>(
                    pos
                ),

                static_cast<unsigned>(
                    prompt_ids[pos]
                )
            );

            return false;
        }


        total_layers_us +=
            micros() - token_t0;


        yield();
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !helloRunToken(
                    next,
                    next_context_pos,
                    false
                )
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}


 
 
 

void prodPrintPrompt() {

    Serial.println();

    Serial.print(
        "You> "
    );
}


void prodProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        prodGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}

}  


 
 
 

void locked_production_setup_unused() {

     
     
     
     
     
     
     
     
     
     
    resolver_probe_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO FINAL SERIAL CHAT ==="
    );


    bool ok =
        g_model_base
        &&
        g_transformer_cache
        &&
        g_sd_scratch
        &&
        g_sd_residual;


    Serial.printf(
        "[INIT] storage=%s\n",
        ok
            ? "PASS"
            : "FAIL"
    );


     
     
     
    if (ok) {

        ok =
            helloAllocateKv();
    }


    if (ok) {

        ok =
            helloInitTokenizer();
    }


    prod_runtime_ready =
        ok;


    if (ok) {

        Serial.printf(
            "[READY] context=%u "
            "max_generate=%u "
            "stop=END "
            "free_psram=%lu\n",

            static_cast<unsigned>(
                PROD_CONTEXT
            ),

            static_cast<unsigned>(
                PROD_MAX_GENERATE
            ),

            static_cast<unsigned long>(
                ESP.getFreePsram()
            )
        );


        Serial.println(
            "Try: hello | how are you? | what can you do?"
        );


        prodPrintPrompt();

    } else {

        Serial.println(
            "[FAIL] XIAO final serial chat init failed"
        );
    }
}


 
 
 
 
 
 
 
 
 
 

void locked_production_loop_unused() {

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                prodProcessLine(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

namespace {

uint8_t* pipe_scratch2 = nullptr;

SemaphoreHandle_t pipe_start_sem = nullptr;
SemaphoreHandle_t pipe_done_sem = nullptr;

TaskHandle_t pipe_worker_task = nullptr;


struct PipeJob {

    std::uint32_t layer;
    xp::MatrixFamily family;

    uint8_t* dst;
    std::size_t capacity;

    ResolvedMatrix result;

    bool ok;
    bool active;

    std::uint32_t read_us;
};


PipeJob pipe_job{};


struct PrefetchedMatrix {

    bool valid;

    std::uint32_t layer;
    xp::MatrixFamily family;

    uint8_t* buffer;

    ResolvedMatrix resolved;

    std::uint32_t read_us;
};


PrefetchedMatrix pipe_next_q{};


alignas(16)
float pipe_baseline_hidden[
    cfg::kDModel
];


 
 
 

void pipeWorkerMain(void*) {

    for (;;) {

        xSemaphoreTake(
            pipe_start_sem,
            portMAX_DELAY
        );


        const std::uint32_t t0 =
            micros();


        pipe_job.ok =
            resolveMatrix(
                pipe_job.layer,
                pipe_job.family,
                pipe_job.dst,
                pipe_job.capacity,
                &pipe_job.result
            );


        pipe_job.read_us =
            micros() - t0;

        pipe_job.active =
            false;


        xSemaphoreGive(
            pipe_done_sem
        );
    }
}


bool pipeStartRead(
    std::uint32_t layer,
    xp::MatrixFamily family,
    uint8_t* dst,
    std::size_t capacity
) {

    if (
        pipe_job.active
        ||
        !dst
    ) {

        return false;
    }


    pipe_job.layer =
        layer;

    pipe_job.family =
        family;

    pipe_job.dst =
        dst;

    pipe_job.capacity =
        capacity;

    pipe_job.ok =
        false;

    pipe_job.read_us =
        0;

    pipe_job.active =
        true;


    xSemaphoreGive(
        pipe_start_sem
    );


    return true;
}


bool pipeFinishRead(
    ResolvedMatrix* out,
    std::uint32_t* read_us
) {

    if (!out) {
        return false;
    }


    if (pipe_job.active) {

        xSemaphoreTake(
            pipe_done_sem,
            portMAX_DELAY
        );

    } else {

         
         
        if (
            xSemaphoreTake(
                pipe_done_sem,
                0
            ) != pdTRUE
        ) {

            return false;
        }
    }


    if (!pipe_job.ok) {
        return false;
    }


    *out =
        pipe_job.result;


    if (read_us) {

        *read_us =
            pipe_job.read_us;
    }


    return true;
}


 
 
 

bool pipeMatvec(
    const ResolvedMatrix& r,
    const float* input,
    float* output
) {

    if (
        !r.spec
        ||
        r.spec->rows % 2u != 0u
        ||
        r.spec->cols % 64u != 0u
    ) {

        return false;
    }


    runtime_q4_rowpair2_fp16(
        r.q4,
        r.scales,
        r.spec->rows,
        r.spec->cols,
        input,
        output
    );


    return true;
}


 
 
 

bool pipeStartNextQ(
    std::uint32_t layer,
    uint8_t* buffer
) {

    if (
        layer >= cfg::kNumLayers
    ) {

        pipe_next_q.valid =
            false;

        return true;
    }


    const auto& p =
        xp::matrixPlacement(
            layer,
            xp::MatrixFamily::QProj
        );


    if (
        p.location
        != xp::MatrixLocation::Sd
    ) {

        pipe_next_q.valid =
            false;

        return true;
    }


    pipe_next_q.valid =
        false;


    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::QProj,
            buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    pipe_next_q.layer =
        layer;

    pipe_next_q.family =
        xp::MatrixFamily::QProj;

    pipe_next_q.buffer =
        buffer;


    return true;
}


bool pipeFinishNextQ(
    std::uint32_t expected_layer
) {

    if (
        pipe_next_q.layer
        != expected_layer
    ) {

        return false;
    }


    std::uint32_t us =
        0;


    if (
        !pipeFinishRead(
            &pipe_next_q.resolved,
            &us
        )
    ) {

        return false;
    }


    pipe_next_q.read_us =
        us;

    pipe_next_q.valid =
        true;


    return true;
}


 
 
 
 
 
 
 
 
 
 
 

bool pipeRunLayer5(
    std::size_t position
) {

    constexpr std::uint32_t layer =
        5u;


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeFinishNextQ(5u)
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;

    uint8_t* q_buffer =
        pipe_next_q.buffer;

    uint8_t* other_buffer =
        (
            q_buffer == g_sd_scratch
        )
        ? pipe_scratch2
        : g_sd_scratch;


     
    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::VProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


     
    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::KProj,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix v{};


    if (
        !pipeFinishRead(
            &v,
            nullptr
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            v,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


     
    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::OProj,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeStartNextQ(
            6u,
            q_buffer
        )
    ) {

        return false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 
 
 
 

bool pipeRunSdLayer(
    std::uint32_t layer,
    std::size_t position
) {

    if (
        layer < 6u
        ||
        layer >= cfg::kNumLayers
    ) {

        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeFinishNextQ(
            layer
        )
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;


    uint8_t* q_buffer =
        pipe_next_q.buffer;

    uint8_t* other_buffer =
        (
            q_buffer == g_sd_scratch
        )
        ? pipe_scratch2
        : g_sd_scratch;


     
     
     

    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::KProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix k{};


    if (
        !pipeFinishRead(
            &k,
            nullptr
        )
    ) {

        return false;
    }


     
     
     

    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::VProj,
            q_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            k,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix v{};


    if (
        !pipeFinishRead(
            &v,
            nullptr
        )
    ) {

        return false;
    }


     
     
     

    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::OProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            v,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


     
     
    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


    ResolvedMatrix o{};


    if (
        !pipeFinishRead(
            &o,
            nullptr
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            o,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     
     
     

    if (
        layer + 1u
        < cfg::kNumLayers
    ) {

        if (
            !pipeStartNextQ(
                layer + 1u,
                q_buffer
            )
        ) {

            return false;
        }

    } else {

        pipe_next_q.valid =
            false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 

bool pipeRunToken(
    std::uint32_t token,
    std::size_t position,
    std::uint32_t* elapsed_ms
) {

    if (
        position >= HELLO_MAX_CONTEXT
    ) {

        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {

        return false;
    }


    pipe_next_q =
        PrefetchedMatrix{};


    const std::uint32_t t0 =
        millis();


     
    if (
        !pipeStartNextQ(
            5u,
            g_sd_scratch
        )
    ) {

        return false;
    }


     
    for (
        std::uint32_t layer = 0;
        layer < 5u;
        ++layer
    ) {

        if (
            !helloRunLayerPosition(
                layer,
                position,
                false
            )
        ) {

            return false;
        }
    }


    if (
        !pipeRunLayer5(
            position
        )
    ) {

        return false;
    }


    for (
        std::uint32_t layer = 6;
        layer < cfg::kNumLayers;
        ++layer
    ) {

        if (
            !pipeRunSdLayer(
                layer,
                position
            )
        ) {

            return false;
        }
    }


    const std::uint32_t ms =
        millis() - t0;


    if (elapsed_ms) {
        *elapsed_ms = ms;
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

}  


void sd_pipeline_token0_probe_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO SD PIPELINE TOKEN0 PROBE V1 ==="
    );


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "PIPE_RUNTIME_STATE: FAIL"
        );

        return;
    }


    Serial.println(
        "PIPE_RUNTIME_STATE: PASS"
    );


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        Serial.printf(
            "PIPE_SCRATCH2: FAIL|"
            "free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    Serial.printf(
        "PIPE_SCRATCH2: PASS|"
        "bytes=%lu|"
        "free=%lu|"
        "largest=%lu\n",

        static_cast<unsigned long>(
            SD_SCRATCH_BYTES
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        Serial.println(
            "PIPE_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        rc != pdPASS
    ) {

        Serial.println(
            "PIPE_WORKER: FAIL"
        );

        return;
    }


    Serial.println(
        "PIPE_WORKER: PASS|core=0"
    );


    delay(100);


     
     
     

    prodResetTurn();


    const std::uint32_t baseline_t0 =
        millis();


    const bool baseline_ok =
        helloRunToken(
            0u,
            0u,
            false
        );


    const std::uint32_t baseline_ms =
        millis() - baseline_t0;


    if (!baseline_ok) {

        Serial.println(
            "BASELINE_TOKEN0: FAIL"
        );

        return;
    }


    std::memcpy(
        pipe_baseline_hidden,
        hidden_rt,
        sizeof(pipe_baseline_hidden)
    );


    Serial.printf(
        "BASELINE_TOKEN0: PASS|ms=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            baseline_ms
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


     
     
     

    prodResetTurn();


    std::uint32_t pipe_ms =
        0;


    const bool pipe_ok =
        pipeRunToken(
            0u,
            0u,
            &pipe_ms
        );


    if (!pipe_ok) {

        Serial.println(
            "PIPE_TOKEN0: FAIL"
        );

        return;
    }


    double max_diff =
        0.0;

    bool bit_exact =
        true;


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        if (
            std::memcmp(
                &pipe_baseline_hidden[i],
                &hidden_rt[i],
                sizeof(float)
            ) != 0
        ) {

            bit_exact =
                false;
        }


        const double d =
            fabs(
                static_cast<double>(
                    pipe_baseline_hidden[i]
                )
                -
                static_cast<double>(
                    hidden_rt[i]
                )
            );


        if (d > max_diff) {
            max_diff = d;
        }
    }


    Serial.printf(
        "PIPE_TOKEN0: PASS|ms=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            pipe_ms
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


    Serial.printf(
        "PIPE_HIDDEN_COMPARE|"
        "bit_exact=%s|"
        "max_abs_diff=%.12f\n",

        bit_exact
            ? "YES"
            : "NO",

        max_diff
    );


     
     
     

    std::uint32_t top1 =
        0;

    float top1_score =
        0.0f;

    std::uint32_t lm_us =
        0;


    const bool head_ok =
        prodPredictNext(
            &top1,
            &top1_score,
            &lm_us
        );


    if (!head_ok) {

        Serial.println(
            "PIPE_LM_HEAD: FAIL"
        );

        return;
    }


    constexpr std::size_t TOPN =
        8;


    std::uint32_t top_tokens[
        TOPN
    ] = {};

    float top_scores[
        TOPN
    ];


    for (
        std::size_t i = 0;
        i < TOPN;
        ++i
    ) {

        top_scores[i] =
            -INFINITY;
    }


    for (
        std::uint32_t token = 0;
        token < VOCAB;
        ++token
    ) {

        insertTop8(
            token,
            logits[token],
            top_tokens,
            top_scores
        );
    }


    constexpr std::uint32_t EXPECTED[
        TOPN
    ] = {
        3168,
        6070,
        3745,
        6098,
        454,
        703,
        759,
        4427
    };


    bool top8_ok =
        true;


    for (
        std::size_t i = 0;
        i < TOPN;
        ++i
    ) {

        Serial.printf(
            "PIPE_TOP|%u|token=%u|score=%.9f\n",

            static_cast<unsigned>(i),

            static_cast<unsigned>(
                top_tokens[i]
            ),

            static_cast<double>(
                top_scores[i]
            )
        );


        if (
            top_tokens[i]
            != EXPECTED[i]
        ) {

            top8_ok =
                false;
        }
    }


    const double speedup =
        pipe_ms > 0
        ? static_cast<double>(
              baseline_ms
          )
          /
          static_cast<double>(
              pipe_ms
          )
        : 0.0;


    Serial.println();

    Serial.printf(
        "TOKEN0_PIPE_SPEEDUP|%.4fx\n",
        speedup
    );


    Serial.printf(
        "TOKEN0_PIPE_TOP1_3168: %s\n",
        top1 == 3168u
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "TOKEN0_PIPE_TOP8_IDS: %s\n",
        top8_ok
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "TOKEN0_PIPE_NUMERIC_EQUIV: %s\n",
        (
            bit_exact
            ||
            max_diff <= 1e-6
        )
            ? "PASS"
            : "FAIL"
    );


    const bool final_ok =
        baseline_ok
        &&
        pipe_ok
        &&
        top1 == 3168u
        &&
        top8_ok
        &&
        (
            bit_exact
            ||
            max_diff <= 1e-6
        )
        &&
        pipe_ms < baseline_ms;


    Serial.println();


    Serial.printf(
        "XIAO_SD_PIPELINE_TOKEN0_V1: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void sd_pipeline_token0_probe_loop_unused() {

    delay(1000);
}


 
 
 
 
 
 
 
 

bool pipeProdGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    for (
        std::size_t pos = 0;
        pos < prompt_count;
        ++pos
    ) {

        const std::uint32_t token_t0 =
            micros();


        if (
            !pipeRunToken(
                prompt_ids[pos],
                pos,
                nullptr)
        ) {

            Serial.printf(
                "[CHAT] prompt token FAIL|"
                "pos=%u|"
                "id=%u\n",

                static_cast<unsigned>(
                    pos
                ),

                static_cast<unsigned>(
                    prompt_ids[pos]
                )
            );

            return false;
        }


        total_layers_us +=
            micros() - token_t0;


        yield();
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !pipeRunToken(
                    next,
                    next_context_pos,
                    nullptr)
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}

void pipeProdProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        pipeProdGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}


void sdpipe_v1_setup_unused() {

     
     
     
     
     
     
     
     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO SD-PIPE CHAT V1 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "SDPIPE_CHAT_RUNTIME: FAIL"
        );

        return;
    }


     
     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        prod_runtime_ready =
            false;

        Serial.printf(
            "SDPIPE_SCRATCH2: FAIL|"
            "free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    Serial.printf(
        "SDPIPE_SCRATCH2: PASS|"
        "bytes=%lu|"
        "free=%lu|"
        "largest=%lu\n",

        static_cast<unsigned long>(
            SD_SCRATCH_BYTES
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "SDPIPE_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "SDPIPE_WORKER: FAIL"
        );

        return;
    }


    delay(100);


    Serial.println(
        "SDPIPE_WORKER: PASS|core=0"
    );


    Serial.printf(
        "[SDPIPE_READY] "
        "context=%u "
        "max_generate=%u "
        "stop=END "
        "free_psram=%lu\n",

        static_cast<unsigned>(
            PROD_CONTEXT
        ),

        static_cast<unsigned>(
            PROD_MAX_GENERATE
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        )
    );


    Serial.println(
        "First validation prompt: hello"
    );


    prodPrintPrompt();
}



void sdpipe_v1_loop_unused() {

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                pipeProdProcessLine(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}


 
 
 
 
 
 
 
 
 
 
 

namespace {

#include "feed_forward_matrix_ops.inc"

enum class DualJobKind : uint8_t {
    Rows,
    Full,
    Attention,
    SwiGlu,
    LmHeadRows, FfnPairRows, FfnSwiGlu
};


struct DualJob {

    DualJobKind kind;
    const float* input2;
    float* output2;

    ResolvedMatrix matrix;

    const float* input;
    float* output;

    std::size_t begin;
    std::size_t end;

    std::size_t layer;
    std::size_t position;

    bool ok;
    std::uint32_t elapsed_us;
};


DualJob dual_job{};

SemaphoreHandle_t dual_start =
    nullptr;

SemaphoreHandle_t dual_done =
    nullptr;

TaskHandle_t dual_worker_task =
    nullptr;


alignas(16)
float dual_attn_scores[
    HELLO_MAX_CONTEXT
];


alignas(16)
float dual_baseline_hidden[
    cfg::kDModel
];


 
 
 
 
 
 

bool dualMatvecRows(
    const ResolvedMatrix& r,
    const float* input,
    float* output,
    std::size_t row_begin,
    std::size_t row_end
) {

    if (
        !r.spec
        ||
        !input
        ||
        !output
        ||
        row_begin >= row_end
        ||
        row_end > r.spec->rows
    ) {

        return false;
    }


    const std::size_t rows =
        r.spec->rows;

    const std::size_t count =
        row_end - row_begin;


    if (
        count % 2u != 0u
        ||
        r.spec->cols % 64u != 0u
        ||
        r.q_bytes % rows != 0u
        ||
        r.scales_bytes % rows != 0u
    ) {

        return false;
    }


    const std::size_t q_bytes_per_row =
        r.q_bytes / rows;

    const std::size_t scale_bytes_per_row =
        r.scales_bytes / rows;


    const std::uint8_t* q =
        r.q4
        + row_begin
            * q_bytes_per_row;


    const std::uint8_t* scale_bytes =
        reinterpret_cast<
            const std::uint8_t*
        >(r.scales)
        + row_begin
            * scale_bytes_per_row;


    const std::uint16_t* scales =
        reinterpret_cast<
            const std::uint16_t*
        >(scale_bytes);


    runtime_q4_rowpair2_fp16(
        q,
        scales,
        count,
        r.spec->cols,
        input,
        output + row_begin
    );


    return true;
}


 
 
 

void dualAttentionHeads(
    std::size_t layer,
    std::size_t position,
    std::size_t head_begin,
    std::size_t head_end,
    float* scores
) {

    const float score_scale =
        1.0f
        /
        sqrtf(
            static_cast<float>(
                cfg::kHeadDim
            )
        );


    for (
        std::size_t head =
            head_begin;

        head < head_end;

        ++head
    ) {

        const std::size_t kv_head =
            head
            / cfg::kKvRepeat;


        const float* q =
            q_rt
            + head
                * cfg::kHeadDim;


        float max_score =
            -INFINITY;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float* k =
                hello_kv_k
                + helloKvOffset(
                    layer,
                    t,
                    kv_head
                );


            float score =
                0.0f;


            for (
                std::size_t d = 0;
                d < cfg::kHeadDim;
                ++d
            ) {

                score +=
                    q[d]
                    * k[d];
            }


            score *=
                score_scale;


            scores[t] =
                score;


            if (
                score
                > max_score
            ) {

                max_score =
                    score;
            }
        }


        float denom =
            0.0f;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float e =
                expf(
                    scores[t]
                    - max_score
                );


            scores[t] =
                e;

            denom +=
                e;
        }


        float* dst =
            attention_rt
            + head
                * cfg::kHeadDim;


        for (
            std::size_t d = 0;
            d < cfg::kHeadDim;
            ++d
        ) {

            dst[d] =
                0.0f;
        }


        const float inv_denom =
            1.0f
            / denom;


        for (
            std::size_t t = 0;
            t <= position;
            ++t
        ) {

            const float prob =
                scores[t]
                * inv_denom;


            const float* v =
                hello_kv_v
                + helloKvOffset(
                    layer,
                    t,
                    kv_head
                );


            for (
                std::size_t d = 0;
                d < cfg::kHeadDim;
                ++d
            ) {

                dst[d] +=
                    prob
                    * v[d];
            }
        }
    }
}


 
 
 

void dualWorkerMain(void*) {

    for (;;) {

        xSemaphoreTake(
            dual_start,
            portMAX_DELAY
        );


        const std::uint32_t t0 =
            micros();


        bool ok =
            false;


        switch (
            dual_job.kind
        ) {

            case DualJobKind::FfnPairRows:
                ok=rf_rows(dual_job.matrix,dual_job.input,dual_job.input2,dual_job.output,dual_job.output2,dual_job.begin,dual_job.end);break;
            case DualJobKind::FfnSwiGlu:
                rf_sw(dual_job.output,dual_job.input,dual_job.begin,dual_job.end);ok=true;break;
            case DualJobKind::LmHeadRows: {
                 
                 
                const std::uint8_t* lm_base =
                    g_model_base + MODEL_LM_HEAD_OFFSET;
                q4_head_rowpair2_rows(
                    lm_base + OFF_Q4,
                    reinterpret_cast<const std::uint16_t*>(
                        lm_base + OFF_SCALES_F16),
                    x,
                    logits,
                    dual_job.begin,
                    dual_job.end
                );
                ok = true;
                break;
            }


            case DualJobKind::Rows:

                ok =
                    dualMatvecRows(
                        dual_job.matrix,
                        dual_job.input,
                        dual_job.output,
                        dual_job.begin,
                        dual_job.end
                    );

                break;


            case DualJobKind::Full:

                ok =
                    pipeMatvec(
                        dual_job.matrix,
                        dual_job.input,
                        dual_job.output
                    );

                break;


            case DualJobKind::Attention:

                dualAttentionHeads(
                    dual_job.layer,
                    dual_job.position,
                    dual_job.begin,
                    dual_job.end,
                    dual_attn_scores
                );

                ok =
                    true;

                break;


            case DualJobKind::SwiGlu:

                for (
                    std::size_t i =
                        dual_job.begin;

                    i < dual_job.end;

                    ++i
                ) {

                    const float g =
                        gate_rt[i];


                    gate_rt[i] =
                        (
                            g
                            /
                            (
                                1.0f
                                + expf(-g)
                            )
                        )
                        * up_rt[i];
                }


                ok =
                    true;

                break;
        }


        dual_job.ok =
            ok;

        dual_job.elapsed_us =
            micros() - t0;


        __sync_synchronize();


        xSemaphoreGive(
            dual_done
        );
    }
}


void dualDrainDone() {

    (void)xSemaphoreTake(
        dual_done,
        0
    );
}


bool dualWait() {

    if (
        xSemaphoreTake(
            dual_done,
            pdMS_TO_TICKS(5000)
        )
        != pdTRUE
    ) {

        return false;
    }


    __sync_synchronize();


    return dual_job.ok;
}


 
 
bool parallelQ4Head(
    const std::uint8_t* q4,
    const std::uint16_t* scales_f16
) {
    constexpr std::size_t split = VOCAB / 2u;
    static_assert(VOCAB == 6144 && D == 448, "fixed accepted LM shape");
    static_assert((split % 2u) == 0u, "never split a row pair");
    if (!prod_runtime_ready || !g_model_base || !q4 || !scales_f16 ||
        !dual_worker_task || !dual_start || !dual_done || pipe_job.active) {
        return false;
    }

    dualDrainDone();
    dual_job.kind = DualJobKind::LmHeadRows;
    dual_job.begin = split;
    dual_job.end = VOCAB;
    dual_job.ok = false;
    __sync_synchronize();
    if (xSemaphoreGive(dual_start) != pdTRUE) {
        prod_runtime_ready = false;
        return false;
    }

    q4_head_rowpair2_rows(q4, scales_f16, x, logits, 0u, split);

     
     
     
    if (!dualWait()) {
        prod_runtime_ready = false;
        return false;
    }
    return true;
}


 
 
 

bool dualRunLayer0(
    std::size_t position
) {

    constexpr std::uint32_t layer =
        0u;


    const std::uint32_t
        attn_norm_relative =
            0u;


    const std::uint32_t
        ffn_norm_relative =
            NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix q{};
    ResolvedMatrix k{};
    ResolvedMatrix v{};
    ResolvedMatrix o{};

    ResolvedMatrix gate{};
    ResolvedMatrix up{};
    ResolvedMatrix down{};


#define RESOLVE_L0(FAMILY, DEST) \
    if (!resolveMatrix( \
            layer, \
            xp::MatrixFamily::FAMILY, \
            g_sd_scratch, \
            SD_SCRATCH_BYTES, \
            &DEST)) { \
        return false; \
    }


    RESOLVE_L0(QProj, q)
    RESOLVE_L0(KProj, k)
    RESOLVE_L0(VProj, v)
    RESOLVE_L0(OProj, o)

    RESOLVE_L0(GateProj, gate)
    RESOLVE_L0(UpProj, up)
    RESOLVE_L0(DownProj, down)


#undef RESOLVE_L0


    if (
        q.location != xp::MatrixLocation::Psram
        ||
        k.location != xp::MatrixLocation::Psram
        ||
        v.location != xp::MatrixLocation::Psram
        ||
        o.location != xp::MatrixLocation::Psram
        ||
        gate.location != xp::MatrixLocation::Psram
        ||
        up.location != xp::MatrixLocation::Psram
        ||
        down.location != xp::MatrixLocation::Psram
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    const std::size_t q_split =
        q.spec->rows / 2;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        q;

    dual_job.input =
        normed_rt;

    dual_job.output =
        q_rt;

    dual_job.begin =
        q_split;

    dual_job.end =
        q.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool q_main_ok =
        dualMatvecRows(
            q,
            normed_rt,
            q_rt,
            0,
            q_split
        );


    const bool q_worker_ok =
        dualWait();


    if (
        !q_main_ok
        ||
        !q_worker_ok
    ) {

        return false;
    }


     
     
     

    dualDrainDone();


    dual_job.kind =
        DualJobKind::Full;

    dual_job.matrix =
        v;

    dual_job.input =
        normed_rt;

    dual_job.output =
        v_rt;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool k_ok =
        pipeMatvec(
            k,
            normed_rt,
            k_rt
        );


    const bool v_ok =
        dualWait();


    if (
        !k_ok
        ||
        !v_ok
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


     
     
     

    const std::size_t head_split =
        cfg::kNHeads / 2;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Attention;

    dual_job.layer =
        layer;

    dual_job.position =
        position;

    dual_job.begin =
        head_split;

    dual_job.end =
        cfg::kNHeads;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    dualAttentionHeads(
        layer,
        position,
        0,
        head_split,
        hello_attn_scores
    );


    if (!dualWait()) {

        return false;
    }


     
     
     

    const std::size_t o_split =
        o.spec->rows / 2;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        o;

    dual_job.input =
        attention_rt;

    dual_job.output =
        o_rt;

    dual_job.begin =
        o_split;

    dual_job.end =
        o.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool o_main_ok =
        dualMatvecRows(
            o,
            attention_rt,
            o_rt,
            0,
            o_split
        );


    const bool o_worker_ok =
        dualWait();


    if (
        !o_main_ok
        ||
        !o_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    dualDrainDone();


    dual_job.kind =
        DualJobKind::Full;

    dual_job.matrix =
        up;

    dual_job.input =
        normed_rt;

    dual_job.output =
        up_rt;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool gate_ok =
        pipeMatvec(
            gate,
            normed_rt,
            gate_rt
        );


    const bool up_ok =
        dualWait();


    if (
        !gate_ok
        ||
        !up_ok
    ) {

        return false;
    }


     
     
     

    const std::size_t swiglu_split =
        cfg::kDff / 2;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::SwiGlu;

    dual_job.begin =
        swiglu_split;

    dual_job.end =
        cfg::kDff;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    for (
        std::size_t i = 0;
        i < swiglu_split;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (!dualWait()) {

        return false;
    }


     
     
     

    const std::size_t down_split =
        down.spec->rows / 2;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        down;

    dual_job.input =
        gate_rt;

    dual_job.output =
        down_rt;

    dual_job.begin =
        down_split;

    dual_job.end =
        down.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool down_main_ok =
        dualMatvecRows(
            down,
            gate_rt,
            down_rt,
            0,
            down_split
        );


    const bool down_worker_ok =
        dualWait();


    if (
        !down_main_ok
        ||
        !down_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

}  


void layer0_dual_probe_setup_unused() {

     
     
     
     
     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO LAYER0 DUAL COMPUTE PROBE V1 ==="
    );


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "DUAL_L0_RUNTIME: FAIL"
        );

        return;
    }


    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        Serial.println(
            "DUAL_L0_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        rc != pdPASS
    ) {

        Serial.println(
            "DUAL_L0_WORKER: FAIL"
        );

        return;
    }


    Serial.println(
        "DUAL_L0_WORKER: PASS|core=0"
    );


    delay(100);


    constexpr std::size_t N =
        8;


    std::uint64_t serial_total =
        0;

    std::uint64_t dual_total =
        0;


    bool all_exact =
        true;


    for (
        std::size_t iter = 0;
        iter < N;
        ++iter
    ) {

         
         
         

        prodResetTurn();


        if (
            !loadEmbeddingRow(
                0u,
                hidden_rt
            )
        ) {

            Serial.println(
                "DUAL_L0_EMBED: FAIL"
            );

            return;
        }


        const std::uint32_t s0 =
            micros();


        const bool serial_ok =
            helloRunLayerPosition(
                0u,
                0u,
                false
            );


        const std::uint32_t serial_us =
            micros() - s0;


        if (!serial_ok) {

            Serial.println(
                "DUAL_L0_SERIAL: FAIL"
            );

            return;
        }


        std::memcpy(
            dual_baseline_hidden,
            hidden_rt,
            sizeof(dual_baseline_hidden)
        );


         
         
         

        prodResetTurn();


        if (
            !loadEmbeddingRow(
                0u,
                hidden_rt
            )
        ) {

            return;
        }


        const std::uint32_t d0 =
            micros();


        const bool dual_ok =
            dualRunLayer0(
                0u
            );


        const std::uint32_t dual_us =
            micros() - d0;


        if (!dual_ok) {

            Serial.printf(
                "DUAL_L0_RUN: FAIL|iter=%u\n",
                static_cast<unsigned>(
                    iter
                )
            );

            return;
        }


        bool exact =
            true;

        double max_diff =
            0.0;


        for (
            std::size_t i = 0;
            i < cfg::kDModel;
            ++i
        ) {

            if (
                std::memcmp(
                    &dual_baseline_hidden[i],
                    &hidden_rt[i],
                    sizeof(float)
                ) != 0
            ) {

                exact =
                    false;
            }


            const double diff =
                fabs(
                    static_cast<double>(
                        dual_baseline_hidden[i]
                    )
                    -
                    static_cast<double>(
                        hidden_rt[i]
                    )
                );


            if (
                diff > max_diff
            ) {

                max_diff =
                    diff;
            }
        }


        if (!exact) {
            all_exact = false;
        }


        serial_total +=
            serial_us;

        dual_total +=
            dual_us;


        Serial.printf(
            "L0ITER|%u|"
            "serial_us=%lu|"
            "dual_us=%lu|"
            "speedup=%.4f|"
            "bit_exact=%s|"
            "max_diff=%.12f\n",

            static_cast<unsigned>(
                iter
            ),

            static_cast<unsigned long>(
                serial_us
            ),

            static_cast<unsigned long>(
                dual_us
            ),

            static_cast<double>(
                serial_us
            )
            /
            static_cast<double>(
                dual_us
            ),

            exact
                ? "YES"
                : "NO",

            max_diff
        );
    }


    const double avg_serial_us =
        static_cast<double>(
            serial_total
        )
        /
        static_cast<double>(
            N
        );


    const double avg_dual_us =
        static_cast<double>(
            dual_total
        )
        /
        static_cast<double>(
            N
        );


    const double speedup =
        avg_serial_us
        /
        avg_dual_us;


    Serial.println();

    Serial.println(
        "=== LAYER0 DUAL SUMMARY ==="
    );


    Serial.printf(
        "L0_SERIAL_AVG_US|%.1f\n",
        avg_serial_us
    );


    Serial.printf(
        "L0_DUAL_AVG_US|%.1f\n",
        avg_dual_us
    );


    Serial.printf(
        "L0_DUAL_SPEEDUP|%.4fx\n",
        speedup
    );


    Serial.printf(
        "L0_DUAL_BIT_EXACT: %s\n",
        all_exact
            ? "PASS"
            : "FAIL"
    );


    const bool final_ok =
        all_exact
        &&
        speedup > 1.0;


    Serial.println();


    Serial.printf(
        "XIAO_LAYER0_DUAL_COMPUTE_V1: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void layer0_dual_probe_loop_unused() {

    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 
 
 

namespace {

bool earlyDualRunPsramLayer(
    std::uint32_t layer,
    std::size_t position
) {

    if (
        layer < 1u
        ||
        layer > 4u
    ) {
        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {
        return false;
    }


    ResolvedMatrix q{};
    ResolvedMatrix k{};
    ResolvedMatrix v{};
    ResolvedMatrix o{};
    ResolvedMatrix gate{};
    ResolvedMatrix up{};
    ResolvedMatrix down{};


#define EARLY_RESOLVE(FAMILY, DEST) \
    if (!resolveMatrix( \
            layer, \
            xp::MatrixFamily::FAMILY, \
            g_sd_scratch, \
            SD_SCRATCH_BYTES, \
            &DEST)) { \
        return false; \
    }


    EARLY_RESOLVE(QProj, q)
    EARLY_RESOLVE(KProj, k)
    EARLY_RESOLVE(VProj, v)
    EARLY_RESOLVE(OProj, o)
    EARLY_RESOLVE(GateProj, gate)
    EARLY_RESOLVE(UpProj, up)
    EARLY_RESOLVE(DownProj, down)


#undef EARLY_RESOLVE


    if (
        q.location != xp::MatrixLocation::Psram
        ||
        k.location != xp::MatrixLocation::Psram
        ||
        v.location != xp::MatrixLocation::Psram
        ||
        o.location != xp::MatrixLocation::Psram
        ||
        gate.location != xp::MatrixLocation::Psram
        ||
        up.location != xp::MatrixLocation::Psram
        ||
        down.location != xp::MatrixLocation::Psram
    ) {
        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    const std::size_t q_split =
        q.spec->rows / 2;

    dualDrainDone();

    dual_job.kind = DualJobKind::Rows;
    dual_job.matrix = q;
    dual_job.input = normed_rt;
    dual_job.output = q_rt;
    dual_job.begin = q_split;
    dual_job.end = q.spec->rows;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool q0 =
        dualMatvecRows(
            q,
            normed_rt,
            q_rt,
            0,
            q_split
        );

    const bool q1 =
        dualWait();

    if (!q0 || !q1) {
        return false;
    }


     
    dualDrainDone();

    dual_job.kind = DualJobKind::Full;
    dual_job.matrix = v;
    dual_job.input = normed_rt;
    dual_job.output = v_rt;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool k_ok =
        pipeMatvec(
            k,
            normed_rt,
            k_rt
        );

    const bool v_ok =
        dualWait();

    if (!k_ok || !v_ok) {
        return false;
    }


    helloApplyRopeQk(position);
    helloStoreCurrentKv(layer, position);


     
    const std::size_t head_split =
        cfg::kNHeads / 2;

    dualDrainDone();

    dual_job.kind =
        DualJobKind::Attention;

    dual_job.layer =
        layer;

    dual_job.position =
        position;

    dual_job.begin =
        head_split;

    dual_job.end =
        cfg::kNHeads;

    dual_job.ok =
        false;

    xSemaphoreGive(dual_start);

    dualAttentionHeads(
        layer,
        position,
        0,
        head_split,
        hello_attn_scores
    );

    if (!dualWait()) {
        return false;
    }


     
    const std::size_t o_split =
        o.spec->rows / 2;

    dualDrainDone();

    dual_job.kind = DualJobKind::Rows;
    dual_job.matrix = o;
    dual_job.input = attention_rt;
    dual_job.output = o_rt;
    dual_job.begin = o_split;
    dual_job.end = o.spec->rows;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool o0 =
        dualMatvecRows(
            o,
            attention_rt,
            o_rt,
            0,
            o_split
        );

    const bool o1 =
        dualWait();

    if (!o0 || !o1) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        hidden_rt[i] += o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    dualDrainDone();

    dual_job.kind = DualJobKind::Full;
    dual_job.matrix = up;
    dual_job.input = normed_rt;
    dual_job.output = up_rt;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool gate_ok =
        pipeMatvec(
            gate,
            normed_rt,
            gate_rt
        );

    const bool up_ok =
        dualWait();

    if (!gate_ok || !up_ok) {
        return false;
    }


     
    const std::size_t swiglu_split =
        cfg::kDff / 2;

    dualDrainDone();

    dual_job.kind =
        DualJobKind::SwiGlu;

    dual_job.begin =
        swiglu_split;

    dual_job.end =
        cfg::kDff;

    dual_job.ok =
        false;

    xSemaphoreGive(dual_start);


    for (
        std::size_t i = 0;
        i < swiglu_split;
        ++i
    ) {

        const float g =
            gate_rt[i];

        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (!dualWait()) {
        return false;
    }


     
    const std::size_t down_split =
        down.spec->rows / 2;

    dualDrainDone();

    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        down;

    dual_job.input =
        gate_rt;

    dual_job.output =
        down_rt;

    dual_job.begin =
        down_split;

    dual_job.end =
        down.spec->rows;

    dual_job.ok =
        false;

    xSemaphoreGive(dual_start);


    const bool d0 =
        dualMatvecRows(
            down,
            gate_rt,
            down_rt,
            0,
            down_split
        );

    const bool d1 =
        dualWait();

    if (!d0 || !d1) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        hidden_rt[i] += down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 

bool earlyDualPipeRunToken(
    std::uint32_t token,
    std::size_t position,
    std::uint32_t* elapsed_ms
) {

    if (
        position >= HELLO_MAX_CONTEXT
    ) {
        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {
        return false;
    }


    pipe_next_q =
        PrefetchedMatrix{};


    const std::uint32_t t0 =
        millis();


     
    if (
        !pipeStartNextQ(
            5u,
            g_sd_scratch
        )
    ) {
        return false;
    }


     
    if (
        !helloRunLayerPosition(
            0u,
            position,
            false
        )
    ) {
        return false;
    }


     
     
     
     
     
    if (
        xSemaphoreTake(
            pipe_done_sem,
            portMAX_DELAY
        )
        != pdTRUE
    ) {
        return false;
    }


    __sync_synchronize();


    xSemaphoreGive(
        pipe_done_sem
    );


     
    for (
        std::uint32_t layer = 1;
        layer < 5u;
        ++layer
    ) {

        if (
            !earlyDualRunPsramLayer(
                layer,
                position
            )
        ) {
            return false;
        }
    }


     
    if (
        !pipeRunLayer5(
            position
        )
    ) {
        return false;
    }


    for (
        std::uint32_t layer = 6;
        layer < cfg::kNumLayers;
        ++layer
    ) {

        if (
            !pipeRunSdLayer(
                layer,
                position
            )
        ) {
            return false;
        }
    }


    const std::uint32_t ms =
        millis() - t0;


    if (elapsed_ms) {
        *elapsed_ms = ms;
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

}  


void earlydual_token0_probe_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO SDPIPE + EARLY DUAL TOKEN0 V1 ==="
    );


    if (!prod_runtime_ready) {
        Serial.println(
            "EARLYDUAL_RUNTIME: FAIL"
        );
        return;
    }


     
    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {
        Serial.println(
            "EARLYDUAL_PIPE_SCRATCH: FAIL"
        );
        return;
    }


     
    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {
        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {
        return;
    }


     
    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {
        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {
        return;
    }


    Serial.println(
        "EARLYDUAL_WORKERS: PASS|core0"
    );


    delay(100);


     
     
     

    prodResetTurn();


    const std::uint32_t b0 =
        millis();


    std::uint32_t base_ms =
        0;


    const bool base_ok =
        pipeRunToken(
            0u,
            0u,
            &base_ms
        );


    const std::uint32_t base_wall =
        millis() - b0;


    if (!base_ok) {
        Serial.println(
            "EARLYDUAL_BASELINE: FAIL"
        );
        return;
    }


    alignas(16)
    float baseline[
        cfg::kDModel
    ];


    std::memcpy(
        baseline,
        hidden_rt,
        sizeof(baseline)
    );


    Serial.printf(
        "SDPIPE_BASELINE|ms=%lu|wall=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            base_ms
        ),

        static_cast<unsigned long>(
            base_wall
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


     
     
     

    prodResetTurn();


    std::uint32_t hybrid_ms =
        0;


    const bool hybrid_ok =
        earlyDualPipeRunToken(
            0u,
            0u,
            &hybrid_ms
        );


    if (!hybrid_ok) {
        Serial.println(
            "EARLYDUAL_HYBRID: FAIL"
        );
        return;
    }


    bool exact =
        true;

    double max_diff =
        0.0;


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        if (
            std::memcmp(
                &baseline[i],
                &hidden_rt[i],
                sizeof(float)
            ) != 0
        ) {
            exact = false;
        }


        const double d =
            fabs(
                static_cast<double>(
                    baseline[i]
                )
                -
                static_cast<double>(
                    hidden_rt[i]
                )
            );


        if (d > max_diff) {
            max_diff = d;
        }
    }


    Serial.printf(
        "EARLYDUAL_HYBRID|ms=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            hybrid_ms
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


    Serial.printf(
        "EARLYDUAL_COMPARE|"
        "bit_exact=%s|"
        "max_diff=%.12f\n",

        exact ? "YES" : "NO",
        max_diff
    );


    const double speedup =
        hybrid_ms > 0
        ? static_cast<double>(
              base_ms
          )
          /
          static_cast<double>(
              hybrid_ms
          )
        : 0.0;


    Serial.printf(
        "EARLYDUAL_TOKEN_SPEEDUP|%.4fx\n",
        speedup
    );


    const bool final_ok =
        base_ok
        &&
        hybrid_ok
        &&
        exact
        &&
        hybrid_ms < base_ms;


    Serial.println();


    Serial.printf(
        "XIAO_SDPIPE_EARLYDUAL_TOKEN0_V1: %s\n",
        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void earlydual_token0_probe_loop_unused() {
    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

bool earlyDualProdGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    for (
        std::size_t pos = 0;
        pos < prompt_count;
        ++pos
    ) {

        const std::uint32_t token_t0 =
            micros();


        if (
            !earlyDualPipeRunToken(
                prompt_ids[pos],
                pos,
                nullptr)
        ) {

            Serial.printf(
                "[CHAT] prompt token FAIL|"
                "pos=%u|"
                "id=%u\n",

                static_cast<unsigned>(
                    pos
                ),

                static_cast<unsigned>(
                    prompt_ids[pos]
                )
            );

            return false;
        }


        total_layers_us +=
            micros() - token_t0;


        yield();
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !earlyDualPipeRunToken(
                    next,
                    next_context_pos,
                    nullptr)
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}

void earlyDualProdProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        earlyDualProdGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}


void earlydual_v2_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO SDPIPE + EARLY DUAL V2 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_SCRATCH2: FAIL"
        );

        return;
    }


     
     
     

    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_PIPE_SEM: FAIL"
        );

        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_PIPE_WORKER: FAIL"
        );

        return;
    }


     
     
     
     
     
     

    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_DUAL_SEM: FAIL"
        );

        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "EARLYDUAL_CHAT_DUAL_WORKER: FAIL"
        );

        return;
    }


    delay(100);


    Serial.printf(
        "EARLYDUAL_CHAT_WORKERS: PASS|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


    Serial.printf(
        "[EARLYDUAL_READY] "
        "context=%u "
        "max_generate=%u "
        "stop=END\n",

        static_cast<unsigned>(
            PROD_CONTEXT
        ),

        static_cast<unsigned>(
            PROD_MAX_GENERATE
        )
    );


    Serial.println(
        "Validation prompt: hello"
    );


    prodPrintPrompt();
}


void earlydual_v2_loop_unused() {

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                earlyDualProdProcessLine(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}


 
 
 
 
 
 
 
 
 
 

namespace {

uint8_t* cached_o11_buffer =
    nullptr;

ResolvedMatrix cached_o11_matrix{};

bool cached_o11_valid =
    false;

bool cachedO11RunSdLayer(
    std::uint32_t layer,
    std::size_t position
) {

    if (
        layer < 6u
        ||
        layer >= cfg::kNumLayers
    ) {

        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeFinishNextQ(
            layer
        )
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;


    uint8_t* q_buffer =
        pipe_next_q.buffer;

    uint8_t* other_buffer =
        (
            q_buffer == g_sd_scratch
        )
        ? pipe_scratch2
        : g_sd_scratch;


     
     
     

    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::KProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix k{};


    if (
        !pipeFinishRead(
            &k,
            nullptr
        )
    ) {

        return false;
    }


     
     
     

    if (
        !pipeStartRead(
            layer,
            xp::MatrixFamily::VProj,
            q_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            k,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


    ResolvedMatrix v{};


    if (
        !pipeFinishRead(
            &v,
            nullptr
        )
    ) {

        return false;
    }


     
     
     

    if (
        layer != 11u
        &&
        !pipeStartRead(
            layer,
            xp::MatrixFamily::OProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            v,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


     
     
    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


    ResolvedMatrix o{};


    if (layer == 11u) {

        if (!cached_o11_valid) {
            return false;
        }

        o =
            cached_o11_matrix;

    } else {

        if (
            !pipeFinishRead(
                &o,
                nullptr
            )
        ) {

            return false;
        }
    }


    if (
        !pipeMatvec(
            o,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     
     
     

    if (
        layer + 1u
        < cfg::kNumLayers
    ) {

        if (
            !pipeStartNextQ(
                layer + 1u,
                q_buffer
            )
        ) {

            return false;
        }

    } else {

        pipe_next_q.valid =
            false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

bool cachedO11RunToken(
    std::uint32_t token,
    std::size_t position,
    std::uint32_t* elapsed_ms
) {

    if (
        position >= HELLO_MAX_CONTEXT
    ) {
        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {
        return false;
    }


    pipe_next_q =
        PrefetchedMatrix{};


    const std::uint32_t t0 =
        millis();


     
    if (
        !pipeStartNextQ(
            5u,
            g_sd_scratch
        )
    ) {
        return false;
    }


     
    if (
        !helloRunLayerPosition(
            0u,
            position,
            false
        )
    ) {
        return false;
    }


     
     
     
     
     
    if (
        xSemaphoreTake(
            pipe_done_sem,
            portMAX_DELAY
        )
        != pdTRUE
    ) {
        return false;
    }


    __sync_synchronize();


    xSemaphoreGive(
        pipe_done_sem
    );


     
    for (
        std::uint32_t layer = 1;
        layer < 5u;
        ++layer
    ) {

        if (
            !earlyDualRunPsramLayer(
                layer,
                position
            )
        ) {
            return false;
        }
    }


     
    if (
        !pipeRunLayer5(
            position
        )
    ) {
        return false;
    }


    for (
        std::uint32_t layer = 6;
        layer < cfg::kNumLayers;
        ++layer
    ) {

        if (
            !cachedO11RunSdLayer(
                layer,
                position
            )
        ) {
            return false;
        }
    }


    const std::uint32_t ms =
        millis() - t0;


    if (elapsed_ms) {
        *elapsed_ms = ms;
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

}  


void cached_o11_probe_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO CACHED O11 TOKEN0 PROBE V1 ==="
    );


    if (!prod_runtime_ready) {

        Serial.println(
            "CACHED_O11_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        Serial.println(
            "CACHED_O11_SCRATCH2: FAIL"
        );

        return;
    }


    Serial.printf(
        "AFTER_SCRATCH2|free=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     
     
     
     

    cached_o11_buffer =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!cached_o11_buffer) {

        Serial.printf(
            "CACHED_O11_ALLOC: FAIL|"
            "need=%lu|free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                SD_SCRATCH_BYTES
            ),

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    const std::uint32_t cache_t0 =
        millis();


    if (
        !resolveMatrix(
            11u,
            xp::MatrixFamily::OProj,
            cached_o11_buffer,
            SD_SCRATCH_BYTES,
            &cached_o11_matrix
        )
    ) {

        Serial.println(
            "CACHED_O11_LOAD: FAIL"
        );

        return;
    }


    const std::uint32_t cache_ms =
        millis() - cache_t0;


    if (
        cached_o11_matrix.location
        != xp::MatrixLocation::Sd
    ) {

        Serial.println(
            "CACHED_O11_SOURCE: FAIL"
        );

        return;
    }


    cached_o11_valid =
        true;


    Serial.printf(
        "CACHED_O11_LOAD: PASS|"
        "q_bytes=%lu|scales_bytes=%lu|"
        "packed=%lu|ms=%lu|"
        "free=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            cached_o11_matrix.q_bytes
        ),

        static_cast<unsigned long>(
            cached_o11_matrix.scales_bytes
        ),

        static_cast<unsigned long>(
            cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
        ),

        static_cast<unsigned long>(
            cache_ms
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        Serial.println(
            "CACHED_O11_PIPE_SEM: FAIL"
        );

        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {

        Serial.println(
            "CACHED_O11_PIPE_WORKER: FAIL"
        );

        return;
    }


     
     
     

    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        Serial.println(
            "CACHED_O11_DUAL_SEM: FAIL"
        );

        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {

        Serial.println(
            "CACHED_O11_DUAL_WORKER: FAIL"
        );

        return;
    }


    Serial.println(
        "CACHED_O11_WORKERS: PASS"
    );


    delay(100);


     
     
     

    prodResetTurn();


    std::uint32_t base_ms =
        0;


    const bool base_ok =
        earlyDualPipeRunToken(
            0u,
            0u,
            &base_ms
        );


    if (!base_ok) {

        Serial.println(
            "CACHED_O11_BASELINE: FAIL"
        );

        return;
    }


    alignas(16)
    float baseline_hidden[
        cfg::kDModel
    ];


    std::memcpy(
        baseline_hidden,
        hidden_rt,
        sizeof(baseline_hidden)
    );


    const double base_l2 =
        vectorL2(
            hidden_rt,
            cfg::kDModel
        );


    const double base_checksum =
        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        );


    Serial.printf(
        "V2_BASELINE|ms=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            base_ms
        ),

        base_l2,
        base_checksum
    );


     
     
     

    prodResetTurn();


    std::uint32_t cached_ms =
        0;


    const bool cached_ok =
        cachedO11RunToken(
            0u,
            0u,
            &cached_ms
        );


    if (!cached_ok) {

        Serial.println(
            "CACHED_O11_TOKEN: FAIL"
        );

        return;
    }


    bool exact =
        true;

    double max_diff =
        0.0;


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        if (
            std::memcmp(
                &baseline_hidden[i],
                &hidden_rt[i],
                sizeof(float)
            ) != 0
        ) {

            exact =
                false;
        }


        const double d =
            fabs(
                static_cast<double>(
                    baseline_hidden[i]
                )
                -
                static_cast<double>(
                    hidden_rt[i]
                )
            );


        if (d > max_diff) {
            max_diff = d;
        }
    }


    Serial.printf(
        "CACHED_O11_TOKEN|ms=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            cached_ms
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


    Serial.printf(
        "CACHED_O11_COMPARE|"
        "bit_exact=%s|"
        "max_diff=%.12f\n",

        exact
            ? "YES"
            : "NO",

        max_diff
    );


    const double speedup =
        cached_ms > 0
        ? static_cast<double>(
              base_ms
          )
          /
          static_cast<double>(
              cached_ms
          )
        : 0.0;


    Serial.printf(
        "CACHED_O11_SPEEDUP|%.4fx\n",
        speedup
    );


    Serial.printf(
        "CACHED_O11_SAVED_MS|%ld\n",

        static_cast<long>(
            base_ms
        )
        -
        static_cast<long>(
            cached_ms
        )
    );


    const bool final_ok =
        exact
        &&
        cached_ms
            < base_ms;


    Serial.println();


    Serial.printf(
        "XIAO_CACHED_O11_TOKEN0_V1: %s\n",

        final_ok
            ? "PASS"
            : "FAIL"
    );
}


void cached_o11_probe_loop_unused() {

    delay(1000);
}


 
 
 
 
 
 
 
 
 
 
 
 
 

bool cachedO11ProdGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    for (
        std::size_t pos = 0;
        pos < prompt_count;
        ++pos
    ) {

        const std::uint32_t token_t0 =
            micros();


        if (
            !cachedO11RunToken(
                prompt_ids[pos],
                pos,
                nullptr)
        ) {

            Serial.printf(
                "[CHAT] prompt token FAIL|"
                "pos=%u|"
                "id=%u\n",

                static_cast<unsigned>(
                    pos
                ),

                static_cast<unsigned>(
                    prompt_ids[pos]
                )
            );

            return false;
        }


        total_layers_us +=
            micros() - token_t0;


        yield();
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !cachedO11RunToken(
                    next,
                    next_context_pos,
                    nullptr)
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}

void cachedO11ProdProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        cachedO11ProdGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}


void v3_tailcache_base_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO SDPIPE + EARLY DUAL + CACHED O11 V3 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_SCRATCH2: FAIL"
        );

        return;
    }


     
     
     

    cached_o11_buffer =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!cached_o11_buffer) {

        prod_runtime_ready =
            false;

        Serial.printf(
            "V3_O11_ALLOC: FAIL|"
            "free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    const std::uint32_t cache_t0 =
        millis();


    if (
        !resolveMatrix(
            11u,
            xp::MatrixFamily::OProj,
            cached_o11_buffer,
            SD_SCRATCH_BYTES,
            &cached_o11_matrix
        )
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LOAD: FAIL"
        );

        return;
    }


    const std::uint32_t cache_ms =
        millis() - cache_t0;


    if (
        cached_o11_matrix.location
            != xp::MatrixLocation::Sd
        ||
        cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
            > SD_SCRATCH_BYTES
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LAYOUT: FAIL"
        );

        return;
    }


    cached_o11_valid =
        true;


    Serial.printf(
        "V3_O11_CACHE: PASS|"
        "packed=%lu|ms=%lu|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
        ),

        static_cast<unsigned long>(
            cache_ms
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_WORKER: FAIL"
        );

        return;
    }


     
     
     

    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_WORKER: FAIL"
        );

        return;
    }


    delay(100);


    Serial.printf(
        "V3_WORKERS: PASS|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


    Serial.printf(
        "[V3_READY] "
        "context=%u "
        "max_generate=%u "
        "stop=END\n",

        static_cast<unsigned>(
            PROD_CONTEXT
        ),

        static_cast<unsigned>(
            PROD_MAX_GENERATE
        )
    );


    Serial.println(
        "Validation prompt: hello"
    );


    prodPrintPrompt();
}


void v3_tailcache_base_loop_unused() {

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                cachedO11ProdProcessLine(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 

bool appTailRunSdLayer(
    std::uint32_t layer,
    std::size_t position
);

namespace {

constexpr uint32_t APP_PHYSICAL_START =
    0x10000u;

constexpr uint32_t APP_PARTITION_BYTES =
    0xA0000u;

constexpr uint32_t APP_TAIL_CACHE_PHYSICAL =
    0x86000u;

constexpr uint32_t APP_TAIL_CACHE_PART_OFFSET =
    APP_TAIL_CACHE_PHYSICAL
    - APP_PHYSICAL_START;

constexpr uint32_t APP_TAIL_CACHE_BYTES =
    159936u;


constexpr uint32_t TAIL_O10_OFFSET =
    0u;

constexpr uint32_t TAIL_K11_OFFSET =
    106624u;

constexpr uint32_t TAIL_V11_OFFSET =
    133280u;


const uint8_t* app_tail_base =
    nullptr;

esp_partition_mmap_handle_t
    app_tail_handle =
        0;


ResolvedMatrix app_tail_o10{};

 
 
 
 
 
 
 
 

constexpr std::uint32_t
K10_PRECACHE_APP_OFFSET =
    0x6F000u;

constexpr std::uint32_t
K10_PRECACHE_BYTES =
    26656u;


esp_partition_mmap_handle_t
k10_precache_handle =
    0;


const std::uint8_t*
k10_precache_base =
    nullptr;


ResolvedMatrix
k10_precache_matrix{};


bool
k10_precache_ready =
    false;


bool initK10Precache() {

    const esp_partition_t* app =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY,
            nullptr
        );


    if (!app) {
        return false;
    }


    const void* mapped =
        nullptr;


    const esp_err_t err =
        esp_partition_mmap(
            app,
            K10_PRECACHE_APP_OFFSET,
            K10_PRECACHE_BYTES,
            ESP_PARTITION_MMAP_DATA,
            &mapped,
            &k10_precache_handle
        );


    if (
        err != ESP_OK
        ||
        !mapped
    ) {

        return false;
    }


    k10_precache_base =
        static_cast<
            const std::uint8_t*
        >(
            mapped
        );


    const cfg::MatrixSpec& spec =
        specFor(
            10u,
            xp::MatrixFamily::KProj
        );


    const std::uint32_t total =
        spec.q4_bytes
        + spec.scales_bytes;


    if (
        total
        != K10_PRECACHE_BYTES
    ) {

        return false;
    }


    k10_precache_matrix.spec =
        &spec;

    k10_precache_matrix.q4 =
        k10_precache_base;

    k10_precache_matrix.scales =
        reinterpret_cast<
            const std::uint16_t*
        >(
            k10_precache_base
            + spec.q4_bytes
        );

    k10_precache_matrix.location =
        xp::MatrixLocation::Flash;

    k10_precache_matrix.q_bytes =
        spec.q4_bytes;

    k10_precache_matrix.scales_bytes =
        spec.scales_bytes;

    k10_precache_matrix.total_bytes =
        total;


    k10_precache_ready =
        true;


    return true;
}


ResolvedMatrix app_tail_k11{};
ResolvedMatrix app_tail_v11{};

bool app_tail_ready =
    false;


 
 
 
 
 
 
 

bool makeAppTailMatrix(
    uint32_t layer,
    xp::MatrixFamily family,
    uint32_t cache_offset,
    ResolvedMatrix* out
) {

    if (
        !out
        ||
        !app_tail_base
        ||
        layer >= cfg::kNumLayers
    ) {

        return false;
    }


    const cfg::MatrixSpec& spec =
        specFor(
            layer,
            family
        );


    const uint32_t q_bytes =
        spec.q4_bytes;

    const uint32_t scales_bytes =
        spec.scales_bytes;

    const uint32_t total =
        q_bytes
        + scales_bytes;


    if (
        cache_offset
        + total
        > APP_TAIL_CACHE_BYTES
    ) {

        return false;
    }


    out->spec =
        &spec;

    out->q4 =
        app_tail_base
        + cache_offset;

    out->scales =
        reinterpret_cast<
            const uint16_t*
        >(
            app_tail_base
            + cache_offset
            + q_bytes
        );

     
     
    out->location =
        xp::MatrixLocation::Flash;

    out->q_bytes =
        q_bytes;

    out->scales_bytes =
        scales_bytes;

    out->total_bytes =
        total;


    return true;
}


 
 
 
 
 
 
 
 
 
 

bool appTailRunLayer11(
    size_t position
) {

    constexpr uint32_t layer =
        11u;


    const uint32_t attn_norm_relative =
        layer
        * 2u
        * NORM_ROW_BYTES;

    const uint32_t ffn_norm_relative =
        attn_norm_relative
        + NORM_ROW_BYTES;


    if (
        !app_tail_ready
        ||
        !cached_o11_valid
    ) {

        return false;
    }


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeFinishNextQ(
            layer
        )
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;


    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


     
    if (
        !pipeMatvec(
            app_tail_k11,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


     
    if (
        !pipeMatvec(
            app_tail_v11,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


     
    if (
        !pipeMatvec(
            cached_o11_matrix,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    pipe_next_q.valid =
        false;


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 
 
 
 
 
 

bool appTailRunToken(
    uint32_t token,
    size_t position,
    uint32_t* elapsed_ms
) {

    if (
        position >= HELLO_MAX_CONTEXT
    ) {

        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {

        return false;
    }


    pipe_next_q =
        PrefetchedMatrix{};


    const uint32_t t0 =
        millis();


    if (
        !pipeStartNextQ(
            5u,
            g_sd_scratch
        )
    ) {

        return false;
    }


     
    if (
        !helloRunLayerPosition(
            0u,
            position,
            false
        )
    ) {

        return false;
    }


     
    if (
        xSemaphoreTake(
            pipe_done_sem,
            portMAX_DELAY
        )
        != pdTRUE
    ) {

        return false;
    }


    __sync_synchronize();


    xSemaphoreGive(
        pipe_done_sem
    );


     
    for (
        uint32_t layer = 1u;
        layer < 5u;
        ++layer
    ) {

        if (
            !earlyDualRunPsramLayer(
                layer,
                position
            )
        ) {

            return false;
        }
    }


     
    if (
        !pipeRunLayer5(
            position
        )
    ) {

        return false;
    }


     
     
     
     
    for (
        uint32_t layer = 6u;
        layer <= 10u;
        ++layer
    ) {

        if (
            !appTailRunSdLayer(
                layer,
                position
            )
        ) {

            return false;
        }
    }


     
    if (
        !appTailRunLayer11(
            position
        )
    ) {

        return false;
    }


    const uint32_t ms =
        millis() - t0;


    if (elapsed_ms) {
        *elapsed_ms = ms;
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


}  


bool appTailRunSdLayer(
    std::uint32_t layer,
    std::size_t position
) {

    if (
        layer < 6u
        ||
        layer >= cfg::kNumLayers
    ) {

        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    if (
        !pipeFinishNextQ(
            layer
        )
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;


    uint8_t* q_buffer =
        pipe_next_q.buffer;

    uint8_t* other_buffer =
        (
            q_buffer == g_sd_scratch
        )
        ? pipe_scratch2
        : g_sd_scratch;


     
     
     
     
     
     
     
     
     
     

    ResolvedMatrix k{};
    ResolvedMatrix v{};


    if (layer == 10u) {

        if (!k10_precache_ready) {
            return false;
        }


         
         
        if (
            !pipeStartRead(
                layer,
                xp::MatrixFamily::VProj,
                other_buffer,
                SD_SCRATCH_BYTES
            )
        ) {

            return false;
        }


        if (
            !pipeMatvec(
                q,
                normed_rt,
                q_rt
            )
        ) {

            return false;
        }


        if (
            !pipeMatvec(
                k10_precache_matrix,
                normed_rt,
                k_rt
            )
        ) {

            return false;
        }


        if (
            !pipeFinishRead(
                &v,
                nullptr
            )
        ) {

            return false;
        }


    } else {

         
         
         
         

        if (
            !pipeStartRead(
                layer,
                xp::MatrixFamily::KProj,
                other_buffer,
                SD_SCRATCH_BYTES
            )
        ) {

            return false;
        }


        if (
            !pipeMatvec(
                q,
                normed_rt,
                q_rt
            )
        ) {

            return false;
        }


        if (
            !pipeFinishRead(
                &k,
                nullptr
            )
        ) {

            return false;
        }


         
         
         
         

        if (
            !pipeStartRead(
                layer,
                xp::MatrixFamily::VProj,
                q_buffer,
                SD_SCRATCH_BYTES
            )
        ) {

            return false;
        }


        if (
            !pipeMatvec(
                k,
                normed_rt,
                k_rt
            )
        ) {

            return false;
        }


        if (
            !pipeFinishRead(
                &v,
                nullptr
            )
        ) {

            return false;
        }
    }


     
     
     

    if (
        layer != 10u
        &&
        layer != 11u
        &&
        !pipeStartRead(
            layer,
            xp::MatrixFamily::OProj,
            other_buffer,
            SD_SCRATCH_BYTES
        )
    ) {

        return false;
    }


    if (
        !pipeMatvec(
            v,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


     
     
    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


    ResolvedMatrix o{};


    if (layer == 10u) {

        if (!app_tail_ready) {
            return false;
        }

        o =
            app_tail_o10;

    } else if (layer == 11u) {

        if (!cached_o11_valid) {
            return false;
        }

        o =
            cached_o11_matrix;

    } else {

        if (
            !pipeFinishRead(
                &o,
                nullptr
            )
        ) {

            return false;
        }
    }


    if (layer < 10u) {

        const std::size_t o_split =
            o.spec->rows
            / 2u;


        dualDrainDone();


        dual_job.kind =
            DualJobKind::Rows;

        dual_job.matrix =
            o;

        dual_job.input =
            attention_rt;

        dual_job.output =
            o_rt;

        dual_job.begin =
            o_split;

        dual_job.end =
            o.spec->rows;

        dual_job.ok =
            false;


        xSemaphoreGive(
            dual_start
        );


        const bool o_main_ok =
            dualMatvecRows(
                o,
                attention_rt,
                o_rt,
                0u,
                o_split
            );


        const bool o_worker_ok =
            dualWait();


        if (
            !o_main_ok
            ||
            !o_worker_ok
        ) {

            return false;
        }


    } else {

         
         
         
         
         
         
         

        const std::size_t o_split =
            o.spec->rows
            / 2u;


        dualDrainDone();


        dual_job.kind =
            DualJobKind::Rows;

        dual_job.matrix =
            o;

        dual_job.input =
            attention_rt;

        dual_job.output =
            o_rt;

        dual_job.begin =
            o_split;

        dual_job.end =
            o.spec->rows;

        dual_job.ok =
            false;


        xSemaphoreGive(
            dual_start
        );


        const bool o_main_ok =
            dualMatvecRows(
                o,
                attention_rt,
                o_rt,
                0u,
                o_split
            );


        const bool o_worker_ok =
            dualWait();


        if (
            !o_main_ok
            ||
            !o_worker_ok
        ) {

            return false;
        }
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     
     
     

    if (
        layer + 1u
        < cfg::kNumLayers
    ) {

        if (
            !pipeStartNextQ(
                layer + 1u,
                q_buffer
            )
        ) {

            return false;
        }

    } else {

        pipe_next_q.valid =
            false;
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            normed_rt,
            gate_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            normed_rt,
            up_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDff;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            gate_rt,
            down_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}


 
 
 

void v4_token0_probe_setup_unused() {

     
     
     
     
     
     
     
    v3_tailcache_base_setup_unused();


    Serial.println();

    Serial.println(
        "=== XIAO V3 APP-TAIL CACHE TOKEN0 PROBE V1 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !cached_o11_valid
    ) {

        Serial.println(
            "APP_TAIL_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    const esp_partition_t* app =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY,
            "app0"
        );


    if (
        !app
        ||
        app->address
            != APP_PHYSICAL_START
        ||
        app->size
            != APP_PARTITION_BYTES
    ) {

        Serial.println(
            "APP_TAIL_PARTITION: FAIL"
        );

        return;
    }


    Serial.printf(
        "APP_TAIL_PARTITION: PASS|"
        "address=0x%lx|size=0x%lx\n",

        static_cast<unsigned long>(
            app->address
        ),

        static_cast<unsigned long>(
            app->size
        )
    );


     
     
     
     

    const void* mapped =
        nullptr;


    const esp_err_t err =
        esp_partition_mmap(
            app,
            APP_TAIL_CACHE_PART_OFFSET,
            APP_TAIL_CACHE_BYTES,
            ESP_PARTITION_MMAP_DATA,
            &mapped,
            &app_tail_handle
        );


    if (
        err != ESP_OK
        ||
        !mapped
    ) {

        Serial.printf(
            "APP_TAIL_MMAP: FAIL|err=%d\n",
            static_cast<int>(err)
        );

        return;
    }


    app_tail_base =
        static_cast<
            const uint8_t*
        >(mapped);


    Serial.printf(
        "APP_TAIL_MMAP: PASS|"
        "partition_offset=0x%lx|"
        "bytes=%lu|ptr=%p\n",

        static_cast<unsigned long>(
            APP_TAIL_CACHE_PART_OFFSET
        ),

        static_cast<unsigned long>(
            APP_TAIL_CACHE_BYTES
        ),

        mapped
    );


    if (
        !makeAppTailMatrix(
            10u,
            xp::MatrixFamily::OProj,
            TAIL_O10_OFFSET,
            &app_tail_o10
        )
        ||
        !makeAppTailMatrix(
            11u,
            xp::MatrixFamily::KProj,
            TAIL_K11_OFFSET,
            &app_tail_k11
        )
        ||
        !makeAppTailMatrix(
            11u,
            xp::MatrixFamily::VProj,
            TAIL_V11_OFFSET,
            &app_tail_v11
        )
    ) {

        Serial.println(
            "APP_TAIL_MATRIX_LAYOUT: FAIL"
        );

        return;
    }


    if (
        app_tail_o10.total_bytes
            != 106624u
        ||
        app_tail_k11.total_bytes
            != 26656u
        ||
        app_tail_v11.total_bytes
            != 26656u
    ) {

        Serial.println(
            "APP_TAIL_MATRIX_SIZES: FAIL"
        );

        return;
    }


    app_tail_ready =
        true;


    Serial.println(
        "APP_TAIL_MATRIX_LAYOUT: PASS"
    );


    delay(100);


     
     
     

    prodResetTurn();


    uint32_t base_ms =
        0;


    const uint32_t b0 =
        micros();


    const bool base_ok =
        cachedO11RunToken(
            0u,
            0u,
            &base_ms
        );


    const uint32_t base_us =
        micros() - b0;


    if (!base_ok) {

        Serial.println(
            "APP_TAIL_BASELINE: FAIL"
        );

        return;
    }


    alignas(16)
    float baseline[
        cfg::kDModel
    ];


    memcpy(
        baseline,
        hidden_rt,
        sizeof(baseline)
    );


    Serial.printf(
        "V3_BASELINE|ms=%lu|us=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            base_ms
        ),

        static_cast<unsigned long>(
            base_us
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


     
     
     

    prodResetTurn();


    uint32_t tail_ms =
        0;


    const uint32_t t0 =
        micros();


    const bool tail_ok =
        appTailRunToken(
            0u,
            0u,
            &tail_ms
        );


    const uint32_t tail_us =
        micros() - t0;


    if (!tail_ok) {

        Serial.println(
            "APP_TAIL_CANDIDATE: FAIL"
        );

        return;
    }


    bool exact =
        true;

    double max_diff =
        0.0;


    for (
        size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        if (
            memcmp(
                &baseline[i],
                &hidden_rt[i],
                sizeof(float)
            ) != 0
        ) {

            exact =
                false;
        }


        const double d =
            fabs(
                static_cast<double>(
                    baseline[i]
                )
                -
                static_cast<double>(
                    hidden_rt[i]
                )
            );


        if (d > max_diff) {
            max_diff = d;
        }
    }


    Serial.printf(
        "APP_TAIL_TOKEN|ms=%lu|us=%lu|"
        "l2=%.9f|checksum=%.9f\n",

        static_cast<unsigned long>(
            tail_ms
        ),

        static_cast<unsigned long>(
            tail_us
        ),

        vectorL2(
            hidden_rt,
            cfg::kDModel
        ),

        vectorChecksum(
            hidden_rt,
            cfg::kDModel
        )
    );


    Serial.printf(
        "APP_TAIL_COMPARE|"
        "bit_exact=%s|"
        "max_diff=%.12f\n",

        exact ? "YES" : "NO",
        max_diff
    );


    const double speedup =
        tail_us > 0u
        ? static_cast<double>(
              base_us
          )
          /
          static_cast<double>(
              tail_us
          )
        : 0.0;


    Serial.printf(
        "APP_TAIL_SPEEDUP|%.4fx\n",
        speedup
    );


    Serial.printf(
        "APP_TAIL_SAVED_US|%ld\n",

        static_cast<long>(
            base_us
        )
        -
        static_cast<long>(
            tail_us
        )
    );


    Serial.println();


    Serial.printf(
        "XIAO_V3_APP_TAIL_CACHE_TOKEN0_V1: %s\n",

        (
            exact
            &&
            tail_us < base_us
        )
        ? "PASS"
        : "FAIL"
    );
}


void v4_token0_probe_loop_unused() {
    delay(1000);
}


namespace {


bool appTailProductionInit() {

    if (
        !prod_runtime_ready
        ||
        !cached_o11_valid
    ) {

        Serial.println(
            "V4_APP_TAIL_PREREQ: FAIL"
        );

        return false;
    }


    const esp_partition_t* app =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_FACTORY,
            "app0"
        );


    if (
        !app
        ||
        app->address
            != APP_PHYSICAL_START
        ||
        app->size
            != APP_PARTITION_BYTES
    ) {

        Serial.println(
            "V4_APP_TAIL_PARTITION: FAIL"
        );

        return false;
    }


    const void* mapped =
        nullptr;


    const esp_err_t err =
        esp_partition_mmap(
            app,
            APP_TAIL_CACHE_PART_OFFSET,
            APP_TAIL_CACHE_BYTES,
            ESP_PARTITION_MMAP_DATA,
            &mapped,
            &app_tail_handle
        );


    if (
        err != ESP_OK
        ||
        !mapped
    ) {

        Serial.printf(
            "V4_APP_TAIL_MMAP: FAIL|err=%d\n",
            static_cast<int>(err)
        );

        return false;
    }


    app_tail_base =
        static_cast<
            const uint8_t*
        >(mapped);


    if (
        !makeAppTailMatrix(
            10u,
            xp::MatrixFamily::OProj,
            TAIL_O10_OFFSET,
            &app_tail_o10
        )
        ||
        !makeAppTailMatrix(
            11u,
            xp::MatrixFamily::KProj,
            TAIL_K11_OFFSET,
            &app_tail_k11
        )
        ||
        !makeAppTailMatrix(
            11u,
            xp::MatrixFamily::VProj,
            TAIL_V11_OFFSET,
            &app_tail_v11
        )
    ) {

        Serial.println(
            "V4_APP_TAIL_LAYOUT: FAIL"
        );

        return false;
    }


    if (
        app_tail_o10.total_bytes
            != 106624u
        ||
        app_tail_k11.total_bytes
            != 26656u
        ||
        app_tail_v11.total_bytes
            != 26656u
    ) {

        Serial.println(
            "V4_APP_TAIL_SIZE: FAIL"
        );

        return false;
    }


    app_tail_ready =
        true;


    Serial.printf(
        "V4_APP_TAIL_CACHE: PASS|"
        "flash=0x%lx|bytes=%lu|end=0x%lx|"
        "ptr=%p\n",

        static_cast<unsigned long>(
            APP_TAIL_CACHE_PHYSICAL
        ),

        static_cast<unsigned long>(
            APP_TAIL_CACHE_BYTES
        ),

        static_cast<unsigned long>(
            APP_TAIL_CACHE_PHYSICAL
            + APP_TAIL_CACHE_BYTES
        ),

        mapped
    );


    return true;
}


bool appTailProdGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    for (
        std::size_t pos = 0;
        pos < prompt_count;
        ++pos
    ) {

        const std::uint32_t token_t0 =
            micros();


        if (
            !appTailRunToken(
                prompt_ids[pos],
                pos,
                nullptr)
        ) {

            Serial.printf(
                "[CHAT] prompt token FAIL|"
                "pos=%u|"
                "id=%u\n",

                static_cast<unsigned>(
                    pos
                ),

                static_cast<unsigned>(
                    prompt_ids[pos]
                )
            );

            return false;
        }


        total_layers_us +=
            micros() - token_t0;


        yield();
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !appTailRunToken(
                    next,
                    next_context_pos,
                    nullptr)
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}

void appTailProdProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        appTailProdGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}

}  

void v5_parent_v4_setup_unused() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO APP-TAIL CACHE V4 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_SCRATCH2: FAIL"
        );

        return;
    }


     
     
     

    cached_o11_buffer =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!cached_o11_buffer) {

        prod_runtime_ready =
            false;

        Serial.printf(
            "V3_O11_ALLOC: FAIL|"
            "free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    const std::uint32_t cache_t0 =
        millis();


    if (
        !resolveMatrix(
            11u,
            xp::MatrixFamily::OProj,
            cached_o11_buffer,
            SD_SCRATCH_BYTES,
            &cached_o11_matrix
        )
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LOAD: FAIL"
        );

        return;
    }


    const std::uint32_t cache_ms =
        millis() - cache_t0;


    if (
        cached_o11_matrix.location
            != xp::MatrixLocation::Sd
        ||
        cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
            > SD_SCRATCH_BYTES
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LAYOUT: FAIL"
        );

        return;
    }


    cached_o11_valid =
        true;


    Serial.printf(
        "V3_O11_CACHE: PASS|"
        "packed=%lu|ms=%lu|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
        ),

        static_cast<unsigned long>(
            cache_ms
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_WORKER: FAIL"
        );

        return;
    }


     
     
     

    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_WORKER: FAIL"
        );

        return;
    }


    delay(100);


    Serial.printf(
        "V3_WORKERS: PASS|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    if (
        !appTailProductionInit()
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V4_RUNTIME: FAIL"
        );

        return;
    }


    Serial.printf(
        "[V4_READY] "
        "context=%u "
        "max_generate=%u "
        "stop=END\n",

        static_cast<unsigned>(
            PROD_CONTEXT
        ),

        static_cast<unsigned>(
            PROD_MAX_GENERATE
        )
    );


    Serial.println(
        "Validation prompt: hello"
    );


    prodPrintPrompt();
}

void v5_parent_v4_loop_unused() {

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                appTailProdProcessLine(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}


 
 
 
 
 

namespace {

bool appTailL11DualRunLayer(
    std::size_t position
) {

    constexpr std::uint32_t layer =
        11u;


    if (
        !app_tail_ready
        ||
        !cached_o11_valid
        ||
        !dual_worker_task
        ||
        !dual_start
        ||
        !dual_done
    ) {

        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer
        * 2u
        * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative
        + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    if (
        !pipeFinishNextQ(
            layer
        )
    ) {

        return false;
    }


    const ResolvedMatrix q =
        pipe_next_q.resolved;


    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


     
     
     

    if (
        !pipeMatvec(
            app_tail_k11,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


     
     
     
     

    const std::size_t v_split =
        app_tail_v11.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        app_tail_v11;

    dual_job.input =
        normed_rt;

    dual_job.output =
        v_rt;

    dual_job.begin =
        v_split;

    dual_job.end =
        app_tail_v11.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool v_main_ok =
        dualMatvecRows(
            app_tail_v11,
            normed_rt,
            v_rt,
            0u,
            v_split
        );


    const bool v_worker_ok =
        dualWait();


    if (
        !v_main_ok
        ||
        !v_worker_ok
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


     
     
     
     

    const std::size_t head_split =
        cfg::kNHeads
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Attention;

    dual_job.layer =
        layer;

    dual_job.position =
        position;

    dual_job.begin =
        head_split;

    dual_job.end =
        cfg::kNHeads;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    dualAttentionHeads(
        layer,
        position,
        0u,
        head_split,
        hello_attn_scores
    );


    if (
        !dualWait()
    ) {

        return false;
    }


     
     
     
     

    const ResolvedMatrix o =
        cached_o11_matrix;

    const std::size_t o_split =
        o.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        o;

    dual_job.input =
        attention_rt;

    dual_job.output =
        o_rt;

    dual_job.begin =
        o_split;

    dual_job.end =
        o.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool o_main_ok =
        dualMatvecRows(
            o,
            attention_rt,
            o_rt,
            0u,
            o_split
        );


    const bool o_worker_ok =
        dualWait();


    if (
        !o_main_ok
        ||
        !o_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    ResolvedMatrix gate{};
    ResolvedMatrix up{};
    ResolvedMatrix down{};


    if (
        !resolveMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &gate
        )
        ||
        !resolveMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &up
        )
        ||
        !resolveMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &down
        )
    ) {

        return false;
    }


    if (
        gate.location
            != xp::MatrixLocation::Flash
        ||
        up.location
            != xp::MatrixLocation::Flash
        ||
        down.location
            != xp::MatrixLocation::Flash
    ) {

        return false;
    }


     
     
     

    dualDrainDone();


    dual_job.kind =
        DualJobKind::Full;

    dual_job.matrix =
        up;

    dual_job.input =
        normed_rt;

    dual_job.output =
        up_rt;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool gate_ok =
        pipeMatvec(
            gate,
            normed_rt,
            gate_rt
        );


    const bool up_ok =
        dualWait();


    if (
        !gate_ok
        ||
        !up_ok
    ) {

        return false;
    }


     
     
     

    const std::size_t swiglu_split =
        cfg::kDff
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::SwiGlu;

    dual_job.begin =
        swiglu_split;

    dual_job.end =
        cfg::kDff;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    for (
        std::size_t i = 0;
        i < swiglu_split;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !dualWait()
    ) {

        return false;
    }


     
     
     

    const std::size_t down_split =
        down.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        down;

    dual_job.input =
        gate_rt;

    dual_job.output =
        down_rt;

    dual_job.begin =
        down_split;

    dual_job.end =
        down.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool down_main_ok =
        dualMatvecRows(
            down,
            gate_rt,
            down_rt,
            0u,
            down_split
        );


    const bool down_worker_ok =
        dualWait();


    if (
        !down_main_ok
        ||
        !down_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    pipe_next_q.valid =
        false;


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

bool appTailL11DualRunToken(
    uint32_t token,
    size_t position,
    uint32_t* elapsed_ms
) {

    if (
        position >= HELLO_MAX_CONTEXT
    ) {

        return false;
    }


    if (
        !loadEmbeddingRow(
            token,
            hidden_rt
        )
    ) {

        return false;
    }


    pipe_next_q =
        PrefetchedMatrix{};


    const uint32_t t0 =
        millis();


    if (
        !pipeStartNextQ(
            5u,
            g_sd_scratch
        )
    ) {

        return false;
    }


     
    if (
        !helloRunLayerPosition(
            0u,
            position,
            false
        )
    ) {

        return false;
    }


     
    if (
        xSemaphoreTake(
            pipe_done_sem,
            portMAX_DELAY
        )
        != pdTRUE
    ) {

        return false;
    }


    __sync_synchronize();


    xSemaphoreGive(
        pipe_done_sem
    );


     
    for (
        uint32_t layer = 1u;
        layer < 5u;
        ++layer
    ) {

        if (
            !earlyDualRunPsramLayer(
                layer,
                position
            )
        ) {

            return false;
        }
    }


     
    if (
        !pipeRunLayer5(
            position
        )
    ) {

        return false;
    }


     
     
     
     
    for (
        uint32_t layer = 6u;
        layer <= 10u;
        ++layer
    ) {

        if (
            !appTailRunSdLayer(
                layer,
                position
            )
        ) {

            return false;
        }
    }


     
    if (
        !appTailL11DualRunLayer(
            position
        )
    ) {

        return false;
    }


    const uint32_t ms =
        millis() - t0;


    if (elapsed_ms) {
        *elapsed_ms = ms;
    }


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}

#include "final_layer_attention.inc"
#include "batched_inference_runtime.inc"
#include "two_token_feed_forward.inc"
#include "batched_transformer_block.inc"
#include "prompt_prefill.inc"

bool appTailL11DualProdGenerateReply(
    const char* user_text,
    char* output,
    std::size_t output_cap
) {

    if (
        !user_text
        ||
        !output
        ||
        output_cap == 0
    ) {

        return false;
    }


    output[0] =
        '\0';


    char canonical[
        256
    ] = {};


    if (
        !final_canonicalize_input(
            user_text,
            canonical,
            sizeof(canonical)
        )
    ) {

        Serial.println(
            "[CHAT] canonicalization FAIL"
        );

        return false;
    }


    Serial.printf(
        "[CHAT] canonical: [%s]\n",
        canonical
    );


    std::uint32_t question_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t prompt_ids[
        PROD_CONTEXT
    ] = {};

    std::uint32_t generated[
        PROD_MAX_GENERATE
    ] = {};


    std::size_t question_count =
        0;

    std::size_t prompt_count =
        0;

    std::size_t generated_count =
        0;


    const std::uint32_t encode_t0 =
        micros();


    const bool encode_ok =
        pocketlm::tokenizer_runtime_encode(
            &hello_tokenizer,
            canonical,
            std::strlen(
                canonical
            ),
            question_ids,
            PROD_CONTEXT - 2,
            &question_count
        );


    const std::uint32_t encode_us =
        micros() - encode_t0;


    if (
        !encode_ok
        ||
        question_count == 0
    ) {

        Serial.printf(
            "[CHAT] encode FAIL|"
            "tokens=%u|"
            "us=%u\n",

            static_cast<unsigned>(
                question_count
            ),

            static_cast<unsigned>(
                encode_us
            )
        );

        return false;
    }


    prompt_ids[
        prompt_count++
    ] = PROD_USER_ID;


    for (
        std::size_t i = 0;
        i < question_count;
        ++i
    ) {

        prompt_ids[
            prompt_count++
        ] = question_ids[i];
    }


    prompt_ids[
        prompt_count++
    ] = PROD_ASSISTANT_ID;


    Serial.printf(
        "[CHAT] native prompt|"
        "question_tokens=%u|"
        "total_tokens=%u|"
        "encode_us=%u\n",

        static_cast<unsigned>(
            question_count
        ),

        static_cast<unsigned>(
            prompt_count
        ),

        static_cast<unsigned>(
            encode_us
        )
    );


    Serial.print(
        "[CHAT] ids="
    );


    for (
        std::size_t i = 0;
        i < prompt_count;
        ++i
    ) {

        Serial.printf(
            "%s%u",
            i == 0
                ? ""
                : ",",
            static_cast<unsigned>(
                prompt_ids[i]
            )
        );
    }


    Serial.println();


     
     
    if (
        prompt_count
            + PROD_MAX_GENERATE
        >= PROD_CONTEXT
    ) {

        Serial.printf(
            "[CHAT] prompt too long|"
            "tokens=%u|"
            "max=%u\n",

            static_cast<unsigned>(
                prompt_count
            ),

            static_cast<unsigned>(
                PROD_CONTEXT
            )
        );

        return false;
    }


    prodResetTurn();


    std::uint32_t total_layers_us =
        0;

    std::uint32_t total_lm_us =
        0;


    const std::uint32_t run_t0 =
        micros();


     
     
     

    if(!bp_prefill(prompt_ids,prompt_count,total_layers_us)) {
        Serial.println("[CHAT] batched prefill FAIL");
        prod_runtime_ready = false;  
        return false;
    }


     
     
     

    const std::uint32_t gen_t0 =
        micros();


    std::size_t next_context_pos =
        prompt_count;


    bool stopped =
        false;


    for (
        std::size_t step = 0;
        step < PROD_MAX_GENERATE;
        ++step
    ) {

        std::uint32_t next =
            0;

        float next_score =
            0.0f;

        std::uint32_t lm_us =
            0;


        if (
            !prodPredictNext(
                &next,
                &next_score,
                &lm_us
            )
        ) {

            Serial.printf(
                "[CHAT] predict FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            prod_runtime_ready = false;  
            return false;
        }


        total_lm_us +=
            lm_us;


        Serial.printf(
            "[GEN] step=%u|"
            "token=%u|"
            "score=%.9f\n",

            static_cast<unsigned>(
                step
            ),

            static_cast<unsigned>(
                next
            ),

            static_cast<double>(
                next_score
            )
        );


         
        if (
            next
            == PROD_END_ID
        ) {

            stopped =
                true;

            Serial.printf(
                "[CHAT] END|"
                "step=%u|"
                "generated=%u\n",

                static_cast<unsigned>(
                    step
                ),

                static_cast<unsigned>(
                    generated_count
                )
            );

            break;
        }


        generated[
            generated_count++
        ] = next;


        if (
            !prodDecode(
                generated,
                generated_count,
                output,
                output_cap
            )
        ) {

            Serial.printf(
                "[CHAT] decode FAIL|"
                "step=%u\n",

                static_cast<unsigned>(
                    step
                )
            );

            prod_runtime_ready = false;  
            return false;
        }


        if (
            PROD_STOP_AT_FIRST_SENTENCE
            &&
            prodHasSentenceEnd(
                output
            )
        ) {

            stopped =
                true;

            break;
        }


         
        if (
            step + 1
            < PROD_MAX_GENERATE
        ) {

            const std::uint32_t token_t0 =
                micros();


            if (
                !appTailL11DualRunToken(
                    next,
                    next_context_pos,
                    nullptr)
            ) {

                Serial.printf(
                    "[CHAT] generated feed FAIL|"
                    "pos=%u|"
                    "id=%u\n",

                    static_cast<unsigned>(
                        next_context_pos
                    ),

                    static_cast<unsigned>(
                        next
                    )
                );

                prod_runtime_ready = false;  
                return false;
            }


            total_layers_us +=
                micros() - token_t0;


            ++next_context_pos;
        }
    }


    trim_ascii_in_place(
        output
    );


    const std::uint32_t run_us =
        micros() - run_t0;

    const std::uint32_t generation_us =
        micros() - gen_t0;


    const float gen_tps =
        generation_us > 0
        ? static_cast<float>(
            generated_count
        )
          /
          (
              static_cast<float>(
                  generation_us
              )
              / 1000000.0f
          )
        : 0.0f;


    Serial.printf(
        "[STATS] tokens=%u|"
        "stopped=%s|"
        "total_ms=%.1f|"
        "generation_ms=%.1f|"
        "gen_tps=%.4f|"
        "layers_ms=%.1f|"
        "lm_head_ms=%.1f\n",

        static_cast<unsigned>(
            generated_count
        ),

        stopped
            ? "yes"
            : "no",

        static_cast<double>(
            run_us
        ) / 1000.0,

        static_cast<double>(
            generation_us
        ) / 1000.0,

        static_cast<double>(
            gen_tps
        ),

        static_cast<double>(
            total_layers_us
        ) / 1000.0,

        static_cast<double>(
            total_lm_us
        ) / 1000.0
    );


    return
        generated_count > 0
        &&
        output[0] != '\0';
}

void appTailL11DualProdProcessLine(
    const char* line
) {

    if (
        !line
        ||
        line[0] == '\0'
    ) {

        return;
    }


    if (
        !prod_runtime_ready
    ) {

        Serial.println(
            "[CHAT] runtime is not ready"
        );

        prodPrintPrompt();

        return;
    }


    Serial.println();

    Serial.printf(
        "You: %s\n",
        line
    );

    Serial.println(
        "PocketGPT: thinking..."
    );


    char reply[
        PROD_DECODE_BYTES
    ] = {};


    const bool ok =
        appTailL11DualProdGenerateReply(
            line,
            reply,
            sizeof(reply)
        );


    if (!ok) {

        Serial.println(
            "PocketGPT: [generation failed]"
        );

    } else {

        Serial.printf(
            "PocketGPT: %s\n",
            reply
        );
    }


    prodPrintPrompt();
}

}  

 
 
 
 
 
 
 
 
 
 
 
 
namespace lcdchat {
bool numericKeyboard = false;

constexpr int SDA_PIN = D4;
constexpr int SCL_PIN = D5;
constexpr int TOUCH_INT_PIN = D7;
constexpr int BACKLIGHT_PIN = D6;
constexpr std::uint8_t TOUCH_ADDR = 0x2E;

constexpr std::uint16_t BG = TFT_BLACK;
constexpr std::uint16_t FG = TFT_WHITE;
constexpr std::uint16_t DIM = 0x8410;
constexpr std::uint16_t USER = 0xFD9F;    
constexpr std::uint16_t AI = 0x7DFF;      
constexpr std::uint16_t THINK = 0xB7FF;   
constexpr std::uint16_t ACCENT = 0xA81F;  
constexpr std::uint16_t ERR = TFT_RED;

char input[PROD_INPUT_BYTES] = {};
std::size_t input_len = 0;
char last_user[PROD_INPUT_BYTES] = {};
char last_reply[PROD_DECODE_BYTES] = {};
bool thinking = false;
bool last_ok = true;
bool touch_down = false;
std::uint32_t last_touch_ms = 0;

 
inline void i2cDelay() { delayMicroseconds(2); }
inline void sdaLow() { pinMode(SDA_PIN, OUTPUT); digitalWrite(SDA_PIN, LOW); }
inline void sclLow() { pinMode(SCL_PIN, OUTPUT); digitalWrite(SCL_PIN, LOW); }
inline void sdaRelease() { pinMode(SDA_PIN, INPUT_PULLUP); }
inline void sclRelease() { pinMode(SCL_PIN, INPUT_PULLUP); }

void i2cStart() {
    sdaRelease(); sclRelease(); i2cDelay();
    sdaLow(); i2cDelay();
    sclLow();
}

void i2cStop() {
    sdaLow(); i2cDelay();
    sclRelease(); i2cDelay();
    sdaRelease(); i2cDelay();
}

bool i2cWriteByte(std::uint8_t v) {
    for (int i = 7; i >= 0; --i) {
        if (v & (1u << i)) sdaRelease(); else sdaLow();
        i2cDelay();
        sclRelease(); i2cDelay();
        sclLow();
    }
    sdaRelease(); i2cDelay();
    sclRelease(); i2cDelay();
    const bool ack = digitalRead(SDA_PIN) == LOW;
    sclLow();
    return ack;
}

std::uint8_t i2cReadByte(bool ack) {
    std::uint8_t v = 0;
    sdaRelease();
    for (int i = 7; i >= 0; --i) {
        sclRelease(); i2cDelay();
        if (digitalRead(SDA_PIN)) v |= (1u << i);
        sclLow(); i2cDelay();
    }
    if (ack) sdaLow(); else sdaRelease();
    sclRelease(); i2cDelay();
    sclLow(); i2cDelay();
    sdaRelease();
    return v;
}

bool readTouch(std::uint8_t* x, std::uint8_t* y) {
    if (!x || !y || digitalRead(TOUCH_INT_PIN) != LOW) return false;

    std::uint8_t d[5] = {};
    i2cStart();
    if (!i2cWriteByte((TOUCH_ADDR << 1) | 1u)) {
        i2cStop();
        return false;
    }
    for (int i = 0; i < 5; ++i) d[i] = i2cReadByte(i != 4);
    i2cStop();

    if (d[0] != 0x01) return false;
    *x = d[2];
    *y = d[4];
    return true;
}

 
void textAt(int x, int y, const char* s, std::uint16_t c = FG) {
    tft.setTextColor(c, BG);
    tft.setCursor(x, y);
    tft.print(s);
}

int wrapped(const char* prefix, const char* s, int y, int max_lines, std::uint16_t c) {
    constexpr int N = 25;
    char line[N + 1];
    int n = 0, lines = 0;

    if (prefix) {
        while (*prefix && n < N) line[n++] = *prefix++;
    }

    while (s && *s && lines < max_lines) {
        char ch = *s++;
        if (ch == '\r') continue;

        if (ch == '\n') {
            if (n) {
                line[n] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines; n = 0;
            }
            continue;
        }

        if (n == N) {
             
            int cut = -1;
            for (int i = N - 1; i > 0; --i) {
                if (line[i] == ' ') { cut = i; break; }
            }

            if (cut > 0) {
                line[cut] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines;

                int j = 0;
                for (int i = cut + 1; i < n; ++i) line[j++] = line[i];
                n = j;
            } else {
                line[n] = 0;
                tft.fillRect(44, y, 152, 9, BG);
                textAt(44, y, line, c);
                y += 9; ++lines; n = 0;
            }

            if (lines >= max_lines) break;
        }

         
        if (n == 0 && ch == ' ') continue;
        line[n++] = ch;
    }

    if (n && lines < max_lines) {
        line[n] = 0;
        tft.fillRect(44, y, 152, 9, BG);
        textAt(44, y, line, c);
        y += 9;
    }
    return y;
}

void drawConversation() {
     
    tft.fillRect(42, 18, 156, 78, BG);
    int y = 30;

    if (last_user[0]) {
        y = wrapped("You: ", last_user, y, 2, USER);
        y += 3;
    }

    if (thinking) wrapped("AI: ", "thinking...", y, 4, THINK);
    else if (!last_ok) wrapped("AI: ", "[generation failed]", y, 4, ERR);
    else if (last_reply[0]) wrapped("AI: ", last_reply, y, 5, AI);
    else textAt(84, 55, "say hi :)", DIM);
}

void drawInput() {
     
    tft.fillRect(38, 96, 164, 25, 0x2104);
    tft.fillRect(44, 121, 152, 1, ACCENT);
    const char* shown = input;
    constexpr std::size_t V = 24;
    if (input_len > V) shown += input_len - V;
    textAt(47, 105, input_len ? shown : "type a message...", input_len ? FG : DIM);
}

void drawKeyboard() {
     
     
    tft.fillRect(18, 127, 204, 99, BG);
    textAt(36, 132, numericKeyboard ? "1  2  3  4  5  6  7  8  9  0" : "q  w  e  r  t  y  u  i  o  p", FG);
    textAt(45, 154, numericKeyboard ? "+  -  *  /  =  (  )  ?  !" : "a  s  d  f  g  h  j  k  l", FG);
    textAt(55, 176, numericKeyboard ? ":  ;  ,  .  %  &  @   <" : "z  x  c  v  b  n  m   <", FG);

    tft.fillRect(34, 197, 172, 1, ACCENT);
    textAt(43, 204, numericKeyboard ? "ABC  ?     SPACE     GO" : "123  ?     SPACE     GO", THINK);
}

void drawAll() {
    tft.fillRect(0, 0, 240, 240, BG);
    tft.setTextSize(1);

     
    drawConversation();
    drawInput();
    drawKeyboard();
}

void clearInput() {
    input_len = 0;
    input[0] = 0;
    drawInput();
}

void appendChar(char c) {
    if (input_len + 1 >= sizeof(input)) return;
    input[input_len++] = c;
    input[input_len] = 0;
    drawInput();
}

void backspace() {
    if (!input_len) return;
    input[--input_len] = 0;
    drawInput();
}

void submit(const char* line) {
    if (!line || !line[0]) return;

    std::strncpy(last_user, line, sizeof(last_user) - 1);
    last_user[sizeof(last_user) - 1] = 0;

    if (!prod_runtime_ready) {
        last_reply[0] = 0;
        last_ok = false;
        thinking = false;
        drawConversation();
        return;
    }

    last_reply[0] = 0;
    last_ok = true;
    thinking = true;
    drawConversation();
    clearInput();

    Serial.println();
    Serial.printf("You: %s\n", line);
    Serial.println("PocketGPT: thinking...");

    char reply[PROD_DECODE_BYTES] = {};

     
    const bool ok = appTailL11DualProdGenerateReply(line, reply, sizeof(reply));

    thinking = false;
    last_ok = ok;

    if (ok) {
        std::strncpy(last_reply, reply, sizeof(last_reply) - 1);
        last_reply[sizeof(last_reply) - 1] = 0;
        Serial.printf("PocketGPT: %s\n", last_reply);
    } else {
        last_reply[0] = 0;
        Serial.println("PocketGPT: [generation failed]");
    }

    if (!pipe_job.active) {
        drawConversation();
        drawInput();
        drawKeyboard();
    }
    prodPrintPrompt();
}

void handleKey(int x, int y) {
    if (thinking) return;

    if (y >= 126 && y < 149) {
        if (x >= 20 && x < 220) {
            int i = (x - 20) / 20;
            if (i < 10) appendChar((numericKeyboard ? "1234567890" : "qwertyuiop")[i]);
        }
        return;
    }

    if (y >= 149 && y < 171) {
        if (x >= 30 && x < 210) {
            int i = (x - 30) / 20;
            if (i < 9) appendChar((numericKeyboard ? "+-*/=()?!" : "asdfghjkl")[i]);
        }
        return;
    }

    if (y >= 171 && y < 194) {
        if (x >= 176 && x < 220) backspace();
        else if (x >= 42 && x < 182) {
            int i = (x - 42) / 20;
            if (i < 7) appendChar((numericKeyboard ? ":;,.%&@" : "zxcvbnm")[i]);
        }
        return;
    }

     
     
    if (y >= 194 && y < 229) {
        if (x < 30 || x >= 215) return;
        if (x < 75) { numericKeyboard = !numericKeyboard; drawKeyboard(); }
        else if (x < 112) appendChar('?');
        else if (x < 166) appendChar(' ');
        else if (x < 215 && input_len) {
            char line[PROD_INPUT_BYTES] = {};
            std::strncpy(line, input, sizeof(line) - 1);
            submit(line);
        }
    }
}

void init() {
    pinMode(BACKLIGHT_PIN, OUTPUT);
    digitalWrite(BACKLIGHT_PIN, HIGH);
    pinMode(TOUCH_INT_PIN, INPUT_PULLUP);
    sdaRelease();
    sclRelease();

     
    tft.setRotation(0);
    tft.setTextWrap(false);
    drawAll();
    Serial.println("[LCD CHAT] soft round UI ready");
}

void updateTouch() {
    std::uint8_t x = 0, y = 0;
    const bool down = readTouch(&x, &y);

    if (down && !touch_down && millis() - last_touch_ms >= 120) {
        last_touch_ms = millis();
        handleKey((int)x, (int)y);
    }
    touch_down = down;
}

}  


void setup() {

     
    locked_production_setup_unused();


    Serial.println();

    Serial.println(
        "=== PocketGPT XIAO APP-TAIL + L11 DUAL V5 ==="
    );


    if (
        !prod_runtime_ready
        ||
        !g_model_base
        ||
        !g_transformer_cache
        ||
        !g_sd_scratch
        ||
        !g_sd_residual
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    pipe_scratch2 =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!pipe_scratch2) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_SCRATCH2: FAIL"
        );

        return;
    }


     
     
     

    cached_o11_buffer =
        static_cast<uint8_t*>(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!cached_o11_buffer) {

        prod_runtime_ready =
            false;

        Serial.printf(
            "V3_O11_ALLOC: FAIL|"
            "free=%lu|largest=%lu\n",

            static_cast<unsigned long>(
                ESP.getFreePsram()
            ),

            static_cast<unsigned long>(
                heap_caps_get_largest_free_block(
                    MALLOC_CAP_SPIRAM
                )
            )
        );

        return;
    }


    const std::uint32_t cache_t0 =
        millis();


    if (
        !resolveMatrix(
            11u,
            xp::MatrixFamily::OProj,
            cached_o11_buffer,
            SD_SCRATCH_BYTES,
            &cached_o11_matrix
        )
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LOAD: FAIL"
        );

        return;
    }


    const std::uint32_t cache_ms =
        millis() - cache_t0;


    if (
        cached_o11_matrix.location
            != xp::MatrixLocation::Sd
        ||
        cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
            > SD_SCRATCH_BYTES
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_O11_LAYOUT: FAIL"
        );

        return;
    }


    cached_o11_valid =
        true;


    Serial.printf(
        "V3_O11_CACHE: PASS|"
        "packed=%lu|ms=%lu|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            cached_o11_matrix.q_bytes
            + cached_o11_matrix.scales_bytes
        ),

        static_cast<unsigned long>(
            cache_ms
        ),

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    pipe_start_sem =
        xSemaphoreCreateBinary();

    pipe_done_sem =
        xSemaphoreCreateBinary();


    if (
        !pipe_start_sem
        ||
        !pipe_done_sem
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t pipe_rc =
        xTaskCreatePinnedToCore(
            pipeWorkerMain,
            "sd_pipe_worker",
            4096,
            nullptr,
            2,
            &pipe_worker_task,
            0
        );


    if (
        pipe_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_PIPE_WORKER: FAIL"
        );

        return;
    }


     
     
     

    dual_start =
        xSemaphoreCreateBinary();

    dual_done =
        xSemaphoreCreateBinary();


    if (
        !dual_start
        ||
        !dual_done
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_SEMAPHORES: FAIL"
        );

        return;
    }


    const BaseType_t dual_rc =
        xTaskCreatePinnedToCore(
            dualWorkerMain,
            "dual_compute_worker",
            4096,
            nullptr,
            2,
            &dual_worker_task,
            0
        );


    if (
        dual_rc != pdPASS
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V3_DUAL_WORKER: FAIL"
        );

        return;
    }


    delay(100);


    Serial.printf(
        "V3_WORKERS: PASS|"
        "free_psram=%lu|largest=%lu\n",

        static_cast<unsigned long>(
            ESP.getFreePsram()
        ),

        static_cast<unsigned long>(
            heap_caps_get_largest_free_block(
                MALLOC_CAP_SPIRAM
            )
        )
    );


     
     
     

    if (
        !appTailProductionInit()
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V4_RUNTIME: FAIL"
        );

        return;
    }


     
     
     

    if (
        !dual_worker_task
        ||
        !dual_start
        ||
        !dual_done
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "V5_L11DUAL_RUNTIME: FAIL"
        );

        return;
    }


    Serial.println(
        "V5_L11DUAL_RUNTIME: PASS"
    );


     
     
     

    if (
        !initK10Precache()
    ) {

        prod_runtime_ready =
            false;

        Serial.println(
            "K10_PRECACHE: FAIL"
        );

        return;
    }


    Serial.println(
        "K10_PRECACHE: PASS"
    );


    Serial.printf(
        "[V5_READY] "
        "context=%u "
        "max_generate=%u "
        "stop=END\n",

        static_cast<unsigned>(
            PROD_CONTEXT
        ),

        static_cast<unsigned>(
            PROD_MAX_GENERATE
        )
    );


    Serial.println(
        "Validation prompt: hello"
    );


    prodPrintPrompt();

     
    lcdchat::init();
}

void loop() {

     
    lcdchat::updateTouch();

    while (
        Serial.available() > 0
    ) {

        const char c =
            static_cast<char>(
                Serial.read()
            );


        if (
            c == '\r'
        ) {

            continue;
        }


        if (
            c == '\n'
        ) {

            prod_input[
                prod_input_len
            ] = '\0';


            while (
                prod_input_len > 0
                &&
                (
                    prod_input[
                        prod_input_len - 1
                    ] == ' '
                    ||
                    prod_input[
                        prod_input_len - 1
                    ] == '\t'
                )
            ) {

                prod_input[
                    --prod_input_len
                ] = '\0';
            }


            char* start =
                prod_input;


            while (
                *start == ' '
                ||
                *start == '\t'
            ) {

                ++start;
            }


            if (
                start
                != prod_input
            ) {

                std::memmove(
                    prod_input,
                    start,
                    std::strlen(start)
                    + 1
                );


                prod_input_len =
                    std::strlen(
                        prod_input
                    );
            }


            if (
                prod_input_len > 0
            ) {

                lcdchat::submit(
                    prod_input
                );

            } else {

                prodPrintPrompt();
            }


            prod_input_len =
                0;

            prod_input[0] =
                '\0';

        } else if (
            c == 8
            ||
            c == 127
        ) {

            if (
                prod_input_len > 0
            ) {

                --prod_input_len;
            }


            prod_input[
                prod_input_len
            ] = '\0';

        } else if (
            prod_input_len + 1
            < sizeof(prod_input)
        ) {

            prod_input[
                prod_input_len++
            ] = c;


            prod_input[
                prod_input_len
            ] = '\0';

        } else {

            Serial.println();

            Serial.println(
                "[CHAT] input buffer full; "
                "press Enter to run or reset to clear."
            );
        }
    }


    delay(10);
}
