import {randomBytes,createHash,timingSafeEqual} from 'node:crypto';
import {seal,unseal} from './crypto.js';
import {id,guard,liveTarget} from './safety.js';
const SCOPES=['tweet.read','users.read','tweet.write'];
class SafeError extends Error {constructor(message,status=400,extra={}){super(message);this.status=status;this.extra=extra;}}
function config(env) {
  const origin=env.APP_ORIGIN;
  let url;try {url=new URL(origin);}catch{throw new SafeError('운영자가 웹 인증 설정을 준비 중입니다.',503);}
  if(url.origin!==origin || (url.protocol!=='https:' && !(url.protocol==='http:' && ['localhost','127.0.0.1'].includes(url.hostname))))throw new SafeError('서버 주소 설정이 잘못되었습니다.',503);
  if(!/^[a-f0-9]{64}$/i.test(env.SESSION_ENCRYPTION_KEY || ''))throw new SafeError('운영자가 웹 인증 설정을 준비 중입니다.',503);
  return {origin,key:env.SESSION_ENCRYPTION_KEY,clientId:env.X_CLIENT_ID || '',secret:env.X_CLIENT_SECRET || '',enabled:env.EXECUTION_ENABLED==='true'};
}
function cookie(name,value,cfg,seconds) {return `${name}=${value}; Path=/; HttpOnly; SameSite=Lax; Max-Age=${seconds}${cfg.origin.startsWith('https:')?'; Secure':''}`;}
function cookies(req) {return Object.fromEntries((req.headers.cookie || '').split(';').map(s=>s.trim().split('=')).filter(x=>x.length===2));}
function personalConfig(req,cfg) {
  const token=cookies(req).xc_config;
  if(!token)return {};
  try{return {...unseal(token,'api-config',cfg.key),personal:true};}catch{return {};}
}
export async function handle(action,req,res,{env=process.env,fetcher=globalThis.fetch}={}) {
  res.setHeader('Cache-Control','no-store');res.setHeader('X-Content-Type-Options','nosniff');
  const send=(status,data)=>{res.statusCode=status;res.setHeader('Content-Type','application/json; charset=utf-8');res.end(JSON.stringify(data));};
  try {
    if(action==='session') {
      if(req.method!=='GET')throw new SafeError('허용하지 않는 요청입니다.',405);
      try {const cfg=config(env), session=unseal(cookies(req).xc_session,'session',cfg.key);return send(200,{authenticated:true,user:session.user,executionEnabled:cfg.enabled});}
      catch {return send(200,{authenticated:false,configured:!!(env.X_CLIENT_ID && env.SESSION_ENCRYPTION_KEY),executionEnabled:env.EXECUTION_ENABLED==='true'});}
    }
    const cfg=config(env);
    Object.assign(cfg,personalConfig(req,cfg));
    if(action==='settings' && req.method==='GET')return send(200,{personal:!!cfg.personal,clientId:cfg.personal?cfg.clientId:'',secretSaved:cfg.personal?!!cfg.secret:false,siteConfigured:!!env.X_CLIENT_ID,callback:`${cfg.origin}/api/auth-callback`});
    if(action!=='auth-callback') {
      if(req.method!=='POST')throw new SafeError('허용하지 않는 요청입니다.',405);
      if(req.headers.origin!==cfg.origin || (req.headers['sec-fetch-site'] && req.headers['sec-fetch-site']!=='same-origin'))throw new SafeError('다른 사이트의 요청을 차단했습니다.',403);
      if(!String(req.headers['content-type'] || '').startsWith('application/json'))throw new SafeError('JSON 요청이 필요합니다.',415);
      if(Number(req.headers['content-length'] || 0)>65536 || Buffer.byteLength(JSON.stringify(req.body || {}))>65536)throw new SafeError('요청이 너무 큽니다.',413);
    }
    const body=req.body || {};
    if(action==='settings') {
      if(body.useSiteApp===true){res.setHeader('Set-Cookie',[cookie('xc_config','',cfg,0),cookie('xc_session','',cfg,0)]);return send(200,{ok:true});}
      const clientId=body.clientId,secret=body.clearSecret?'':body.clientSecret || (cfg.personal && cfg.clientId===body.clientId?cfg.secret:'');
      if(typeof clientId!=='string' || !clientId || clientId.length>512 || /\s|\x00/.test(clientId) || typeof secret!=='string' || secret.length>512 || /\s|\x00/.test(secret))throw new SafeError('Client ID와 Client Secret 형식을 확인하세요.');
      res.setHeader('Set-Cookie',[cookie('xc_config',seal({clientId,secret},'api-config',cfg.key,86400000),cfg,86400),cookie('xc_session','',cfg,0)]);
      return send(200,{ok:true});
    }
    const xrequest=async(path,token,method='GET',form=null,allowMissing=false)=> {
      const headers=form ? {'Content-Type':'application/x-www-form-urlencoded'} : {Authorization:`Bearer ${token}`};
      if(form && cfg.secret)headers.Authorization=`Basic ${Buffer.from(`${cfg.clientId}:${cfg.secret}`).toString('base64')}`;
      let response;try{response=await fetcher(`https://api.x.com${path}`,{method,headers,body:form?.toString(),redirect:'error',signal:AbortSignal.timeout(15000)});}catch{throw new SafeError('X 응답을 확인할 수 없습니다. 요청 결과를 재검증하세요.',502,{uncertain:method==='DELETE'});}
      if(response.status===429) {
        let problem;try{problem=await response.json();}catch{}
        if(/usage-capped|usage_cap|credits?|billing/i.test(JSON.stringify(problem || {})))throw new SafeError('X 결제·사용량 한도를 확인하세요. 자동 재시도를 중단합니다.',402);
        const reset=Number(response.headers.get('x-rate-limit-reset'))*1000, retry=Number(response.headers.get('retry-after'));
        const waitUntil=Number.isFinite(reset)&&reset>Date.now()?reset:Number.isFinite(retry)&&retry>0?Date.now()+retry*1000:null;
        throw new SafeError(waitUntil?'X 요청 제한에 도달했습니다. 안내된 시각 이후 재시도합니다.':'X 사용량 또는 요청 제한입니다. 자동 재시도를 중단합니다.',429,{waitUntil});
      }
      if([401,402,403].includes(response.status))throw new SafeError('X 인증·권한·결제 설정을 확인하세요. 반복 요청을 중단했습니다.',response.status);
      if(response.status===404)return null;
      if(!response.ok)throw new SafeError('X가 요청을 처리하지 못했습니다. 작업을 중단했습니다.',502,{uncertain:method==='DELETE'});
      let data;try{data=await response.json();}catch{throw new SafeError('X 응답을 확인할 수 없습니다.',502,{uncertain:method==='DELETE'});}
      if(data.errors?.length && (!allowMissing || data.errors.some(e=>!String(e.type).endsWith('/resource-not-found'))))throw new SafeError('X가 일부 항목을 확인하지 못했습니다. 실행을 차단합니다.',400);
      return data;
    };
    if(action==='auth-start') {
      if(!cfg.clientId)throw new SafeError('API 설정 화면에서 내 Client ID를 저장하세요.');
      if(!cfg.personal && env.PUBLIC_OAUTH_ENABLED!=='true') {
        const supplied=createHash('sha256').update(typeof body.accessCode==='string'?body.accessCode:'').digest();
        const required=createHash('sha256').update(env.ACCESS_CODE || randomBytes(32).toString('hex')).digest();
        if(!env.ACCESS_CODE || env.ACCESS_CODE.length<16 || !timingSafeEqual(supplied,required))throw new SafeError('운영자가 제공한 사용 코드를 입력하세요.',403);
      }
      const verifier=randomBytes(48).toString('base64url'),state=randomBytes(32).toString('base64url');
      res.setHeader('Set-Cookie',cookie('xc_oauth',seal({verifier,state,clientId:cfg.clientId,secret:cfg.secret},'oauth',cfg.key,600000),cfg,600));
      const url=new URL('https://x.com/i/oauth2/authorize');
      url.search=new URLSearchParams({response_type:'code',client_id:cfg.clientId,redirect_uri:`${cfg.origin}/api/auth-callback`,scope:SCOPES.join(' '),state,code_challenge:createHash('sha256').update(verifier).digest('base64url'),code_challenge_method:'S256'}).toString();
      return send(200,{url:url.toString()});
    }
    if(action==='auth-callback') {
      if(req.method!=='GET')throw new SafeError('허용하지 않는 요청입니다.',405);
      res.setHeader('Set-Cookie',cookie('xc_oauth','',cfg,0));
      const state=unseal(cookies(req).xc_oauth,'oauth',cfg.key),url=new URL(req.url,cfg.origin);
      if(url.searchParams.getAll('state').length!==1 || url.searchParams.get('state')!==state.state || url.searchParams.getAll('code').length!==1 || !url.searchParams.get('code') || url.searchParams.has('error'))throw new SafeError('X 승인 또는 state 검증에 실패했습니다. 다시 연결하세요.');
      if(state.clientId){cfg.clientId=state.clientId;cfg.secret=state.secret || '';}
      const form=new URLSearchParams({grant_type:'authorization_code',code:url.searchParams.get('code'),redirect_uri:`${cfg.origin}/api/auth-callback`,code_verifier:state.verifier,client_id:cfg.clientId});
      const token=await xrequest('/2/oauth2/token',null,'POST',form);
      if(!token?.access_token || token.token_type?.toLowerCase()!=='bearer' || !SCOPES.every(s=>token.scope?.split(' ').includes(s)) || !Number.isFinite(token.expires_in) || token.expires_in<=0)throw new SafeError('필요한 X 권한 또는 토큰 유효기간을 확인할 수 없습니다.');
      const me=await xrequest('/2/users/me',token.access_token);
      if(!me?.data?.username)throw new SafeError('인증 계정을 확인할 수 없습니다.');
      const user={id:id(me.data.id),username:me.data.username},seconds=Math.min(Math.floor(token.expires_in),7200);
      res.setHeader('Set-Cookie',[cookie('xc_oauth','',cfg,0),cookie('xc_session',seal({token:token.access_token,user},'session',cfg.key,seconds*1000),cfg,seconds)]);
      res.statusCode=303;res.setHeader('Location',`${cfg.origin}/?connected=1`);return res.end();
    }
    if(action==='logout') {res.setHeader('Set-Cookie',cookie('xc_session','',cfg,0));return send(200,{ok:true});}
    let session;try{session=unseal(cookies(req).xc_session,'session',cfg.key);}catch{throw new SafeError('X에 다시 연결하세요.',401);}
    const lookup=async(ids)=>xrequest(`/2/tweets?ids=${ids.map(id).join(',')}&tweet.fields=author_id,referenced_tweets,edit_history_tweet_ids,created_at`,session.token,'GET',null,true);
    if(action==='plan') {
      const plan=body.plan;
      if(!plan || id(plan.owner?.id)!==session.user.id || !Array.isArray(plan.targets) || !plan.targets.length || plan.targets.length>50 || !Array.isArray(plan.keeps) || plan.keeps.length>500)throw new SafeError('아카이브 소유 계정 또는 선택 목록을 확인하세요.');
      plan.keeps.forEach(id);
      if(new Set(plan.targets.map(t=>id(t.id))).size!==plan.targets.length)throw new SafeError('선택 목록의 중복 ID를 제거하세요.');
      const data=await lookup(plan.targets.map(t=>t.id));
      const targets=[],unavailable=[];
      for(const t of plan.targets){guard(plan,t,session.user.id);const record=data?.data?.find(r=>r.id===t.id);if(!record && data?.errors?.some(e=>(e.resource_id || e.value)===t.id && String(e.type).endsWith('/resource-not-found'))){unavailable.push(t.id);continue;}targets.push(liveTarget(record,t,session.user.id,plan.keeps));}
      const approved={owner:{id:session.user.id},keeps:plan.keeps,targets,fingerprint:plan.fingerprint};
      return send(200,{permit:targets.length?seal(approved,'plan',cfg.key,7200000):null,targets,unavailable,user:session.user});
    }
    if(action==='execute') {
      if(!cfg.enabled)throw new SafeError('운영자가 실제 실행을 활성화하지 않았습니다. 미리보기만 가능합니다.',403);
      const plan=unseal(body.permit,'plan',cfg.key),target=plan.targets.find(t=>t.id===body.id);
      if(body.execute!==true || body.confirmation!==`선택한 ${plan.targets.length}개 실행`)throw new SafeError('명시적인 실행과 확인 문구가 모두 필요합니다.');
      guard(plan,target,session.user.id);
      const result=await lookup([target.id]);
      if(!result?.data?.length)return send(200,{status:'unavailable',message:'현재 조회할 수 없습니다. 이미 삭제되었는지 확정할 수 없어 삭제 요청을 보내지 않았습니다.'});
      const live=liveTarget(result.data[0],target,session.user.id,plan.keeps);
      guard(plan,live,session.user.id);
      // The wrapper still exists and belongs to this user: do not undo a new, unselected repost.
      const path=live.kind==='repost'?`/2/users/${session.user.id}/retweets/${live.sourceId}`:`/2/tweets/${live.id}`;
      const deletion=await xrequest(path,session.token,'DELETE');
      if(!deletion)return send(200,{status:'unavailable',message:'X가 대상 없음으로 응답했습니다. 이미 삭제 여부는 확정하지 않았습니다.'});
      if(live.kind==='repost'?deletion.data?.retweeted!==false:deletion.data?.deleted!==true)throw new SafeError('실행 결과를 확정할 수 없습니다. 재검증이 필요합니다.',502,{uncertain:true});
      return send(200,{status:'success',message:live.kind==='repost'?'리포스트를 취소했습니다.':'선택한 게시물을 삭제했습니다.'});
    }
    throw new SafeError('허용하지 않는 API 경로입니다.',404);
  } catch(error) {return send(error.status || 400,{error:error instanceof SafeError?error.message:(error.message?.startsWith('인증 또는') || !String(error.message).includes('token')?error.message:'요청 검증에 실패했습니다.'),...(error.extra || {})});}
}
