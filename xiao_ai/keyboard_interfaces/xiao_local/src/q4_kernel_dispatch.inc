 
 
static bool row4_enabled=true;
__attribute__((noinline)) IRAM_ATTR
void runtime_q4_rowpair2_fp16(const std::uint8_t*q,const std::uint16_t*s,std::size_t rows,std::size_t cols,const float*x,float*y){
#ifdef REGO_PROBE
 const unsigned core=xPortGetCoreID();
 const unsigned bit=rows==56?1u:rows==112?2u:rows==224?4u:rows==448?8u:rows==672?16u:rows==1344?32u:0u;
 __atomic_fetch_or(&rego_coverage[core],bit,__ATOMIC_RELAXED);
#endif
 if(rego_enabled&&(rows==56||rows==112||rows==224||rows==448||rows==1344)&&(cols==448||cols==1344)){
  q4_register_offset(q,s,rows,cols,x,y);return;
 }
 if(row4_enabled&&((rows==448&&(cols==448||cols==1344))||(rows==1344&&cols==448))
    &&esp_ptr_external_ram(q)&&esp_ptr_external_ram(s))bp_q4_row4(q,s,rows,cols,x,y);
 else runtime_q4_rowpair2_fp16_reference(q,s,rows,cols,x,y);
}
