import { describe, it, expect } from 'vitest';
import { isSlotOverlapping, generateSchedules, filterAndSortSchedules } from '../scheduler';
import type { Course, FilterOptions } from '../../types';

describe('timetable scheduler tests', () => {
  describe('isSlotOverlapping', () => {
    it('should detect overlap when slots intersect', () => {
      // Mon 09:00 - 10:00 (540 - 600)
      const slot1 = { day: 0, start: 540, end: 600, displayStr: '월 09:00-10:00' };
      // Mon 09:30 - 10:30 (570 - 630)
      const slot2 = { day: 0, start: 570, end: 630, displayStr: '월 09:30-10:30' };
      
      expect(isSlotOverlapping(slot1, slot2)).toBe(true);
    });

    it('should not detect overlap when slots are contiguous but do not overlap', () => {
      // Mon 09:00 - 10:00
      const slot1 = { day: 0, start: 540, end: 600, displayStr: '월 09:00-10:00' };
      // Mon 10:00 - 11:00
      const slot2 = { day: 0, start: 600, end: 660, displayStr: '월 10:00-11:00' };
      
      expect(isSlotOverlapping(slot1, slot2)).toBe(false);
    });

    it('should not detect overlap on different days', () => {
      // Mon 09:00 - 10:00
      const slot1 = { day: 0, start: 540, end: 600, displayStr: '월 09:00-10:00' };
      // Tue 09:00 - 10:00
      const slot2 = { day: 1, start: 540, end: 600, displayStr: '화 09:00-10:00' };
      
      expect(isSlotOverlapping(slot1, slot2)).toBe(false);
    });
  });

  describe('generateSchedules', () => {
    it('should generate valid conflict-free combinations', () => {
      const mockCourses: Course[] = [
        {
          id: 'MATH',
          name: 'Calculus',
          code: 'MATH101',
          color: '#ff0000',
          credits: 3,
          divisions: [
            {
              id: 'MATH_01',
              divisionName: '01',
              instructor: 'MathProf',
              slots: [{ day: 0, start: 540, end: 600, displayStr: '월 09:00-10:00' }] // Mon 9-10
            },
            {
              id: 'MATH_02',
              divisionName: '02',
              instructor: 'MathProf',
              slots: [{ day: 0, start: 600, end: 660, displayStr: '월 10:00-11:00' }] // Mon 10-11
            }
          ]
        },
        {
          id: 'PHYS',
          name: 'Physics',
          code: 'PHYS101',
          color: '#00ff00',
          credits: 3,
          divisions: [
            {
              id: 'PHYS_01',
              divisionName: '01',
              instructor: 'PhysProf',
              slots: [{ day: 0, start: 570, end: 630, displayStr: '월 09:30-10:30' }] // Mon 9:30-10:30 (Conflicts with MATH_01 and MATH_02)
            },
            {
              id: 'PHYS_02',
              divisionName: '02',
              instructor: 'PhysProf',
              slots: [{ day: 1, start: 540, end: 600, displayStr: '화 09:00-10:00' }] // Tue 9-10 (No conflict)
            }
          ]
        }
      ];

      const schedules = generateSchedules(mockCourses);
      
      expect(schedules).toHaveLength(2);
      
      const d1 = schedules[0].divisions;
      expect(d1['MATH'].id).toBe('MATH_01');
      expect(d1['PHYS'].id).toBe('PHYS_02');

      const d2 = schedules[1].divisions;
      expect(d2['MATH'].id).toBe('MATH_02');
      expect(d2['PHYS'].id).toBe('PHYS_02');
    });
  });

  describe('filterAndSortSchedules professor filter', () => {
    it('should correctly filter by preferred instructors and handle empty array properly', () => {
      const mockCourses: Course[] = [
        {
          id: 'COURSE1',
          name: 'Algorithm',
          code: 'CS101',
          color: '#ff0000',
          credits: 3,
          divisions: [
            {
              id: 'CS101_01',
              divisionName: '01',
              instructor: '김철수',
              slots: [{ day: 0, start: 540, end: 600, displayStr: '월 09:00-10:00' }]
            },
            {
              id: 'CS101_02',
              divisionName: '02',
              instructor: '이영희',
              slots: [{ day: 1, start: 540, end: 600, displayStr: '화 09:00-10:00' }]
            }
          ]
        }
      ];

      const allSchedules = generateSchedules(mockCourses);
      expect(allSchedules).toHaveLength(2);

      const baseOptions: FilterOptions = {
        emptyFriday: false,
        noMorning: false,
        noNight: false,
        lunchBreak: false,
        maxGapHours: 24,
        sortBy: 'gapAsc'
      };

      // 1. Filter only '이영희'
      const filteredLee = filterAndSortSchedules(allSchedules, {
        ...baseOptions,
        preferredInstructors: {
          COURSE1: ['이영희']
        }
      });
      expect(filteredLee).toHaveLength(1);
      expect(filteredLee[0].divisions['COURSE1'].instructor).toBe('이영희');

      // 2. Uncheck all (preferredInstructors: { COURSE1: [] }) -> should yield 0 schedules
      const filteredNone = filterAndSortSchedules(allSchedules, {
        ...baseOptions,
        preferredInstructors: {
          COURSE1: []
        }
      });
      expect(filteredNone).toHaveLength(0);

      // 3. Both checked ['김철수', '이영희'] -> should yield 2 schedules
      const filteredBoth = filterAndSortSchedules(allSchedules, {
        ...baseOptions,
        preferredInstructors: {
          COURSE1: ['김철수', '이영희']
        }
      });
      expect(filteredBoth).toHaveLength(2);
    });
  });
});

