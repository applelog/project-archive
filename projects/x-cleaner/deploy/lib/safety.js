export function id(value) {
  if (typeof value !== 'string' || !/^[1-9][0-9]{0,18}$/.test(value)) throw new Error('ID는 정확한 1~19자리 숫자 문자열이어야 합니다.');
  return value;
}
export function inputId(value) {
  const text = value.trim();
  if (/^\d+$/.test(text)) return id(text);
  let url;
  try { url = new URL(text); } catch { throw new Error('X 링크 또는 게시물 ID를 입력하세요.'); }
  const match = url.pathname.match(/^\/(?:\w+\/status|i\/(?:web\/)?status)\/(\d+)(?:\/(?:photo|video)\/\d+)?\/?$/);
  if (url.protocol !== 'https:' || !['x.com','www.x.com','twitter.com','www.twitter.com'].includes(url.host) || !match) throw new Error('X HTTPS 게시물 링크를 확인하세요.');
  return id(match[1]);
}
export function preview(archive, selectedIds, keepValues) {
  const keeps = [...new Set(keepValues.map(inputId))];
  const known = new Set(archive.records.flatMap(r => [r.id, ...(r.sourceId ? [r.sourceId] : [])]));
  const missing = keeps.filter(k => !known.has(k));
  if (missing.length) throw new Error(`보존 ID가 아카이브에 없습니다: ${missing.join(', ')}. 실행을 차단합니다.`);
  const selected = [...new Set(selectedIds)].map(key => {
    const record = archive.records.find(r => r.id === key);
    if (!record) throw new Error('선택한 ID가 아카이브에 없습니다.');
    return record;
  });
  const targets = selected.filter(r => !keeps.includes(r.id) && !keeps.includes(r.sourceId));
  if (!targets.length) throw new Error('삭제할 항목을 직접 선택하세요. 보존한 항목은 대상에서 제외됩니다.');
  if (targets.length > 50) throw new Error('한 작업에서 최대 50개까지 선택할 수 있습니다.');
  return {owner: archive.owner, fingerprint: archive.fingerprint, total: archive.records.length, keeps, targets,
    kept: archive.records.filter(r => keeps.includes(r.id) || keeps.includes(r.sourceId)).length,
    untouched: archive.records.length - targets.length};
}
export function guard(plan, target, owner) {
  if (!owner || id(plan.owner.id) !== id(owner)) throw new Error('인증 계정과 아카이브 소유 계정이 다릅니다.');
  if (!target || !plan.targets.some(r => r.id === target.id && r.kind === target.kind)) throw new Error('승인된 선택 목록 밖의 항목입니다.');
  if (plan.keeps.includes(target.id) || (target.sourceId && plan.keeps.includes(target.sourceId))) throw new Error('보존 ID는 요청할 수 없습니다.');
  if (!['post','reply','repost'].includes(target.kind)) throw new Error('지원하지 않는 항목 종류입니다.');
}
export function liveTarget(record, expected, owner, keeps, now = Date.now()) {
  if (!record || id(record.id) !== id(expected.id) || id(record.author_id) !== id(owner)) throw new Error('게시물 소유자 또는 ID를 확인할 수 없습니다.');
  const refs = record.referenced_tweets || [];
  const repost = refs.find(r => r.type === 'retweeted');
  const kind = repost ? 'repost' : refs.some(r => r.type === 'replied_to') ? 'reply' : 'post';
  if (kind !== expected.kind) throw new Error('선택한 종류와 X의 현재 종류가 다릅니다. 다시 선택하세요.');
  const edits = record.edit_history_tweet_ids;
  // Newer posts must expose edit metadata. Old posts predate X's edit feature.
  const created = Date.parse(record.created_at);
  if (!Array.isArray(edits) && (!Number.isFinite(created) || created >= Date.UTC(2022,8,1))) throw new Error('수정 이력을 확인할 수 없어 실행을 차단합니다.');
  if (Array.isArray(edits) && (edits.length !== 1 || edits[0] !== record.id)) throw new Error('수정된 게시물은 웹판에서 차단합니다. 개인용 CLI에서 확인하세요.');
  if (!Number.isFinite(created) || now - created < 60 * 60 * 1000) throw new Error('작성 시각을 확인할 수 없거나 수정 가능 시간 1시간이 지나지 않았습니다.');
  const sourceId = repost ? id(repost.id) : null;
  if (expected.sourceId && sourceId !== expected.sourceId) throw new Error('리포스트 원본 관계가 달라졌습니다.');
  if (keeps.includes(record.id) || (sourceId && keeps.includes(sourceId))) throw new Error('보존 ID가 포함되어 실행을 차단합니다.');
  return {id: record.id, kind, sourceId};
}
export function resumeCandidates(job) {
  return job.targets.filter(t => !['success','unavailable','already_deleted'].includes(job.results[t.id]?.status));
}
export function matchesDraft(job,archive,selectedIds,keepText) {
  if(!job || !archive || job.fingerprint !== archive.fingerprint || keepText !== job.inputKeepText)return false;
  const selected=[...new Set(selectedIds)].sort();
  return JSON.stringify(selected)===JSON.stringify([...(job.selectionIds || [])].sort());
}
