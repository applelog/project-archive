export interface TimeSlot {
  day: number;      // 0: Mon, 1: Tue, 2: Wed, 3: Thu, 4: Fri, 5: Sat
  start: number;    // Minutes from midnight, e.g., 09:00 = 540
  end: number;      // Minutes from midnight, e.g., 10:15 = 615
  displayStr: string; // e.g., "월 11:50-13:05" or "월4ab5a"
}

export interface ClassDivision {
  id: string;          // Unique ID (e.g. courseId + "_" + divisionName)
  divisionName: string; // e.g., "01", "A"
  instructor: string;   // e.g., "김철수"
  classroom?: string;   // e.g., "샬808"
  slots: TimeSlot[];    // Time slots for this division
}

export interface Course {
  id: string;          // Unique ID (e.g. course code "NE11702")
  name: string;        // Course name (e.g., "서양역사의이해")
  code: string;        // Course code (e.g., "NE11702")
  color: string;       // HSL or Hex color for visual grid
  credits: number;     // Credits, e.g., 3
  divisions: ClassDivision[];
  active?: boolean;    // Whether this course is selected for scheduling (defaults to true)
}

export interface ScheduleCombination {
  id: string;          // Unique ID (hash or comma-separated division IDs)
  divisions: { [courseId: string]: ClassDivision };
  totalCredits: number;
  gapTime: number;     // Total gap/empty time in minutes
  daysWithClasses: number; // Number of days with classes
  hasFriday: boolean;
  hasMorning: boolean; // Has classes starting at 09:00 (period 1a/1b)
  hasNight: boolean;   // Has classes ending after 18:00
  hasLunchConflict: boolean; // True if any class overlaps with 12:00-13:00
}

export interface FilterOptions {
  emptyFriday: boolean;    // No classes on Friday
  noMorning: boolean;      // No classes starting before 10:00
  noNight: boolean;        // No classes ending after 18:00
  lunchBreak: boolean;     // Protect lunch hours 12:00-13:00
  maxGapHours: number;     // Max allowable gap hours (999 for infinity)
  sortBy: 'none' | 'gapAsc' | 'gapDesc' | 'daysAsc' | 'daysDesc' | 'creditsDesc'; // Sorting criteria
  preferredInstructors?: { [courseId: string]: string[] }; // Map of course ID to preferred instructors
}

// Kangnam University Time Period Mapping
export interface PeriodTime {
  start: string; // "HH:MM"
  end: string;   // "HH:MM"
  startMin: number;
  endMin: number;
}

export const PERIODS: { [key: string]: PeriodTime } = {
  "1a": { start: "09:00", end: "09:25", startMin: 540, endMin: 565 },
  "1b": { start: "09:26", end: "09:50", startMin: 566, endMin: 590 },
  "2a": { start: "09:50", end: "10:15", startMin: 590, endMin: 615 },
  "2b": { start: "10:26", end: "10:50", startMin: 626, endMin: 650 },
  "3a": { start: "10:50", end: "11:15", startMin: 650, endMin: 675 },
  "3b": { start: "11:16", end: "11:40", startMin: 676, endMin: 700 },
  "4a": { start: "11:50", end: "12:15", startMin: 710, endMin: 735 },
  "4b": { start: "12:16", end: "12:40", startMin: 736, endMin: 760 },
  "5a": { start: "12:40", end: "13:05", startMin: 760, endMin: 785 },
  "5b": { start: "13:16", end: "13:40", startMin: 796, endMin: 820 },
  "6a": { start: "13:40", end: "14:05", startMin: 820, endMin: 845 },
  "6b": { start: "14:06", end: "14:30", startMin: 846, endMin: 870 },
  "7a": { start: "14:40", end: "15:05", startMin: 880, endMin: 905 },
  "7b": { start: "15:06", end: "15:30", startMin: 906, endMin: 930 },
  "8a": { start: "15:30", end: "15:55", startMin: 930, endMin: 955 },
  "8b": { start: "16:06", end: "16:30", startMin: 966, endMin: 990 },
  "9a": { start: "16:30", end: "16:55", startMin: 990, endMin: 1015 },
  "9b": { start: "16:56", end: "17:20", startMin: 1016, endMin: 1040 },
  "10a": { start: "17:30", end: "17:55", startMin: 1050, endMin: 1075 },
  "10b": { start: "17:55", end: "18:20", startMin: 1076, endMin: 1100 },
  "11a": { start: "18:20", end: "18:45", startMin: 1100, endMin: 1125 },
  "11b": { start: "18:45", end: "19:10", startMin: 1126, endMin: 1150 },
  "12a": { start: "19:10", end: "19:35", startMin: 1150, endMin: 1175 },
  "12b": { start: "19:35", end: "20:00", startMin: 1176, endMin: 1200 },
  "13a": { start: "20:10", end: "20:35", startMin: 1210, endMin: 1235 },
  "13b": { start: "20:35", end: "21:00", startMin: 1236, endMin: 1260 },
  "14a": { start: "21:00", end: "21:25", startMin: 1260, endMin: 1285 },
  "14b": { start: "21:25", end: "21:50", startMin: 1285, endMin: 1310 },
  "15a": { start: "21:50", end: "22:15", startMin: 1310, endMin: 1335 },
  "15b": { start: "22:15", end: "22:40", startMin: 1335, endMin: 1360 }
};

export const DAY_NAMES = ["월", "화", "수", "목", "금", "토"];

// Helper to convert day index to string
export function getDayName(day: number): string {
  return DAY_NAMES[day] || "";
}

// Helper to convert day string to index
export function getDayIndex(dayStr: string): number {
  return DAY_NAMES.indexOf(dayStr);
}
