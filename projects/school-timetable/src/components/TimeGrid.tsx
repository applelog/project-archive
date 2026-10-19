import React from 'react';
import type { TimeSlot, ClassDivision, Course } from '../types';
import { DAY_NAMES } from '../types';

interface TimeGridProps {
  activeSchedule: { [courseId: string]: ClassDivision } | null;
  previewSchedule: { [courseId: string]: ClassDivision } | null;
  courses: Course[];
  theme: 'light' | 'dark';
}

export const TimeGrid: React.FC<TimeGridProps> = ({
  activeSchedule,
  previewSchedule,
  courses,
  theme
}) => {
  // Always show night classes (09:00 - 23:00) so night classes are never cut off
  const showNight = true;

  const startMin = 540; // 09:00
  const endMin = showNight ? 1380 : 1140; // 23:00 (1380) or 19:00 (1140)
  const totalMinutes = endMin - startMin;

  // Generate hourly labels for time axis
  const hours: number[] = [];
  const startHour = 9;
  const endHour = showNight ? 22 : 18;
  for (let h = startHour; h <= endHour; h++) {
    hours.push(h);
  }

  // Get color for a course
  const getCourseColor = (courseId: string): string => {
    const course = courses.find(c => c.id === courseId);
    return course ? course.color : 'hsl(263, 90%, 55%)';
  };

  // Get name for a course
  const getCourseName = (courseId: string): string => {
    const course = courses.find(c => c.id === courseId);
    return course ? course.name : '';
  };

  // Get code for a course
  const getCourseCode = (courseId: string): string => {
    const course = courses.find(c => c.id === courseId);
    return course ? course.code : '';
  };

  // Convert minutes to percentage top
  const getTopPercent = (minutes: number) => {
    const relative = minutes - startMin;
    return Math.max(0, (relative / totalMinutes) * 100);
  };

  // Convert duration to height percentage
  const getHeightPercent = (start: number, end: number) => {
    const duration = end - start;
    return Math.max(5, (duration / totalMinutes) * 100);
  };

  // Render a time slot on the grid
  const renderSlot = (
    slot: TimeSlot,
    courseId: string,
    divName: string,
    instructor: string,
    classroom: string,
    isPreview: boolean
  ) => {
    const color = getCourseColor(courseId);
    const courseName = getCourseName(courseId);
    const courseCode = getCourseCode(courseId);
    const top = getTopPercent(slot.start);
    const height = getHeightPercent(slot.start, slot.end);
    
    // Parse HSL parts to create a soft background and intense border
    const colorMatch = color.match(/hsl\((\d+),\s*(\d+)%,\s*(\d+)%\)/);
    let bgStyle: React.CSSProperties = { backgroundColor: color };
    let borderStyle = '1px solid rgba(255, 255, 255, 0.2)';

    if (colorMatch) {
      const [_, h, s, l] = colorMatch;
      const lightness = parseInt(l);
      if (theme === 'dark') {
        bgStyle = {
          backgroundColor: `hsla(${h}, ${s}%, ${lightness}%, ${isPreview ? 0.15 : 0.22})`,
          color: `hsl(${h}, ${s}%, ${Math.min(95, lightness + 20)}%)`,
          borderLeft: `4px solid hsl(${h}, ${s}%, ${lightness}%)`
        };
        borderStyle = `1px solid hsla(${h}, ${s}%, ${lightness}%, ${isPreview ? 0.25 : 0.5})`;
      } else {
        bgStyle = {
          backgroundColor: `hsla(${h}, ${s}%, ${lightness}%, ${isPreview ? 0.06 : 0.12})`,
          color: `hsl(${h}, ${s}%, ${lightness > 60 ? lightness - 30 : Math.max(15, lightness - 25)}%)`,
          borderLeft: `4px solid hsl(${h}, ${s}%, ${lightness}%)`
        };
        borderStyle = `1px solid hsla(${h}, ${s}%, ${lightness}%, ${isPreview ? 0.2 : 0.4})`;
      }
    }

    return (
      <div
        key={`${courseId}_${slot.day}_${slot.start}_${isPreview ? 'p' : 'a'}`}
        className={`class-block ${isPreview ? 'class-block-preview' : ''}`}
        style={{
          top: `${top}%`,
          height: `${height}%`,
          ...bgStyle,
          border: isPreview ? '1.5px dashed rgba(255,255,255,0.4)' : borderStyle,
          borderLeftWidth: isPreview ? '1.5px' : '4px'
        }}
        title={`${courseName} (${divName}분반) - ${instructor} | ${slot.displayStr} | ${classroom}`}
      >
        <div>
          <div className="class-block-title">{courseName}</div>
          {courseCode && (
            <div style={{ fontSize: '0.65rem', opacity: 0.6, fontFamily: 'monospace', marginTop: '0.05rem' }}>
              {courseCode}
            </div>
          )}
        </div>
        <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.65rem', opacity: 0.8 }}>
          <span>{divName}분반 | {instructor}</span>
          <span className="class-block-room">{classroom}</span>
        </div>
      </div>
    );
  };

  return (
    <div className="main-timetable-area">
      <div className="timetable-grid" style={{ height: '600px' }}>
        {/* Row 1: Header (Empty axis corner + Day columns headers) */}
        <div className="timetable-header-cell" style={{ gridRow: '1', borderRight: '1px solid var(--glass-border)' }}></div>
        {DAY_NAMES.map(day => (
          <div key={day} className="timetable-header-cell" style={{ gridRow: '1' }}>
            {day}요일
          </div>
        ))}

        {/* Time Axis Column + Grid Body */}
        <div style={{ gridColumn: '1', gridRow: '2', display: 'flex', flexDirection: 'column', position: 'relative' }}>
          {hours.map(h => (
            <div
              key={h}
              className="time-axis-cell"
              style={{
                height: `${(60 / totalMinutes) * 100}%`,
                position: 'absolute',
                top: `${((h * 60 - startMin) / totalMinutes) * 100}%`,
                left: 0,
                right: 0,
                borderBottom: '1px solid var(--glass-border)'
              }}
            >
              {h.toString().padStart(2, '0')}:00
            </div>
          ))}
        </div>

        {/* Day Columns (rendered as standard grid cells covering the calendar area) */}
        {DAY_NAMES.map((_, dayIdx) => (
          <div
            key={dayIdx}
            className="grid-day-column"
            style={{
              gridColumn: `${dayIdx + 2}`,
              gridRow: '2',
              height: '100%',
              position: 'relative'
            }}
          >
            {/* Draw hour guide lines */}
            {hours.map(h => (
              <div
                key={h}
                style={{
                  position: 'absolute',
                  top: `${((h * 60 - startMin) / totalMinutes) * 100}%`,
                  left: 0,
                  right: 0,
                  borderBottom: theme === 'dark'
                    ? '1px dashed rgba(255, 255, 255, 0.08)'
                    : '1px dashed rgba(0, 0, 0, 0.06)',
                  height: 1
                }}
              />
            ))}

            {/* Render Active Schedule Slots (solid) */}
            {activeSchedule &&
              Object.entries(activeSchedule).map(([courseId, div]) =>
                div.slots
                  .filter(slot => slot.day === dayIdx)
                  .map(slot =>
                    renderSlot(
                      slot,
                      courseId,
                      div.divisionName,
                      div.instructor,
                      div.classroom || '',
                      false
                    )
                  )
              )}

            {/* Render Preview/Hover Schedule Slots (semi-transparent dashed) */}
            {previewSchedule &&
              Object.entries(previewSchedule).map(([courseId, div]) =>
                div.slots
                  .filter(slot => slot.day === dayIdx)
                  .map(slot =>
                    renderSlot(
                      slot,
                      courseId,
                      div.divisionName,
                      div.instructor,
                      div.classroom || '',
                      true
                    )
                  )
              )}
          </div>
        ))}
      </div>
    </div>
  );
};
