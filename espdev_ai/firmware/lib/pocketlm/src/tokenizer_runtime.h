#pragma once

#include <FS.h>

#include <cstddef>
#include <cstdint>

#include "tokenizer_decoder.h"
#include "tokenizer_encoder.h"
#include "tokenizer_normalizer.h"

namespace pocketlm {


                                      
  
        
  
                                   
                      
                          
  
                                                       
struct TokenizerRuntime {
    TokenizerNormalizer normalizer;
    TokenizerEncoder encoder;
    TokenizerDecoder decoder;

    bool initialized;
};


                                       
  
                             
                          
                          
  
                                          
                                                         
bool tokenizer_runtime_init(
    TokenizerRuntime* tokenizer,
    File* normalizer_file,
    File* encoder_file,
    File* decoder_file
);


                               
  
                 
  
             
         
                        
         
                                 
         
              
  
                                                       
                                            
  
                                                  
bool tokenizer_runtime_encode(
    const TokenizerRuntime* tokenizer,
    const char* input,
    std::size_t input_bytes,
    std::uint32_t* token_ids,
    std::size_t token_capacity,
    std::size_t* token_count
);


                           
  
                                                         
bool tokenizer_runtime_decode(
    const TokenizerRuntime* tokenizer,
    const std::uint32_t* token_ids,
    std::size_t token_count,
    char* output,
    std::size_t output_capacity,
    std::size_t* output_bytes
);


                                     
  
                                    
void tokenizer_runtime_free(
    TokenizerRuntime* tokenizer
);

}                       
