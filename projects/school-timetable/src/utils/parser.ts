import type { TimeSlot, ClassDivision } from '../types';
import { PERIODS, getDayIndex } from '../types';

// Helper to convert minutes to "HH:MM"
export function minutesToTimeStr(minutes: number): string {
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  return `${h.toString().padStart(2, '0')}:${m.toString().padStart(2, '0')}`;
}

// Merge overlapping or adjacent time slots on the same day
export function mergeSlots(slots: TimeSlot[]): TimeSlot[] {
  if (slots.length <= 1) return slots;
  
  // Group slots by day
  const slotsByDay: { [day: number]: TimeSlot[] } = {};
  slots.forEach(slot => {
    if (!slotsByDay[slot.day]) slotsByDay[slot.day] = [];
    slotsByDay[slot.day].push(slot);
  });

  const merged: TimeSlot[] = [];

  Object.keys(slotsByDay).forEach(dayKey => {
    const day = parseInt(dayKey);
    const daySlots = slotsByDay[day].sort((a, b) => a.start - b.start);
    
    let current = daySlots[0];
    for (let i = 1; i < daySlots.length; i++) {
      const next = daySlots[i];
      // If overlapping or adjacent (Kangnam Univ has 1-minute gaps like 09:25 and 09:26, treat <= 15 min gap as contiguous)
      if (next.start <= current.end + 15) {
        current = {
          day,
          start: current.start,
          end: Math.max(current.end, next.end),
          displayStr: `${current.displayStr.split(' ')[0]} ${minutesToTimeStr(current.start)}-${minutesToTimeStr(Math.max(current.end, next.end))}`
        };
      } else {
        merged.push(current);
        current = next;
      }
    }
    merged.push(current);
  });

  return merged;
}

// Parse a single shorthand time slot, e.g., "월4ab5ab6ab" or "화3,4"
export function parseShorthandSlot(dayStr: string, periodStr: string): TimeSlot[] {
  const day = getDayIndex(dayStr);
  if (day === -1) return [];

  const slots: TimeSlot[] = [];
  
  // Regex to extract digits and optional suffix: e.g. "4ab", "5a", "3", "4"
  const regex = /(\d+)(ab|a|b)?/g;
  let match;
  while ((match = regex.exec(periodStr)) !== null) {
    const num = match[1];
    const suffix = match[2] || '';
    
    const targetPeriods: string[] = [];
    if (suffix === 'ab') {
      targetPeriods.push(num + 'a');
      targetPeriods.push(num + 'b');
    } else if (suffix === 'a' || suffix === 'b') {
      targetPeriods.push(num + suffix);
    } else {
      // Default to both a and b periods if no suffix
      targetPeriods.push(num + 'a');
      targetPeriods.push(num + 'b');
    }

    targetPeriods.forEach(p => {
      if (PERIODS[p]) {
        slots.push({
          day,
          start: PERIODS[p].startMin,
          end: PERIODS[p].endMin,
          displayStr: `${dayStr} ${PERIODS[p].start}-${PERIODS[p].end}`
        });
      }
    });
  }

  return mergeSlots(slots);
}

// Parse a single time string like "월11:50-14:30" or "화11:50 - 13:05"
export function parseDirectSlot(dayStr: string, timeRangeStr: string): TimeSlot | null {
  const day = getDayIndex(dayStr);
  if (day === -1) return null;

  // Match HH:MM-HH:MM
  const timeRegex = /(\d{1,2}):(\d{2})\s*[-~~]\s*(\d{1,2}):(\d{2})/;
  const match = timeRangeStr.match(timeRegex);
  if (!match) return null;

  const startHour = parseInt(match[1]);
  const startMin = parseInt(match[2]);
  const endHour = parseInt(match[3]);
  const endMin = parseInt(match[4]);

  const start = startHour * 60 + startMin;
  const end = endHour * 60 + endMin;

  return {
    day,
    start,
    end,
    displayStr: `${dayStr} ${minutesToTimeStr(start)}-${minutesToTimeStr(end)}`
  };
}

