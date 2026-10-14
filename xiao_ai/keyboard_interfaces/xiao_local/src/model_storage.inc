#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>

#include <cstdint>
#include <cstring>

#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>

#include "model_offsets.h"
#include "model_storage_layout.h"
#include "model_storage_checksums.h"


namespace cfg = pocketgpt_final_v7b50;
namespace xp  = pocketgpt::xiao_final;


namespace {

constexpr int SD_CS   = D2;
constexpr int SD_SCK  = D8;
constexpr int SD_MISO = D9;
constexpr int SD_MOSI = D10;

constexpr uint32_t SD_HZ =
    20000000;

constexpr std::size_t
    SD_SCRATCH_BYTES =
        106624;

constexpr std::size_t
    KV_MATRIX_BYTES =
        26656;


const char* PSRAM_PATH =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "transformer_psram_boot.bin";

const char* SD_RESIDUAL_PATH =
    "/pocketgpt_xiao_v7b50_final_v1/"
    "transformer_sd_residual.bin";


TFT_eSPI tft = TFT_eSPI();


const uint8_t* g_model_base =
    nullptr;

esp_partition_mmap_handle_t
    g_model_handle = 0;


uint8_t* g_transformer_cache =
    nullptr;

uint8_t* g_sd_scratch =
    nullptr;


File g_sd_residual;


 
 
 
 

const cfg::MatrixSpec& specFor(
    std::uint32_t layer,
    xp::MatrixFamily family
) {
    const auto& l =
        cfg::kLayers[layer];

    switch (family) {

        case xp::MatrixFamily::QProj:
            return l.q;

        case xp::MatrixFamily::KProj:
            return l.k;

        case xp::MatrixFamily::VProj:
            return l.v;

        case xp::MatrixFamily::OProj:
            return l.o;

        case xp::MatrixFamily::GateProj:
            return l.gate;

        case xp::MatrixFamily::UpProj:
            return l.up;

        case xp::MatrixFamily::DownProj:
            return l.down;
    }

    return l.q;
}


const char* familyName(
    xp::MatrixFamily family
) {
    switch (family) {

        case xp::MatrixFamily::QProj:
            return "q";

        case xp::MatrixFamily::KProj:
            return "k";

        case xp::MatrixFamily::VProj:
            return "v";

        case xp::MatrixFamily::OProj:
            return "o";

        case xp::MatrixFamily::GateProj:
            return "gate";

        case xp::MatrixFamily::UpProj:
            return "up";

        case xp::MatrixFamily::DownProj:
            return "down";
    }

    return "?";
}


const char* locationName(
    xp::MatrixLocation loc
) {
    switch (loc) {

        case xp::MatrixLocation::Psram:
            return "PSRAM";

        case xp::MatrixLocation::Flash:
            return "FLASH";

        case xp::MatrixLocation::Sd:
            return "SD";
    }

    return "?";
}


 
 
 

bool readFileExactAt(
    File& file,
    std::uint32_t offset,
    uint8_t* dst,
    std::size_t bytes
) {
    if (!dst) {
        return false;
    }

    if (!file.seek(
            offset,
            SeekSet
        )) {
        return false;
    }


    std::size_t done = 0;

    while (done < bytes) {

        std::size_t wanted =
            bytes - done;

        if (wanted > 65536) {
            wanted = 65536;
        }


        const int n =
            file.read(
                dst + done,
                wanted
            );


        if (
            n <= 0
            ||
            static_cast<std::size_t>(n)
                != wanted
        ) {
            return false;
        }


        done +=
            static_cast<std::size_t>(n);

        yield();
    }


    return done == bytes;
}


bool loadExactFile(
    const char* path,
    uint8_t* dst,
    std::size_t expected
) {
    File f =
        SD.open(
            path,
            FILE_READ
        );

    if (!f) {
        return false;
    }


    if (
        static_cast<std::size_t>(
            f.size()
        )
        != expected
    ) {
        f.close();
        return false;
    }


    const bool ok =
        readFileExactAt(
            f,
            0,
            dst,
            expected
        );


    f.close();

    return ok;
}


 
 
 

struct ResolvedMatrix {
    const cfg::MatrixSpec* spec;

    const uint8_t* q4;

    const uint16_t* scales;

    xp::MatrixLocation location;

    std::uint32_t q_bytes;

    std::uint32_t scales_bytes;

