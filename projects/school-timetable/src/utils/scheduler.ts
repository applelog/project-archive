import type { Course, ClassDivision, ScheduleCombination, TimeSlot, FilterOptions } from '../types';

// Check if two time slots overlap
export function isSlotOverlapping(slot1: TimeSlot, slot2: TimeSlot): boolean {
  return (
    slot1.day === slot2.day &&
    Math.max(slot1.start, slot2.start) < Math.min(slot1.end, slot2.end)
  );
}

// Check if a division overlaps with a list of already selected divisions
export function isDivisionConflicting(
  division: ClassDivision,
  selected: ClassDivision[]
): boolean {
  for (const sel of selected) {
    for (const slot1 of division.slots) {
      for (const slot2 of sel.slots) {
        if (isSlotOverlapping(slot1, slot2)) {
          return true;
        }
      }
    }
  }
  return false;
}

// Calculate the total gap time (empty time between classes on the same day) in minutes
export function calculateGapTime(divisions: ClassDivision[]): number {
  // Group all slots by day
  const slotsByDay: { [day: number]: { start: number; end: number }[] } = {};
  
  divisions.forEach(div => {
    div.slots.forEach(slot => {
      if (!slotsByDay[slot.day]) {
        slotsByDay[slot.day] = [];
      }
      slotsByDay[slot.day].push({ start: slot.start, end: slot.end });
    });
  });

  let totalGap = 0;

  Object.keys(slotsByDay).forEach(dayKey => {
    const daySlots = slotsByDay[parseInt(dayKey)];
    if (daySlots.length <= 1) return;

    // Sort slots by start time
    daySlots.sort((a, b) => a.start - b.start);

    // Calculate gap between adjacent slots
    for (let i = 0; i < daySlots.length - 1; i++) {
      const current = daySlots[i];
      const next = daySlots[i + 1];
      
      // If there is a gap (next starts after current ends)
      if (next.start > current.end) {
        totalGap += (next.start - current.end);
      }
    }
  });

  return totalGap;
}

// Generate all conflict-free combinations of courses
export function generateSchedules(courses: Course[]): ScheduleCombination[] {
  if (courses.length === 0) return [];
  
  // Filter out courses that are inactive or have no divisions
  const activeCourses = courses.filter(c => c.active !== false && c.divisions.length > 0);
  if (activeCourses.length === 0) return [];

  // Sort courses by number of divisions (smallest first) to optimize DFS pruning
  const sortedCourses = [...activeCourses].sort((a, b) => a.divisions.length - b.divisions.length);
  
  const results: ScheduleCombination[] = [];

  function backtrack(courseIndex: number, selected: ClassDivision[]) {
    if (courseIndex === sortedCourses.length) {
      // Form combination
      const divisionsMap: { [courseId: string]: ClassDivision } = {};
      let totalCredits = 0;
      let hasFriday = false;
      let hasMorning = false;
      let hasNight = false;
      let hasLunchConflict = false;
      const daysSet = new Set<number>();

      selected.forEach((div, idx) => {
        const course = sortedCourses[idx];
        divisionsMap[course.id] = div;
        totalCredits += course.credits;

        div.slots.forEach(slot => {
          daysSet.add(slot.day);
          if (slot.day === 4) { // Friday (0: Mon, 1: Tue, 2: Wed, 3: Thu, 4: Fri)
            hasFriday = true;
          }
          if (slot.start < 600) { // Starts before 10:00 (600 minutes)
            hasMorning = true;
          }
          if (slot.end > 1080) { // Ends after 18:00 (1080 minutes)
            hasNight = true;
          }
          // Lunch Hour is 12:00 to 13:00 (720 to 780 minutes from midnight)
          if (Math.max(slot.start, 720) < Math.min(slot.end, 780)) {
            hasLunchConflict = true;
          }
        });
      });

      const gapTime = calculateGapTime(selected);
      const id = selected.map(d => d.id).sort().join(',');

      results.push({
        id,
        divisions: divisionsMap,
        totalCredits,
        gapTime,
        daysWithClasses: daysSet.size,
        hasFriday,
        hasMorning,
        hasNight,
        hasLunchConflict
      });
      return;
    }

    const currentCourse = sortedCourses[courseIndex];
    for (const division of currentCourse.divisions) {
      // If it doesn't conflict with selected divisions, proceed
      if (!isDivisionConflicting(division, selected)) {
        selected.push(division);
        backtrack(courseIndex + 1, selected);
        selected.pop(); // backtrack
      }
    }
  }

  backtrack(0, []);
  return results;
}

// Filter and sort schedule combinations based on options
export function filterAndSortSchedules(
  schedules: ScheduleCombination[],
  options: FilterOptions
): ScheduleCombination[] {
  let filtered = [...schedules];

  // Apply filters
  if (options.emptyFriday) {
    filtered = filtered.filter(s => !s.hasFriday);
  }
  if (options.noMorning) {
    filtered = filtered.filter(s => !s.hasMorning);
  }
  if (options.noNight) {
    filtered = filtered.filter(s => !s.hasNight);
  }
  if (options.lunchBreak) {
    filtered = filtered.filter(s => !s.hasLunchConflict);
  }
  if (options.preferredInstructors) {
    Object.entries(options.preferredInstructors).forEach(([courseId, val]) => {
      const instructors = val as string[];
      if (Array.isArray(instructors)) {
        filtered = filtered.filter(s => {
          const div = s.divisions[courseId];
          if (!div) return true; // Course not in this combination
          const divInst = (div.instructor?.trim() || '담당교수 미정');
          return instructors.some(inst => {
            const cleanInst = inst.trim();
            return divInst === cleanInst || divInst.includes(cleanInst) || cleanInst.includes(divInst);
          });
        });
      }
    });
  }
  if (options.maxGapHours < 24) {
    const maxGapMinutes = options.maxGapHours * 60;
    filtered = filtered.filter(s => s.gapTime <= maxGapMinutes);
  }

  // Apply sorting
  if (options.sortBy === 'gapAsc') {
    filtered.sort((a, b) => a.gapTime - b.gapTime);
  } else if (options.sortBy === 'gapDesc') {
    filtered.sort((a, b) => b.gapTime - a.gapTime);
  } else if (options.sortBy === 'daysAsc') {
    filtered.sort((a, b) => a.daysWithClasses - b.daysWithClasses);
  } else if (options.sortBy === 'daysDesc') {
    filtered.sort((a, b) => b.daysWithClasses - a.daysWithClasses);
  } else if (options.sortBy === 'creditsDesc') {
    filtered.sort((a, b) => b.totalCredits - a.totalCredits);
  }

  return filtered;
}
