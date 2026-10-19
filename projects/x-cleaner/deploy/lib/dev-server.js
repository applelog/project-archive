import http from 'node:http';
import {createServer} from 'vite';
import {readFileSync,existsSync,appendFileSync} from 'node:fs';
import {randomBytes} from 'node:crypto';
import {handle} from './server.js';
// Local-only env loading, never included in the browser bundle.
if(existsSync('.env.local'))for(const line of readFileSync('.env.local','utf8').split('\n')) {
  const m=line.match(/^([A-Z_]+)=(.*)$/);if(m && process.env[m[1]]===undefined)process.env[m[1]]=m[2].replace(/^['"]|['"]$/g,'');
}
process.env.APP_ORIGIN ||= 'REDACTED_CONFIG_VALUE';
if(!process.env.SESSION_ENCRYPTION_KEY){process.env.SESSION_ENCRYPTION_KEY=randomBytes(32).toString('hex');appendFileSync('.env.local',`\nSESSION_ENCRYPTION_KEY=${process.env.SESSION_ENCRYPTION_KEY}\n`,{mode:0o600});}
const vite=await createServer({server:{middlewareMode:true},appType:'spa'});
http.createServer(async(req,res)=> {
  if(req.url.startsWith('/api/')) {
    try {
      let raw='',size=0;for await(const chunk of req){size+=chunk.length;if(size>65536)throw new Error();raw+=chunk.toString();}
      req.body=raw?JSON.parse(raw):{};
      return await handle(req.url.split('?')[0].split('/').at(-1),req,res);
    }catch{res.statusCode=400;res.end('{"error":"잘못되거나 너무 큰 요청입니다."}');return;}
  }
  vite.middlewares(req,res);
}).listen(5173,'localhost',()=>console.log('X Cleaner: REDACTED_CONFIG_VALUE (actual execution disabled by default)'));