// Parse multiple slots, e.g., "화11:50-13:05, 수13:15-14:30" or "월4ab5ab6ab / 목1ab2ab"
export function parseTimeSlots(input: string): TimeSlot[] {
  const cleaned = input.trim();
  if (!cleaned) return [];

  const slots: TimeSlot[] = [];
  
  // Find each day prefix and its subsequent period specifiers
  const dayPattern = /([월화수목금토])([^월화수목금토]*)/g;
  let match;
  
  while ((match = dayPattern.exec(cleaned)) !== null) {
    const dayStr = match[1];
    const timeOrPeriodStr = match[2].trim().replace(/[,/;|\s]+$/, '');
    if (!timeOrPeriodStr) continue;

    if (timeOrPeriodStr.includes(':')) {
      // Direct time format (HH:MM-HH:MM)
      const timeRegex = /(\d{1,2}):(\d{2})\s*[-~~]\s*(\d{1,2}):(\d{2})/g;
      let timeMatch;
      while ((timeMatch = timeRegex.exec(timeOrPeriodStr)) !== null) {
        const directSlot = parseDirectSlot(dayStr, timeMatch[0]);
        if (directSlot) slots.push(directSlot);
      }
    } else {
      // Shorthand period format
      const shorthandSlots = parseShorthandSlot(dayStr, timeOrPeriodStr);
      slots.push(...shorthandSlots);
    }
  }

  return mergeSlots(slots);
}

// Smart Parser for a copy-pasted row from the booklet
export function parseBookletLine(line: string, courseIdPrefix: string = 'course'): {
  courseCode: string;
  courseName: string;
  divisionName: string;
  instructor: string;
  classroom: string;
  slots: TimeSlot[];
  credits: number;
} | null {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith('#')) return null;

  // Split line by pipe, tab, or double+ spaces
  let tokens = trimmed.split(/[|\t]|\s{2,}/).map(t => t.trim()).filter(Boolean);
  if (tokens.length <= 1) {
    tokens = trimmed.split(/\s+/).map(t => t.trim()).filter(Boolean);
  }

  if (tokens.length === 0) return null;

  let courseCode = '';
  let classroom = '';
  let credits = 3;
  let divisionName = '01';

  // Find course code
  const codeRegex = /^[A-Z]{2,4}\d{4,5}/i;
  // Detect time slots starting with day name followed by digit, space, or end of token
  const timeSlotRegex = /^[월화수목금토](\d|\s|$)/;

  const potentialSlots: string[] = [];

  tokens.forEach(token => {
    if (codeRegex.test(token)) {
      courseCode = token;
    } else if (timeSlotRegex.test(token) || token.includes('원격수업') || token.includes('S러닝')) {
      potentialSlots.push(token);
    } else if (/^\d+$/.test(token)) {
      const val = parseInt(token);
      if (val >= 0 && val <= 6) { // Allow 0 credits (e.g. Chapel)
        credits = val;
      }
    }
  });

  const timeStr = potentialSlots.join(', ');
  const slots = parseTimeSlots(timeStr);

  // Classroom pattern: look for classroom codes (like 샬808, 경208, 승301, etc.)
  const classroomRegex = /^[샬경이천예인교승심][가-힣A-Z]*\d+|관\s*\d+|호$/i;
  tokens.forEach(token => {
    if (classroomRegex.test(token) || token.includes('강당') || token.includes('세미나실')) {
      classroom = token.replace(/[()]/g, '').trim();
    }
  });

  const categoryKeywords = [
    '전공선택', '전공필수', '교양필수', '교양선택', '균형교양', '일반교양', '기초교양',
    '학부기초', '전공기초', '복수선택', '부전공', '교직', '자유선택', '1전공', '2전공',
    '자기주도', '자기주도역량', '자기주도 역량',
    '창의융합', '창의융합역량', '창의융합 역량',
    '소통·협력', '소통협력', '소통협력역량', '소통·협력역량',
    '글로벌시민', '글로벌 시민', '글로벌시민역량', '글로벌 시민 역량',
    '종합적사고', '종합적 사고', '종합적사고역량', '종합적 사고 역량',
    '정보·자원기술활용', '정보자원기술활용', '정보·자원 기술 활용', '정보·자원 기술 활용 역량',
    '핵심역량'
  ];

  const normalizeForCheck = (str: string) => str.replace(/[\s·•\-\_]/g, '');
  const normalizedCategoryKeywords = categoryKeywords.map(normalizeForCheck);

  // Extract text-only tokens in order of appearance
  const textTokens = tokens.filter(token => {
    if (codeRegex.test(token)) return false;
    if (timeSlotRegex.test(token)) return false;
    if (token === courseCode) return false;
    if (token === timeStr) return false;
    if (/^\d+$/.test(token)) return false;
    if (classroomRegex.test(token) || token.includes('강당') || token.includes('세미나실')) return false;
    if (token.includes('원격수업') || token.includes('S러닝')) return false;
    
    const normalized = normalizeForCheck(token);
    if (normalizedCategoryKeywords.includes(normalized)) return false;
    return true;
  });

  // Position-based text resolution
  let courseName = '';
  let instructor = '';

  // 1. Try to find the instructor as the token right before the first time slot token
  const firstSlotIndex = tokens.findIndex(token => timeSlotRegex.test(token));
  if (firstSlotIndex > 0) {
    const candidate = tokens[firstSlotIndex - 1];
    const normalizedCandidate = normalizeForCheck(candidate);
    if (
      !classroomRegex.test(candidate) &&
      !/^\d+$/.test(candidate) &&
      !codeRegex.test(candidate) &&
      !normalizedCategoryKeywords.includes(normalizedCandidate) &&
      candidate !== '원격수업' &&
      candidate !== 'S러닝'
    ) {
      instructor = candidate;
    }
  }

  // 2. Resolve courseName and fallback instructor
  if (textTokens.length > 0) {
    const nonInstructorTextTokens = textTokens.filter(t => t !== instructor);
    if (nonInstructorTextTokens.length > 0) {
      courseName = nonInstructorTextTokens[0];
    } else {
      courseName = textTokens[0];
    }

    if (!instructor && textTokens.length > 1) {
      instructor = textTokens[1];
    }
  }

  // Fallback defaults
  if (!courseCode) {
    courseCode = courseIdPrefix + Math.random().toString(36).substr(2, 5).toUpperCase();
  }
  if (!courseName) {
    courseName = '이름 없는 과목';
  }

  // Extract division (first 2-digit number)
  const divMatch = line.match(/\b(\d{2})\b/);
  if (divMatch) {
    divisionName = divMatch[1];
  }

  return {
    courseCode,
    courseName,
    divisionName,
    instructor: instructor || '담당교수 미정',
    classroom: classroom || '강의실 미정',
    slots,
    credits
  };
}

