 
 
static bool bp_sd_layer(unsigned layer, size_t first, unsigned count,
                        float bh[4][448], float bn[4][448], float bq[4][448]) {
    if(layer<5 || layer>10 || !count || count>4 || first>=HELLO_MAX_CONTEXT
       || count>HELLO_MAX_CONTEXT-first) return false;
    if(!loadNormRow(layer*2u*NORM_ROW_BYTES,attn_norm_weight_rt)
       || !loadNormRow((layer*2u+1u)*NORM_ROW_BYTES,ffn_norm_weight_rt)) return false;
    for(unsigned t=0;t<count;t++) runtimeRmsNorm(bh[t],attn_norm_weight_rt,bn[t],cfg::kDModel);
    if(!pipeFinishNextQ(layer)) return false;
    const ResolvedMatrix q=pipe_next_q.resolved;
    auto* qb=pipe_next_q.buffer;
    auto* other=qb==g_sd_scratch?pipe_scratch2:g_sd_scratch;
    ResolvedMatrix k{},v{},o{};
    if(layer==5 || layer==10) {
        if(layer==10) {if(!k10_precache_ready)return false;k=k10_precache_matrix;}
        else if(!resolveMatrix(layer,xp::MatrixFamily::KProj,other,SD_SCRATCH_BYTES,&k))return false;
        if(!pipeStartRead(layer,xp::MatrixFamily::VProj,other,SD_SCRATCH_BYTES))return false;
        for(unsigned t=0;t<count;) {
            if(t+1<count){if(!bp_matvec_pair(q,bn[t],bq[t]))return false;t+=2;}
            else {if(!pipeMatvec(q,bn[t],bq[t]))return false;t++;}
        }
    } else {
        if(!pipeStartRead(layer,xp::MatrixFamily::KProj,other,SD_SCRATCH_BYTES))return false;
        for(unsigned t=0;t<count;) {
            if(t+1<count){if(!bp_matvec_pair(q,bn[t],bq[t]))return false;t+=2;}
            else {if(!pipeMatvec(q,bn[t],bq[t]))return false;t++;}
        }
        if(!pipeFinishRead(&k,nullptr))return false;
        if(!pipeStartRead(layer,xp::MatrixFamily::VProj,qb,SD_SCRATCH_BYTES))return false;
    }
    for(unsigned t=0;t<count;) {
        if(t+1<count) {
            if(!bp_matvec_pair(k,bn[t],gate_rt))return false;
            for(unsigned j=0;j<2;j++)std::memcpy(hello_kv_k+helloKvOffset(layer,first+t+j,0),gate_rt+j*448,sizeof(k_rt));
            t+=2;
        } else {
            if(!pipeMatvec(k,bn[t],k_rt))return false;
            std::memcpy(hello_kv_k+helloKvOffset(layer,first+t,0),k_rt,sizeof(k_rt));t++;
        }
    }
    if(!pipeFinishRead(&v,nullptr))return false;
     
    auto* ob=(layer==5||layer==10)?qb:other;
    if(layer>=6 && layer<=9 && !pipeStartRead(layer,xp::MatrixFamily::OProj,ob,SD_SCRATCH_BYTES))return false;
    for(unsigned t=0;t<count;) {
        const unsigned take=t+1<count?2u:1u;
        if(take==2){if(!bp_matvec_pair(v,bn[t],gate_rt))return false;}
        else if(!pipeMatvec(v,bn[t],v_rt))return false;
        for(unsigned j=0;j<take;j++) {
            if(take==2)std::memcpy(v_rt,gate_rt+j*448,sizeof(v_rt));
            std::memcpy(q_rt,bq[t+j],sizeof(q_rt));
            std::memcpy(k_rt,hello_kv_k+helloKvOffset(layer,first+t+j,0),sizeof(k_rt));
            helloApplyRopeQk(first+t+j);
            helloStoreCurrentKv(layer,first+t+j);
            helloAttentionFromCache(layer,first+t+j);
            std::memcpy(bq[t+j],attention_rt,sizeof(attention_rt));
        }
        t+=take;
    }
    if(layer==10) {if(!app_tail_ready)return false;o=app_tail_o10;}
    else if(layer==5) {if(!resolveMatrix(layer,xp::MatrixFamily::OProj,ob,SD_SCRATCH_BYTES,&o))return false;}
    else if(!pipeFinishRead(&o,nullptr))return false;
    for(unsigned t=0;t<count;t++) {
        if(!bp_o_matvec(layer,o,bq[t],o_rt))return false;
        for(size_t i=0;i<cfg::kDModel;i++)bh[t][i]+=o_rt[i];
    }
     
    if(!pipeStartNextQ(layer+1,qb))return false;
     
     
     
    unsigned t=0;
    for(;t+1<count;t+=2) {
        float* g1=reinterpret_cast<float*>(bq);
        float* u1=reinterpret_cast<float*>(bn)+448;
        runtimeRmsNorm(bh[t],ffn_norm_weight_rt,normed_rt,448);
        runtimeRmsNorm(bh[t+1],ffn_norm_weight_rt,bn[0],448);
        if(!bp_run_ffn_pair(layer,xp::MatrixFamily::GateProj,normed_rt,bn[0],gate_rt,g1)
           ||!bp_run_ffn_pair(layer,xp::MatrixFamily::UpProj,normed_rt,bn[0],up_rt,u1))return false;
        for(size_t i=0;i<1344;i++) {
            const float g0=gate_rt[i],g=g1[i];
            gate_rt[i]=(g0/(1.0f+expf(-g0)))*up_rt[i];
            g1[i]=(g/(1.0f+expf(-g)))*u1[i];
        }
        if(!bp_run_ffn_pair(layer,xp::MatrixFamily::DownProj,gate_rt,g1,down_rt,bn[0]))return false;
        for(size_t i=0;i<448;i++){bh[t][i]+=down_rt[i];bh[t+1][i]+=bn[0][i];}
        if(!finiteVector(bh[t],448)||!finiteVector(bh[t+1],448))return false;
    }
    for(;t<count;t++) {
        runtimeRmsNorm(bh[t],ffn_norm_weight_rt,normed_rt,cfg::kDModel);
        if(!runMatrix(layer,xp::MatrixFamily::GateProj,normed_rt,gate_rt)
           || !runMatrix(layer,xp::MatrixFamily::UpProj,normed_rt,up_rt))return false;
        for(size_t i=0;i<cfg::kDff;i++) {
            const float g=gate_rt[i];
            gate_rt[i]=(g/(1.0f+expf(-g)))*up_rt[i];
        }
        if(!runMatrix(layer,xp::MatrixFamily::DownProj,gate_rt,down_rt))return false;
        for(size_t i=0;i<cfg::kDModel;i++)bh[t][i]+=down_rt[i];
        if(!finiteVector(bh[t],cfg::kDModel))return false;
    }
    return true;
}
