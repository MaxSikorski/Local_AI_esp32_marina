"""Random-init 30M Base V2, immutable saved 300M schedule.

Explicit --train is required for a fresh run. --dry-run never constructs an
optimizer, loads weights, computes backward, or writes any output.
This is full next-token base pretraining, NOT assistant-masked V4 SFT.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import random
import signal
import time

import numpy as np
import torch
import torch.nn.functional as F

from pocketlm.training import audit_friend_base_v2_training_stream_v1 as audit
from pocketlm.training.pocketlm_30m import create_model_30m, EXPECTED_PARAMETERS_30M

ROOT = audit.ROOT
PLAN = audit.OUT / 'schedule_spec.json'
OUT = ROOT / 'checkpoints/_candidate_friend_base_30m_v2_300m_v1'
RUN_KIND = 'friend_base_30m_v2_300m_v1'
RECIPE = {'seed':42, 'parameters':EXPECTED_PARAMETERS_30M, 'seq_len':160,
          'batch_size':32, 'updates':58594, 'optimizer':'AdamW',
          'betas':[0.9,0.95], 'eps':1e-8, 'matrix_weight_decay':0.1,
          'other_weight_decay':0.0, 'clip':1.0, 'peak_lr':2e-4,
          'min_lr':2e-5, 'warmup_updates':1000,
          'lr_indexing':'one-based; cosine reaches min at final update',
          'objective':'full-LM shifted CE, all tokens including roles/END/EOS',
          'auxiliary':'NONE', 'initialization':'random; no starting checkpoint',
          'save_every':1000, 'eval_holdout_windows':256, 'probe_cap':40}
PROBES = ('I feel worn out after today.', "I'm bored and don't know what to do.",
          'I finally finished my project!', 'Today has been such a mess.',
          'Someone I used to know texted me.',
          'I cannot decide whether to go out or stay home.',
          'Hey, how is your day going?', "I'm heading to bed.")
V4 = ROOT / 'data/pocketlm/friend_distill_pilot_v4'
V4_SHA = {'friend_v4_val_inputs.npy':'64b248ceea1c676f9715f0c82d4d5d8b48f12c0bfcf0c51b39fe33aee7234b0d',
          'friend_v4_val_labels.npy':'831fe648e83e215c67ecfaf80d4d554beaacc429aa45b86c563748768c7479b4'}


def chat_counts():
    edges = np.arange(audit.TOTAL_UPDATES+1,dtype=np.int64)*audit.CHAT_WINDOWS//audit.TOTAL_UPDATES
    return np.diff(edges)


def cursors(completed):
    if not 0 <= completed <= audit.TOTAL_UPDATES:
        raise ValueError('completed update outside schedule')
    chat = completed*audit.CHAT_WINDOWS//audit.TOTAL_UPDATES
    return completed*audit.BATCH_SIZE-chat, chat


def make_batch(update, general_ids, chat_ids, streams):
    """Zero-based update. Consecutive target windows never cross source streams."""
    if not 0 <= update < audit.TOTAL_UPDATES:
        raise ValueError('batch outside schedule')
    g,c = cursors(update)
    _,next_c = cursors(update+1)
    k=next_c-c
    a_windows=(len(streams[0])-1)//audit.SEQ_LEN
    refs=[]
    for idx in general_ids[g:g+32-k]:
        refs.append({'stream':int(idx>=a_windows),
                     'window':int(idx if idx<a_windows else idx-a_windows)})
    refs.extend({'stream':2,'window':int(idx)} for idx in chat_ids[c:next_c])
    audit.require(len(refs)==32,'complete scheduled batch')
    chunks=[]
    for ref in refs:
        start=ref['window']*160
        chunk=np.asarray(streams[ref['stream']][start:start+161],dtype=np.int64)
        audit.require(len(chunk)==161,'window has final shifted target')
        chunks.append(chunk)
    packed=np.stack(chunks)
    return packed[:,:-1].copy(), packed[:,1:].copy(), refs


def learning_rate(update):
    if not 1 <= update <= audit.TOTAL_UPDATES:
        raise ValueError('LR update outside schedule')
    if update <= 1000:
        return 2e-4*update/1000
    p=(update-1000)/(audit.TOTAL_UPDATES-1000)
    return 2e-5+(2e-4-2e-5)*(1+math.cos(math.pi*p))/2


def parser():
    p=argparse.ArgumentParser(description=__doc__)
    modes=p.add_mutually_exclusive_group(required=True)
    modes.add_argument('--dry-run',action='store_true')
    modes.add_argument('--train',action='store_true')
    modes.add_argument('--resume',action='store_true')
    p.add_argument('--device',choices=('auto','cpu','mps','cuda'),default='auto')
    return p


def choose_device(name):
    if name=='auto':
        name='cuda' if torch.cuda.is_available() else 'mps' if torch.backends.mps.is_available() else 'cpu'
    if name=='mps':audit.require(torch.backends.mps.is_available(),'MPS available')
    if name=='cuda':audit.require(torch.cuda.is_available(),'CUDA available')
    return torch.device(name)


def seed_all():
    random.seed(42);np.random.seed(42);torch.manual_seed(42)
    if torch.cuda.is_available():torch.cuda.manual_seed_all(42)
    if torch.backends.mps.is_available():torch.mps.manual_seed(42)


def prepare():
    saved=json.loads(PLAN.read_text())
    summary,general,counts=audit.build_schedule()
                                                                      
    audit.require({k:summary[k] for k in saved}==saved,'exact saved schedule match')
    chat=np.arange(audit.CHAT_WINDOWS,dtype=np.int64)
    np.random.default_rng(audit.CHAT_SHUFFLE_SEED).shuffle(chat)
    audit.require(np.array_equal(counts,chat_counts()),'integer allocation matches saved floor schedule')
    audit.require(audit.array_sha(chat)==saved['schedule_sha256']['shuffled_chat_window_ids'],'chat permutation')
    audit.require(len(np.unique(general))==len(general),'no repeated general windows')
    audit.require(len(np.unique(chat))==len(chat),'no repeated chat windows')
    streams=[np.memmap(p,dtype=np.uint16,mode='r') for p in (audit.STAGE1A_TRAIN,audit.STAGE1B_TRAIN,audit.CHAT_TRAIN)]
    v4_hash={}
    for name,expected in V4_SHA.items():
        actual=audit.sha256(V4/name)
        audit.require(actual==expected,'frozen V4 passive validation SHA: '+name)
        v4_hash[name]=actual
    vx=np.load(V4/'friend_v4_val_inputs.npy',mmap_mode='r')
    vy=np.load(V4/'friend_v4_val_labels.npy',mmap_mode='r')
    audit.require(vx.shape==vy.shape==(180,160),'V4 validation shapes')
    audit.require(vx.dtype==np.uint16 and vy.dtype==np.int16,'V4 validation dtypes')
    binding={'saved_plan_sha256':audit.sha256(PLAN),'schedule':saved,
             'v4_passive_sha256':v4_hash,'runner_sha256':audit.sha256(Path(__file__))}
    return summary,general,chat,streams,binding


def state_hash(model):
    h=hashlib.sha256()
    for name,value in model.state_dict().items():
        h.update(name.encode());h.update(value.detach().cpu().contiguous().numpy().tobytes())
    return h.hexdigest()


def dry_run(prepared,device):
    summary,general,chat,streams,binding=prepared
    checked={}
    for u in sorted({0,1,1000,29297,58593,*[v['update']-1 for v in summary['evaluation_checkpoints']]}):
        x,y,refs=make_batch(u,general,chat,streams)
        audit.require(np.array_equal(x[:,1:],y[:,:-1]),'shifted CE target consistency')
        audit.require(0<=int(x.min()) and int(y.max())<6144,'batch token range')
                                                                                  
        rx,ry,rrefs=make_batch(u,general,chat,streams)
        audit.require(np.array_equal(x,rx) and np.array_equal(y,ry) and refs==rrefs,'resume batch identity')
        checked[str(u+1)]={'shape':list(x.shape),'chat':sum(r['stream']==2 for r in refs),
                           'lr':learning_rate(u+1),'window_refs':refs}
    seed_all()
    model=create_model_30m().to(device).eval()
    before=state_hash(model)
    x,y,_=make_batch(0,general,chat,streams)
    with torch.no_grad():
        logits,_=model(torch.from_numpy(x).to(device))
        loss=F.cross_entropy(logits.reshape(-1,6144),torch.from_numpy(y).to(device).reshape(-1))
        audit.require(logits.shape==(32,160,6144) and bool(torch.isfinite(loss)),'finite full-batch forward')
        value=float(loss.detach().cpu())
    audit.require(before==state_hash(model),'forward leaves weights unchanged')
    audit.require(all(p.grad is None for p in model.parameters()),'no diagnostic gradients')
    print(json.dumps({'classification':'BASE_V2_TRAINER_DRY_RUN_PASS','device':str(device),
          'parameters':sum(p.numel() for p in model.parameters()),'recipe':RECIPE,
          'saved_schedule':summary,'batch_checks':checked,'random_init_forward_loss':value,
          'resume_check':'deterministic batch/cursors; optimizer-step equivalence NOT RUN',
          'intended_output_directory':str(OUT),'weights_unchanged':True},indent=2),flush=True)
    print('Files written: NONE\nModel weights loaded: NO\nOptimizer constructed: NO\nTraining updates: 0\nCheckpoints created: 0\nTraining: NOT RUN',flush=True)


def validate_resume(payload,binding):
    audit.require(payload['run_kind']==RUN_KIND,'resume run kind')
    audit.require(payload['binding']==binding,'resume binds exact runner/sources/schedule')
    audit.require(payload['recipe']==RECIPE,'resume recipe')
    u=payload['completed_updates']
    audit.require(isinstance(u,int) and 0<=u<=audit.TOTAL_UPDATES,'resume update count')


def capture_rng():
    result={'python':random.getstate(),'numpy':np.random.get_state(),'torch':torch.get_rng_state()}
    if torch.cuda.is_available():result['cuda']=torch.cuda.get_rng_state_all()
    if torch.backends.mps.is_available():result['mps']=torch.mps.get_rng_state()
    return result


def restore_rng(state):
    random.setstate(state['python']);np.random.set_state(state['numpy']);torch.set_rng_state(state['torch'])
    if 'cuda' in state:torch.cuda.set_rng_state_all(state['cuda'])
    if 'mps' in state:torch.mps.set_rng_state(state['mps'])


def atomic_save(payload,path,replace=False):
    audit.require(not path.is_symlink(),'no symlink output')
    if not replace:audit.require(not path.exists(),'refuse overwrite '+str(path))
    temp=path.with_name(path.name+'.tmp')
    audit.require(not temp.exists(),'no stale temporary '+str(temp))
    with temp.open('xb') as f:
        torch.save(payload,f);f.flush();os.fsync(f.fileno())
    if replace:os.replace(temp,path)
    else:os.link(temp,path);temp.unlink()


def optimizer_for(model):
    decay=[p for p in model.parameters() if p.requires_grad and p.ndim>=2]
    other=[p for p in model.parameters() if p.requires_grad and p.ndim<2]
    return torch.optim.AdamW([{'params':decay,'weight_decay':0.1},
                             {'params':other,'weight_decay':0.0}],
                            lr=2e-4,betas=(0.9,0.95),eps=1e-8,foreach=False)


@torch.no_grad()
def evaluate(model,device,update):
    import sentencepiece as spm
    model.eval()
    record={'update':update,'processed_tokens':update*5120,'holdout_nll':{}}
    for name,path in [('stage1a',audit.STAGE1A_VAL),('stage1b',audit.STAGE1B_VAL),('chat_v2',audit.CHAT_VAL)]:
        data=np.memmap(path,dtype=np.uint16,mode='r')
        starts=np.linspace(0,(len(data)-1)//160-1,256,dtype=np.int64)*160
        total=0.0
        for j in range(0,len(starts),16):
            packed=np.stack([np.asarray(data[s:s+161],dtype=np.int64) for s in starts[j:j+16]])
            logits,_=model(torch.tensor(packed[:,:-1],device=device))
            total+=float(F.cross_entropy(logits.reshape(-1,6144),torch.tensor(packed[:,1:].copy(),device=device).reshape(-1),reduction='sum').cpu())
        record['holdout_nll'][name]=total/(256*160)
    vx=np.load(V4/'friend_v4_val_inputs.npy');vy=np.load(V4/'friend_v4_val_labels.npy')
    total=0.0;tokens=0;first_ranks=[];end_ranks=[];end_ps=[]
    for j in range(0,180,16):
        x=torch.tensor(vx[j:j+16].astype(np.int64),device=device)
        y=torch.tensor(vy[j:j+16].astype(np.int64),device=device)
        logits,_=model(x)
        total+=float(F.cross_entropy(logits.reshape(-1,6144),y.reshape(-1),ignore_index=-100,reduction='sum').cpu())
        tokens+=int((y!=-100).sum().cpu())
        for row in range(len(x)):
            positions=torch.nonzero(y[row]!=-100).flatten()
            fp=int(positions[0]);ep=int(positions[-1])
            audit.require(int(y[row,ep])==9,'END final validation target')
            for pos,dest in ((fp,first_ranks),(ep,end_ranks)):
                gold=int(y[row,pos]);v=logits[row,pos]
                dest.append(int((v>v[gold]).sum().cpu())+1)
            end_ps.append(float(logits[row,ep].softmax(-1)[9].cpu()))
    record['v4']={'nll':total/tokens,'first_median_rank':float(np.median(first_ranks)),
                  'first_top1':float(np.mean(np.array(first_ranks)<=1)),
                  'first_top5':float(np.mean(np.array(first_ranks)<=5)),
                  'first_top10':float(np.mean(np.array(first_ranks)<=10)),
                  'end_mean_probability':float(np.mean(end_ps)),
                  'end_median_rank':float(np.median(end_ranks)),
                  'end_top10':float(np.mean(np.array(end_ranks)<=10))}
    tokenizer=spm.SentencePieceProcessor(model_file=str(audit.TOKENIZER))
    probes=[]
    for prompt in PROBES:
        prefix=[4]+tokenizer.encode(prompt,out_type=int)+[5]
        generated=model.generate(torch.tensor([prefix],device=device),max_new_tokens=40,
                                 temperature=1.0,top_k=1,eos_token_id=9)
        ids=generated[0].cpu().tolist()[len(prefix):]
        visible=[]
        for tok in ids:
            if tok in (3,9):break
            visible.append(tok)
        grams=[tuple(visible[i:i+3]) for i in range(max(0,len(visible)-2))]
        reply=tokenizer.decode(visible).strip()
        probes.append({'prompt':prompt,'token_ids':ids,'reply':reply,
                       'end':bool(ids and ids[-1]==9),'eos_first':bool(ids and ids[0]==3),
                       'empty':not reply,'cap':len(ids)==40 and ids[-1]!=9,
                       'repeated_trigram_excess':len(grams)-len(set(grams))})
    record['probes']=probes
    record['probe_summary']={'end_count':sum(p['end'] for p in probes),
                             'empty_count':sum(p['empty'] for p in probes),
                             'eos_first_count':sum(p['eos_first'] for p in probes),
                             'repeated_trigram_excess':sum(p['repeated_trigram_excess'] for p in probes)}
    model.train()
    print('EVAL '+json.dumps(record),flush=True)
    return record


def train(prepared,device,resume):
    summary,general,chat,streams,binding=prepared
    seed_all()
    payload=None
    if resume:
        audit.require(OUT.is_dir() and not OUT.is_symlink(),'existing isolated run directory')
        payload=torch.load(OUT/'resume.pt',map_location='cpu',weights_only=False)
        validate_resume(payload,binding)
        audit.require(payload['device']==str(device),'same resume backend')
        audit.require(payload['torch_version']==torch.__version__,'same resume torch version')
        if payload['completed_updates']==58594:
            print('Run already complete; no updates executed.',flush=True);return
    else:
        audit.require(not OUT.exists() and not OUT.is_symlink(),'fresh isolated output directory')
    model=create_model_30m().to(device)
    optimizer=optimizer_for(model)
    completed=0;evaluations=[]
    if payload is not None:
        model.load_state_dict(payload['model'],strict=True)
        optimizer.load_state_dict(payload['optimizer'])
        completed=payload['completed_updates'];evaluations=payload['evaluations']
        restore_rng(payload['rng'])
        del payload
    else:
        OUT.mkdir(parents=True,exist_ok=False)
    stopped=False
    def request_stop(signum,frame):
        nonlocal stopped
        stopped=True
        print('Stop requested; saving at next completed-update boundary.',flush=True)
    previous=signal.signal(signal.SIGINT,request_stop)
    def save(update,milestone=False):
        data={'run_kind':RUN_KIND,'binding':binding,'recipe':RECIPE,'device':str(device),
              'torch_version':torch.__version__,'completed_updates':update,
              'model':{k:v.detach().cpu() for k,v in model.state_dict().items()},
              'optimizer':optimizer.state_dict(),'rng':capture_rng(),'evaluations':evaluations,
              'checkpoint_selected':False}
        atomic_save(data,OUT/'resume.pt',replace=True)
        if milestone:atomic_save(data,OUT/f'base_v2_update_{update:06d}.pt')
    milestones={v['update'] for v in summary['evaluation_checkpoints']}
    started=time.monotonic();initial=completed
    try:
        if completed==0 and not evaluations:
            evaluations.append(evaluate(model,device,0));save(0,milestone=True)
        for u in range(completed,58594):
            if stopped:break
            x,y,_=make_batch(u,general,chat,streams)
            lr=learning_rate(u+1)
            for group in optimizer.param_groups:group['lr']=lr
            optimizer.zero_grad(set_to_none=True)
            logits,_=model(torch.from_numpy(x).to(device))
            loss=F.cross_entropy(logits.reshape(-1,6144),torch.from_numpy(y).to(device).reshape(-1))
            audit.require(bool(torch.isfinite(loss)),'finite train loss')
            loss.backward()
            norm=torch.nn.utils.clip_grad_norm_(model.parameters(),1.0,error_if_nonfinite=True)
            optimizer.step()
            completed=u+1
            if completed%100==0:
                rate=(completed-initial)*5120/max(1e-6,time.monotonic()-started)
                print(f'TRAIN {completed}/58594 loss={float(loss.detach().cpu()):.5f} lr={lr:.3e} grad={float(norm.detach().cpu()):.3f} tok/s={rate:,.0f}',flush=True)
            del logits,loss
            if completed in milestones:
                evaluations.append(evaluate(model,device,completed));save(completed,milestone=True)
            elif completed%1000==0 or stopped:save(completed)
        if stopped:save(completed)
        else:
            report={'status':'COMPLETE; MANUAL SEMANTIC REVIEW REQUIRED','completed_updates':completed,
                    'checkpoint_selected':False,'evaluations':evaluations,'binding':binding,'recipe':RECIPE}
            audit.write_new(OUT/'summary.json',json.dumps(report,indent=2)+'\n')
        print(f'Friend Base V2 run: {"PAUSED" if stopped else "COMPLETE"}\nCompleted updates: {completed}\nV4 fine-tuning: NOT RUN\nCheckpoint selected: NO',flush=True)
    finally:signal.signal(signal.SIGINT,previous)


def main():
    args=parser().parse_args()
    prepared=prepare()
    device=choose_device(args.device)
    if args.dry_run:dry_run(prepared,device)
    else:train(prepared,device,args.resume)


if __name__=='__main__':main()
