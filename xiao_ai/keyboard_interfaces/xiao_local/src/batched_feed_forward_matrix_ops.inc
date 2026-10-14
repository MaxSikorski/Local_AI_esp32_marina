 
static bool bp_run_ffn_pair(unsigned layer,xp::MatrixFamily family,
    const float* x0,const float* x1,float* y0,float* y1) {
    if(layer<5||layer>10||(family!=xp::MatrixFamily::GateProj
       &&family!=xp::MatrixFamily::UpProj&&family!=xp::MatrixFamily::DownProj))return false;
    ResolvedMatrix r{};
    if(!resolveMatrix(layer,family,nullptr,0,&r)||(r.location!=xp::MatrixLocation::Psram&&r.location!=xp::MatrixLocation::Flash)
       ||!r.spec||!r.q4||!r.scales||!x0||!x1||!y0||!y1)return false;
    const bool down=family==xp::MatrixFamily::DownProj;
    if(r.spec->rows!=(down?448u:1344u)||r.spec->cols!=(down?1344u:448u)
       ||(reinterpret_cast<uintptr_t>(r.q4)&3u)||(reinterpret_cast<uintptr_t>(r.scales)&1u)
       ||((reinterpret_cast<uintptr_t>(x0)|reinterpret_cast<uintptr_t>(x1)
          |reinterpret_cast<uintptr_t>(y0)|reinterpret_cast<uintptr_t>(y1))&15u))return false;
    const float* inputs[2]={x0,x1};float* outputs[2]={y0,y1};
    bp_q4_ffn(r.q4,r.scales,r.spec->rows,r.spec->cols,inputs,outputs);return true;
}
