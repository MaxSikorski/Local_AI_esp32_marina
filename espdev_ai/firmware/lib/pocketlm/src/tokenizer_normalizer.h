#pragma once

#include <FS.h>

#include <cstddef>
#include <cstdint>

namespace pocketlm {


                                               
  
                                             
  
                   
  
                            
      
                                         
  
                                                       
                                                        
struct TokenizerNormalizer {
    std::uint32_t* trie_units;
    std::uint8_t* normalized_blob;

    std::size_t trie_unit_count;
    std::size_t normalized_bytes;

    bool initialized;
};


                                              
  
                                          
                                                        
bool tokenizer_normalizer_init(
    TokenizerNormalizer* normalizer,
    File* file
);


                                                         
                            
  
             
                            
                                    
                              
  
                                                         
  
                                                     
  
                                                       
                       
  
                                                    
                                                        
bool tokenizer_normalizer_normalize(
    const TokenizerNormalizer* normalizer,
    const char* input,
    std::size_t input_bytes,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
);


                                  
  
                                       
void tokenizer_normalizer_free(
    TokenizerNormalizer* normalizer
);

}                       