// Parse a single division line inside a course card
export function parseDivisionLine(line: string, courseId: string): ClassDivision | null {
  const trimmed = line.trim();
  if (!trimmed) return null;

  const parsed = parseBookletLine(trimmed, courseId);
  if (!parsed || parsed.slots.length === 0) return null;

  let instructor = parsed.instructor;
  if (instructor === '담당교수 미정' && parsed.courseName && parsed.courseName !== '이름 없는 과목') {
    instructor = parsed.courseName;
  }

  return {
    id: `${courseId}_${parsed.divisionName}`,
    divisionName: parsed.divisionName,
    instructor,
    classroom: parsed.classroom,
    slots: parsed.slots
  };
}

// Preprocess copy-pasted booklet text to merge multiline time slots
export function preprocessBookletText(text: string): string[] {
  const rawLines = text.split('\n');
  const processedLines: string[] = [];
  
  rawLines.forEach(line => {
    const trimmed = line.trim();
    if (!trimmed) return;
    
    // A line is a time-only continuation if it starts with a day name
    // and contains ONLY day names, digits, spaces, colons, hyphens, tildes, commas, and slashes.
    const isOnlyTime = /^[월화수목금토]/.test(trimmed) && /^[월화수목금토\d\s:~\-,\/]+$/.test(trimmed);
      
    if (isOnlyTime && processedLines.length > 0) {
      const lastIdx = processedLines.length - 1;
      const lastLine = processedLines[lastIdx];
      processedLines[lastIdx] = lastLine.endsWith(',') ? `${lastLine} ${trimmed}` : `${lastLine}, ${trimmed}`;
    } else {
      processedLines.push(trimmed);
    }
  });
  
  return processedLines;
}
