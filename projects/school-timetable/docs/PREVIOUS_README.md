> 정리 전 문서입니다. 상대 경로와 실행 안내는 이전 위치 기준이며, 기능·검증 주장은 당시 기록입니다. 현재 안내는 프로젝트 README와 루트 README를 참고하세요.

# 🧙‍♂️ KNU Timetable Wizard (강남대학교 시간표 마법사)

[![React](https://img.shields.io/badge/React-20232A?style=for-the-badge&logo=react&logoColor=61DAFB)](https://react.dev/)
[![TypeScript](https://img.shields.io/badge/TypeScript-007ACC?style=for-the-badge&logo=typescript&logoColor=white)](https://www.typescriptlang.org/)
[![Vite](https://img.shields.io/badge/Vite-B73BFE?style=for-the-badge&logo=vite&logoColor=FFD62B)](https://vite.dev/)
[![Vitest](https://img.shields.io/badge/Vitest-3B7B3E?style=for-the-badge&logo=vitest&logoColor=FFD62B)](https://vitest.dev/)
[![GitHub Actions](https://img.shields.io/badge/GitHub_Actions-2088FF?style=for-the-badge&logo=github-actions&logoColor=white)](https://github.com/features/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

An advanced, privacy-first, lightweight client-side application designed to generate, simulate, and filter all collision-free university timetable combinations from raw text inputs.

🚀 **Live Demos:** 
* [Vercel Production](https://knu-timetable.vercel.app)
* [GitHub Pages Release](https://applelog.github.io/knu-timetable/)

---

## 💡 Motivation (개발 동기)

When planning university timetables, I always prioritize the courses I actually want to take rather than which professor is teaching them. However, manually mapping and combining different course divisions while ensuring zero time overlaps was extremely tedious and frustrating to do every single semester. 

This tool was built to solve that exact problem: to instantly automate the combinatorics of schedule generation, allowing students to select their desired curriculum and get all valid, collision-free timetable options in milliseconds.

---


## ⚡ Key Features

* **📋 Smart Raw Text Parser:** Automatically extracts course codes, names, credits, classroom locations, instructor names, and weekly time slots directly from university portal transcripts (KNU Yoram / Everytime) using robust regex heuristics.
* **🔀 Backtracking Constraint Solver:** Computes every valid, non-overlapping weekly schedule combination in $\mathcal{O}(N)$ optimized search space.
* **🔍 Fine-Grained Advanced Filters:**
  * **Day-Off (공강) Selector:** Lock specific days (e.g., Friday-off) to exclude schedules.
  * **Time Range Restrictor:** Exclude morning classes (e.g., 9:00 AM) or late-evening sessions.
  * **Continuous Lecture (연강) Optimizer:** Prioritize schedules with adjacent classes over scattered intervals.
  * **Instructor Blacklist/Whitelist:** Instantly filter schedules based on specific lecturers.
* **🔒 Privacy-by-Design:** No server-side backend. All data parsing, combination generation, and exports (JSON/Image) occur strictly inside the browser sandbox.
* **🌗 Adaptive Glassmorphic Theme:** Automatically switches between Light and Dark modes based on local time and system preferences.

---

## 📁 Repository Directory Structure

```text
school_timetable/
├── .github/
│   └── workflows/
│       └── deploy.yml          # GitHub Actions CI/CD Pipeline
├── public/
│   ├── favicon.svg             # Web Application Favicon
│   └── icons.svg               # Lucide SVG Icon Assets
├── src/
│   ├── assets/                 # Static visual resources
│   ├── components/
│   │   ├── CourseInput.tsx     # Bulk import text parser UI
│   │   ├── ScheduleList.tsx    # Filter criteria and cards grid
│   │   └── TimeGrid.tsx        # Interactive calendar rendering
│   ├── utils/
│   │   ├── parser.ts           # Heuristic regex text parser engine
│   │   ├── scheduler.ts        # Timetable backtracking combinatorics solver
│   │   └── __tests__/          # Vitest suite for parsers and solvers
│   ├── App.tsx                 # App Root & State Coordinator
│   ├── index.css               # HSL design tokens & global CSS variables
│   ├── main.tsx                # Client bundle mount point
│   └── types.ts                # TypeScript strict interface definitions
├── EVERYTIME_POST.md           # Everytime community upload template
├── package.json                # Project dependencies and scripting
├── tsconfig.json               # TypeScript compilation configurations
└── vite.config.ts              # Vite asset bundler configurations
```

---

## ⚙️ Technical Architecture

### 1. Heuristic Parsing Engine (`parser.ts`)
The text parser utilizes complex regex mapping to process raw copy-pasted rows from academic systems. It handles non-standardized formats such as:
```text
수09:00~11:40(새304), 금11:50~13:05(새301)
```
The parser converts this string into structured weekly slots:
$$\text{Day Index} \in [0..4] \quad (\text{Mon..Fri})$$
$$\text{Start / End Time} \in [0..1440] \quad (\text{Minutes from midnight})$$

### 2. Constraint Satisfaction Scheduler (`scheduler.ts`)
The timetable combinations generator treats schedule generation as a **Constraint Satisfaction Problem (CSP)**. It runs a depth-first search (DFS) with backtracking:
* **Pruning:** If a selected course section overlaps with already scheduled blocks, the sub-tree is pruned immediately.
* **Sorting:** Timetables are ranked dynamically using preference scoring (e.g., number of continuous lectures, total school days, credit load).

---

## 🛠️ Development Setup & Installation

### Prerequisites
* **Node.js:** `v20.x` or higher
* **npm:** `v10.x` or higher

### Local Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/applelog/knu-timetable.git
   cd knu-timetable
   ```

2. Install dependency packages:
   ```bash
   npm install
   ```

3. Start local development server (Vite hot reload):
   ```bash
   npm run dev
   ```
   Open `REDACTED_CONFIG_VALUE` on your browser.

4. Compile and bundle for production:
   ```bash
   npm run build
   ```

---

## 🧪 Unit Testing

We maintain high test coverage using **Vitest** for parsers and combinatorial solvers.

Run unit tests:
```bash
npx vitest run
```

---

## 🛡️ License

Distributed under the MIT License. See `LICENSE` for more information.
