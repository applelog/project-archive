import { describe, it, expect } from 'vitest';
import { parseTimeSlots, parseBookletLine, parseDivisionLine, preprocessBookletText } from '../parser';

describe('brochure parser tests', () => {
  describe('parseTimeSlots', () => {
    it('should parse direct HH:MM time ranges', () => {
      const slots = parseTimeSlots('월11:50-14:30');
      expect(slots).toHaveLength(1);
      expect(slots[0].day).toBe(0); // Mon
      expect(slots[0].start).toBe(11 * 60 + 50); // 710
      expect(slots[0].end).toBe(14 * 60 + 30); // 870
    });

    it('should parse direct time ranges with spaces and tildes', () => {
      const slots = parseTimeSlots('화11:50 ~ 13:05');
      expect(slots).toHaveLength(1);
      expect(slots[0].day).toBe(1); // Tue
      expect(slots[0].start).toBe(710);
      expect(slots[0].end).toBe(13 * 60 + 5); // 785
    });

    it('should parse period shorthand notation (e.g. 4ab5ab6ab)', () => {
      const slots = parseTimeSlots('월4ab5ab6ab');
      expect(slots).toHaveLength(1); // should merge contiguous
      expect(slots[0].day).toBe(0); // Mon
      expect(slots[0].start).toBe(710); // 4a start: 11:50 (710)
      expect(slots[0].end).toBe(870); // 6b end: 14:30 (870)
    });

    it('should parse comma-separated periods (e.g. 3,4)', () => {
      const slots = parseTimeSlots('화3,4');
      expect(slots).toHaveLength(1);
      expect(slots[0].day).toBe(1); // Tue
      expect(slots[0].start).toBe(650); // 3a start: 10:50 (650)
      expect(slots[0].end).toBe(760); // 4b end: 12:40 (760) (note: 4a starts at 11:50, 3b ends at 11:40. Adjacent with small gap)
    });

    it('should parse multi-day slot strings', () => {
      const slots = parseTimeSlots('화11:50-13:05, 수13:15-14:30');
      expect(slots).toHaveLength(2);
      expect(slots[0].day).toBe(1);
      expect(slots[1].day).toBe(2);
    });
  });

  describe('parseBookletLine', () => {
    it('should parse a typical pipe-delimited brochure row', () => {
      const line = '00 | 보육실습 | 3 | 3 | 교102 | 장교수 | 금09:00-11:40';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseName).toBe('보육실습');
      expect(parsed?.instructor).toBe('장교수');
      expect(parsed?.classroom).toBe('교102');
      expect(parsed?.credits).toBe(3);
      expect(parsed?.divisionName).toBe('00');
      expect(parsed?.slots).toHaveLength(1);
      expect(parsed?.slots[0].day).toBe(4); // Fri
    });

    it('should parse a space-delimited brochure row with shorthand periods', () => {
      const line = '00 신입생세미나II 1 1 샬308 정교수 목14:40-17:20';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseName).toBe('신입생세미나II');
      expect(parsed?.instructor).toBe('정교수');
      expect(parsed?.classroom).toBe('샬308');
      expect(parsed?.divisionName).toBe('00');
      expect(parsed?.slots).toHaveLength(1);
      expect(parsed?.slots[0].day).toBe(3); // Thu
    });

    it('should parse user format with category keywords', () => {
      const line = '2 전공선택 EB14202 01 머신러닝 3 3 심산225 홍길동 월11:50-14:30';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseCode).toBe('EB14202');
      expect(parsed?.divisionName).toBe('01');
      expect(parsed?.courseName).toBe('머신러닝');
      expect(parsed?.classroom).toBe('심산225');
      expect(parsed?.instructor).toBe('홍길동');
      expect(parsed?.credits).toBe(3);
      expect(parsed?.slots).toHaveLength(1);
      expect(parsed?.slots[0].day).toBe(0); // Mon
      expect(parsed?.slots[0].start).toBe(710); // 11:50
    });

    it('should extract instructor as the token right before the time slot', () => {
      const line = '2 전공선택 BB14201 02 노인복지론 3 3 샬304 박교수 화11:50-13:05';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.instructor).toBe('박교수');
    });

    it('should parse 0 credit courses correctly (e.g. Chapel)', () => {
      const line = '2 기초교양 ND01609 02 채플(행복나눔)IV 종합적사고 0 0 우1 대강당 이교수 목14:40-15:30';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseCode).toBe('ND01609');
      expect(parsed?.courseName).toBe('채플(행복나눔)IV');
      expect(parsed?.credits).toBe(0);
      expect(parsed?.instructor).toBe('이교수');
    });

    it('should parse lines containing various core competencies (핵심역량)', () => {
      const line = '2 전공선택 BB14201 02 노인복지론 창의융합 3 3 샬304 박교수 화11:50-13:05, 수13:15-14:30';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseName).toBe('노인복지론');
      expect(parsed?.instructor).toBe('박교수');
    });

    it('should parse lines with core competency containing space or special characters', () => {
      const line = '2 전공선택 BB14201 02 노인복지론 소통·협력 3 3 샬304 박교수 화11:50-13:05';
      const parsed = parseBookletLine(line);
      expect(parsed).not.toBeNull();
      expect(parsed?.courseName).toBe('노인복지론');
      expect(parsed?.instructor).toBe('박교수');
    });
  });

  describe('parseDivisionLine', () => {
    it('should parse raw division lines successfully inside a course card context', () => {
      const line = '01 홍길동 심산225 월11:50-14:30';
      const parsed = parseDivisionLine(line, 'EB14202');
      expect(parsed).not.toBeNull();
      expect(parsed?.divisionName).toBe('01');
      expect(parsed?.instructor).toBe('홍길동');
      expect(parsed?.classroom).toBe('심산225');
      expect(parsed?.slots).toHaveLength(1);
      expect(parsed?.slots[0].day).toBe(0);
    });

    it('should parse division lines even with name trailing', () => {
      const line = '02 화11:50-14:30 홍길동';
      const parsed = parseDivisionLine(line, 'EB14202');
      expect(parsed).not.toBeNull();
      expect(parsed?.divisionName).toBe('02');
      expect(parsed?.instructor).toBe('홍길동');
      expect(parsed?.classroom).toBe('강의실 미정');
      expect(parsed?.slots).toHaveLength(1);
      expect(parsed?.slots[0].day).toBe(1);
    });
  });

  describe('preprocessBookletText', () => {
    it('should merge multiline split time rows correctly', () => {
      const text = 
        '2 전공선택 BB14201 02 노인복지론 3 3 샬304 박영선 화11:50-13:05,\n' +
        '수13:15-14:30';
      const lines = preprocessBookletText(text);
      expect(lines).toHaveLength(1);
      expect(lines[0]).toBe('2 전공선택 BB14201 02 노인복지론 3 3 샬304 박영선 화11:50-13:05, 수13:15-14:30');
    });
  });
});
