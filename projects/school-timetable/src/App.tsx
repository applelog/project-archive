import React, { useState, useMemo, useEffect } from 'react';
import type { Course, ClassDivision } from './types';
import { generateSchedules } from './utils/scheduler';
import { CourseInput } from './components/CourseInput';
import { TimeGrid } from './components/TimeGrid';

import { ScheduleList } from './components/ScheduleList';
import { Sparkles, Clock } from 'lucide-react';

const MOCK_COURSES: Course[] = [
  {
    id: 'EB14202',
    name: '머신러닝 [인공지능]',
    code: 'EB14202',
    color: 'hsl(263, 85%, 60%)',
    credits: 3,
    divisions: [
      {
        id: 'EB14202_01',
        divisionName: '01',
        instructor: '김철수',
        classroom: '심산225',
        slots: [{ day: 0, start: 710, end: 870, displayStr: '월 11:50-14:30' }]
      },
      {
        id: 'EB14202_02',
        divisionName: '02',
        instructor: '김철수',
        classroom: '심산225',
        slots: [{ day: 1, start: 710, end: 870, displayStr: '화 11:50-14:30' }]
      }
    ]
  },
  {
    id: 'EB14204',
    name: '신경망 [인공지능]',
    code: 'EB14204',
    color: 'hsl(180, 85%, 45%)',
    credits: 3,
    divisions: [
      {
        id: 'EB14204_01',
        divisionName: '01',
        instructor: '이영희',
        classroom: '심산224',
        slots: [{ day: 2, start: 540, end: 700, displayStr: '수 09:00-11:40' }]
      }
    ]
  },
  {
    id: 'BB04101',
    name: '인간행동과사회환경 [사회복지]',
    code: 'BB04101',
    color: 'hsl(142, 70%, 45%)',
    credits: 3,
    divisions: [
      {
        id: 'BB04101_01',
        divisionName: '01',
        instructor: '박교수',
        classroom: '샬304',
        slots: [{ day: 2, start: 540, end: 700, displayStr: '수 09:00-11:40' }]
      },
      {
        id: 'BB04101_02',
        divisionName: '02',
        instructor: '최교수',
        classroom: '샬204',
        slots: [{ day: 2, start: 540, end: 700, displayStr: '수 09:00-11:40' }]
      }
    ]
  },
  {
    id: 'BB14201',
    name: '노인복지론 [사회복지]',
    code: 'BB14201',
    color: 'hsl(35, 90%, 50%)',
    credits: 3,
    divisions: [
      {
        id: 'BB14201_01',
        divisionName: '01',
        instructor: '정교수',
        classroom: '샬309',
        slots: [{ day: 3, start: 540, end: 700, displayStr: '목 09:00-11:40' }]
      },
      {
        id: 'BB14201_02',
        divisionName: '02',
        instructor: '박교수',
        classroom: '샬304',
        slots: [
          { day: 1, start: 710, end: 785, displayStr: '화 11:50-13:05' },
          { day: 2, start: 795, end: 870, displayStr: '수 13:15-14:30' }
        ]
      }
    ]
  },
  {
    id: 'BD14101',
    name: 'VR메이커스페이스 [Wel-Tech]',
    code: 'BD14101',
    color: 'hsl(320, 80%, 55%)',
    credits: 3,
    divisions: [
      {
        id: 'BD14101_00',
        divisionName: '00',
        instructor: '강교수',
        classroom: '심산220',
        slots: [{ day: 0, start: 880, end: 1040, displayStr: '월 14:40-17:20' }]
      }
    ]
  },
  {
    id: 'BD14201',
    name: '디지털복지기술 [Wel-Tech]',
    code: 'BD14201',
    color: 'hsl(210, 85%, 55%)',
    credits: 3,
    divisions: [
      {
        id: 'BD14201_00',
        divisionName: '00',
        instructor: '조교수',
        classroom: '경208',
        slots: [{ day: 4, start: 540, end: 700, displayStr: '금 09:00-11:40' }]
      }
    ]
  },
  {
    id: 'NE11702',
    name: '서양역사의이해 [균형교양]',
    code: 'NE11702',
    color: 'hsl(0, 85%, 60%)',
    credits: 3,
    divisions: [
      {
        id: 'NE11702_00',
        divisionName: '00',
        instructor: '한교수',
        classroom: '샬808',
        slots: [{ day: 0, start: 710, end: 870, displayStr: '월 11:50-14:30' }]
      }
    ]
  }
];

