 
static bool bp_o_matvec(unsigned layer,const ResolvedMatrix& o,const float* input,float* out) {
    if(layer==5)return pipeMatvec(o,input,out);
    const auto split=o.spec->rows/2u;
    dualDrainDone();dual_job.kind=DualJobKind::Rows;dual_job.matrix=o;
    dual_job.input=input;dual_job.output=out;dual_job.begin=split;
    dual_job.end=o.spec->rows;dual_job.ok=false;xSemaphoreGive(dual_start);
    const bool a=dualMatvecRows(o,input,out,0u,split);const bool b=dualWait();
    return a&&b;
}
#include "two_token_matrix_ops.inc"
#include "batched_feed_forward_matrix_ops.inc"
#include "batched_sd_layer.inc"
static float (*bp_planes())[4][448] {
    auto p=reinterpret_cast<uintptr_t>(g_transformer_cache+xp::kPsramTransformerBytes);
    p=(p+15u)&~uintptr_t(15u);
    return reinterpret_cast<float (*)[4][448]>(p+16u);
}
static bool bp_guards(bool arm) {
    auto* p=reinterpret_cast<unsigned char*>(bp_planes());
    static_assert(3*4*448*4+32+15<=xp::kPsramBudgetBytes-xp::kPsramTransformerBytes);
    for(unsigned i=0;i<16;i++){
        if(arm){p[int(i)-16]=0xA5;p[21504+i]=0x5A;}
        else if(p[int(i)-16]!=0xA5||p[21504+i]!=0x5A)return false;
    }return true;
}
static bool bp_block_reference(const uint32_t* ids,size_t first,unsigned count) {
    if(!ids || !count || count>4 || first>=HELLO_MAX_CONTEXT || count>HELLO_MAX_CONTEXT-first)return false;
    auto* planes=bp_planes();auto& bh=planes[0];auto& bn=planes[1];auto& bq=planes[2];
    for(unsigned t=0;t<count;t++)if(!loadEmbeddingRow(ids[t],bh[t]))return false;
    pipe_next_q=PrefetchedMatrix{};
    if(!pipeStartNextQ(5u,g_sd_scratch))return false;
    for(unsigned t=0;t<count;t++) {
        std::memcpy(hidden_rt,bh[t],sizeof(hidden_rt));
        if(!helloRunLayerPosition(0u,first+t,false))return false;
         
         
        if(t==0){if(xSemaphoreTake(pipe_done_sem,portMAX_DELAY)!=pdTRUE)return false;
            __sync_synchronize();xSemaphoreGive(pipe_done_sem);}
        for(unsigned l=1;l<5;l++)if(!earlyDualRunPsramLayer(l,first+t))return false;
        std::memcpy(bh[t],hidden_rt,sizeof(hidden_rt));
    }
    for(unsigned l=5;l<=10;l++)if(!bp_sd_layer(l,first,count,bh,bn,bq))return false;
    if(!pipeFinishNextQ(11))return false;
    const ResolvedMatrix q=pipe_next_q.resolved;
    for(unsigned t=0;t<count;t++){
        std::memcpy(hidden_rt,bh[t],sizeof(hidden_rt));
        if(!bp_l11(first+t,q))return false;
        std::memcpy(bh[t],hidden_rt,sizeof(hidden_rt));
    }
     
    return true;
}
