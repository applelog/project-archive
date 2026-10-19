import React from 'react';

interface RiskGaugeProps {
  score: number;
  level: string;
  isDark?: boolean;
}

export const RiskGauge: React.FC<RiskGaugeProps> = ({ score, level, isDark }) => {
  let colorClass = 'text-emerald-500';
  let bgClass = 'bg-emerald-50 dark:bg-emerald-900/20 text-emerald-600 dark:text-emerald-400 border-emerald-100 dark:border-emerald-800/30';
  
  if (score > 30) { colorClass = 'text-amber-500'; bgClass = 'bg-amber-50 dark:bg-amber-900/20 text-amber-600 dark:text-amber-400 border-amber-100 dark:border-amber-800/30'; }
  if (score > 60) { colorClass = 'text-orange-500'; bgClass = 'bg-orange-50 dark:bg-orange-900/20 text-orange-600 dark:text-orange-400 border-orange-100 dark:border-orange-800/30'; }
  if (score > 80) { colorClass = 'text-rose-600'; bgClass = 'bg-rose-50 dark:bg-rose-900/20 text-rose-600 dark:text-rose-400 border-rose-100 dark:border-rose-800/30'; }

  const radius = 38;
  const circumference = 2 * Math.PI * radius;
  const offset = circumference - (score / 100) * circumference;

  return (
    <div className="flex flex-col items-center justify-center">
      <div className="relative w-32 h-32">
        <svg className="w-full h-full transform -rotate-90" viewBox="0 0 100 100">
          <circle
            cx="50"
            cy="50"
            r={radius}
            stroke="currentColor"
            strokeWidth="5"
            fill="transparent"
            className="text-gray-100 dark:text-gray-800"
          />
          <circle
            cx="50"
            cy="50"
            r={radius}
            stroke="currentColor"
            strokeWidth="5"
            fill="transparent"
            strokeDasharray={circumference}
            strokeDashoffset={offset}
            strokeLinecap="round"
            className={`${colorClass} transition-all duration-1000 ease-out shadow-lg`}
          />
        </svg>
        <div className="absolute top-0 left-0 w-full h-full flex flex-col items-center justify-center">
          <span className={`text-3xl font-black tracking-tighter ${colorClass}`}>{score}</span>
          <span className="text-[9px] text-gray-400 dark:text-gray-500 font-bold uppercase mt-1 tracking-widest">Risk</span>
        </div>
      </div>
      <div className={`mt-3 px-4 py-1.5 rounded-full text-[10px] font-black border uppercase tracking-wider shadow-sm ${bgClass}`}>
        {level}
      </div>
    </div>
  );
};