    std::uint32_t total_bytes;
};


bool resolveMatrix(
    std::uint32_t layer,
    xp::MatrixFamily family,

    uint8_t* sd_destination,
    std::size_t sd_capacity,

    ResolvedMatrix* out
) {
    if (
        !out
        ||
        layer >= cfg::kNumLayers
    ) {
        return false;
    }


    const auto& spec =
        specFor(
            layer,
            family
        );

    const auto& p =
        xp::matrixPlacement(
            layer,
            family
        );


     
    if (
        p.q_bytes != spec.q4_bytes
        ||
        p.scales_bytes
            != spec.scales_bytes
        ||
        p.total_bytes
            != p.q_bytes
                + p.scales_bytes
    ) {
        return false;
    }


    out->spec =
        &spec;

    out->location =
        p.location;

    out->q_bytes =
        p.q_bytes;

    out->scales_bytes =
        p.scales_bytes;

    out->total_bytes =
        p.total_bytes;


    switch (p.location) {

         
         
         

        case xp::MatrixLocation::Psram:
        {
            if (
                !g_transformer_cache
                ||
                p.q_offset
                    + p.q_bytes
                    > xp::kPsramTransformerBytes
                ||
                p.scales_offset
                    + p.scales_bytes
                    > xp::kPsramTransformerBytes
            ) {
                return false;
            }


            out->q4 =
                g_transformer_cache
                + p.q_offset;


            out->scales =
                reinterpret_cast<
                    const uint16_t*
                >(
                    g_transformer_cache
                    + p.scales_offset
                );


            return true;
        }


         
         
         

        case xp::MatrixLocation::Flash:
        {
            if (
                !g_model_base
                ||
                p.q_offset
                    < xp::kModelTransformerOffset
                ||
                p.q_offset
                    + p.q_bytes
                    > xp::kModelUsedEnd
                ||
                p.scales_offset
                    + p.scales_bytes
                    > xp::kModelUsedEnd
            ) {
                return false;
            }


            out->q4 =
                g_model_base
                + p.q_offset;


            out->scales =
                reinterpret_cast<
                    const uint16_t*
                >(
                    g_model_base
                    + p.scales_offset
                );


            return true;
        }


         
         
         
         
         
         
         
         

        case xp::MatrixLocation::Sd:
        {
            if (
                !sd_destination
                ||
                sd_capacity
                    < p.total_bytes
                ||
                p.q_offset
                    + p.total_bytes
                    > xp::kSdTransformerBytes
                ||
                p.scales_offset
                    != p.q_offset
                        + p.q_bytes
            ) {
                return false;
            }


            if (
                (
                    reinterpret_cast<
                        uintptr_t
                    >(sd_destination)
                    & 0x3u
                )
                != 0u
            ) {
                return false;
            }


            if (
                !readFileExactAt(
                    g_sd_residual,
                    p.q_offset,
                    sd_destination,
                    p.total_bytes
                )
            ) {
                return false;
            }


            out->q4 =
                sd_destination;


            out->scales =
                reinterpret_cast<
                    const uint16_t*
                >(
                    sd_destination
                    + p.q_bytes
                );


            return true;
        }
    }


    return false;
}


 
 
 

bool shaMemory(
    const void* ptr,
    std::size_t bytes,
    uint8_t digest[32]
) {
    if (!ptr) {
        return false;
    }


    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(
        &ctx
    );


    if (
        mbedtls_sha256_starts(
            &ctx,
            0
        )
        != 0
    ) {
        mbedtls_sha256_free(
            &ctx
        );

        return false;
    }


    const auto* p =
        static_cast<
            const uint8_t*
        >(ptr);


    std::size_t done = 0;

    while (done < bytes) {

        std::size_t n =
            bytes - done;

        if (n > 65536) {
            n = 65536;
        }


        if (
            mbedtls_sha256_update(
                &ctx,
                p + done,
                n
            )
            != 0
        ) {
            mbedtls_sha256_free(
                &ctx
            );

            return false;
        }


        done += n;

        yield();
    }


    const int rc =
        mbedtls_sha256_finish(
            &ctx,
            digest
        );


    mbedtls_sha256_free(
        &ctx
    );


    return rc == 0;
}


bool digestEqual(
    const uint8_t a[32],
    const uint8_t b[32]
) {
    return std::memcmp(
        a,
        b,
        32
    ) == 0;
}


bool validateResolvedHash(
    std::uint32_t layer,
    xp::MatrixFamily family,
    const ResolvedMatrix& r
) {
    const std::uint32_t fi =
        static_cast<
            std::uint32_t
        >(family);


    const auto& expected =
        xp::kExpectedMatrixDigests[
            layer
        ][fi];


    uint8_t q_digest[32];
    uint8_t s_digest[32];


    const bool q_ok =
        shaMemory(
            r.q4,
            r.q_bytes,
            q_digest
        )
        &&
        digestEqual(
            q_digest,
            expected.q_sha256
        );


    const bool s_ok =
        shaMemory(
            r.scales,
            r.scales_bytes,
            s_digest
        )
        &&
        digestEqual(
            s_digest,
            expected.scales_sha256
        );


    return q_ok && s_ok;
}


 
 
 

struct Representative {
    std::uint32_t layer;

