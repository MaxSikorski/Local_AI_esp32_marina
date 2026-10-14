#include "tokenizer_normalizer.h"

#include <Arduino.h>

#include <cstring>

#include "esp_heap_caps.h"

namespace pocketlm {

namespace {

constexpr std::uint8_t kMagic[4] = {
    'P', 'K', 'T', 'N'
};

constexpr std::uint16_t kVersion = 1;

constexpr std::size_t kHeaderBytes = 16;

constexpr std::uint8_t kSpaceMarker[3] = {
    0xE2, 0x96, 0x81
};


 
constexpr const char* kUserDefinedSymbols[] = {
    "<|end_memory|>",
    "<|assistant|>",
    "<|system|>",
    "<|memory|>",
    "<|user|>",
    "<|end|>",
};

constexpr std::size_t kUserDefinedSymbolCount =
    sizeof(kUserDefinedSymbols)
    / sizeof(kUserDefinedSymbols[0]);


std::uint16_t read_u16_le(
    const std::uint8_t* p
) {
    return static_cast<std::uint16_t>(
        p[0]
    )
        | (
            static_cast<std::uint16_t>(
                p[1]
            )
            << 8
        );
}


std::uint32_t read_u32_le(
    const std::uint8_t* p
) {
    return static_cast<std::uint32_t>(
        p[0]
    )
        | (
            static_cast<std::uint32_t>(
                p[1]
            )
            << 8
        )
        | (
            static_cast<std::uint32_t>(
                p[2]
            )
            << 16
        )
        | (
            static_cast<std::uint32_t>(
                p[3]
            )
            << 24
        );
}


bool read_exact(
    File& file,
    std::uint8_t* dst,
    std::size_t bytes
) {
    std::size_t total = 0;

    while (total < bytes) {
        const std::size_t remaining =
            bytes - total;

        const std::size_t chunk =
            remaining > 4096
                ? 4096
                : remaining;

        const std::size_t n =
            file.read(
                dst + total,
                chunk
            );

        if (n == 0) {
            break;
        }

        total += n;
    }

    return total == bytes;
}


 
 
 

bool unit_has_leaf(
    std::uint32_t unit
) {
    return (
        (unit >> 8)
        & 1U
    ) != 0;
}


std::uint32_t unit_label(
    std::uint32_t unit
) {
    return unit
        & (
            (1UL << 31)
            | 0xFFUL
        );
}


std::uint32_t unit_offset(
    std::uint32_t unit
) {
    const std::uint32_t shift =
        (
            unit
            & (1UL << 9)
        )
        >> 6;

    return (
        unit >> 10
    ) << shift;
}


std::uint32_t unit_value(
    std::uint32_t unit
) {
    return unit
        & 0x7FFFFFFFUL;
}


 
 
 

std::size_t utf8_sequence_length(
    std::uint8_t first
) {
    if (first < 0x80) {
        return 1;
    }

    if (
        first >= 0xC2
        && first <= 0xDF
    ) {
        return 2;
    }

    if (
        first >= 0xE0
        && first <= 0xEF
    ) {
        return 3;
    }

    if (
        first >= 0xF0
        && first <= 0xF4
    ) {
        return 4;
    }

    return 0;
}


bool valid_utf8_sequence(
    const std::uint8_t* data,
    std::size_t available,
    std::size_t length
) {
    if (
        data == nullptr
        || length == 0
        || available < length
    ) {
        return false;
    }

    if (length == 1) {
        return data[0] < 0x80;
    }

    for (
        std::size_t i = 1;
        i < length;
        ++i
    ) {
        if (
            (data[i] & 0xC0)
            != 0x80
        ) {
            return false;
        }
    }

     
     
    if (length == 3) {
        if (
            data[0] == 0xE0
            && data[1] < 0xA0
        ) {
            return false;
        }

        if (
            data[0] == 0xED
            && data[1] >= 0xA0
        ) {
            return false;
        }
    }

    if (length == 4) {
        if (
            data[0] == 0xF0
            && data[1] < 0x90
        ) {
            return false;
        }

        if (
            data[0] == 0xF4
            && data[1] > 0x8F
        ) {
            return false;
        }
    }

    return true;
}


 
 
 

bool append_bytes(
    char* output,
    std::size_t output_capacity,
    std::size_t* written,
    const std::uint8_t* data,
    std::size_t bytes
) {
    if (
        output == nullptr
        || written == nullptr
        || (
            bytes > 0
            && data == nullptr
        )
        || *written + bytes
            >= output_capacity
    ) {
        return false;
    }

    if (bytes > 0) {
        std::memcpy(
            output + *written,
            data,
            bytes
        );
    }

    *written += bytes;

    return true;
}


bool append_space_marker(
    char* output,
    std::size_t output_capacity,
    std::size_t* written
) {
    return append_bytes(
        output,
        output_capacity,
        written,
        kSpaceMarker,
        sizeof(kSpaceMarker)
    );
}


 
 
 

struct PrefixResult {
    const std::uint8_t* normalized;
    std::size_t normalized_bytes;

