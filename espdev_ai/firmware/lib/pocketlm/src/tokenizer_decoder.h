#pragma once

#include <FS.h>

#include <cstddef>
#include <cstdint>

namespace pocketlm {

constexpr std::size_t kTokenizerDecoderVocabSize =
    6144;


                                                     
  
                                        
                          
  
                  
                               
                            
  
                                           
                                                  
struct TokenizerDecoder {
    std::uint16_t* offsets;
    std::uint8_t* string_blob;

    std::size_t offsets_count;
    std::size_t string_bytes;

    std::uint16_t byte_first_id;
    std::uint16_t byte_count;

    bool initialized;
};


                                           
  
                                                 
                                                         
                                                   
bool tokenizer_decoder_init(
    TokenizerDecoder* decoder,
    File* file
);


                                              
  
                                     
  
                                 
                             
                                         
                                   
                                   
  
                                       
                       
  
                                                           
                                 
  
                                                      
                   
bool tokenizer_decoder_decode(
    const TokenizerDecoder* decoder,
    const std::uint32_t* token_ids,
    std::size_t token_count,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
);


                               
  
                                    
void tokenizer_decoder_free(
    TokenizerDecoder* decoder
);

}                       
