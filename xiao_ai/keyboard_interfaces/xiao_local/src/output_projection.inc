#include <Arduino.h>

#include <cstdint>
#include <cstring>
#include <cmath>

#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <esp_err.h>


namespace {

constexpr std::size_t VOCAB = 6144;
constexpr std::size_t D = 448;

constexpr std::size_t GROUP = 64;
constexpr std::size_t GROUPS_PER_ROW = D / GROUP;    
constexpr std::size_t BYTES_PER_ROW = D / 2;         

constexpr std::size_t Q4_BYTES = 1'376'256;
constexpr std::size_t SCALE_COUNT = 43'008;
constexpr std::size_t SCALE_F16_BYTES = 86'016;
constexpr std::size_t SCALE_F32_BYTES =
    SCALE_COUNT * sizeof(float);                     

constexpr std::size_t REPAIR_COUNT = 5;

constexpr std::size_t OFF_Q4 = 0x000000;
constexpr std::size_t OFF_SCALES_F16 = 0x150000;
constexpr std::size_t OFF_REPAIR_IDS = 0x165000;
constexpr std::size_t OFF_REPAIR_WEIGHTS = 0x165014;
constexpr std::size_t OFF_REPAIR_SCALES = 0x1658D4;

constexpr std::size_t TOTAL_BYTES = 0x1658E8;

alignas(16) float x[D];
alignas(16) float logits[VOCAB];


float fp16_to_float(std::uint16_t h) {
    const std::uint32_t sign =
        static_cast<std::uint32_t>(
            h & 0x8000u
        ) << 16;

    std::uint32_t exp =
        (h >> 10) & 0x1Fu;

    std::uint32_t mant =
        h & 0x03FFu;

    std::uint32_t bits;

    if (exp == 0) {
        if (mant == 0) {
            bits = sign;
        } else {
            int e = -14;

            while (
                (mant & 0x0400u) == 0
            ) {
                mant <<= 1;
                --e;
            }

            mant &= 0x03FFu;

            const std::uint32_t exp32 =
                static_cast<std::uint32_t>(
                    e + 127
                );

            bits =
                sign
                | (exp32 << 23)
                | (mant << 13);
        }
    } else if (exp == 31) {
        bits =
            sign
            | 0x7F800000u
            | (mant << 13);
    } else {
        bits =
            sign
            | ((exp + 112u) << 23)
            | (mant << 13);
    }

    float out;

    std::memcpy(
        &out,
        &bits,
        sizeof(out)
    );

    return out;
}


static inline float q4_lut_load_addx4(
    const float* base,
    std::uint32_t index
) {

#if defined(__XTENSA__)

    std::uintptr_t addr;

    const std::uintptr_t base_addr =
        reinterpret_cast<std::uintptr_t>(
            base
        );

    __asm__ __volatile__(
        "addx4 %0, %1, %2"
        : "=r"(addr)
        : "r"(index), "r"(base_addr)
    );

    return *reinterpret_cast<
        const float*
    >(addr);

#else

    return base[index];

#endif
}


__attribute__((noinline))
void q4_head_rowpair2(
    const std::uint8_t* packed_q,
    const std::uint16_t* scales_fp16,
    const float* input,
    float* output
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
        std::size_t row = 0;
        row < VOCAB;
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


static inline float int8_repair_score(
    const std::int8_t* row,
    float scale,
    const float* input
) {

    float acc0 = 0.0f;
    float acc1 = 0.0f;
    float acc2 = 0.0f;
    float acc3 = 0.0f;

    float acc4 = 0.0f;
    float acc5 = 0.0f;
    float acc6 = 0.0f;
    float acc7 = 0.0f;


    for (
        std::size_t i = 0;
        i < D;
        i += 8
    ) {

        acc0 +=
            static_cast<float>(
                row[i + 0]
            ) * input[i + 0];

        acc1 +=
            static_cast<float>(
                row[i + 1]
            ) * input[i + 1];

        acc2 +=
            static_cast<float>(
                row[i + 2]
            ) * input[i + 2];

        acc3 +=
            static_cast<float>(
                row[i + 3]
            ) * input[i + 3];

        acc4 +=
            static_cast<float>(
                row[i + 4]
            ) * input[i + 4];

        acc5 +=
            static_cast<float>(
                row[i + 5]
            ) * input[i + 5];

        acc6 +=
            static_cast<float>(
                row[i + 6]
            ) * input[i + 6];

        acc7 +=
            static_cast<float>(
                row[i + 7]
            ) * input[i + 7];
    }


    const float sum =
        ((acc0 + acc1)
        + (acc2 + acc3))
        + ((acc4 + acc5)
        + (acc6 + acc7));

    return sum * scale;
}


double checksum(
    const float* values,
    std::size_t count
) {
    double s = 0.0;

    for (
        std::size_t i = 0;
        i < count;
        ++i
    ) {
        s += static_cast<double>(
            values[i]
        );
    }

    return s;
}


}  


void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println(
        "=== XIAO MIXED Q4+INT8 "
        "LM HEAD FLASH BENCH ==="
    );

    Serial.printf(
        "q4_bytes=%u\n",
        static_cast<unsigned>(
            Q4_BYTES
        )
    );