    std::size_t consumed_bytes;

    bool direct_input;
};


 
 
 

bool match_user_defined_symbol(
    const std::uint8_t* data,
    std::size_t available,
    PrefixResult* result
) {
    for (
        std::size_t i = 0;
        i < kUserDefinedSymbolCount;
        ++i
    ) {
        const char* symbol =
            kUserDefinedSymbols[i];

        const std::size_t bytes =
            std::strlen(symbol);

        if (
            available >= bytes
            && std::memcmp(
                data,
                symbol,
                bytes
            ) == 0
        ) {
            result->normalized = data;
            result->normalized_bytes =
                bytes;
            result->consumed_bytes =
                bytes;
            result->direct_input = true;

            return true;
        }
    }

    return false;
}


 
 
 

bool trie_longest_prefix(
    const TokenizerNormalizer*
        normalizer,
    const std::uint8_t* data,
    std::size_t available,
    PrefixResult* result
) {
    if (
        normalizer == nullptr
        || data == nullptr
        || available == 0
        || result == nullptr
        || normalizer->trie_units
            == nullptr
        || normalizer->trie_unit_count
            == 0
    ) {
        return false;
    }

    std::uint32_t node = 0;

    std::uint32_t unit =
        normalizer->trie_units[node];

    node ^= unit_offset(unit);

    bool found = false;

    std::size_t best_length = 0;
    std::uint32_t best_value = 0;

    for (
        std::size_t i = 0;
        i < available;
        ++i
    ) {
        node ^= data[i];

        if (
            node
            >= normalizer->
                trie_unit_count
        ) {
            break;
        }

        unit =
            normalizer->trie_units[node];

        if (
            unit_label(unit)
            != data[i]
        ) {
            break;
        }

        node ^= unit_offset(unit);

        if (
            node
            >= normalizer->
                trie_unit_count
        ) {
            return false;
        }

        if (unit_has_leaf(unit)) {
            best_length = i + 1;

            best_value =
                unit_value(
                    normalizer->
                        trie_units[node]
                );

            found = true;
        }
    }

    if (!found) {
        return false;
    }

    if (
        best_value
        >= normalizer->
            normalized_bytes
    ) {
        return false;
    }

    const std::uint8_t* start =
        normalizer->normalized_blob
        + best_value;

    std::size_t length = 0;

    while (
        best_value + length
            < normalizer->
                normalized_bytes
        && start[length] != 0
    ) {
        ++length;
    }

    if (
        best_value + length
        >= normalizer->
            normalized_bytes
    ) {
        return false;
    }

    result->normalized = start;
    result->normalized_bytes =
        length;
    result->consumed_bytes =
        best_length;
    result->direct_input = false;

    return true;
}


 
 
 
 
 
 
 
 
 

bool normalize_prefix(
    const TokenizerNormalizer*
        normalizer,
    const std::uint8_t* data,
    std::size_t available,
    PrefixResult* result
) {
    if (
        data == nullptr
        || available == 0
        || result == nullptr
    ) {
        return false;
    }

    if (
        match_user_defined_symbol(
            data,
            available,
            result
        )
    ) {
        return true;
    }

    PrefixResult trie_result = {};

    if (
        trie_longest_prefix(
            normalizer,
            data,
            available,
            &trie_result
        )
    ) {
        *result = trie_result;

        return true;
    }

    const std::size_t length =
        utf8_sequence_length(
            data[0]
        );

    if (
        length == 0
        || !valid_utf8_sequence(
            data,
            available,
            length
        )
    ) {
        return false;
    }

    result->normalized = data;
    result->normalized_bytes =
        length;
    result->consumed_bytes =
        length;
    result->direct_input = true;

    return true;
}

}   


bool tokenizer_normalizer_init(
    TokenizerNormalizer* normalizer,
    File* file
) {
    if (
        normalizer == nullptr
        || file == nullptr
        || !(*file)
    ) {
        return false;
    }

    std::memset(
        normalizer,
        0,
        sizeof(*normalizer)
    );

    if (!file->seek(0)) {
        return false;
    }

    std::uint8_t header[
        kHeaderBytes
    ] = {};

    if (
        !read_exact(
            *file,
            header,
            sizeof(header)
        )
    ) {
        return false;
    }

    if (
        std::memcmp(
            header,
            kMagic,
            sizeof(kMagic)
        ) != 0
    ) {
        return false;
    }

    const std::uint16_t version =
        read_u16_le(
            header + 4
        );

    const std::uint16_t reserved =
        read_u16_le(
            header + 6
        );

    const std::uint32_t trie_bytes =
        read_u32_le(
            header + 8
        );

    const std::uint32_t
        normalized_bytes =
            read_u32_le(
                header + 12
            );

    if (
        version != kVersion
        || reserved != 0
        || trie_bytes == 0
        || trie_bytes % 4 != 0
        || normalized_bytes == 0
    ) {
        return false;
    }

    const std::size_t expected_bytes =
        kHeaderBytes
        + static_cast<std::size_t>(
            trie_bytes
        )
        + static_cast<std::size_t>(
            normalized_bytes
        );

    if (
        file->size()
        != expected_bytes
    ) {
        return false;
    }

    normalizer->trie_units =
        static_cast<std::uint32_t*>(
            heap_caps_malloc(
                trie_bytes,
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (
        normalizer->trie_units
        == nullptr
    ) {
        tokenizer_normalizer_free(
            normalizer
        );

        return false;
    }

    normalizer->normalized_blob =
        static_cast<std::uint8_t*>(
            heap_caps_malloc(
                normalized_bytes,
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (
        normalizer->normalized_blob
        == nullptr
    ) {
        tokenizer_normalizer_free(
            normalizer
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            reinterpret_cast<
                std::uint8_t*
            >(
                normalizer->trie_units
            ),
            trie_bytes
        )
    ) {
        tokenizer_normalizer_free(
            normalizer
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            normalizer->
                normalized_blob,
            normalized_bytes
        )
    ) {
        tokenizer_normalizer_free(
            normalizer
        );

        return false;
    }

    if (
        normalizer->
            normalized_blob[
                normalized_bytes - 1
            ]
        != 0
    ) {
        tokenizer_normalizer_free(
            normalizer
        );

        return false;
    }

    normalizer->trie_unit_count =
        trie_bytes
        / sizeof(std::uint32_t);

    normalizer->normalized_bytes =
        normalized_bytes;

    normalizer->initialized = true;

    return true;
}


bool tokenizer_normalizer_normalize(
    const TokenizerNormalizer* normalizer,
    const char* input,
    std::size_t input_bytes,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
) {
    if (
        normalizer == nullptr
        || !normalizer->initialized
        || (
            input_bytes > 0
            && input == nullptr
        )
        || output == nullptr
        || output_capacity == 0
    ) {
        return false;
    }

    output[0] = '\0';

    if (output_bytes != nullptr) {
        *output_bytes = 0;
    }

    if (input_bytes == 0) {
        return true;
    }

    const auto* data =
        reinterpret_cast<
            const std::uint8_t*
        >(input);

    std::size_t position = 0;

     
     
     

    while (position < input_bytes) {
        PrefixResult prefix = {};

        if (
            !normalize_prefix(
                normalizer,
                data + position,
                input_bytes - position,
                &prefix
            )
        ) {
            return false;
        }

        if (
            prefix.normalized_bytes != 1
            || prefix.normalized[0]
                != 0x20
        ) {
            break;
        }

        position +=
            prefix.consumed_bytes;
    }

    if (position >= input_bytes) {
        return true;
    }

    std::size_t written = 0;

     
    if (
        !append_space_marker(
            output,
            output_capacity,
            &written
        )
    ) {
        return false;
    }

    bool previous_space = true;

     
     
     

    while (position < input_bytes) {
        PrefixResult prefix = {};

        if (
            !normalize_prefix(
                normalizer,
                data + position,
                input_bytes - position,
                &prefix
            )
        ) {
            output[0] = '\0';
            return false;
        }

        const std::uint8_t* piece =
            prefix.normalized;

        std::size_t piece_bytes =
            prefix.normalized_bytes;

         
         
         
        while (
            previous_space
            && piece_bytes > 0
            && piece[0] == 0x20
        ) {
            ++piece;
            --piece_bytes;
        }

        if (piece_bytes > 0) {
            for (
                std::size_t i = 0;
                i < piece_bytes;
                ++i
            ) {
                if (piece[i] == 0x20) {
                    if (
                        !append_space_marker(
                            output,
                            output_capacity,
                            &written
                        )
                    ) {
                        output[0] = '\0';
                        return false;
                    }
                } else {
                    if (
                        !append_bytes(
                            output,
                            output_capacity,
                            &written,
                            piece + i,
                            1
                        )
                    ) {
                        output[0] = '\0';
                        return false;
                    }
                }
            }

            previous_space =
                piece[
                    piece_bytes - 1
                ] == 0x20;
        }

        position +=
            prefix.consumed_bytes;
    }

     
     
     

    while (
        written >= sizeof(kSpaceMarker)
        && std::memcmp(
            output
                + written
                - sizeof(kSpaceMarker),
            kSpaceMarker,
            sizeof(kSpaceMarker)
        ) == 0
    ) {
        written -=
            sizeof(kSpaceMarker);
    }

    output[written] = '\0';

    if (output_bytes != nullptr) {
        *output_bytes = written;
    }

    return true;
}


void tokenizer_normalizer_free(
    TokenizerNormalizer* normalizer
) {
    if (normalizer == nullptr) {
        return;
    }

    if (
        normalizer->normalized_blob
        != nullptr
    ) {
        heap_caps_free(
            normalizer->
                normalized_blob
        );
    }

    if (
        normalizer->trie_units
        != nullptr
    ) {
        heap_caps_free(
            normalizer->trie_units
        );
    }

    normalizer->trie_units = nullptr;
    normalizer->normalized_blob = nullptr;

    normalizer->trie_unit_count = 0;
    normalizer->normalized_bytes = 0;

    normalizer->initialized = false;
}

}   
