#include "tokenizer_runtime.h"

#include <Arduino.h>

#include <cstring>
#include <limits>

#include "esp_heap_caps.h"

namespace pocketlm {

namespace {

 
 
 
 
 
 
 
 
constexpr std::size_t
    kMaxNormalizedBytesPerInputByte = 33;

 
 
constexpr std::size_t
    kNormalizationExtraBytes = 4;

}   


bool tokenizer_runtime_init(
    TokenizerRuntime* tokenizer,
    File* normalizer_file,
    File* encoder_file,
    File* decoder_file
) {
    if (
        tokenizer == nullptr
        || normalizer_file == nullptr
        || encoder_file == nullptr
        || decoder_file == nullptr
    ) {
        return false;
    }

    std::memset(
        tokenizer,
        0,
        sizeof(*tokenizer)
    );

    if (
        !tokenizer_normalizer_init(
            &tokenizer->normalizer,
            normalizer_file
        )
    ) {
        tokenizer_runtime_free(
            tokenizer
        );

        return false;
    }

    if (
        !tokenizer_encoder_init(
            &tokenizer->encoder,
            encoder_file
        )
    ) {
        tokenizer_runtime_free(
            tokenizer
        );

        return false;
    }

    if (
        !tokenizer_decoder_init(
            &tokenizer->decoder,
            decoder_file
        )
    ) {
        tokenizer_runtime_free(
            tokenizer
        );

        return false;
    }

    tokenizer->initialized = true;

    return true;
}


bool tokenizer_runtime_encode(
    const TokenizerRuntime* tokenizer,
    const char* input,
    std::size_t input_bytes,
    std::uint32_t* token_ids,
    std::size_t token_capacity,
    std::size_t* token_count
) {
    if (token_count != nullptr) {
        *token_count = 0;
    }

    if (
        tokenizer == nullptr
        || !tokenizer->initialized
        || (
            input_bytes > 0
            && input == nullptr
        )
    ) {
        return false;
    }

     
     
    if (input_bytes == 0) {
        return true;
    }

    if (
        token_ids == nullptr
        || token_capacity == 0
    ) {
        return false;
    }

     
    if (
        input_bytes
        > (
            std::numeric_limits<
                std::size_t
            >::max()
            - kNormalizationExtraBytes
        )
        / kMaxNormalizedBytesPerInputByte
    ) {
        return false;
    }

    const std::size_t
        normalized_capacity =
            input_bytes
                * kMaxNormalizedBytesPerInputByte
            + kNormalizationExtraBytes;

    char* normalized =
        static_cast<char*>(
            heap_caps_malloc(
                normalized_capacity,
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (normalized == nullptr) {
        return false;
    }

    std::size_t normalized_bytes = 0;

    const bool normalize_ok =
        tokenizer_normalizer_normalize(
            &tokenizer->normalizer,
            input,
            input_bytes,
            normalized,
            normalized_capacity,
            &normalized_bytes
        );

    if (!normalize_ok) {
        heap_caps_free(
            normalized
        );

        return false;
    }

    const bool encode_ok =
        tokenizer_encoder_encode_normalized(
            &tokenizer->encoder,
            normalized,
            normalized_bytes,
            token_ids,
            token_capacity,
            token_count
        );

    heap_caps_free(
        normalized
    );

    if (!encode_ok) {
        if (token_count != nullptr) {
            *token_count = 0;
        }

        return false;
    }

    return true;
}


bool tokenizer_runtime_decode(
    const TokenizerRuntime* tokenizer,
    const std::uint32_t* token_ids,
    std::size_t token_count,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
) {
    if (
        tokenizer == nullptr
        || !tokenizer->initialized
    ) {
        return false;
    }

    return tokenizer_decoder_decode(
        &tokenizer->decoder,
        token_ids,
        token_count,
        output,
        output_capacity,
        output_bytes
    );
}


void tokenizer_runtime_free(
    TokenizerRuntime* tokenizer
) {
    if (tokenizer == nullptr) {
        return;
    }

    tokenizer_decoder_free(
        &tokenizer->decoder
    );

    tokenizer_encoder_free(
        &tokenizer->encoder
    );

    tokenizer_normalizer_free(
        &tokenizer->normalizer
    );

    tokenizer->initialized = false;
}

}   
