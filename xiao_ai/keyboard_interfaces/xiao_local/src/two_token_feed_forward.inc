 
static bool rf_resolve(unsigned layer,ResolvedMatrix& g,ResolvedMatrix& u,ResolvedMatrix& d) {
 if(layer>11||!loadNormRow((layer*2u+1u)*NORM_ROW_BYTES,ffn_norm_weight_rt))return false;
 return resolveMatrix(layer,xp::MatrixFamily::GateProj,nullptr,0,&g)
     &&resolveMatrix(layer,xp::MatrixFamily::UpProj,nullptr,0,&u)
     &&resolveMatrix(layer,xp::MatrixFamily::DownProj,nullptr,0,&d)
     &&rf_valid(g,1344,448)&&rf_valid(u,1344,448)&&rf_valid(d,448,1344);
}
static bool rf_submit(DualJobKind kind,const ResolvedMatrix& r,const float* x0,const float* x1,
                      float* y0,float* y1,size_t first,size_t end) {
 dualDrainDone();dual_job.kind=kind;dual_job.matrix=r;dual_job.input=x0;dual_job.input2=x1;
 dual_job.output=y0;dual_job.output2=y1;dual_job.begin=first;dual_job.end=end;dual_job.ok=false;
 __sync_synchronize();
 if(xSemaphoreGive(dual_start)!=pdTRUE){prod_runtime_ready=false;return false;}return true;
}
static bool rf_pair(unsigned layer,float* h0,float* h1,float bn[4][448],float bq[4][448]) {
 ResolvedMatrix g{},u{},d{};if(!rf_resolve(layer,g,u,d))return false;
 float* n1=bn[0];float* u1=reinterpret_cast<float*>(bn)+448;float* g1=reinterpret_cast<float*>(bq);
 runtimeRmsNorm(h0,ffn_norm_weight_rt,normed_rt,448);runtimeRmsNorm(h1,ffn_norm_weight_rt,n1,448);
 bool ok;
 if(layer==0) {
  if(!rf_rows(g,normed_rt,n1,gate_rt,g1,0,1344)||!rf_rows(u,normed_rt,n1,up_rt,u1,0,1344))return false;
  rf_sw(gate_rt,up_rt,0,1344);rf_sw(g1,u1,0,1344);
  if(!rf_rows(d,gate_rt,g1,down_rt,n1,0,448))return false;
 } else {
  if(!rf_submit(DualJobKind::FfnPairRows,u,normed_rt,n1,up_rt,u1,0,1344))return false;
  ok=rf_rows(g,normed_rt,n1,gate_rt,g1,0,1344);const bool upok=dualWait();if(!ok||!upok)return false;
  if(!rf_submit(DualJobKind::FfnSwiGlu,ResolvedMatrix{},u1,nullptr,g1,nullptr,0,1344))return false;
  rf_sw(gate_rt,up_rt,0,1344);if(!dualWait())return false;
  if(!rf_submit(DualJobKind::FfnPairRows,d,gate_rt,g1,down_rt,n1,224,448))return false;
  ok=rf_rows(d,gate_rt,g1,down_rt,n1,0,224);const bool downok=dualWait();if(!ok||!downok)return false;
 }
 for(size_t i=0;i<448;i++){h0[i]+=down_rt[i];h1[i]+=n1[i];}
 return finiteVector(h0,448)&&finiteVector(h1,448);
}
 
static bool rf_single(unsigned layer,float* h) {
 ResolvedMatrix g{},u{},d{};if(!rf_resolve(layer,g,u,d))return false;
 runtimeRmsNorm(h,ffn_norm_weight_rt,normed_rt,448);
 if(layer==0) {
  if(!pipeMatvec(g,normed_rt,gate_rt)||!pipeMatvec(u,normed_rt,up_rt))return false;
  rf_sw(gate_rt,up_rt,0,1344);if(!pipeMatvec(d,gate_rt,down_rt))return false;
 } else {
  if(!rf_submit(DualJobKind::Full,u,normed_rt,nullptr,up_rt,nullptr,0,1344))return false;
  const bool a=pipeMatvec(g,normed_rt,gate_rt);const bool b=dualWait();if(!a||!b)return false;
  if(!rf_submit(DualJobKind::SwiGlu,ResolvedMatrix{},nullptr,nullptr,nullptr,nullptr,672,1344))return false;
  rf_sw(gate_rt,up_rt,0,672);if(!dualWait())return false;
  if(!rf_submit(DualJobKind::Rows,d,gate_rt,nullptr,down_rt,nullptr,224,448))return false;
  const bool c=dualMatvecRows(d,gate_rt,down_rt,0,224);const bool e=dualWait();if(!c||!e)return false;
 }
 for(size_t i=0;i<448;i++)h[i]+=down_rt[i];return finiteVector(h,448);
}
