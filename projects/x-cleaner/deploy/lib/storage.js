function openDB() {
  return new Promise((resolve,reject)=> {
    const request=indexedDB.open('x-cleaner-web',1);
    request.onupgradeneeded=()=>request.result.createObjectStore('jobs');
    request.onsuccess=()=>resolve(request.result);request.onerror=()=>reject(new Error('브라우저 작업 기록을 열지 못했습니다. 비공개 모드 또는 저장 권한을 확인하세요.'));
  });
}
export async function storeJob(job) {
  const db=await openDB();
  return new Promise((resolve,reject)=> {
    const tx=db.transaction('jobs','readwrite');tx.objectStore('jobs').put(job,'current');
    tx.oncomplete=()=>{db.close();resolve();};tx.onerror=()=>{db.close();reject(new Error('작업 기록을 저장하지 못해 실행을 중단했습니다.'));};tx.onabort=tx.onerror;
  });
}
export async function loadJob() {
  const db=await openDB();
  return new Promise((resolve,reject)=> {
    const tx=db.transaction('jobs','readonly'),req=tx.objectStore('jobs').get('current');
    req.onsuccess=()=>{db.close();resolve(req.result || null);};req.onerror=()=>{db.close();reject(new Error('작업 기록을 불러오지 못했습니다.'));};
  });
}
export async function clearJob() {
  const db=await openDB();
  return new Promise((resolve,reject)=> {
    const tx=db.transaction('jobs','readwrite');tx.objectStore('jobs').delete('current');
    tx.oncomplete=()=>{db.close();resolve();};tx.onerror=()=>{db.close();reject(new Error('작업 기록을 삭제하지 못했습니다.'));};
  });
}
