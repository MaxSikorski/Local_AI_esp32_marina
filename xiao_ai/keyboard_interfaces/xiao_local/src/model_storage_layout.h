#pragma once

#include <stdint.h>

 
 
 
 
 

namespace pocketgpt {
namespace xiao_final {

enum class MatrixLocation : uint8_t {
    Psram = 0,
    Flash = 1,
    Sd    = 2,
};

enum class MatrixFamily : uint8_t {
    QProj = 0,
    KProj = 1,
    VProj = 2,
    OProj = 3,
    GateProj = 4,
    UpProj = 5,
    DownProj = 6
};

struct MatrixPlacement {
    MatrixLocation location;
    uint32_t q_offset;
    uint32_t q_bytes;
    uint32_t scales_offset;
    uint32_t scales_bytes;
    uint32_t total_bytes;
};

static constexpr uint32_t kLayerCount = 12u;
static constexpr uint32_t kFamilyCount = 7u;
static constexpr uint32_t kMatrixCount = 84u;
static constexpr uint32_t kModelPhysicalStart = 720896u;
static constexpr uint32_t kModelPartitionBytes = 7602176u;
static constexpr uint32_t kModelTransformerOffset = 1495040u;
static constexpr uint32_t kModelTransformerBytes = 6104224u;
static constexpr uint32_t kModelUsedEnd = 7599264u;
static constexpr uint32_t kPsramBudgetBytes = 6900000u;
static constexpr uint32_t kPsramTransformerBytes = 6877248u;
static constexpr uint32_t kPsramUnusedBudgetBytes = 22752u;
static constexpr uint32_t kSdTransformerBytes = 1732640u;
static constexpr uint32_t kEmbeddingBytes = 5505024u;
static constexpr uint32_t kEmbeddingRowBytes = 896u;

static constexpr MatrixPlacement kMatrices[kLayerCount][kFamilyCount] = {
     
    {
        {MatrixLocation::Psram, 0u, 100352u, 100352u, 6272u, 106624u},  
        {MatrixLocation::Psram, 106624u, 25088u, 131712u, 1568u, 26656u},  
        {MatrixLocation::Psram, 133280u, 25088u, 158368u, 1568u, 26656u},  
        {MatrixLocation::Psram, 159936u, 100352u, 260288u, 6272u, 106624u},  
        {MatrixLocation::Psram, 266560u, 301056u, 567616u, 18816u, 319872u},  
        {MatrixLocation::Psram, 586432u, 301056u, 887488u, 18816u, 319872u},  
        {MatrixLocation::Psram, 906304u, 301056u, 1207360u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Psram, 1226176u, 100352u, 1326528u, 6272u, 106624u},  
        {MatrixLocation::Psram, 1332800u, 25088u, 1357888u, 1568u, 26656u},  
        {MatrixLocation::Psram, 1359456u, 25088u, 1384544u, 1568u, 26656u},  
        {MatrixLocation::Psram, 1386112u, 100352u, 1486464u, 6272u, 106624u},  
        {MatrixLocation::Psram, 1492736u, 301056u, 1793792u, 18816u, 319872u},  
        {MatrixLocation::Psram, 1812608u, 301056u, 2113664u, 18816u, 319872u},  
        {MatrixLocation::Psram, 2132480u, 301056u, 2433536u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Psram, 2452352u, 100352u, 2552704u, 6272u, 106624u},  
        {MatrixLocation::Psram, 2558976u, 25088u, 2584064u, 1568u, 26656u},  
        {MatrixLocation::Psram, 2585632u, 25088u, 2610720u, 1568u, 26656u},  
        {MatrixLocation::Psram, 2612288u, 100352u, 2712640u, 6272u, 106624u},  
        {MatrixLocation::Psram, 2718912u, 301056u, 3019968u, 18816u, 319872u},  
        {MatrixLocation::Psram, 3038784u, 301056u, 3339840u, 18816u, 319872u},  
        {MatrixLocation::Psram, 3358656u, 301056u, 3659712u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Psram, 3678528u, 100352u, 3778880u, 6272u, 106624u},  
        {MatrixLocation::Psram, 3785152u, 25088u, 3810240u, 1568u, 26656u},  
        {MatrixLocation::Psram, 3811808u, 25088u, 3836896u, 1568u, 26656u},  
        {MatrixLocation::Psram, 3838464u, 100352u, 3938816u, 6272u, 106624u},  
        {MatrixLocation::Psram, 3945088u, 301056u, 4246144u, 18816u, 319872u},  
        {MatrixLocation::Psram, 4264960u, 301056u, 4566016u, 18816u, 319872u},  
        {MatrixLocation::Psram, 4584832u, 301056u, 4885888u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Psram, 4904704u, 100352u, 5005056u, 6272u, 106624u},  
        {MatrixLocation::Psram, 5011328u, 25088u, 5036416u, 1568u, 26656u},  
        {MatrixLocation::Psram, 5037984u, 25088u, 5063072u, 1568u, 26656u},  
        {MatrixLocation::Psram, 5064640u, 100352u, 5164992u, 6272u, 106624u},  
        {MatrixLocation::Psram, 5171264u, 301056u, 5472320u, 18816u, 319872u},  
        {MatrixLocation::Psram, 5491136u, 301056u, 5792192u, 18816u, 319872u},  
        {MatrixLocation::Psram, 5811008u, 301056u, 6112064u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 0u, 100352u, 100352u, 6272u, 106624u},  
        {MatrixLocation::Flash, 1495040u, 25088u, 1520128u, 1568u, 26656u},  
        {MatrixLocation::Sd, 106624u, 25088u, 131712u, 1568u, 26656u},  
        {MatrixLocation::Psram, 6130880u, 100352u, 6231232u, 6272u, 106624u},  
        {MatrixLocation::Psram, 6237504u, 301056u, 6538560u, 18816u, 319872u},  
        {MatrixLocation::Psram, 6557376u, 301056u, 6858432u, 18816u, 319872u},  
        {MatrixLocation::Flash, 1521696u, 301056u, 1822752u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 133280u, 100352u, 233632u, 6272u, 106624u},  
        {MatrixLocation::Sd, 239904u, 25088u, 264992u, 1568u, 26656u},  
        {MatrixLocation::Sd, 266560u, 25088u, 291648u, 1568u, 26656u},  
        {MatrixLocation::Sd, 293216u, 100352u, 393568u, 6272u, 106624u},  
        {MatrixLocation::Flash, 1841568u, 301056u, 2142624u, 18816u, 319872u},  
        {MatrixLocation::Flash, 2161440u, 301056u, 2462496u, 18816u, 319872u},  
        {MatrixLocation::Flash, 2481312u, 301056u, 2782368u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 399840u, 100352u, 500192u, 6272u, 106624u},  
        {MatrixLocation::Sd, 506464u, 25088u, 531552u, 1568u, 26656u},  
        {MatrixLocation::Sd, 533120u, 25088u, 558208u, 1568u, 26656u},  
        {MatrixLocation::Sd, 559776u, 100352u, 660128u, 6272u, 106624u},  
        {MatrixLocation::Flash, 2801184u, 301056u, 3102240u, 18816u, 319872u},  
        {MatrixLocation::Flash, 3121056u, 301056u, 3422112u, 18816u, 319872u},  
        {MatrixLocation::Flash, 3440928u, 301056u, 3741984u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 666400u, 100352u, 766752u, 6272u, 106624u},  
        {MatrixLocation::Sd, 773024u, 25088u, 798112u, 1568u, 26656u},  
        {MatrixLocation::Sd, 799680u, 25088u, 824768u, 1568u, 26656u},  
        {MatrixLocation::Sd, 826336u, 100352u, 926688u, 6272u, 106624u},  
        {MatrixLocation::Flash, 3760800u, 301056u, 4061856u, 18816u, 319872u},  
        {MatrixLocation::Flash, 4080672u, 301056u, 4381728u, 18816u, 319872u},  
        {MatrixLocation::Flash, 4400544u, 301056u, 4701600u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 932960u, 100352u, 1033312u, 6272u, 106624u},  
        {MatrixLocation::Sd, 1039584u, 25088u, 1064672u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1066240u, 25088u, 1091328u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1092896u, 100352u, 1193248u, 6272u, 106624u},  
        {MatrixLocation::Flash, 4720416u, 301056u, 5021472u, 18816u, 319872u},  
        {MatrixLocation::Flash, 5040288u, 301056u, 5341344u, 18816u, 319872u},  
        {MatrixLocation::Flash, 5360160u, 301056u, 5661216u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 1199520u, 100352u, 1299872u, 6272u, 106624u},  
        {MatrixLocation::Sd, 1306144u, 25088u, 1331232u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1332800u, 25088u, 1357888u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1359456u, 100352u, 1459808u, 6272u, 106624u},  
        {MatrixLocation::Flash, 5680032u, 301056u, 5981088u, 18816u, 319872u},  
        {MatrixLocation::Flash, 5999904u, 301056u, 6300960u, 18816u, 319872u},  
        {MatrixLocation::Flash, 6319776u, 301056u, 6620832u, 18816u, 319872u}  
    },
     
    {
        {MatrixLocation::Sd, 1466080u, 100352u, 1566432u, 6272u, 106624u},  
        {MatrixLocation::Sd, 1572704u, 25088u, 1597792u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1599360u, 25088u, 1624448u, 1568u, 26656u},  
        {MatrixLocation::Sd, 1626016u, 100352u, 1726368u, 6272u, 106624u},  
        {MatrixLocation::Flash, 6639648u, 301056u, 6940704u, 18816u, 319872u},  
        {MatrixLocation::Flash, 6959520u, 301056u, 7260576u, 18816u, 319872u},  
        {MatrixLocation::Flash, 7279392u, 301056u, 7580448u, 18816u, 319872u}  
    }
};

static constexpr const MatrixPlacement& matrixPlacement(
    uint32_t layer,
    MatrixFamily family
) {
    return kMatrices[layer]
        [static_cast<uint8_t>(family)];
}

}  
}  
