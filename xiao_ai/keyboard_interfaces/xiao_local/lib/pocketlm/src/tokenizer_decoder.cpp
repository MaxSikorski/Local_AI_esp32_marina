#include "tokenizer_decoder.h"

#include <Arduino.h>

#include <cstring>

#include "esp_heap_caps.h"

namespace pocketlm {

namespace {

constexpr std::uint8_t kMagic[4] = {
    'P', 'K', 'T', 'D'
};

constexpr std::uint16_t kVersion = 1;

constexpr std::size_t kHeaderBytes = 20;

constexpr std::uint32_t kPadId = 0;
constexpr std::uint32_t kUnkId = 1;
constexpr std::uint32_t kBosId = 2;
constexpr std::uint32_t kEosId = 3;

constexpr std::uint8_t kUnderlineUtf8[3] = {
    0xE2, 0x96, 0x81
};

constexpr std::uint8_t kUnkUtf8[] = {
    0x20,
    0xE2, 0x81, 0x87,
    0x20
};


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


bool append_byte(
    char* output,
    std::size_t output_capacity,
    std::size_t* output_size,
    std::uint8_t value
) {
    if (
        output == nullptr
        || output_size == nullptr
        || *output_size + 1
            >= output_capacity
    ) {
        return false;
    }

    output[*output_size] =
        static_cast<char>(value);

    ++(*output_size);

    return true;
}


bool append_bytes(
    char* output,
    std::size_t output_capacity,
    std::size_t* output_size,
    const std::uint8_t* data,
    std::size_t bytes
) {
    if (
        output == nullptr
        || output_size == nullptr
        || (
            bytes > 0
            && data == nullptr
        )
        || *output_size + bytes
            >= output_capacity
    ) {
        return false;
    }

    if (bytes > 0) {
        std::memcpy(
            output + *output_size,
            data,
            bytes
        );
    }

    *output_size += bytes;

    return true;
}


bool append_piece_with_space_marker(
    char* output,
    std::size_t output_capacity,
    std::size_t* output_size,
    const std::uint8_t* piece,
    std::size_t piece_bytes
) {
    std::size_t i = 0;

    while (i < piece_bytes) {
        if (
            i + 3 <= piece_bytes
            && piece[i]
                == kUnderlineUtf8[0]
            && piece[i + 1]
                == kUnderlineUtf8[1]
            && piece[i + 2]
                == kUnderlineUtf8[2]
        ) {
            if (
                !append_byte(
                    output,
                    output_capacity,
                    output_size,
                    static_cast<
                        std::uint8_t
                    >(' ')
                )
            ) {
                return false;
            }

            i += 3;
            continue;
        }

        if (
            !append_byte(
                output,
                output_capacity,
                output_size,
                piece[i]
            )
        ) {
            return false;
        }

        ++i;
    }

    return true;
}

}   


bool tokenizer_decoder_init(
    TokenizerDecoder* decoder,
    File* file
) {
    if (
        decoder == nullptr
        || file == nullptr
        || !(*file)
    ) {
        return false;
    }

    std::memset(
        decoder,
        0,
        sizeof(*decoder)
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

    const std::uint16_t vocab_size =
        read_u16_le(
            header + 6
        );

    const std::uint16_t byte_first_id =
        read_u16_le(
            header + 8
        );

    const std::uint16_t byte_count =
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
        || vocab_size
            != kTokenizerDecoderVocabSize
        || offsets_count
            != kTokenizerDecoderVocabSize
                + 1
        || byte_count != 256
    ) {
        return false;
    }

    const std::size_t
        expected_file_bytes =
            kHeaderBytes
            + static_cast<std::size_t>(
                offsets_count
            ) * sizeof(std::uint16_t)
            + static_cast<std::size_t>(
                string_bytes
            );

    if (
        file->size()
        != expected_file_bytes
    ) {
        return false;
    }

    decoder->offsets =
        static_cast<std::uint16_t*>(
            heap_caps_malloc(
                static_cast<std::size_t>(
                    offsets_count
                )
                    * sizeof(std::uint16_t),
                MALLOC_CAP_SPIRAM
                    | MALLOC_CAP_8BIT
            )
        );

    if (decoder->offsets == nullptr) {
        tokenizer_decoder_free(
            decoder
        );

        return false;
    }

    decoder->string_blob =
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
        decoder->string_blob
        == nullptr
    ) {
        tokenizer_decoder_free(
            decoder
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            reinterpret_cast<
                std::uint8_t*
            >(
                decoder->offsets
            ),
            static_cast<std::size_t>(
                offsets_count
            )
                * sizeof(std::uint16_t)
        )
    ) {
        tokenizer_decoder_free(
            decoder
        );

        return false;
    }

