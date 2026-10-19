import React, { useState } from 'react';
import type { Course, ClassDivision } from '../types';
import { parseDivisionLine, preprocessBookletText, parseBookletLine } from '../utils/parser';
import { Plus, Trash2, BookOpen, Info, Edit2 } from 'lucide-react';

interface CourseInputProps {
  courses: Course[];
  onAddCourse: (course: Course) => void;
  onDeleteCourse: (courseId: string) => void;
  onAddDivision: (courseId: string, division: ClassDivision) => void;
  onDeleteDivision: (courseId: string, divisionId: string) => void;
  onUpdateCourse: (courseId: string, updatedFields: Partial<Course>) => void;
  onImportBulk: (importedCourses: Course[]) => void;
}

const PRESET_COLORS = [
  'hsl(263, 85%, 60%)', // Violet
  'hsl(180, 85%, 45%)', // Cyan
  'hsl(320, 80%, 55%)', // Magenta
  'hsl(210, 85%, 55%)', // Blue
  'hsl(142, 70%, 45%)', // Green
  'hsl(35, 90%, 50%)',  // Orange
  'hsl(0, 85%, 60%)'    // Red
];

export const CourseInput: React.FC<CourseInputProps> = ({
  courses,
  onAddCourse,
  onDeleteCourse,
  onAddDivision,
  onDeleteDivision,
  onUpdateCourse,
  onImportBulk
}) => {
  // Course form state
  const [courseName, setCourseName] = useState('');
  const [courseCode, setCourseCode] = useState('');
  const [credits, setCredits] = useState(3);
  const [selectedColor, setSelectedColor] = useState(PRESET_COLORS[0]);

  // Division form state (keyed by courseId to show division form inside each course card)
  const [activeCourseId, setActiveCourseId] = useState<string | null>(null);
  const [divisionsInput, setDivisionsInput] = useState('');
  const [parsedDivisions, setParsedDivisions] = useState<ClassDivision[]>([]);

  // Edit course state
  const [editingCourseId, setEditingCourseId] = useState<string | null>(null);
  const [editName, setEditName] = useState('');
  const [editCode, setEditCode] = useState('');
  const [editCredits, setEditCredits] = useState(3);

  // Bulk paste state
  const [bulkInput, setBulkInput] = useState('');
  const [bulkResults, setBulkResults] = useState<{
    successCount: number;
    failedLines: string[];
    skippedItems: string[];
  } | null>(null);
  const [isManualOpen, setIsManualOpen] = useState(false);

  const handleDivisionsInputChange = (val: string, courseId: string) => {
    setDivisionsInput(val);
    if (!val.trim()) {
      setParsedDivisions([]);
      return;
    }
    const lines = preprocessBookletText(val);
    const list: ClassDivision[] = [];
    lines.forEach(line => {
      const parsed = parseDivisionLine(line, courseId);
      if (parsed) {
        list.push(parsed);
      }
    });
    setParsedDivisions(list);
  };
  const handleBulkAdd = () => {
    const lines = preprocessBookletText(bulkInput);
    const courseMap: { [courseName: string]: Course } = {};
    const failedLines: string[] = [];
    const skippedItems: string[] = [];
    let successCount = 0;
    let colorIdx = courses.length;

    lines.forEach((line, idx) => {
      const trimmed = line.trim();
      if (!trimmed || trimmed.startsWith('#')) return;

      const parsed = parseBookletLine(line, `course_${idx}`);
      if (parsed && parsed.slots.length > 0) {
        const hasRealCode = parsed.courseCode && !parsed.courseCode.startsWith('course_');
        const groupKey = hasRealCode ? parsed.courseCode : parsed.courseName;

        const existingCourse = courses.find(c => (hasRealCode && c.code === parsed.courseCode) || c.name === parsed.courseName);
        const isDuplicateDb = existingCourse?.divisions.some(d => d.divisionName === parsed.divisionName);
        const isDuplicateBatch = courseMap[groupKey]?.divisions.some(d => d.divisionName === parsed.divisionName);

        if (isDuplicateDb || isDuplicateBatch) {
          skippedItems.push(`${parsed.courseName} (${parsed.divisionName}분반)`);
          return;
        }

        successCount++;
        
        if (!courseMap[groupKey]) {
          courseMap[groupKey] = {
            id: hasRealCode ? parsed.courseCode : `course_${idx}`,
            name: parsed.courseName,
            code: hasRealCode ? parsed.courseCode : '',
            color: PRESET_COLORS[colorIdx % PRESET_COLORS.length],
            credits: parsed.credits,
            divisions: [],
            active: true
          };
          colorIdx++;
        }

        const newDiv: ClassDivision = {
          id: `${courseMap[groupKey].id}_${parsed.divisionName}`,
          divisionName: parsed.divisionName,
          instructor: parsed.instructor,
          classroom: parsed.classroom,
          slots: parsed.slots
        };

        const exists = courseMap[groupKey].divisions.some(d => d.divisionName === parsed.divisionName);
        if (!exists) {
          courseMap[groupKey].divisions.push(newDiv);
        }
      } else {
        if (trimmed) {
          failedLines.push(line);
        }
      }
    });

    const coursesArray = Object.values(courseMap);
    if (coursesArray.length > 0) {
      onImportBulk(coursesArray);
    }

    setBulkResults({
      successCount,
      failedLines,
      skippedItems
    });
    setBulkInput('');
  };
  const handleCreateCourse = (e: React.FormEvent) => {
    e.preventDefault();
    if (!courseName.trim()) return;

    const code = courseCode.trim() || 'C' + Math.random().toString(36).substr(2, 5).toUpperCase();
    const newCourse: Course = {
      id: code,
      name: courseName.trim(),
      code,
      color: selectedColor,
      credits,
      divisions: []
    };

    onAddCourse(newCourse);
    setCourseName('');
    setCourseCode('');
    // Automatically select a different preset color for next course
    const nextColorIndex = (PRESET_COLORS.indexOf(selectedColor) + 1) % PRESET_COLORS.length;
    setSelectedColor(PRESET_COLORS[nextColorIndex]);
  };

  const handleCreateDivisions = (courseId: string) => {
    if (parsedDivisions.length === 0) return;
    parsedDivisions.forEach(div => {
      onAddDivision(courseId, div);
    });
    setDivisionsInput('');
    setParsedDivisions([]);
    setActiveCourseId(null);
  };

  return (
    <div style={{ display: 'flex', flexDirection: 'column', gap: '1.5rem' }}>
      
      {/* 1. Bulk Import Card (Primary Copy-Paste input) */}
      <div className="glass-card" style={{ border: '1px solid rgba(255, 255, 255, 0.08)' }}>
        <h2 style={{ marginBottom: '0.75rem', display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
          <BookOpen size={18} style={{ color: 'hsl(var(--accent-cyan))' }} />
          요람 줄 복사 붙여넣기
        </h2>
        <p className="subtitle" style={{ fontSize: '0.75rem', marginBottom: '0.5rem' }}>
          요람 PDF에서 복사한 한 줄 또는 여러 줄의 강의 정보 행을 아래에 붙여넣어 주세요. 과목명, 학수번호, 분반, 시간, 교수명이 자동으로 등록됩니다.
        </p>
        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.6rem' }}>
          <textarea
            rows={4}
            placeholder="예시: 2 전공선택 EB14202 01 머신러닝 3 3 심산225 홍길동 월11:50-14:30"
            value={bulkInput}
            onChange={e => setBulkInput(e.target.value)}
            style={{ fontSize: '0.75rem', padding: '0.4rem', borderRadius: '6px', resize: 'vertical' }}
          />
          <button
            type="button"
            className="btn btn-primary"
            style={{ fontSize: '0.8rem', padding: '0.4rem 0.75rem', width: '100%' }}
            onClick={handleBulkAdd}
            disabled={!bulkInput.trim()}
          >
            <Plus size={14} /> 과목 추가
          </button>
        </div>

        {bulkResults && (
          <div style={{ marginTop: '0.75rem', padding: '0.6rem', borderRadius: '6px', background: 'rgba(255, 255, 255, 0.02)', border: '1px solid var(--glass-border)', fontSize: '0.75rem' }}>
            <div style={{ color: 'hsl(var(--success))', fontWeight: 600, display: 'flex', alignItems: 'center', gap: '0.2rem' }}>
              ✓ {bulkResults.successCount}개 분반 추가 완료
            </div>
            {bulkResults.skippedItems.length > 0 && (
              <div style={{ color: 'hsl(var(--accent-cyan))', marginTop: '0.25rem' }}>
                • 중복 제외 {bulkResults.skippedItems.length}개
                <div style={{ maxHeight: '60px', overflowY: 'auto', padding: '0.2rem 0', opacity: 0.8, fontSize: '0.7rem' }}>
                  {bulkResults.skippedItems.map((item, i) => (
                    <div key={i}>- {item}</div>
                  ))}
                </div>
              </div>
            )}
            {bulkResults.failedLines.length > 0 && (
              <div style={{ color: 'hsl(var(--danger))', marginTop: '0.25rem' }}>
                • 실패 {bulkResults.failedLines.length}개 행
              </div>
            )}
            <div style={{ display: 'flex', justifyContent: 'flex-end', marginTop: '0.4rem' }}>
              <button
                type="button"
                className="btn btn-secondary"
                style={{ fontSize: '0.65rem', padding: '0.15rem 0.4rem' }}
                onClick={() => setBulkResults(null)}
              >
                닫기
              </button>
            </div>
          </div>
        )}

        <div style={{ textAlign: 'center', marginTop: '0.75rem' }}>
          <button
            type="button"
            className="btn btn-secondary"
            style={{ fontSize: '0.7rem', padding: '0.2rem 0.5rem', background: 'transparent', border: 'none', textDecoration: 'underline' }}
            onClick={() => setIsManualOpen(!isManualOpen)}
          >
            {isManualOpen ? "수동 과목 추가 양식 닫기" : "또는 수동으로 과목 직접 만들기"}
          </button>
        </div>

        {isManualOpen && (
          <div style={{ marginTop: '0.75rem', paddingTop: '0.75rem', borderTop: '1px dashed rgba(255,255,255,0.08)' }}>
            <h3 style={{ fontSize: '0.8rem', fontWeight: 600, marginBottom: '0.5rem' }}>수동 과목 추가</h3>
            <form onSubmit={handleCreateCourse} style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
              <div>
                <label className="subtitle" style={{ display: 'block', fontSize: '0.7rem', marginBottom: '0.15rem' }}>과목명 (필수)</label>
                <input
                  type="text"
                  placeholder="예: 컴퓨터네트워크"
                  value={courseName}
                  onChange={e => setCourseName(e.target.value)}
                  style={{ fontSize: '0.75rem', padding: '0.25rem 0.4rem' }}
                />
              </div>
              <div style={{ display: 'grid', gridTemplateColumns: '1fr 70px', gap: '0.5rem' }}>
                <div>
                  <label className="subtitle" style={{ display: 'block', fontSize: '0.7rem', marginBottom: '0.15rem' }}>과목코드</label>
                  <input
                    type="text"
                    placeholder="예: CS203"
                    value={courseCode}
                    onChange={e => setCourseCode(e.target.value)}
                    style={{ fontSize: '0.75rem', padding: '0.25rem 0.4rem' }}
                  />
                </div>
                <div>
                  <label className="subtitle" style={{ display: 'block', fontSize: '0.7rem', marginBottom: '0.15rem' }}>학점</label>
                  <select
                    value={credits}
                    onChange={e => setCredits(parseInt(e.target.value))}
                    style={{ fontSize: '0.75rem', padding: '0.25rem 0.4rem' }}
                  >
                    <option value={1}>1학점</option>
                    <option value={2}>2학점</option>
                    <option value={3}>3학점</option>
                    <option value={4}>4학점</option>
                  </select>
                </div>
              </div>
              <button type="submit" className="btn btn-primary" style={{ marginTop: '0.25rem', fontSize: '0.75rem', padding: '0.3rem' }}>
                <Plus size={12} /> 과목 생성
              </button>
            </form>
          </div>
        )}
      </div>

      {/* 2. Course List */}
      <div style={{ display: 'flex', flexDirection: 'column', gap: '1rem' }}>
        <h2 style={{ fontSize: '1rem', opacity: 0.9 }}>후보 과목 목록 ({courses.length})</h2>
        {courses.length === 0 ? (
          <div className="glass-card" style={{ textAlign: 'center', padding: '2rem 1rem', color: 'hsl(var(--text-secondary))' }}>
            <Info size={28} style={{ margin: '0 auto 0.75rem', opacity: 0.5 }} />
            등록된 과목이 없습니다.<br />위 폼에서 과목을 생성하거나 텍스트 일괄 입력을 이용해 주세요.
          </div>
        ) : (
          courses.map(course => (
            <div
              key={course.id}
              className="glass-card"
              style={{
                borderLeft: `4px solid ${course.color}`,
                display: 'flex',
                flexDirection: 'column',
                gap: '0.75rem'
              }}
            >
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'flex-start' }}>
                {editingCourseId === course.id ? (
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '0.4rem', flex: 1, marginRight: '0.5rem' }}>
                    <input
                      type="text"
                      value={editName}
                      onChange={e => setEditName(e.target.value)}
                      style={{ fontSize: '0.9rem', padding: '0.2rem 0.4rem' }}
                      placeholder="과목명"
                    />
                    <div style={{ display: 'grid', gridTemplateColumns: '1fr 80px', gap: '0.4rem' }}>
                      <input
                        type="text"
                        value={editCode}
                        onChange={e => setEditCode(e.target.value)}
                        style={{ fontSize: '0.8rem', padding: '0.2rem 0.4rem' }}
                        placeholder="과목코드"
                      />
                      <input
                        type="number"
                        min={0}
                        max={6}
                        value={editCredits}
                        onChange={e => setEditCredits(parseInt(e.target.value) || 0)}
                        style={{ fontSize: '0.8rem', padding: '0.2rem 0.4rem' }}
                        placeholder="학점"
                      />
                    </div>
                    <div style={{ display: 'flex', gap: '0.4rem', marginTop: '0.2rem' }}>
                      <button
                        className="btn btn-primary"
                        style={{ padding: '0.2rem 0.5rem', fontSize: '0.75rem' }}
                        onClick={() => {
                          onUpdateCourse(course.id, { name: editName.trim(), code: editCode.trim(), credits: editCredits });
                          setEditingCourseId(null);
                        }}
                        disabled={!editName.trim()}
                      >
                        저장
                      </button>
                      <button
                        className="btn btn-secondary"
                        style={{ padding: '0.2rem 0.5rem', fontSize: '0.75rem' }}
                        onClick={() => setEditingCourseId(null)}
                      >
                        취소
                      </button>
                    </div>
                  </div>
                ) : (
                  <div>
                    <div style={{ display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
                      <input
                        type="checkbox"
                        checked={course.active !== false}
                        onChange={e => onUpdateCourse(course.id, { active: e.target.checked })}
                        title={course.active !== false ? "시간표 조합에서 제외 (후보 보관)" : "시간표 조합에 포함"}
                        style={{ cursor: 'pointer' }}
                      />
                      <h3 style={{ fontSize: '1rem', fontWeight: 600, opacity: course.active !== false ? 1 : 0.6 }}>
                        {course.name}
                      </h3>
                      {course.active === false && (
                        <span className="badge" style={{ fontSize: '0.65rem', padding: '0.1rem 0.3rem', backgroundColor: 'rgba(255,255,255,0.05)' }}>
                          후보
                        </span>
                      )}
                    </div>
                    <span className="subtitle" style={{ opacity: 0.6, marginLeft: '1.4rem', display: 'block' }}>
                      {course.code} • {course.credits}학점
                    </span>
                  </div>
                )}

                {editingCourseId !== course.id && (
                  <div style={{ display: 'flex', gap: '0.3rem' }}>
                    <button
                      className="btn btn-secondary"
                      style={{ padding: '0.3rem', borderRadius: '6px' }}
                      onClick={() => {
                        setEditingCourseId(course.id);
                        setEditName(course.name);
                        setEditCode(course.code);
                        setEditCredits(course.credits);
                      }}
                      title="과목 정보 수정"
                    >
                      <Edit2 size={14} />
                    </button>
                    <button
                      className="btn btn-danger"
                      style={{ padding: '0.3rem', borderRadius: '6px' }}
                      onClick={() => onDeleteCourse(course.id)}
                      title="과목 삭제"
                    >
                      <Trash2 size={14} />
                    </button>
                  </div>
                )}
              </div>

              {/* Divisions List */}
              <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem' }}>
                <span className="subtitle" style={{ fontSize: '0.75rem', fontWeight: 600 }}>분반 후보 ({course.divisions.length})</span>
                {course.divisions.map(div => (
                  <div
                    key={div.id}
                    style={{
                      display: 'flex',
                      justifyContent: 'space-between',
                      alignItems: 'center',
                      background: 'rgba(255,255,255,0.03)',
                      padding: '0.4rem 0.6rem',
                      borderRadius: '6px',
                      fontSize: '0.8rem',
                      border: '1px solid rgba(255,255,255,0.04)'
                    }}
                  >
                    <div>
                      <span style={{ fontWeight: 600, marginRight: '0.5rem' }}>{div.divisionName}분반</span>
                      <span style={{ opacity: 0.8, marginRight: '0.5rem' }}>{div.instructor}</span>
                      <span style={{ opacity: 0.6 }}>{div.classroom}</span>
                      <div style={{ fontSize: '0.7rem', color: 'hsl(var(--accent-cyan))', marginTop: '0.15rem' }}>
                        {div.slots.map(s => s.displayStr).join(', ')}
                      </div>
                    </div>
                    <button
                      className="btn"
                      style={{
                        padding: '0.2rem',
                        background: 'transparent',
                        color: 'hsl(var(--danger))',
                        opacity: 0.7
                      }}
                      onClick={() => onDeleteDivision(course.id, div.id)}
                      title="분반 삭제"
                    >
                      <Trash2 size={12} />
                    </button>
                  </div>
                ))}
              </div>

              {/* Add Division Action */}
              {activeCourseId === course.id ? (
                <div
                  style={{
                    background: 'rgba(255, 255, 255, 0.02)',
                    padding: '0.75rem',
                    borderRadius: '8px',
                    border: '1px solid rgba(255,255,255,0.05)',
                    display: 'flex',
                    flexDirection: 'column',
                    gap: '0.5rem',
                    marginTop: '0.5rem'
                  }}
                >
                  <label className="subtitle" style={{ fontSize: '0.75rem', display: 'block' }}>
                    분반 정보 일괄 입력 (한 줄에 하나씩)
                  </label>
                  <textarea
                    rows={3}
                    placeholder="예시:&#10;01 홍길동 심산225 월11:50-14:30&#10;02 홍길동 심산225 화11:50-14:30"
                    value={divisionsInput}
                    onChange={e => handleDivisionsInputChange(e.target.value, course.id)}
                    style={{ fontSize: '0.8rem', resize: 'vertical' }}
                  />

                  {parsedDivisions.length > 0 && (
                    <div
                      style={{
                        fontSize: '0.7rem',
                        color: 'hsl(var(--success))',
                        background: 'rgba(0,0,0,0.1)',
                        padding: '0.4rem',
                        borderRadius: '4px',
                        display: 'flex',
                        flexDirection: 'column',
                        gap: '0.25rem'
                      }}
                    >
                      <span style={{ fontWeight: 600 }}>✓ 인식된 분반 ({parsedDivisions.length}개):</span>
                      {parsedDivisions.map((d, i) => (
                        <div key={i} style={{ opacity: 0.9 }}>
                          {d.divisionName}분반 | {d.instructor} | {d.classroom} ({d.slots.map(s => s.displayStr).join(', ')})
                        </div>
                      ))}
                    </div>
                  )}

                  <div style={{ display: 'flex', gap: '0.5rem', marginTop: '0.25rem' }}>
                    <button
                      className="btn btn-primary"
                      style={{ flex: 1, padding: '0.4rem' }}
                      onClick={() => handleCreateDivisions(course.id)}
                      disabled={parsedDivisions.length === 0}
                    >
                      분반 추가 ({parsedDivisions.length})
                    </button>
                    <button
                      className="btn btn-secondary"
                      style={{ padding: '0.4rem' }}
                      onClick={() => {
                        setActiveCourseId(null);
                        setDivisionsInput('');
                        setParsedDivisions([]);
                      }}
                    >
                      취소
                    </button>
                  </div>
                </div>
              ) : (
                <button
                  className="btn btn-secondary"
                  style={{ width: '100%', fontSize: '0.8rem', padding: '0.4rem', borderStyle: 'dashed' }}
                  onClick={() => {
                    setActiveCourseId(course.id);
                    setDivisionsInput('');
                    setParsedDivisions([]);
                  }}
                >
                  <Plus size={12} /> 분반 후보 일괄 추가
                </button>
              )}
            </div>
          ))
        )}
      </div>
    </div>
  );
};
