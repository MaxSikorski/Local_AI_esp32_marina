 
static bool bp_matvec_pair(const ResolvedMatrix& r,const float* input,float* output) {
    if(!r.spec||r.spec->cols!=448||(r.spec->rows!=112&&r.spec->rows!=448)
       ||!r.q4||!r.scales||!input||!output
       ||(reinterpret_cast<uintptr_t>(r.q4)&3u)||(reinterpret_cast<uintptr_t>(r.scales)&1u)
       ||(reinterpret_cast<uintptr_t>(input)&15u)||(reinterpret_cast<uintptr_t>(output)&15u))return false;
    bp_q4_two(r.q4,r.scales,r.spec->rows,r.spec->cols,input,output);return true;
}