    Serial.printf(
        "scale_count=%u "
        "scale_fp16_bytes=%u "
        "scale_fp32_cache_bytes=%u\n",
        static_cast<unsigned>(
            SCALE_COUNT
        ),
        static_cast<unsigned>(
            SCALE_F16_BYTES
        ),
        static_cast<unsigned>(
            SCALE_F32_BYTES
        )
    );

    Serial.printf(
        "repair_count=%u "
        "total_image_bytes=%u\n",
        static_cast<unsigned>(
            REPAIR_COUNT
        ),
        static_cast<unsigned>(
            TOTAL_BYTES
        )
    );

    Serial.printf(
        "psram_before=%lu\n",
        static_cast<unsigned long>(
            ESP.getFreePsram()
        )
    );


    const esp_partition_t* app1 =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_ANY,
            "app1"
        );

    if (!app1) {
        Serial.println(
            "[FAIL] app1 not found"
        );
        return;
    }

    Serial.printf(
        "app1 address=0x%lx "
        "size=%lu\n",
        static_cast<unsigned long>(
            app1->address
        ),
        static_cast<unsigned long>(
            app1->size
        )
    );


    const void* mapped_base = nullptr;

    esp_partition_mmap_handle_t handle = 0;

    const std::uint32_t map_t0 =
        micros();

    const esp_err_t err =
        esp_partition_mmap(
            app1,
            0,
            TOTAL_BYTES,
            ESP_PARTITION_MMAP_DATA,
            &mapped_base,
            &handle
        );

    const std::uint32_t map_us =
        micros() - map_t0;

    if (
        err != ESP_OK
        || !mapped_base
    ) {
        Serial.printf(
            "[FAIL] mmap: %s\n",
            esp_err_to_name(err)
        );
        return;
    }

    Serial.printf(
        "[PASS] mmap=%p "
        "map_ms=%.3f\n",
        mapped_base,
        static_cast<double>(
            map_us
        ) / 1000.0
    );


    const auto* base =
        static_cast<
            const std::uint8_t*
        >(mapped_base);

    const auto* q4 =
        base + OFF_Q4;

    const auto* scales_f16 =
        reinterpret_cast<
            const std::uint16_t*
        >(
            base + OFF_SCALES_F16
        );

    const auto* repair_ids =
        reinterpret_cast<
            const std::uint32_t*
        >(
            base + OFF_REPAIR_IDS
        );

    const auto* repair_weights =
        reinterpret_cast<
            const std::int8_t*
        >(
            base + OFF_REPAIR_WEIGHTS
        );

    const auto* repair_scales =
        reinterpret_cast<
            const float*
        >(
            base + OFF_REPAIR_SCALES
        );


    Serial.printf(
        "psram_after_mmap=%lu\n",
        static_cast<unsigned long>(
            ESP.getFreePsram()
        )
    );


    Serial.print(
        "repair_ids:"
    );

    for (
        std::size_t r = 0;
        r < REPAIR_COUNT;
        ++r
    ) {
        Serial.printf(
            " %u",
            static_cast<unsigned>(
                repair_ids[r]
            )
        );
    }

    Serial.println();


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
                "[FAIL] repair ID mismatch"
            );
            return;
        }
    }

    Serial.println(
        "[PASS] repair IDs"
    );


     
     
    for (
        std::size_t i = 0;
        i < D;
        ++i
    ) {
        x[i] =
            static_cast<float>(
                static_cast<int>(
                    i % 17
                ) - 8
            )
            * 0.001f;
    }


    Serial.println("warmup...");

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


    Serial.println();
    Serial.println(
        "=== TIMING ==="
    );


    for (
        int run = 1;
        run <= 5;
        ++run
    ) {

        const std::uint32_t total_t0 =
            micros();

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


        const std::uint32_t argmax_t0 =
            micros();

        std::uint32_t best_token = 0;
        float best_score = logits[0];

        for (
            std::uint32_t tok = 1;
            tok < VOCAB;
            ++tok
        ) {
            const float score =
                logits[tok];

            if (
                score > best_score
            ) {
                best_score = score;
                best_token = tok;
            }
        }

        const std::uint32_t argmax_us =
            micros() - argmax_t0;

        const std::uint32_t total_us =
            micros() - total_t0;


        Serial.printf(
            "run=%d "
            "q4_ms=%.3f "
            "repair_us=%u "
            "argmax_us=%u "
            "total_ms=%.3f "
            "best_token=%u "
            "best_score=%.9f\n",
            run,
            static_cast<double>(
                q4_us
            ) / 1000.0,
            static_cast<unsigned>(
                repair_us
            ),
            static_cast<unsigned>(
                argmax_us
            ),
            static_cast<double>(
                total_us
            ) / 1000.0,
            static_cast<unsigned>(
                best_token
            ),
            static_cast<double>(
                best_score
            )
        );
    }


    Serial.printf(
        "checksum=%.9f\n",
        checksum(
            logits,
            VOCAB
        )
    );

    Serial.println();
    Serial.println(
        "=== MIXED LM HEAD FP16-SCALE BENCH DONE ==="
    );
}


void loop() {
    delay(1000);
}
