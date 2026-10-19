
import React from 'react';

interface MetricCardProps {
  label: string;
  value: string | number;
  thresholdText: string;
  isWarning?: boolean;
}

export const MetricCard: React.FC<MetricCardProps> = ({ label, value, thresholdText, isWarning = false }) => {
  return (
    <div className={`relative overflow-hidden p-6 rounded-[2.5rem] border transition-all duration-500 group hover:-translate-y-1 ${
      isWarning 
        ? 'bg-red-50/50 dark:bg-red-950/20 border-red-100 dark:border-red-900/30' 
        : 'bg-white dark:bg-[#1C1C1E] border-gray-100 dark:border-white/5 shadow-sm hover:shadow-xl'
    }`}>
      {isWarning && (
        <div className="absolute top-0 right-0 p-3">
          <div className="w-1.5 h-1.5 rounded-full bg-red-500 animate-ping"></div>
        </div>
      )}
      <div className="flex flex-col h-full justify-between gap-4">
        <div>
           <p className="text-[10px] font-black text-gray-400 dark:text-gray-500 uppercase tracking-widest mb-2 opacity-60 group-hover:opacity-100 transition-opacity">{label}</p>
           <p className={`text-xl font-black tracking-tighter ${isWarning ? 'text-red-600 dark:text-red-400' : 'text-gray-900 dark:text-white'}`}>
             {value}
           </p>
        </div>
        <div className="pt-4 border-t border-gray-100 dark:border-white/5">
          <div className="flex items-center gap-2">
            <div className={`w-1 h-1 rounded-full ${isWarning ? 'bg-red-500' : 'bg-gray-300 dark:bg-gray-600'}`}></div>
            <p className="text-[9px] text-gray-400 dark:text-gray-500 font-black uppercase tracking-tighter">
              Ref: {thresholdText}
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};