    xp::MatrixFamily family;

    xp::MatrixLocation
        expected_location;

    std::uint32_t rows;

    std::uint32_t cols;

    const char* label;
};


constexpr Representative REPS[] = {

    {
        0,
        xp::MatrixFamily::KProj,
        xp::MatrixLocation::Psram,
        112,
        448,
        "PSRAM_112x448"
    },

    {
        0,
        xp::MatrixFamily::QProj,
        xp::MatrixLocation::Psram,
        448,
        448,
        "PSRAM_448x448"
    },

    {
        0,
        xp::MatrixFamily::DownProj,
        xp::MatrixLocation::Psram,
        448,
        1344,
        "PSRAM_448x1344"
    },

    {
        0,
        xp::MatrixFamily::GateProj,
        xp::MatrixLocation::Psram,
        1344,
        448,
        "PSRAM_1344x448"
    },


    {
        5,
        xp::MatrixFamily::KProj,
        xp::MatrixLocation::Flash,
        112,
        448,
        "FLASH_112x448"
    },

    {
        5,
        xp::MatrixFamily::DownProj,
        xp::MatrixLocation::Flash,
        448,
        1344,
        "FLASH_448x1344"
    },

    {
        6,
        xp::MatrixFamily::GateProj,
        xp::MatrixLocation::Flash,
        1344,
        448,
        "FLASH_1344x448"
    },


    {
        5,
        xp::MatrixFamily::VProj,
        xp::MatrixLocation::Sd,
        112,
        448,
        "SD_112x448"
    },

    {
        5,
        xp::MatrixFamily::QProj,
        xp::MatrixLocation::Sd,
        448,
        448,
        "SD_448x448"
    },
};


constexpr std::size_t REP_COUNT =
    sizeof(REPS)
    / sizeof(REPS[0]);


static_assert(
    REP_COUNT == 9,
    "expected nine source/shape combinations"
);


bool pointerSemanticsOk(
    const Representative& rep,
    const ResolvedMatrix& r,
    uint8_t* sd_destination
) {
    const auto& p =
        xp::matrixPlacement(
            rep.layer,
            rep.family
        );


    switch (rep.expected_location) {

        case xp::MatrixLocation::Psram:

            return
                r.q4
                    == g_transformer_cache
                        + p.q_offset
                &&
                reinterpret_cast<
                    const uint8_t*
                >(r.scales)
                    == g_transformer_cache
                        + p.scales_offset;


        case xp::MatrixLocation::Flash:

            return
                r.q4
                    == g_model_base
                        + p.q_offset
                &&
                reinterpret_cast<
                    const uint8_t*
                >(r.scales)
                    == g_model_base
                        + p.scales_offset;


        case xp::MatrixLocation::Sd:

            return
                r.q4
                    == sd_destination
                &&
                reinterpret_cast<
                    const uint8_t*
                >(r.scales)
                    == sd_destination
                        + p.q_bytes;
    }


    return false;
}

}  


