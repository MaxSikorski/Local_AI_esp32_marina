#include "batched_attention.inc"
static bool rf_enabled=true;
static bool rf_layer_ffn(unsigned layer,unsigned count,float bh[4][448],float bn[4][448],float bq[4][448]) {
 unsigned t=0;for(;t+1<count;t+=2)if(!rf_pair(layer,bh[t],bh[t+1],bn,bq))return false;
 return t==count||rf_single(layer,bh[t]);
}
static bool rf_block(const uint32_t* ids,size_t first,unsigned count) {
 if(!ids||count<2||count>4||first>=HELLO_MAX_CONTEXT||count>HELLO_MAX_CONTEXT-first)return false;
 auto*p=bp_planes();auto&bh=p[0];auto&bn=p[1];auto&bq=p[2];
 for(unsigned t=0;t<count;t++)if(!loadEmbeddingRow(ids[t],bh[t]))return false;
 pipe_next_q=PrefetchedMatrix{};if(!pipeStartNextQ(5u,g_sd_scratch))return false;
 for(unsigned t=0;t<count;t++) {
  std::memcpy(hidden_rt,bh[t],sizeof(hidden_rt));
  if(!rf_attention0(0u,first+t,false))return false;
  std::memcpy(bh[t],hidden_rt,sizeof(hidden_rt));
 }
 if(!rf_layer_ffn(0,count,bh,bn,bq))return false;
  
 if(xSemaphoreTake(pipe_done_sem,portMAX_DELAY)!=pdTRUE)return false;
 __sync_synchronize();xSemaphoreGive(pipe_done_sem);
 for(unsigned l=1;l<5;l++) {
  for(unsigned t=0;t<count;t++) {
   std::memcpy(hidden_rt,bh[t],sizeof(hidden_rt));
   if(!rf_attention_early(l,first+t))return false;
   std::memcpy(bh[t],hidden_rt,sizeof(hidden_rt));
  }
  if(!rf_layer_ffn(l,count,bh,bn,bq))return false;
 }
 for(unsigned l=5;l<=10;l++)if(!bp_sd_layer(l,first,count,bh,bn,bq))return false;
 if(!pipeFinishNextQ(11))return false;const ResolvedMatrix q=pipe_next_q.resolved;
 for(unsigned t=0;t<count;t++) {
  std::memcpy(hidden_rt,bh[t],sizeof(hidden_rt));
  if(!rf_attention11(first+t,q))return false;
  std::memcpy(bh[t],hidden_rt,sizeof(hidden_rt));
 }
 if(!rf_layer_ffn(11,count,bh,bn,bq))return false;
 std::memcpy(hidden_rt,bh[count-1],sizeof(hidden_rt));return true;
}
static bool bp_block(const uint32_t* ids,size_t first,unsigned count) {
 return rf_enabled?rf_block(ids,first,count):bp_block_reference(ids,first,count);
}
