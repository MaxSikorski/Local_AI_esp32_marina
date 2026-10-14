#pragma once
#include <cstdint>

#include "model_offsets.h"

                                                                                                 
                                                                                         
                                                                                       
namespace pocketgpt_tied30m {

namespace base = pocketgpt_final_v7b50;

constexpr uint32_t kLmHeadKeptOffset = base::kTokEmbKeptOffset;
constexpr uint32_t kLmHeadRowBytes = base::kTokEmbeddingRowBytes;
constexpr uint32_t kSharedMatrixBytes = base::kVocabSize * base::kTokEmbeddingRowBytes;

constexpr uint32_t kKeptPayloadBytes = 5527424u;                                          
constexpr uint32_t kFooterOffset = kKeptPayloadBytes;
constexpr uint32_t kFooterBytes = 64u;
constexpr uint32_t kKeptTensorsBytes = kKeptPayloadBytes + kFooterBytes;
constexpr uint32_t kTotalPackBytes = base::kWeightsQ4Bytes + base::kScalesF16Bytes + kKeptTensorsBytes;

                                    
constexpr char kFooterMagic[8] = {'P', 'G', 'T', 'I', 'E', 'D', 'v', '1'};
constexpr uint32_t kLayoutVersion = 1u;

constexpr uint8_t kWeightsQ4Sha256[32] = {
  0x3f, 0x74, 0x7b, 0xb8, 0x2a, 0x16, 0xae, 0x6b, 0x4f, 0x72, 0x87, 0x3d, 0x10, 0xa4, 0xfd, 0x17,
  0x5a, 0x4d, 0x67, 0x82, 0xb9, 0xf1, 0x08, 0x10, 0x16, 0x68, 0xe2, 0xf0, 0x3a, 0x0e, 0xf8, 0x02};
constexpr uint8_t kScalesF16Sha256[32] = {
  0x9f, 0xec, 0x62, 0xa1, 0x57, 0xd1, 0x7c, 0x78, 0xb0, 0x07, 0x4c, 0x6a, 0x68, 0x5d, 0x65, 0xc9,
  0xe1, 0x0d, 0x18, 0x7f, 0x33, 0xa6, 0x91, 0x00, 0x0c, 0x53, 0xaa, 0x21, 0x4f, 0x4b, 0x44, 0x35};
constexpr uint8_t kKeptTensorsSha256[32] = {
  0xd3, 0x2a, 0xf8, 0x94, 0x23, 0x9e, 0xe6, 0x60, 0x78, 0x84, 0x83, 0xbc, 0x66, 0x6d, 0xa0, 0x76,
  0xaf, 0x8b, 0x08, 0xa2, 0xa5, 0x2b, 0x0c, 0x84, 0xa3, 0x80, 0xe3, 0xce, 0x2c, 0x2e, 0x3e, 0xcd};

                                                                                           
constexpr uint32_t kHeadCacheCrc32 = 0x6306a56au;

static_assert(kLmHeadKeptOffset == base::kTokEmbKeptOffset, "lm_head must alias tok_embeddings");
static_assert(kLmHeadRowBytes == base::kLmHeadRowBytes, "head row size changed");
static_assert(kKeptPayloadBytes == base::kLmHeadKeptOffset, "payload must end where the old head began");
static_assert(base::kTokEmbKeptOffset + kSharedMatrixBytes <= base::kLayers[0].attn_norm_kept_offset,
              "shared matrix overlaps norms");
static_assert(base::kFinalNormKeptOffset + base::kDModel * 2u <= kKeptPayloadBytes, "final norm outside payload");
static_assert(kTotalPackBytes == 20241616u, "unexpected dedup package size");

}                                
