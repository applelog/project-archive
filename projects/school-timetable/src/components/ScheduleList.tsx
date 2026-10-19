import React, { useState } from 'react';
import type { ScheduleCombination, FilterOptions, ClassDivision, Course } from '../types';
import { filterAndSortSchedules, isDivisionConflicting } from '../utils/scheduler';
import { SlidersHorizontal, ChevronLeft, ChevronRight, Sparkles, AlertCircle } from 'lucide-react';

interface ScheduleListProps {
  schedules: ScheduleCombination[];
  activeSchedule: { [courseId: string]: ClassDivision } | null;
  courses: Course[];
  onHoverSchedule: (divisions: { [courseId: string]: ClassDivision } | null) => void;
  onSelectSchedule: (divisions: { [courseId: string]: ClassDivision }) => void;
}

export const ScheduleList: React.FC<ScheduleListProps> = ({
  schedules,
  activeSchedule,
  courses,
  onHoverSchedule,
  onSelectSchedule
}) => {
  // Filters State
  const [filterOptions, setFilterOptions] = useState<FilterOptions>({
    emptyFriday: false,
    noMorning: false,
    noNight: false,
    lunchBreak: false,
    maxGapHours: 24, // 24 = no limit
    sortBy: 'gapAsc' // Default sort: minimizing gaps
  });

  // For each course, find all unique instructors in its divisions
  const courseInstructors = React.useMemo(() => {
    const map: { [courseId: string]: string[] } = {};
    courses.forEach(course => {
      if (course.divisions.length > 0) {
        const instructors = Array.from(
          new Set(course.divisions.map(d => (d.instructor?.trim() || '담당교수 미정')).filter(Boolean))
        );
        if (instructors.length > 1) {
          map[course.id] = instructors;
        }
      }
    });
    return map;
  }, [courses]);

  // Pagination State
  const [currentPage, setCurrentPage] = useState(1);
  const itemsPerPage = 8;

  // Apply filters and sorting
  const processed = filterAndSortSchedules(schedules, filterOptions);

  // Pagination Math
  const totalPages = Math.ceil(processed.length / itemsPerPage);
  const startIndex = (currentPage - 1) * itemsPerPage;
  const currentSchedules = processed.slice(startIndex, startIndex + itemsPerPage);

  const handleFilterChange = (key: keyof FilterOptions, value: any) => {
    setFilterOptions(prev => ({ ...prev, [key]: value }));
    setCurrentPage(1); // Reset to page 1
  };

  const getCourseColor = (courseId: string): string => {
    const course = courses.find(c => c.id === courseId);
    return course ? course.color : 'hsl(263, 90%, 55%)';
  };

  const getCourseName = (courseId: string): string => {
    const course = courses.find(c => c.id === courseId);
    return course ? course.name : '';
  };

  // Compare active selection to combination
  const isActive = (combination: ScheduleCombination): boolean => {
    if (!activeSchedule) return false;
    
    // Check if the division IDs match
    for (const [courseId, div] of Object.entries(activeSchedule)) {
      if (combination.divisions[courseId]?.id !== div.id) {
        return false;
      }
    }
    return true;
  };

  return (
    <div
      style={{
        width: '100%',
        borderRight: '1px solid hsl(var(--border))',
        display: 'flex',
        flexDirection: 'column',
        height: '100%',
        minHeight: 0,
        backgroundColor: 'hsl(var(--bg-secondary))'
      }}
    >
      {/* 1. Header & Filters Toggle */}
      <div
        style={{
          padding: '1.25rem',
          borderBottom: '1px solid hsl(var(--border))',
          display: 'flex',
          flexDirection: 'column',
          gap: '0.75rem'
        }}
      >
        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <h2 style={{ fontSize: '0.95rem', fontWeight: 600, display: 'flex', alignItems: 'center', gap: '0.4rem' }}>
            <Sparkles size={14} style={{ color: 'hsl(var(--knu-blue))' }} />
            조합 추천 ({processed.length})
          </h2>
        </div>

        {/* Filters Panel */}
        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', fontSize: '0.8rem' }}>
          <div style={{ display: 'flex', alignItems: 'center', justifyItems: 'center', gap: '0.5rem' }}>
            <SlidersHorizontal size={12} style={{ color: 'hsl(var(--text-secondary))' }} />
            <span style={{ fontWeight: 600 }}>필터 옵션</span>
          </div>

          <label style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', cursor: 'pointer' }}>
            <input
              type="checkbox"
              checked={filterOptions.emptyFriday}
              onChange={e => handleFilterChange('emptyFriday', e.target.checked)}
            />
            금공강 희망 (금요일 수업 제외)
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', cursor: 'pointer' }}>
            <input
              type="checkbox"
              checked={filterOptions.noMorning}
              onChange={e => handleFilterChange('noMorning', e.target.checked)}
            />
            1교시 기피 (10시 이전 수업 제외)
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', cursor: 'pointer' }}>
            <input
              type="checkbox"
              checked={filterOptions.noNight}
              onChange={e => handleFilterChange('noNight', e.target.checked)}
            />
            야간 수업 제외 (18시 이후 수업 제외)
          </label>
          <label style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', cursor: 'pointer' }}>
            <input
              type="checkbox"
              checked={filterOptions.lunchBreak}
              onChange={e => handleFilterChange('lunchBreak', e.target.checked)}
            />
            점심시간 보장 (12시-13시 수업 비우기)
          </label>
          
          <div style={{ marginTop: '0.25rem' }}>
            <label className="subtitle" style={{ fontSize: '0.7rem', display: 'flex', justifyContent: 'space-between', marginBottom: '0.15rem' }}>
              <span>하루 최대 우주공강 시간:</span>
              <span style={{ fontWeight: 600, color: 'hsl(var(--accent-cyan))' }}>
                {filterOptions.maxGapHours >= 24 ? '제한 없음' : `${filterOptions.maxGapHours}시간`}
              </span>
            </label>
            <input
              type="range"
              min={1}
              max={24}
              value={filterOptions.maxGapHours}
              onChange={e => handleFilterChange('maxGapHours', parseInt(e.target.value))}
              style={{ width: '100%', height: '4px', cursor: 'pointer' }}
            />
          </div>

          <div style={{ display: 'flex', flexDirection: 'column', gap: '0.15rem', marginTop: '0.25rem' }}>
            <span className="subtitle" style={{ fontSize: '0.7rem' }}>정렬 기준</span>
            <select
              value={filterOptions.sortBy}
              onChange={e => handleFilterChange('sortBy', e.target.value)}
              style={{ padding: '0.3rem', fontSize: '0.75rem' }}
            >
              <option value="gapAsc">우주공강 최소화 (연강 선호)</option>
              <option value="daysAsc">등교 일수 최소화 (학교 조금 가기)</option>
              <option value="gapDesc">공강시간 최대화 정렬</option>
              <option value="daysDesc">수업 등교일수 많은 순</option>
              <option value="creditsDesc">이수학점 많은 순</option>
            </select>
          </div>

          {/* Professor Filters */}
          {Object.keys(courseInstructors).length > 0 && (
            <div style={{ borderTop: '1px solid hsl(var(--border))', paddingTop: '0.5rem', marginTop: '0.4rem' }}>
              <span className="subtitle" style={{ fontSize: '0.7rem', display: 'block', marginBottom: '0.25rem' }}>교수님 필터 (다중 분반인 경우)</span>
              <div style={{ display: 'flex', flexDirection: 'column', gap: '0.4rem', paddingRight: '0.2rem' }}>
                {Object.entries(courseInstructors).map(([cid, instructors]) => {
                  const courseName = courses.find(c => c.id === cid)?.name || cid;
                  return (
                    <div key={cid} style={{ fontSize: '0.7rem' }}>
                      <div style={{ fontWeight: 600, color: 'hsl(var(--knu-blue))', opacity: 0.9 }}>
                        {courseName}
                      </div>
                      <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.4rem', marginTop: '0.1rem' }}>
                        {instructors.map(inst => {
                          const isChecked = !filterOptions.preferredInstructors?.[cid] || 
                            filterOptions.preferredInstructors[cid].includes(inst);
                          return (
                            <label key={inst} style={{ display: 'flex', alignItems: 'center', gap: '0.2rem', cursor: 'pointer', opacity: 0.85 }}>
                              <input
                                type="checkbox"
                                checked={isChecked}
                                onChange={e => {
                                  const currentPreferred = filterOptions.preferredInstructors?.[cid] ?? [...instructors];
                                  let nextPreferred: string[];
                                  if (e.target.checked) {
                                    nextPreferred = Array.from(new Set([...currentPreferred, inst]));
                                  } else {
                                    nextPreferred = currentPreferred.filter(name => name !== inst);
                                  }
                                  setFilterOptions(prev => ({
                                    ...prev,
                                    preferredInstructors: {
                                      ...prev.preferredInstructors,
                                      [cid]: nextPreferred
                                    }
                                  }));
                                  setCurrentPage(1);
                                }}
                              />
                              {inst}
                            </label>
                          );
                        })}
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          )}
        </div>
      </div>

      {/* 2. Schedule Options List */}
      <div style={{ flex: 1, overflowY: 'auto', padding: '0.75rem', display: 'flex', flexDirection: 'column', gap: '0.6rem' }}>
        {processed.length === 0 ? (
          <div
            style={{
              padding: '1rem 0.5rem',
              color: 'hsl(var(--text-secondary))',
              fontSize: '0.8rem',
              display: 'flex',
              flexDirection: 'column',
              gap: '1rem',
              lineHeight: '1.4'
            }}
          >
            <div style={{ textAlign: 'center', color: 'hsl(var(--text-muted))' }}>
              <AlertCircle size={24} style={{ margin: '0 auto 0.5rem', opacity: 0.6, color: 'hsl(var(--danger))' }} />
              <div style={{ fontWeight: 600, color: 'hsl(var(--text-primary))', marginBottom: '0.25rem' }}>
                조건에 맞는 조합 없음
              </div>
              선택한 과목 및 필터 조건에 부합하는 시간표 조합이 존재하지 않습니다.
            </div>

            {/* Analysis of why */}
            <div
              style={{
                background: 'rgba(255, 255, 255, 0.02)',
                border: '1px solid rgba(255, 255, 255, 0.05)',
                borderRadius: '8px',
                padding: '0.75rem',
                display: 'flex',
                flexDirection: 'column',
                gap: '0.5rem'
              }}
            >
              <div style={{ fontWeight: 600, fontSize: '0.75rem', color: 'hsl(var(--accent-cyan))' }}>
                원인 분석 & 해결 제안:
              </div>
              <ul style={{ paddingLeft: '1rem', margin: 0, display: 'flex', flexDirection: 'column', gap: '0.4rem', fontSize: '0.75rem' }}>
                {schedules.length === 0 ? (
                  // Hard conflicts
                  (() => {
                    const conflicts: string[] = [];
                    // Check if no active courses
                    const activeCourses = courses.filter(c => c.divisions.length > 0);
                    if (activeCourses.length === 0) {
                      return <li>등록된 과목 중 분반 정보가 입력된 과목이 없습니다. 과목 카드 하단에서 분반 후보를 입력해주세요.</li>;
                    }
                    
                    // Check pairwise conflicts
                    for (let i = 0; i < activeCourses.length; i++) {
                      for (let j = i + 1; j < activeCourses.length; j++) {
                        const c1 = activeCourses[i];
                        const c2 = activeCourses[j];
                        let hasValidPair = false;
                        for (const d1 of c1.divisions) {
                          for (const d2 of c2.divisions) {
                            if (!isDivisionConflicting(d1, [d2])) {
                              hasValidPair = true;
                              break;
                            }
                          }
                          if (hasValidPair) break;
                        }
                        if (!hasValidPair) {
                          conflicts.push(`'${c1.name}'와/과 '${c2.name}'의 모든 분반이 동시간대에 겹칩니다.`);
                        }
                      }
                    }

                    if (conflicts.length > 0) {
                      return conflicts.map((c, i) => <li key={i}>{c}</li>);
                    }
                    return <li>선택된 과목들 간의 시간대 충돌이 너무 많아 조합할 수 없습니다. 일부 분반을 추가하거나 과목을 비활성화해주세요.</li>;
                  })()
                ) : (
                  // Filtered out conflicts
                  (() => {
                    const suggestions: React.ReactNode[] = [];
                    
                    if (filterOptions.emptyFriday) {
                      const count = schedules.filter(s => !s.hasFriday).length;
                      if (count === 0) {
                        suggestions.push(<li key="friday">모든 시간표 조합에 <strong>금요일 수업</strong>이 포함되어 있습니다. (금공강 필터 해제 권장)</li>);
                      }
                    }
                    if (filterOptions.noMorning) {
                      const count = schedules.filter(s => !s.hasMorning).length;
                      if (count === 0) {
                        suggestions.push(<li key="morning">모든 시간표 조합에 <strong>1교시(10시 이전) 수업</strong>이 포함되어 있습니다. (1교시 기피 필터 해제 권장)</li>);
                      }
                    }
                    if (filterOptions.noNight) {
                      const count = schedules.filter(s => !s.hasNight).length;
                      if (count === 0) {
                        suggestions.push(<li key="night">모든 시간표 조합에 <strong>야간(18시 이후) 수업</strong>이 포함되어 있습니다. (야간 수업 제외 필터 해제 권장)</li>);
                      }
                    }
                    if (filterOptions.lunchBreak) {
                      const count = schedules.filter(s => !s.hasLunchConflict).length;
                      if (count === 0) {
                        suggestions.push(<li key="lunch">모든 시간표 조합이 <strong>점심시간(12:00-13:00)</strong>과 겹칩니다. (점심시간 보장 필터 해제 권장)</li>);
                      }
                    }
                    if (filterOptions.maxGapHours < 24) {
                      const maxGapMinutes = filterOptions.maxGapHours * 60;
                      const count = schedules.filter(s => s.gapTime <= maxGapMinutes).length;
                      if (count === 0) {
                        suggestions.push(<li key="gap">모든 시간표 조합의 우주공강 시간이 <strong>{filterOptions.maxGapHours}시간</strong>보다 깁니다. (우주공강 시간제한 확장 권장)</li>);
                      }
                    }

                    if (filterOptions.preferredInstructors) {
                      Object.entries(filterOptions.preferredInstructors).forEach(([cid, val]) => {
                        const insts = val as string[];
                        const course = courses.find(c => c.id === cid);
                        if (course && Array.isArray(insts)) {
                          const allInsts = Array.from(new Set(course.divisions.map(d => (d.instructor?.trim() || '담당교수 미정')).filter(Boolean)));
                          if (insts.length < allInsts.length) {
                            const testOptions = {
                              ...filterOptions,
                              preferredInstructors: {
                                ...filterOptions.preferredInstructors,
                                [cid]: allInsts
                              }
                            };
                            const count = filterAndSortSchedules(schedules, testOptions).length;
                            if (count > 0) {
                              suggestions.push(
                                <li key={`prof-${cid}`}>
                                  <strong>'{course.name}'</strong>의 일부 교수님 필터로 인해 조합이 없습니다. (교수님 선택을 늘려보세요)
                                </li>
                              );
                            }
                          }
                        }
                      });
                    }

                    if (suggestions.length === 0) {
                      suggestions.push(<li key="other">필터 조건들이 너무 까다롭습니다. 필터 옵션들을 하나씩 해제해보며 조정해보세요.</li>);
                    }
                    return suggestions;
                  })()
                )}
              </ul>
            </div>
          </div>
        ) : (
          currentSchedules.map((schedule, idx) => {
            const currentIdx = startIndex + idx + 1;
            const active = isActive(schedule);
            
            return (
              <div
                key={schedule.id}
                className="glass-card glass-card-interactive"
                style={{
                  padding: '0.75rem',
                  borderRadius: '8px',
                  border: active ? '2px solid hsl(var(--knu-blue))' : '1px solid hsl(var(--border))',
                  backgroundColor: active ? 'hsl(var(--knu-blue) / 0.08)' : 'hsl(var(--bg-secondary))',
                  fontSize: '0.75rem',
                  display: 'flex',
                  flexDirection: 'column',
                  gap: '0.4rem'
                }}
                onMouseEnter={() => onHoverSchedule(schedule.divisions)}
                onMouseLeave={() => onHoverSchedule(null)}
                onClick={() => onSelectSchedule(schedule.divisions)}
              >
                <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
                  <span style={{ fontWeight: 700, color: active ? 'hsl(var(--knu-blue))' : 'hsl(var(--text-primary))' }}>
                    추천 #{currentIdx}
                  </span>
                  <span style={{ fontSize: '0.7rem', color: 'hsl(var(--text-secondary))' }}>
                    {schedule.totalCredits}학점
                  </span>
                </div>
                
                {/* Visual indicator of division swatches */}
                <div style={{ display: 'flex', gap: '2px', height: '4px', borderRadius: '99px', overflow: 'hidden' }}>
                  {Object.keys(schedule.divisions).map(cid => (
                    <div key={cid} style={{ flex: 1, backgroundColor: getCourseColor(cid) }} />
                  ))}
                </div>

                <div style={{ color: 'hsl(var(--text-secondary))', display: 'flex', flexDirection: 'column', gap: '0.15rem' }}>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span>우주공강:</span>
                    <span style={{ color: schedule.gapTime > 120 ? 'hsl(var(--danger))' : 'hsl(var(--text-primary))', fontWeight: 600 }}>
                      {schedule.gapTime === 0 ? '0분 (연강)' : `${Math.floor(schedule.gapTime / 60)}시간 ${schedule.gapTime % 60}분`}
                    </span>
                  </div>
                  <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                    <span>등교 요일:</span>
                    <span style={{ fontWeight: 600, color: 'hsl(var(--text-primary))' }}>{schedule.daysWithClasses}일 등교</span>
                  </div>
                </div>

                {/* Sub-list of division names in combination */}
                <div style={{ display: 'flex', flexWrap: 'wrap', gap: '0.2rem', marginTop: '0.2rem', opacity: 0.8 }}>
                  {Object.entries(schedule.divisions).map(([cid, div]) => (
                    <span
                      key={cid}
                      style={{
                        padding: '0.1rem 0.3rem',
                        borderRadius: '4px',
                        backgroundColor: 'hsl(var(--bg-tertiary))',
                        border: `1px solid ${getCourseColor(cid)}`,
                        fontSize: '0.65rem',
                        color: 'hsl(var(--text-primary))'
                      }}
                      title={`${getCourseName(cid)} (${div.divisionName}분반)`}
                    >
                      {div.divisionName}분반
                    </span>
                  ))}
                </div>
              </div>
            );
          })
        )}
      </div>

      {/* 3. Pagination Controls */}
      {totalPages > 1 && (
        <div
          style={{
            padding: '0.75rem',
            borderTop: '1px solid hsl(var(--border))',
            display: 'flex',
            alignItems: 'center',
            justifyContent: 'space-between',
            fontSize: '0.8rem'
          }}
        >
          <button
            className="btn btn-secondary"
            style={{ padding: '0.25rem', minWidth: 'auto' }}
            disabled={currentPage === 1}
            onClick={() => setCurrentPage(prev => prev - 1)}
          >
            <ChevronLeft size={16} />
          </button>
          
          <span style={{ color: 'hsl(var(--text-secondary))' }}>
            {currentPage} / {totalPages}
          </span>

          <button
            className="btn btn-secondary"
            style={{ padding: '0.25rem', minWidth: 'auto' }}
            disabled={currentPage === totalPages}
            onClick={() => setCurrentPage(prev => prev + 1)}
          >
            <ChevronRight size={16} />
          </button>
        </div>
      )}
    </div>
  );
};
