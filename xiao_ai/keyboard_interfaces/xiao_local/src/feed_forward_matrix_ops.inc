static bool rf_valid(const ResolvedMatrix& r,size_t rows,size_t cols) {
 return r.spec&&r.q4&&r.scales&&r.spec->rows==rows&&r.spec->cols==cols
    &&(r.location==xp::MatrixLocation::Psram||r.location==xp::MatrixLocation::Flash)
    &&!(reinterpret_cast<uintptr_t>(r.q4)&3u)&&!(reinterpret_cast<uintptr_t>(r.scales)&1u);
}
static bool rf_rows(const ResolvedMatrix& r,const float* x0,const float* x1,float* y0,float* y1,size_t begin,size_t end) {
 if(!r.spec||!((r.spec->rows==1344&&r.spec->cols==448)||(r.spec->rows==448&&r.spec->cols==1344))
    ||!rf_valid(r,r.spec->rows,r.spec->cols)||!x0||!x1||!y0||!y1||begin>=end||end>r.spec->rows
    ||((begin|end)&1u)||((reinterpret_cast<uintptr_t>(x0)|reinterpret_cast<uintptr_t>(x1)
      |reinterpret_cast<uintptr_t>(y0)|reinterpret_cast<uintptr_t>(y1))&15u))return false;
 const float* xx[2]={x0,x1};float* yy[2]={y0+begin,y1+begin};
 bp_q4_ffn(r.q4+begin*(r.spec->cols/2),r.scales+begin*(r.spec->cols/64),end-begin,r.spec->cols,xx,yy);
 return true;
}
static void rf_sw(float* g,const float* u,size_t begin,size_t end) {
 for(size_t i=begin;i<end;i++){const float v=g[i];g[i]=(v/(1.0f+expf(-v)))*u[i];}
}