    if (
        !read_exact(
            *file,
            decoder->string_blob,
            string_bytes
        )
    ) {
        tokenizer_decoder_free(
            decoder
        );

        return false;
    }

    if (
        decoder->offsets[
            offsets_count - 1
        ] != string_bytes
    ) {
        tokenizer_decoder_free(
            decoder
        );

        return false;
    }

    for (
        std::size_t i = 1;
        i < offsets_count;
        ++i
    ) {
        if (
            decoder->offsets[i]
            < decoder->offsets[i - 1]
            || decoder->offsets[i]
                > string_bytes
        ) {
            tokenizer_decoder_free(
                decoder
            );

            return false;
        }
    }

    decoder->offsets_count =
        offsets_count;

    decoder->string_bytes =
        string_bytes;

    decoder->byte_first_id =
        byte_first_id;

    decoder->byte_count =
        byte_count;

    decoder->initialized = true;

    return true;
}


bool tokenizer_decoder_decode(
    const TokenizerDecoder* decoder,
    const std::uint32_t* token_ids,
    std::size_t token_count,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
) {
    if (
        decoder == nullptr
        || !decoder->initialized
        || (
            token_count > 0
            && token_ids == nullptr
        )
        || output == nullptr
        || output_capacity == 0
    ) {
        return false;
    }

    std::size_t written = 0;

    bool remove_initial_dummy_space =
        true;

    for (
        std::size_t token_index = 0;
        token_index < token_count;
        ++token_index
    ) {
        const std::uint32_t token_id =
            token_ids[token_index];

        if (
            token_id
            >= kTokenizerDecoderVocabSize
        ) {
            output[0] = '\0';

            if (output_bytes != nullptr) {
                *output_bytes = 0;
            }

            return false;
        }

         
        if (
            token_id == kPadId
            || token_id == kBosId
            || token_id == kEosId
        ) {
            continue;
        }

         
        if (token_id == kUnkId) {
            if (
                !append_bytes(
                    output,
                    output_capacity,
                    &written,
                    kUnkUtf8,
                    sizeof(kUnkUtf8)
                )
            ) {
                output[0] = '\0';

                if (
                    output_bytes
                    != nullptr
                ) {
                    *output_bytes = 0;
                }

                return false;
            }

            remove_initial_dummy_space =
                false;

            continue;
        }

         
        if (
            token_id
                >= decoder->byte_first_id
            && token_id
                < decoder->byte_first_id
                    + decoder->byte_count
        ) {
            if (
                !append_byte(
                    output,
                    output_capacity,
                    &written,
                    static_cast<std::uint8_t>(
                        token_id
                        - decoder->
                            byte_first_id
                    )
                )
            ) {
                output[0] = '\0';

                if (
                    output_bytes
                    != nullptr
                ) {
                    *output_bytes = 0;
                }

                return false;
            }

            remove_initial_dummy_space =
                false;

            continue;
        }

        const std::size_t start =
            decoder->offsets[
                token_id
            ];

        const std::size_t end =
            decoder->offsets[
                token_id + 1
            ];

        if (
            end < start
            || end > decoder->string_bytes
        ) {
            output[0] = '\0';

            if (output_bytes != nullptr) {
                *output_bytes = 0;
            }

            return false;
        }

        const std::size_t before =
            written;

        if (
            !append_piece_with_space_marker(
                output,
                output_capacity,
                &written,
                decoder->string_blob
                    + start,
                end - start
            )
        ) {
            output[0] = '\0';

            if (output_bytes != nullptr) {
                *output_bytes = 0;
            }

            return false;
        }

        if (
            remove_initial_dummy_space
            && written > before
        ) {
            if (
                output[before] == ' '
            ) {
                std::memmove(
                    output + before,
                    output + before + 1,
                    written - before - 1
                );

                --written;
            }

            remove_initial_dummy_space =
                false;
        }
    }

    if (
        written
        >= output_capacity
    ) {
        return false;
    }

    output[written] = '\0';

    if (output_bytes != nullptr) {
        *output_bytes = written;
    }

    return true;
}


void tokenizer_decoder_free(
    TokenizerDecoder* decoder
) {
    if (decoder == nullptr) {
        return;
    }

    if (
        decoder->string_blob
        != nullptr
    ) {
        heap_caps_free(
            decoder->string_blob
        );
    }

    if (decoder->offsets != nullptr) {
        heap_caps_free(
            decoder->offsets
        );
    }

    decoder->offsets = nullptr;
    decoder->string_blob = nullptr;

    decoder->offsets_count = 0;
    decoder->string_bytes = 0;

    decoder->byte_first_id = 0;
    decoder->byte_count = 0;

    decoder->initialized = false;
}

}   
