#pragma once

#include <FS.h>

#include <cstddef>
#include <cstdint>

namespace pocketlm {

constexpr std::size_t kTokenizerEncoderVocabSize =
    6144;

constexpr std::size_t kTokenizerEncoderMaxTokens =
    160;


                                                
  
                                          
  
                  
  
                         
                            
                            
  
                                   
  
                                                     
                                                        
struct TokenizerEncoder {
    std::uint16_t* offsets;
    std::uint16_t* hash_table;
    std::uint8_t* string_blob;

    std::size_t offsets_count;
    std::size_t hash_slots;
    std::size_t string_bytes;

    std::uint16_t normal_first_id;
    std::uint16_t normal_count;

    bool initialized;
};


                                           
  
                                          
                                                          
bool tokenizer_encoder_init(
    TokenizerEncoder* encoder,
    File* file
);


                                                      
  
                              
  
                   
                         
                                 
  
           
  
                  
                           
                   
  
                        
  
                        
                        
                        
                        
                        
                        
  
                                             
  
                          
        
                           
  
                                                  
  
                                                       
                                         
bool tokenizer_encoder_encode_normalized(
    const TokenizerEncoder* encoder,
    const char* normalized_input,
    std::size_t input_bytes,
    std::uint32_t* token_ids,
    std::size_t token_capacity,
    std::size_t* token_count
);


                               
  
                                    
void tokenizer_encoder_free(
    TokenizerEncoder* encoder
);

}                       