export const App: React.FC = () => {
  // Load initial courses from localStorage or default to mock courses
  const [courses, setCourses] = useState<Course[]>(() => {
    const saved = localStorage.getItem('schedules_courses');
    return saved ? JSON.parse(saved) : MOCK_COURSES;
  });

  const [activeSchedule, setActiveSchedule] = useState<{ [courseId: string]: ClassDivision } | null>(() => {
    const saved = localStorage.getItem('schedules_active');
    return saved ? JSON.parse(saved) : null;
  });

  const [previewSchedule, setPreviewSchedule] = useState<{ [courseId: string]: ClassDivision } | null>(null);

  // Theme state: 'light' | 'dark' | 'auto'
  const [themeMode, setThemeMode] = useState<'light' | 'dark' | 'auto'>(() => {
    const saved = localStorage.getItem('schedules_theme_mode');
    return (saved as 'light' | 'dark' | 'auto') || 'auto';
  });

  const [resolvedTheme, setResolvedTheme] = useState<'light' | 'dark'>('light');

  // Sync theme mode to DOM and localStorage
  useEffect(() => {
    localStorage.setItem('schedules_theme_mode', themeMode);
    
    const updateTheme = () => {
      let targetTheme: 'light' | 'dark' = 'light';
      if (themeMode === 'light') {
        targetTheme = 'light';
      } else if (themeMode === 'dark') {
        targetTheme = 'dark';
      } else {
        // Auto: Night (Sunset to Sunrise) = 18:00 PM to 07:00 AM
        const hour = new Date().getHours();
        const isNight = hour >= 18 || hour < 7;
        if (isNight) {
          targetTheme = 'dark';
        } else if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
          targetTheme = 'dark';
        } else {
          targetTheme = 'light';
        }
      }
      
      setResolvedTheme(targetTheme);
      document.documentElement.setAttribute('data-theme', targetTheme);
    };

    updateTheme();

    if (themeMode === 'auto') {
      const mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
      const handleSystemChange = () => updateTheme();
      mediaQuery.addEventListener('change', handleSystemChange);
      
      const intervalId = window.setInterval(updateTheme, 60000);
      
      return () => {
        mediaQuery.removeEventListener('change', handleSystemChange);
        clearInterval(intervalId);
      };
    }
  }, [themeMode]);

  // Sync to localStorage
  useEffect(() => {
    localStorage.setItem('schedules_courses', JSON.stringify(courses));
  }, [courses]);

  useEffect(() => {
    if (activeSchedule) {
      localStorage.setItem('schedules_active', JSON.stringify(activeSchedule));
    } else {
      localStorage.removeItem('schedules_active');
    }
  }, [activeSchedule]);

  // Calculate schedules combination dynamically
  const schedules = useMemo(() => {
    return generateSchedules(courses);
  }, [courses]);

  // Auto-select first schedule on load if nothing is selected
  useEffect(() => {
    if (!activeSchedule && schedules.length > 0) {
      setActiveSchedule(schedules[0].divisions);
    }
  }, [schedules, activeSchedule]);

  // Actions
  const handleAddCourse = (course: Course) => {
    setCourses(prev => [...prev, course]);
  };

  const handleDeleteCourse = (courseId: string) => {
    setCourses(prev => prev.filter(c => c.id !== courseId));
    // Clear active selection if it contains deleted course
    if (activeSchedule && activeSchedule[courseId]) {
      setActiveSchedule(null);
    }
  };

  const handleUpdateCourse = (courseId: string, updatedFields: Partial<Course>) => {
    setCourses(prev =>
      prev.map(c => {
        if (c.id !== courseId) return c;
        return {
          ...c,
          ...updatedFields
        };
      })
    );
    if (updatedFields.active !== undefined) {
      setActiveSchedule(null);
    }
  };

  const handleAddDivision = (courseId: string, division: ClassDivision) => {
    setCourses(prev =>
      prev.map(c => {
        if (c.id !== courseId) return c;
        // Avoid duplicate division names
        if (c.divisions.some(d => d.divisionName === division.divisionName)) {
          return c;
        }
        return {
          ...c,
          divisions: [...c.divisions, division]
        };
      })
    );
    setActiveSchedule(null); // Force recalculating selected option
  };

  const handleDeleteDivision = (courseId: string, divisionId: string) => {
    setCourses(prev =>
      prev.map(c => {
        if (c.id !== courseId) return c;
        return {
          ...c,
          divisions: c.divisions.filter(d => d.id !== divisionId)
        };
      })
    );
    if (activeSchedule && activeSchedule[courseId]?.id === divisionId) {
      setActiveSchedule(null);
    }
  };

  const handleImportBulk = (imported: Course[]) => {
    setCourses(prev => {
      const merged = [...prev];
      imported.forEach(imp => {
        const existingIdx = merged.findIndex(c => (imp.code && c.code === imp.code) || c.name === imp.name);
        if (existingIdx !== -1) {
          // Merge divisions
          const existing = merged[existingIdx];
          imp.divisions.forEach(newDiv => {
            const hasDiv = existing.divisions.some(d => d.divisionName === newDiv.divisionName);
            if (!hasDiv) {
              existing.divisions.push(newDiv);
            }
          });
        } else {
          merged.push(imp);
        }
      });
      return merged;
    });
    setActiveSchedule(null);
  };

  // Export to JSON
  const handleExportJSON = () => {
    const exportData = {
      courses,
      activeSchedule
    };
    const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(exportData, null, 2));
    const downloadAnchor = document.createElement('a');
    downloadAnchor.setAttribute("href", dataStr);
    downloadAnchor.setAttribute("download", `timetable-${new Date().toISOString().slice(0, 10)}.json`);
    document.body.appendChild(downloadAnchor);
    downloadAnchor.click();
    downloadAnchor.remove();
  };

  // Import from JSON
  const handleImportJSON = (e: React.ChangeEvent<HTMLInputElement>) => {
    const fileReader = new FileReader();
    if (e.target.files && e.target.files.length > 0) {
      fileReader.readAsText(e.target.files[0], "UTF-8");
      fileReader.onload = event => {
        try {
          const parsed = JSON.parse(event.target?.result as string);
          let loadedCourses: Course[] = [];
          let loadedActive: { [courseId: string]: ClassDivision } | null = null;

          if (Array.isArray(parsed)) {
            // Backward compatibility
            loadedCourses = parsed;
          } else if (parsed && typeof parsed === 'object') {
            loadedCourses = Array.isArray(parsed.courses) ? parsed.courses : [];
            loadedActive = parsed.activeSchedule || null;
          } else {
            alert('올바른 시간표 백업 파일 형식이 아닙니다.');
            e.target.value = '';
            return;
          }

          setCourses(loadedCourses);
          setActiveSchedule(loadedActive);
          // Clear file input value to allow re-upload
          e.target.value = '';
          alert('시간표 데이터를 성공적으로 불러왔습니다!');
        } catch (err) {
          alert('파일을 읽는 중 오류가 발생했습니다.');
          e.target.value = '';
        }
      };
    }
  };

  const handleReset = () => {
    if (window.confirm('모든 데이터가 초기화되고 강남대 예시 과목 데이터로 리셋됩니다. 진행할까요?')) {
      setCourses(MOCK_COURSES);
      setActiveSchedule(null);
      setPreviewSchedule(null);
    }
  };

  return (
    <div className="app-container">
      
      {/* 1. Sidebar - Course list & additions */}
      <aside className="sidebar">
        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.25rem' }}>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', width: '100%' }}>
            <h1 style={{ display: 'flex', alignItems: 'center', gap: '0.4rem', fontSize: '1.25rem' }}>
              <Sparkles size={18} style={{ color: 'hsl(var(--knu-blue))', fill: 'hsl(var(--knu-blue) / 0.1)' }} />
              시간표 마법사
            </h1>
            {/* Theme switcher */}
            <div style={{ display: 'flex', gap: '0.15rem', background: resolvedTheme === 'dark' ? 'rgba(255,255,255,0.06)' : 'rgba(0,0,0,0.05)', padding: '0.15rem', borderRadius: '6px' }}>
              <button
                onClick={() => setThemeMode('light')}
                title="밝은 모드 고정"
                style={{
                  border: 'none',
                  background: themeMode === 'light' ? 'hsl(var(--knu-blue))' : 'transparent',
                  color: themeMode === 'light' ? '#fff' : (resolvedTheme === 'dark' ? '#94A3B8' : '#475569'),
                  padding: '0.15rem 0.35rem',
                  fontSize: '0.65rem',
                  borderRadius: '4px',
                  cursor: 'pointer',
                  fontWeight: 600,
                  transition: 'all 0.1s ease'
                }}
              >
                낮
              </button>
              <button
                onClick={() => setThemeMode('dark')}
                title="어두운 모드 고정"
                style={{
                  border: 'none',
                  background: themeMode === 'dark' ? 'hsl(var(--knu-blue))' : 'transparent',
                  color: themeMode === 'dark' ? '#fff' : (resolvedTheme === 'dark' ? '#94A3B8' : '#475569'),
                  padding: '0.15rem 0.35rem',
                  fontSize: '0.65rem',
                  borderRadius: '4px',
                  cursor: 'pointer',
                  fontWeight: 600,
                  transition: 'all 0.1s ease'
                }}
              >
                밤
              </button>
              <button
                onClick={() => setThemeMode('auto')}
                title="시간(일몰) 및 시스템 설정에 맞춰 자동 변경"
                style={{
                  border: 'none',
                  background: themeMode === 'auto' ? 'hsl(var(--knu-blue))' : 'transparent',
                  color: themeMode === 'auto' ? '#fff' : (resolvedTheme === 'dark' ? '#94A3B8' : '#475569'),
                  padding: '0.15rem 0.35rem',
                  fontSize: '0.65rem',
                  borderRadius: '4px',
                  cursor: 'pointer',
                  fontWeight: 600,
                  transition: 'all 0.1s ease'
                }}
              >
                자동
              </button>
            </div>
          </div>
          <span className="subtitle" style={{ fontSize: '0.75rem' }}>Kangnam University Edition</span>
        </div>

        <div style={{ display: 'flex', flexDirection: 'column', gap: '0.5rem', width: '100%', marginTop: '0.25rem' }}>
          <div style={{ display: 'flex', gap: '0.3rem', width: '100%' }}>
            <button
              className="btn btn-secondary"
              style={{ flex: 1, padding: '0.3rem 0.5rem', fontSize: '0.7rem', borderRadius: '6px' }}
              onClick={handleExportJSON}
              title="시간표 백업 파일 내보내기"
            >
              내보내기 (JSON)
            </button>
            <label
              htmlFor="import-json-file"
              className="btn btn-secondary"
              style={{ flex: 1, padding: '0.3rem 0.5rem', fontSize: '0.7rem', borderRadius: '6px', cursor: 'pointer', margin: 0, textAlign: 'center' }}
              title="시간표 백업 파일 가져오기"
            >
              불러오기
            </label>
            <input
              id="import-json-file"
              type="file"
              accept=".json"
              onChange={handleImportJSON}
              style={{ display: 'none' }}
            />
            <button
              className="btn btn-danger"
              style={{ padding: '0.3rem 0.6rem', fontSize: '0.7rem', borderRadius: '6px' }}
              onClick={handleReset}
              title="초기 예시 데이터로 리셋"
            >
              리셋
            </button>
          </div>
        </div>

        <CourseInput
          courses={courses}
          onAddCourse={handleAddCourse}
          onDeleteCourse={handleDeleteCourse}
          onAddDivision={handleAddDivision}
          onDeleteDivision={handleDeleteDivision}
          onUpdateCourse={handleUpdateCourse}
          onImportBulk={handleImportBulk}
        />
      </aside>

      {/* 2. Visual Calendar Area & Combination recommendation list */}
      <div className="grid-container">
        
        {/* Top Navbar */}
        <header className="grid-header">
          <div style={{ display: 'flex', alignItems: 'center', gap: '1rem' }}>
            <h2 style={{ fontSize: '1.15rem', display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
              <Clock size={18} style={{ color: 'hsl(var(--knu-blue))' }} />
              시각 시간표 그리드
            </h2>
            {activeSchedule && (
              <span className="badge badge-violet" style={{ fontSize: '0.8rem' }}>
                총 이수학점: {Object.keys(activeSchedule).reduce((acc, cid) => {
                  const course = courses.find(c => c.id === cid);
                  return acc + (course?.credits || 0);
                }, 0)}학점
              </span>
            )}
          </div>
        </header>

        {/* Contents grid */}
        <div className="grid-content">
          {/* Combinations column */}
          <ScheduleList
            schedules={schedules}
            activeSchedule={activeSchedule}
            courses={courses}
            onHoverSchedule={setPreviewSchedule}
            onSelectSchedule={setActiveSchedule}
          />

          {/* Visual weekly calendar grid */}
          <TimeGrid
            activeSchedule={activeSchedule}
            previewSchedule={previewSchedule}
            courses={courses}
            theme={resolvedTheme}
          />
        </div>
      </div>

    </div>
  );
};