void setup() {
    Serial.begin(115200);

    delay(2000);


    Serial.println();

    Serial.println(
        "=== XIAO FINAL GENERIC "
        "MATRIX RESOLVER PROBE ==="
    );


    bool all_ok = true;


     
     
     

    const esp_partition_t* model =
        esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_ANY,
            "model"
        );


    if (
        !model
        ||
        model->address
            != xp::kModelPhysicalStart
        ||
        model->size
            != xp::kModelPartitionBytes
    ) {
        Serial.println(
            "MODEL_PARTITION: FAIL"
        );

        return;
    }


    Serial.println(
        "MODEL_PARTITION: PASS"
    );


    const void* mapped =
        nullptr;


    const esp_err_t map_err =
        esp_partition_mmap(
            model,
            0,
            xp::kModelPartitionBytes,
            ESP_PARTITION_MMAP_DATA,
            &mapped,
            &g_model_handle
        );


    if (
        map_err != ESP_OK
        ||
        !mapped
    ) {
        Serial.println(
            "MODEL_MMAP: FAIL"
        );

        return;
    }


    g_model_base =
        static_cast<
            const uint8_t*
        >(mapped);


    Serial.println(
        "MODEL_MMAP: PASS"
    );


     
     
     

    tft.init();


    SPI.begin(
        SD_SCK,
        SD_MISO,
        SD_MOSI,
        SD_CS
    );


    if (
        !SD.begin(
            SD_CS,
            SPI,
            SD_HZ
        )
    ) {
        Serial.println(
            "SD_BEGIN: FAIL"
        );

        return;
    }


    Serial.println(
        "SD_BEGIN: PASS"
    );


    g_sd_residual =
        SD.open(
            SD_RESIDUAL_PATH,
            FILE_READ
        );


    if (
        !g_sd_residual
        ||
        static_cast<
            std::uint32_t
        >(
            g_sd_residual.size()
        )
            != xp::kSdTransformerBytes
    ) {
        Serial.println(
            "SD_RESIDUAL_OPEN: FAIL"
        );

        return;
    }


    Serial.println(
        "SD_RESIDUAL_OPEN: PASS"
    );


     
     
     

    g_transformer_cache =
        static_cast<
            uint8_t*
        >(
            heap_caps_malloc(
                xp::kPsramBudgetBytes,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!g_transformer_cache) {
        Serial.println(
            "TRANSFORMER_CACHE_ALLOC: FAIL"
        );

        return;
    }


    Serial.printf(
        "TRANSFORMER_CACHE_ALLOC: PASS|"
        "bytes=%lu\n",
        static_cast<unsigned long>(
            xp::kPsramBudgetBytes
        )
    );


    const uint32_t load_t0 =
        millis();


    if (
        !loadExactFile(
            PSRAM_PATH,
            g_transformer_cache,
            xp::kPsramTransformerBytes
        )
    ) {
        Serial.println(
            "PSRAM_BOOT_LOAD: FAIL"
        );

        return;
    }


    Serial.printf(
        "PSRAM_BOOT_LOAD: PASS|"
        "bytes=%lu|ms=%lu\n",
        static_cast<unsigned long>(
            xp::kPsramTransformerBytes
        ),
        static_cast<unsigned long>(
            millis() - load_t0
        )
    );


     
     
     

    g_sd_scratch =
        static_cast<
            uint8_t*
        >(
            heap_caps_malloc(
                SD_SCRATCH_BYTES,
                MALLOC_CAP_SPIRAM
                | MALLOC_CAP_8BIT
            )
        );


    if (!g_sd_scratch) {
        Serial.println(
            "SD_SCRATCH_ALLOC: FAIL"
        );

        return;
    }


    const bool scratch_aligned =
        (
            reinterpret_cast<
                uintptr_t
            >(g_sd_scratch)
            & 0x3u
        )
        == 0u;


    Serial.printf(
        "SD_SCRATCH_ALLOC: %s|"
        "bytes=%lu|aligned4=%s\n",
        scratch_aligned
            ? "PASS"
            : "FAIL",
        static_cast<unsigned long>(
            SD_SCRATCH_BYTES
        ),
        scratch_aligned
            ? "yes"
            : "NO"
    );


    if (!scratch_aligned) {
        return;
    }


     
     
     

    Serial.println();

    Serial.println(
        "=== NINE SOURCE/SHAPE "
        "COMBINATIONS ==="
    );


    std::uint32_t rep_passes = 0;


    for (
        std::size_t i = 0;
        i < REP_COUNT;
        ++i
    ) {

        const auto& rep =
            REPS[i];


        ResolvedMatrix r{};


        const bool resolve_ok =
            resolveMatrix(
                rep.layer,
                rep.family,
                g_sd_scratch,
                SD_SCRATCH_BYTES,
                &r
            );


        bool shape_ok = false;
        bool location_ok = false;
        bool pointer_ok = false;
        bool hash_ok = false;


        if (resolve_ok) {

            shape_ok =
                r.spec
                &&
                r.spec->rows
                    == rep.rows
                &&
                r.spec->cols
                    == rep.cols;


            location_ok =
                r.location
                    == rep.expected_location;


            pointer_ok =
                pointerSemanticsOk(
                    rep,
                    r,
                    g_sd_scratch
                );


            hash_ok =
                validateResolvedHash(
                    rep.layer,
                    rep.family,
                    r
                );
        }


        const bool pass =
            resolve_ok
            &&
            shape_ok
            &&
            location_ok
            &&
            pointer_ok
            &&
            hash_ok;


        if (pass) {
            ++rep_passes;
        } else {
            all_ok = false;
        }


        Serial.printf(
            "REP|%s|"
            "L%u.%s|"
            "shape=%ux%u|"
            "loc=%s|"
            "resolve=%s|"
            "ptr=%s|"
            "sha=%s|"
            "%s\n",

            rep.label,

            static_cast<unsigned>(
                rep.layer
            ),

            familyName(
                rep.family
            ),

            static_cast<unsigned>(
                rep.rows
            ),

            static_cast<unsigned>(
                rep.cols
            ),

            locationName(
                rep.expected_location
            ),

            resolve_ok
                ? "PASS"
                : "FAIL",

            pointer_ok
                ? "PASS"
                : "FAIL",

            hash_ok
                ? "PASS"
                : "FAIL",

            pass
                ? "PASS"
                : "FAIL"
        );
    }


    Serial.printf(
        "REP_PASSES|%lu/%lu\n",
        static_cast<unsigned long>(
            rep_passes
        ),
        static_cast<unsigned long>(
            REP_COUNT
        )
    );


     
     
     
     
     
     
     
     
     
     
     
     

    Serial.println();

    Serial.println(
        "=== SD KV CORESIDENCY ==="
    );


    ResolvedMatrix k{};
    ResolvedMatrix v{};


    uint8_t* k_dst =
        g_sd_scratch;


    uint8_t* v_dst =
        g_sd_scratch
        + KV_MATRIX_BYTES;


    const bool k_ok =
        resolveMatrix(
            6,
            xp::MatrixFamily::KProj,
            k_dst,
            KV_MATRIX_BYTES,
            &k
        );


    const bool v_ok =
        resolveMatrix(
            6,
            xp::MatrixFamily::VProj,
            v_dst,
            KV_MATRIX_BYTES,
            &v
        );


    const bool kv_shape_ok =
        k_ok
        &&
        v_ok
        &&
        k.spec->rows == 112
        &&
        k.spec->cols == 448
        &&
        v.spec->rows == 112
        &&
        v.spec->cols == 448
        &&
        k.total_bytes
            == KV_MATRIX_BYTES
        &&
        v.total_bytes
            == KV_MATRIX_BYTES;


     
     
    const bool k_hash_after_v =
        k_ok
        &&
        validateResolvedHash(
            6,
            xp::MatrixFamily::KProj,
            k
        );


    const bool v_hash =
        v_ok
        &&
        validateResolvedHash(
            6,
            xp::MatrixFamily::VProj,
            v
        );


    const bool no_overlap =
        (
            k_dst
            + KV_MATRIX_BYTES
        )
        <= v_dst;


    const bool within_scratch =
        (
            v_dst
            + KV_MATRIX_BYTES
        )
        <= (
            g_sd_scratch
            + SD_SCRATCH_BYTES
        );


    const bool kv_ok =
        kv_shape_ok
        &&
        k_hash_after_v
        &&
        v_hash
        &&
        no_overlap
        &&
        within_scratch;


    Serial.printf(
        "KV_PAIR|"
        "K_bytes=%lu|"
        "V_bytes=%lu|"
        "combined=%lu|"
        "K_after_V_sha=%s|"
        "V_sha=%s|"
        "%s\n",

        static_cast<unsigned long>(
            k.total_bytes
        ),

        static_cast<unsigned long>(
            v.total_bytes
        ),

        static_cast<unsigned long>(
            k.total_bytes
            + v.total_bytes
        ),

        k_hash_after_v
            ? "PASS"
            : "FAIL",

        v_hash
            ? "PASS"
            : "FAIL",

        kv_ok
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "SD_KV_CORESIDENT_53312: %s\n",
        (
            kv_ok
            &&
            k.total_bytes
                + v.total_bytes
                == 53312u
        )
            ? "PASS"
            : "FAIL"
    );


    if (!kv_ok) {
        all_ok = false;
    }


     
     
     

    const bool reps_exact =
        rep_passes == REP_COUNT;


    Serial.println();


    Serial.printf(
        "GENERIC_RESOLVER_9_COMBOS: %s\n",
        reps_exact
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "GENERIC_RESOLVER_KVDUAL_LAYOUT: %s\n",
        kv_ok
            ? "PASS"
            : "FAIL"
    );


    Serial.printf(
        "FINAL_MATRIX_RESOLVER_RUNTIME: %s\n",
        (
            all_ok
            &&
            reps_exact
        )
            ? "PASS"
            : "FAIL"
    );
}


void loop() {
    delay(1000);
}
