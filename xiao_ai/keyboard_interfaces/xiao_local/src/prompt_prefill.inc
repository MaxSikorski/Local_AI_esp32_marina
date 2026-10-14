static bool bp_prefill(const uint32_t* ids,size_t count,uint32_t& elapsed_us) {
    if(!prod_runtime_ready||!g_transformer_cache||!ids||!count
       ||count>=PROD_CONTEXT-PROD_MAX_GENERATE)return false;
    const uint32_t start=micros();
    bp_guards(true);
    for(size_t pos=0;pos<count;) {
        const unsigned n=static_cast<unsigned>(std::min(size_t(4),count-pos));
        const bool ok=n==1?appTailL11DualRunToken(ids[pos],pos,nullptr):bp_block(ids+pos,pos,n);
        if(!ok){prod_runtime_ready=false;return false;}
        pos+=n;yield();
    }
    const bool guards=bp_guards(false);
    elapsed_us+=micros()-start;
    if(!guards)prod_runtime_ready=false;
    return guards;
}
