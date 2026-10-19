import React, { useState } from 'react';
import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { CrisisReport } from '../types';

interface HistoryChartProps {
  history: CrisisReport[];
  isDark?: boolean;
}

type MetricKey = 'totalScore' | 'exchangeRate' | 'bondYield10Yr' | 'pensionBondShare' | 'rpBalance';

const LABELS: Record<MetricKey, string> = {
  totalScore: '위험 점수',
  exchangeRate: '환율 (KRW)',
  bondYield10Yr: '국채 10년물 (%)',
  pensionBondShare: '연기금 채권비중 (%)',
  rpBalance: 'RP/유동성 (조원)',
};

const COLORS: Record<MetricKey, string> = {
  totalScore: '#EF4444',
  exchangeRate: '#3B82F6',
  bondYield10Yr: '#10B981',
  pensionBondShare: '#F59E0B',
  rpBalance: '#8B5CF6',
};

const CustomTooltip = ({ active, payload, label, dataKeyName, isDark }: any) => {
  if (active && payload && payload.length) {
    // Format tooltip date to KST
    const kstDate = new Intl.DateTimeFormat('ko-KR', {
      timeZone: 'Asia/Seoul',
      month: 'long',
      day: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      hour12: false
    }).format(new Date(label));

    return (
      <div className={`${isDark ? 'bg-gray-900 border-gray-700 text-white' : 'bg-white border-gray-200 text-gray-900'} p-3 border rounded-xl shadow-xl text-[10px] font-medium`}>
        <p className="mb-1 text-gray-400 font-bold uppercase tracking-tight">{kstDate} (KST)</p>
        <p className="font-black text-xs" style={{ color: payload[0].stroke }}>
          {dataKeyName}: {payload[0].value?.toLocaleString()}
        </p>
      </div>
    );
  }
  return null;
};

export const HistoryChart: React.FC<HistoryChartProps> = ({ history, isDark }) => {
  const [selectedMetric, setSelectedMetric] = useState<MetricKey>('totalScore');

  const data = history.map(h => {
    let rpVal = 0;
    const rpRaw = h.metrics.rpBalance;
    if (typeof rpRaw === 'number') rpVal = rpRaw;
    else if (typeof rpRaw === 'string') {
      const match = rpRaw.replace(/,/g, '').match(/(\d+(\.\d+)?)/);
      rpVal = match ? parseFloat(match[0]) : 0;
    }

    return {
      date: h.timestamp, // ISO String
      totalScore: h.totalScore,
      exchangeRate: h.metrics.exchangeRate,
      bondYield10Yr: h.metrics.bondYield10Yr,
      pensionBondShare: h.metrics.pensionBondShare,
      rpBalance: rpVal,
    };
  }).sort((a, b) => new Date(a.date).getTime() - new Date(b.date).getTime());

  if (data.length === 0) return null;

  return (
    <div className="w-full">
      <div className="flex p-1 bg-gray-100 dark:bg-gray-800 rounded-xl mb-8 overflow-x-auto no-scrollbar border border-gray-50 dark:border-gray-800">
        {(Object.keys(LABELS) as MetricKey[]).map((key) => (
          <button
            key={key}
            onClick={() => setSelectedMetric(key)}
            className={`flex-1 py-2 px-4 text-[10px] font-bold rounded-lg transition-all whitespace-nowrap ${
              selectedMetric === key
                ? 'bg-white dark:bg-gray-700 text-gray-900 dark:text-white shadow-sm'
                : 'text-gray-400 dark:text-gray-500 hover:text-gray-600 dark:hover:text-gray-300'
            }`}
          >
            {LABELS[key].split(' ')[0]}
          </button>
        ))}
      </div>

      <div className="w-full h-72">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={data} margin={{ top: 10, right: 10, left: -25, bottom: 0 }}>
            <defs>
              <linearGradient id={`color-${selectedMetric}`} x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor={COLORS[selectedMetric]} stopOpacity={0.2}/>
                <stop offset="95%" stopColor={COLORS[selectedMetric]} stopOpacity={0}/>
              </linearGradient>
            </defs>
            <CartesianGrid strokeDasharray="3 3" vertical={false} stroke={isDark ? "#333" : "#F3F4F6"} />
            <XAxis 
              dataKey="date" 
              tick={{fontSize: 9, fill: '#9CA3AF', fontWeight: 'bold'}} 
              tickFormatter={(v) => {
                // Force X-Axis tick date to Asia/Seoul
                return new Intl.DateTimeFormat('ko-KR', {
                  timeZone: 'Asia/Seoul',
                  month: 'numeric',
                  day: 'numeric'
                }).format(new Date(v));
              }}
            />
            <YAxis domain={['auto', 'auto']} tick={{fontSize: 9, fill: '#9CA3AF'}} width={35} />
            <Tooltip content={<CustomTooltip dataKeyName={LABELS[selectedMetric]} isDark={isDark} />} />
            
            <Area 
              type="monotone" 
              dataKey={selectedMetric} 
              stroke={COLORS[selectedMetric]} 
              strokeWidth={3} 
              fillOpacity={1} 
              fill={`url(#color-${selectedMetric})`} 
              animationDuration={1500}
            />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
};