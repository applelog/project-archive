import {randomBytes,createCipheriv,createDecipheriv} from 'node:crypto';
export function seal(value, purpose, keyHex, ttl) {
  if (!/^[a-f0-9]{64}$/i.test(keyHex || '')) throw new Error('서버 암호화 키 설정이 필요합니다.');
  const iv=randomBytes(12), cipher=createCipheriv('aes-256-gcm',Buffer.from(keyHex,'hex'),iv);
  cipher.setAAD(Buffer.from(purpose));
  const encrypted=Buffer.concat([cipher.update(JSON.stringify({value,expires:Date.now()+ttl})),cipher.final()]);
  return Buffer.concat([iv,cipher.getAuthTag(),encrypted]).toString('base64url');
}
export function unseal(token,purpose,keyHex) {
  try {
    if(typeof token !== 'string' || token.length>50000) throw new Error();
    const data=Buffer.from(token,'base64url'), decipher=createDecipheriv('aes-256-gcm',Buffer.from(keyHex,'hex'),data.subarray(0,12));
    decipher.setAuthTag(data.subarray(12,28));decipher.setAAD(Buffer.from(purpose));
    const result=JSON.parse(Buffer.concat([decipher.update(data.subarray(28)),decipher.final()]).toString());
    if(!Number.isFinite(result.expires) || result.expires <= Date.now())throw new Error();
    return result.value;
  } catch {throw new Error('인증 또는 승인 목록이 만료·변경되었습니다. 다시 연결하고 검증하세요.');}
}
