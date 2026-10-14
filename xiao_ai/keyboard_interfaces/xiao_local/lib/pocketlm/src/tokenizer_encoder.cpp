#include "tokenizer_encoder.h"

#include <Arduino.h>

#include <cstring>
#include <limits>

#include "esp_heap_caps.h"

namespace pocketlm {

namespace {

constexpr std::uint8_t kMagic[4] = {
    'P', 'K', 'T', 'E'
};

constexpr std::uint16_t kVersion = 1;

constexpr std::size_t kHeaderBytes = 20;

constexpr std::uint16_t kExpectedNormalFirstId =
    266;

constexpr std::uint16_t kExpectedNormalCount =
    5878;

constexpr std::uint16_t kExpectedHashSlots =
    8192;

constexpr std::uint16_t kByteFirstId =
    10;


 
 
 
 
 

struct SpecialSymbol {
    const char* text;
    std::uint16_t token_id;
};


constexpr SpecialSymbol kSpecialSymbols[] = {
    {"<|end_memory|>", 8},
    {"<|assistant|>", 5},
    {"<|system|>", 6},
    {"<|memory|>", 7},
    {"<|user|>", 4},
    {"<|end|>", 9},
};


constexpr std::size_t kSpecialSymbolCount =
    sizeof(kSpecialSymbols)
    / sizeof(kSpecialSymbols[0]);


 
 
 

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


 
 
 
 
 

std::uint32_t fnv1a32(
    const std::uint8_t* data,
    std::size_t bytes
) {
    std::uint32_t hash =
        0x811C9DC5UL;

    for (
        std::size_t i = 0;
        i < bytes;
        ++i
    ) {
        hash ^= data[i];

        hash *=
            0x01000193UL;
    }

    return hash;
}


 
 
 
 
 
 
 

std::uint16_t lookup_normal_piece(
    const TokenizerEncoder* encoder,
    const std::uint8_t* piece,
    std::size_t piece_bytes
) {
    if (
        encoder == nullptr
        || !encoder->initialized
        || piece == nullptr
        || piece_bytes == 0
    ) {
        return 0;
    }

    const std::size_t mask =
        encoder->hash_slots - 1;

    std::size_t slot =
        static_cast<std::size_t>(
            fnv1a32(
                piece,
                piece_bytes
            )
        )
        & mask;

    for (
        std::size_t probe = 0;
        probe < encoder->hash_slots;
        ++probe
    ) {
        const std::uint16_t token_id =
            encoder->hash_table[slot];

        if (token_id == 0) {
            return 0;
        }

        if (
            token_id
                >= encoder->normal_first_id
            && token_id
                < encoder->normal_first_id
                    + encoder->normal_count
        ) {
            const std::size_t local =
                token_id
                - encoder->normal_first_id;

            const std::size_t start =
                encoder->offsets[local];

            const std::size_t end =
                encoder->offsets[
                    local + 1
                ];

            if (
                end >= start
                && end - start
                    == piece_bytes
                && std::memcmp(
                    encoder->string_blob
                        + start,
                    piece,
                    piece_bytes
                ) == 0
            ) {
                return token_id;
            }
        }

        slot =
            (slot + 1)
            & mask;
    }

    return 0;
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


 
 
 

bool match_special_symbol(
    const std::uint8_t* data,
    std::size_t available,
    std::size_t* matched_bytes,
    std::uint16_t* token_id
) {
    if (
        data == nullptr
        || matched_bytes == nullptr
        || token_id == nullptr
    ) {
        return false;
    }

    for (
        std::size_t i = 0;
        i < kSpecialSymbolCount;
        ++i
    ) {
        const char* text =
            kSpecialSymbols[i].text;

        const std::size_t bytes =
            std::strlen(text);

        if (
            available >= bytes
            && std::memcmp(
                data,
                text,
                bytes
            ) == 0
        ) {
            *matched_bytes = bytes;

            *token_id =
                kSpecialSymbols[i]
                    .token_id;

            return true;
        }
    }

    return false;
}


 
 
 
 
 
 
 
 
 

struct BpeSymbol {
    std::size_t start;
    std::size_t end;

    std::uint16_t token_id;

    bool special;
};

}   


bool tokenizer_encoder_init(
    TokenizerEncoder* encoder,
    File* file
) {
    if (
        encoder == nullptr
        || file == nullptr
        || !(*file)
    ) {
        return false;
    }

    std::memset(
        encoder,
        0,
        sizeof(*encoder)
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

    const std::uint16_t normal_first_id =
        read_u16_le(
            header + 6
        );

    const std::uint16_t normal_count =
        read_u16_le(
            header + 8
        );

    const std::uint16_t hash_slots =
        read_u16_le(
            header + 10
        );

    const std::uint32_t offsets_count =
        read_u32_le(
            header + 12
        );

    const std::uint32_t string_bytes =
        read_u32_le(
            header + 16
        );

    if (
        version != kVersion
        || normal_first_id
            != kExpectedNormalFirstId
        || normal_count
            != kExpectedNormalCount
        || hash_slots
            != kExpectedHashSlots
        || offsets_count
            != static_cast<std::uint32_t>(
                normal_count
            ) + 1
        || string_bytes == 0
        || (
            hash_slots
            & (hash_slots - 1)
        ) != 0
    ) {
        return false;
    }

    const std::size_t offset_bytes =
        static_cast<std::size_t>(
            offsets_count
        )
        * sizeof(std::uint16_t);

    const std::size_t hash_bytes =
        static_cast<std::size_t>(
            hash_slots
        )
        * sizeof(std::uint16_t);

    const std::size_t expected_file_bytes =
        kHeaderBytes
        + offset_bytes
        + hash_bytes
        + static_cast<std::size_t>(
            string_bytes
        );

    if (
        file->size()
        != expected_file_bytes
    ) {
        return false;
    }

    encoder->offsets =
        static_cast<std::uint16_t*>(
            heap_caps_malloc(
                offset_bytes,
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (encoder->offsets == nullptr) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    encoder->hash_table =
        static_cast<std::uint16_t*>(
            heap_caps_malloc(
                hash_bytes,
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (
        encoder->hash_table
        == nullptr
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    encoder->string_blob =
        static_cast<std::uint8_t*>(
            heap_caps_malloc(
                static_cast<std::size_t>(
                    string_bytes
                ),
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (
        encoder->string_blob
        == nullptr
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            reinterpret_cast<
                std::uint8_t*
            >(
                encoder->offsets
            ),
            offset_bytes
        )
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            reinterpret_cast<
                std::uint8_t*
            >(
                encoder->hash_table
            ),
            hash_bytes
        )
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            encoder->string_blob,
            string_bytes
        )
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    if (
        encoder->offsets[
            offsets_count - 1
        ] != string_bytes
    ) {
        tokenizer_encoder_free(
            encoder
        );

        return false;
    }

    for (
        std::size_t i = 1;
        i < offsets_count;
        ++i
    ) {
        if (
            encoder->offsets[i]
            < encoder->offsets[i - 1]
            || encoder->offsets[i]
                > string_bytes
        ) {
            tokenizer_encoder_free(
                encoder
            );

            return false;
        }
    }

    for (
        std::size_t i = 0;
        i < hash_slots;
        ++i
    ) {
        const std::uint16_t token_id =
            encoder->hash_table[i];

        if (token_id == 0) {
            continue;
        }

        if (
            token_id < normal_first_id
            || token_id
                >= normal_first_id
                    + normal_count
        ) {
            tokenizer_encoder_free(
                encoder
            );

            return false;
        }
    }

    encoder->offsets_count =
        offsets_count;

    encoder->hash_slots =
        hash_slots;

    encoder->string_bytes =
        string_bytes;

    encoder->normal_first_id =
        normal_first_id;

    encoder->normal_count =
        normal_count;

    encoder->initialized = true;

    return true;
}


bool tokenizer_encoder_encode_normalized(
    const TokenizerEncoder* encoder,
    const char* normalized_input,
    std::size_t input_bytes,
    std::uint32_t* token_ids,
    std::size_t token_capacity,
    std::size_t* token_count
) {
    if (token_count != nullptr) {
        *token_count = 0;
    }

    if (
        encoder == nullptr
        || !encoder->initialized
        || (
            input_bytes > 0
            && normalized_input == nullptr
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
        > std::numeric_limits<
            std::size_t
        >::max()
            / sizeof(BpeSymbol)
    ) {
        return false;
    }

    const auto* data =
        reinterpret_cast<
            const std::uint8_t*
        >(
            normalized_input
        );

     
     
    BpeSymbol* symbols =
        static_cast<BpeSymbol*>(
            heap_caps_malloc(
                input_bytes
                    * sizeof(BpeSymbol),
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (symbols == nullptr) {
        return false;
    }

    auto fail = [&]() -> bool {
        heap_caps_free(symbols);

        if (token_count != nullptr) {
            *token_count = 0;
        }

        return false;
    };

    std::size_t symbol_count = 0;
    std::size_t position = 0;

     
     
     
     
     
     

    while (position < input_bytes) {
        std::size_t special_bytes = 0;
        std::uint16_t special_id = 0;

        if (
            match_special_symbol(
                data + position,
                input_bytes - position,
                &special_bytes,
                &special_id
            )
        ) {
            symbols[symbol_count] = {
                position,
                position + special_bytes,
                special_id,
                true,
            };

            ++symbol_count;
            position += special_bytes;

            continue;
        }

        const std::size_t length =
            utf8_sequence_length(
                data[position]
            );

        if (
            length == 0
            || !valid_utf8_sequence(
                data + position,
                input_bytes - position,
                length
            )
        ) {
            return fail();
        }

        symbols[symbol_count] = {
            position,
            position + length,
            0,
            false,
        };

        ++symbol_count;
        position += length;
    }

     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     
     

    while (symbol_count >= 2) {
        bool found_merge = false;

        std::size_t best_index = 0;

        std::uint16_t best_token_id = 0;

        for (
            std::size_t i = 0;
            i + 1 < symbol_count;
            ++i
        ) {
            const BpeSymbol& left =
                symbols[i];

            const BpeSymbol& right =
                symbols[i + 1];

            if (
                left.special
                || right.special
            ) {
                continue;
            }

            if (
                right.end < left.start
                || right.end > input_bytes
            ) {
                return fail();
            }

            const std::size_t merged_bytes =
                right.end - left.start;

            const std::uint16_t token_id =
                lookup_normal_piece(
                    encoder,
                    data + left.start,
                    merged_bytes
                );

            if (token_id == 0) {
                continue;
            }

            if (
                !found_merge
                || token_id
                    < best_token_id
            ) {
                found_merge = true;

                best_index = i;

                best_token_id =
                    token_id;
            }
        }

        if (!found_merge) {
            break;
        }

        const std::size_t merged_end =
            symbols[
                best_index + 1
            ].end;

        symbols[best_index].end =
            merged_end;

        symbols[best_index].token_id =
            best_token_id;

        symbols[best_index].special =
            false;

        for (
            std::size_t i =
                best_index + 1;
            i + 1 < symbol_count;
            ++i
        ) {
            symbols[i] =
                symbols[i + 1];
        }

        --symbol_count;
    }

     
     
     

    std::size_t written = 0;

    for (
        std::size_t i = 0;
        i < symbol_count;
        ++i
    ) {
        const BpeSymbol& symbol =
            symbols[i];

        if (
            symbol.end < symbol.start
            || symbol.end > input_bytes
        ) {
            return fail();
        }

        if (symbol.special) {
            if (
                written
                >= token_capacity
            ) {
                return fail();
            }

            token_ids[written++] =
                symbol.token_id;

            continue;
        }

        const std::size_t piece_bytes =
            symbol.end
            - symbol.start;

        const std::uint16_t token_id =
            lookup_normal_piece(
                encoder,
                data + symbol.start,
                piece_bytes
            );

        if (token_id != 0) {
            if (
                written
                >= token_capacity
            ) {
                return fail();
            }

            token_ids[written++] =
                token_id;

            continue;
        }

         
         
         
         
         
         
         
         

        for (
            std::size_t j =
                symbol.start;
            j < symbol.end;
            ++j
        ) {
            if (
                written
                >= token_capacity
            ) {
                return fail();
            }

            token_ids[written++] =
                static_cast<std::uint32_t>(
                    kByteFirstId
                    + data[j]
                );
        }
    }

    heap_caps_free(symbols);

    if (token_count != nullptr) {
        *token_count = written;
    }

    return true;
}


void tokenizer_encoder_free(
    TokenizerEncoder* encoder
) {
    if (encoder == nullptr) {
        return;
    }

    if (
        encoder->string_blob
        != nullptr
    ) {
        heap_caps_free(
            encoder->string_blob
        );
    }

    if (
        encoder->hash_table
        != nullptr
    ) {
        heap_caps_free(
            encoder->hash_table
        );
    }

    if (
        encoder->offsets
        != nullptr
    ) {
        heap_caps_free(
            encoder->offsets
        );
    }

    encoder->offsets = nullptr;
    encoder->hash_table = nullptr;
    encoder->string_blob = nullptr;

    encoder->offsets_count = 0;
    encoder->hash_slots = 0;
    encoder->string_bytes = 0;

    encoder->normal_first_id = 0;
    encoder->normal_count = 0;

    encoder->initialized = false;
}

}   
