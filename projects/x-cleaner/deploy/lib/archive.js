import JSZip from 'jszip';
import {parse, isLosslessNumber} from 'lossless-json';
import {id} from './safety.js';
const filename = /^(account|tweets?|tweet-headers)(?:-part(\d+))?\.(js|json)$/;
const MAX_FILE = 32 * 1024 * 1024;
const MAX_TOTAL = 96 * 1024 * 1024;
function archiveId(v) { return id(isLosslessNumber(v) ? v.toString() : v); }
export function parseData(text, family, part = 0) {
  text = text.replace(/^\uFEFF/, '').trim();
  if (!text.startsWith('[')) {
    const match = text.match(/^window\.YTD\.([a-zA-Z_]+)\.part(\d+)\s*=\s*/);
    const allowed = family === 'tweet-headers' ? ['tweet_headers'] : family === 'account' ? ['account'] : ['tweet','tweets'];
    if (!match || !allowed.includes(match[1]) || Number(match[2]) !== part) throw new Error('지원하지 않는 아카이브 형식입니다.');
    text = text.slice(match[0].length).replace(/;\s*$/, '');
  }
  const data = parse(text);
  if (!Array.isArray(data)) throw new Error('아카이브 데이터가 배열이 아닙니다.');
  return data;
}
export function recordsFromData(data) {
  return data.map(row => {
    const r = row.tweet;
    if (!r || typeof r !== 'object') throw new Error('게시물 데이터를 읽을 수 없습니다.');
    const key = archiveId(r.id_str ?? r.id);
    if (r.id != null && archiveId(r.id) !== key) throw new Error('게시물 ID 정보가 충돌합니다.');
    const source = r.retweeted_status;
    const sourceId = source ? archiveId(source.id_str ?? source.id) : null;
    const text = r.full_text ?? r.text ?? '';
    if (typeof text !== 'string') throw new Error('게시물 본문 형식이 잘못되었습니다.');
    const kind = source || /^RT @\w+:/.test(text) ? 'repost' : (r.in_reply_to_status_id_str ?? r.in_reply_to_status_id) != null ? 'reply' : 'post';
    return {id:key,kind,sourceId,text,createdAt:typeof r.created_at === 'string' ? r.created_at : '',inferred:kind === 'repost' && !source};
  });
}
export async function buildArchive(entries) {
  const records = new Map(), parts = new Set(), roots = new Set();
  let owner = {id:null,username:''}, accountSeen = false, headers = null, total = 0;
  for (const {name,read} of entries) {
    if (name.startsWith('/') || name.includes('\\') || name.split('/').includes('..')) throw new Error('안전하지 않은 아카이브 경로입니다.');
    const match = name.split('/').at(-1).match(filename);
    if (!match) continue;
    roots.add(name.slice(0,name.lastIndexOf('/')));
    if (roots.size > 1) throw new Error('여러 데이터 폴더가 섞여 있습니다.');
    const raw = await read();
    total += raw.byteLength;
    if (raw.byteLength > MAX_FILE || total > MAX_TOTAL) throw new Error('데이터 크기 제한(파일 32 MiB, 합계 96 MiB)을 초과했습니다. 개인용 CLI를 사용하세요.');
    const family = match[1], part = Number(match[2] ?? 0);
    const data = parseData(new TextDecoder('utf-8',{fatal:true}).decode(raw),family,part);
    if (family === 'account') {
      if (accountSeen || data.length !== 1) throw new Error('계정 데이터가 중복되거나 잘못되었습니다.');
      const account = data[0].account;
      owner = {id:archiveId(account.accountId),username:typeof account.username === 'string' ? account.username : ''};
      accountSeen = true;
    } else if (family === 'tweet-headers') {
      if (headers !== null || part !== 0) throw new Error('중복되거나 지원하지 않는 헤더 파일입니다.');
      headers = data.map(row => archiveId((row.tweet || row.tweet_header || row).tweetId ?? (row.tweet || row.tweet_header || row).id_str ?? (row.tweet || row.tweet_header || row).id));
    } else {
      if (parts.has(part)) throw new Error('중복 게시물 분할 파일입니다.');
      parts.add(part);
      for (const record of recordsFromData(data)) {
        if (records.has(record.id)) throw new Error('중복 게시물 ID가 발견되었습니다.');
        records.set(record.id,record);
      }
    }
  }
  if (!parts.size) throw new Error('data/tweets.js 또는 tweet(s)-partN.js 파일을 찾을 수 없습니다.');
  const sorted = [...parts].sort((a,b)=>a-b);
  if (sorted.some((p,i)=>p !== i)) throw new Error('게시물 분할 파일이 누락되었습니다.');
  if (headers && (new Set(headers).size !== headers.length || headers.length !== records.size || headers.some(h=>!records.has(h)))) throw new Error('헤더와 본문 ID 목록이 다릅니다.');
  const list = [...records.values()];
  const payload = JSON.stringify({owner,records:list});
  const hash = await globalThis.crypto.subtle.digest('SHA-256',new TextEncoder().encode(payload));
  return {owner,records:list,fingerprint:[...new Uint8Array(hash)].map(b=>b.toString(16).padStart(2,'0')).join('')};
}
function limitedZipRead(file) {
  return new Promise((resolve,reject)=> {
    const chunks=[]; let size=0;
    const stream=file.internalStream('uint8array');
    stream.on('data',chunk=> {size+=chunk.length; if(size>MAX_FILE){stream.pause();reject(new Error('압축 데이터 크기 제한을 초과했습니다.'));return;} chunks.push(chunk);});
    stream.on('error',reject);
    stream.on('end',()=>{const result=new Uint8Array(size);let offset=0;for(const c of chunks){result.set(c,offset);offset+=c.length;}resolve(result);});
    stream.resume();
  });
}
export async function readFiles(files) {
  files = [...files];
  if (files.length === 1 && /\.zip$/i.test(files[0].name)) {
    if (files[0].size > 128 * 1024 * 1024) throw new Error('ZIP은 최대 128 MiB입니다. 큰 아카이브는 압축을 푼 폴더로 선택하세요.');
    const buffer=await files[0].arrayBuffer();
    inspectZip(buffer);
    const zip = await JSZip.loadAsync(buffer);
    const entries = Object.values(zip.files).filter(f=>!f.dir);
    let total=0;
    for(const f of entries) {
      const original=f.unsafeOriginalName || f.name;
      if(original.startsWith('/') || original.includes('\\') || original.split('/').includes('..') || ((f.unixPermissions >>> 12) & 15) === 10) throw new Error('안전하지 않은 ZIP 항목입니다.');
      if(filename.test(f.name.split('/').at(-1))) {total+=f._data?.uncompressedSize || 0;if(total>MAX_TOTAL || f._data?.uncompressedSize>MAX_FILE) throw new Error('압축 데이터 크기 제한을 초과했습니다.');}
    }
    return buildArchive(entries.map(f=>({name:f.name,read:()=>limitedZipRead(f)})));
  }
  return buildArchive(files.map(f=>({name:f.webkitRelativePath || f.name,read:async()=>{if(f.size>MAX_FILE)throw new Error('데이터 파일이 너무 큽니다.');return new Uint8Array(await f.arrayBuffer());}})));
}
export function inspectZip(buffer) {
  const view=new DataView(buffer),length=buffer.byteLength;
  let end=-1;
  for(let i=length-22;i>=Math.max(0,length-65557);i--)if(view.getUint32(i,true)===0x06054b50 && i+22+view.getUint16(i+20,true)===length){end=i;break;}
  if(end<0)throw new Error('ZIP 중앙 디렉터리를 읽을 수 없습니다.');
  const count=view.getUint16(end+10,true),size=view.getUint32(end+12,true),offset=view.getUint32(end+16,true);
  if(view.getUint16(end+4,true)!==0 || view.getUint16(end+6,true)!==0 || count!==view.getUint16(end+8,true) || count===65535 || count>20000 || offset+size!==end)throw new Error('분할·ZIP64 또는 너무 많은 항목이 있는 ZIP은 지원하지 않습니다. 폴더 입력을 사용하세요.');
  const seen=new Set();let pos=offset,total=0;
  for(let i=0;i<count;i++) {
    if(pos+46>end || view.getUint32(pos,true)!==0x02014b50)throw new Error('ZIP 항목이 손상되었습니다.');
    const nameSize=view.getUint16(pos+28,true),extra=view.getUint16(pos+30,true),comment=view.getUint16(pos+32,true),next=pos+46+nameSize+extra+comment;
    if(next>end)throw new Error('ZIP 항목이 손상되었습니다.');
    const name=new TextDecoder('utf-8',{fatal:true}).decode(new Uint8Array(buffer,pos+46,nameSize));
    const mode=view.getUint32(pos+38,true)>>>16;
    if(name.startsWith('/') || name.includes('\\') || name.split('/').includes('..') || seen.has(name) || (mode&0xf000)===0xa000)throw new Error('위험한 경로, 중복 항목 또는 심볼릭 링크가 있는 ZIP입니다.');
    seen.add(name);
    if(filename.test(name.split('/').at(-1))){const bytes=view.getUint32(pos+24,true);total+=bytes;if(bytes>MAX_FILE || total>MAX_TOTAL)throw new Error('압축 데이터 크기 제한을 초과했습니다.');}
    pos=next;
  }
  if(pos!==end)throw new Error('ZIP 중앙 디렉터리 범위가 잘못되었습니다.');
}
