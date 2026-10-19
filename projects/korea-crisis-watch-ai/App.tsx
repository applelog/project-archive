
import React, { useState, useEffect, useCallback } from 'react';
import { fetchCrisisReport, fetchHistory } from './services/geminiService';
import { CrisisReport } from './types';
import { HistoryChart } from './components/HistoryChart';
import { RiskGauge } from './components/RiskGauge';
import { MetricCard } from './components/MetricCard';

const App: React.FC = () => {
  const [history, setHistory] = useState<CrisisReport[]>([]);
  const [loading, setLoading] = useState(true);
  const [statusMessage, setStatusMessage] = useState('시스템 로드 중...');
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [currentTime, setCurrentTime] = useState(new Date());

  const getKST = useCallback((date: Date = new Date()) => {
    const formatter = new Intl.DateTimeFormat('en-US', {
      timeZone: 'Asia/Seoul',
      year: 'numeric', month: '2-digit', day: '2-digit',
      hour: 'numeric', minute: 'numeric', second: 'numeric',
      hour12: false
    });
    const parts = formatter.formatToParts(date);
    const getPart = (type: string) => parts.find(p => p.type === type)?.value || '';
    
    return {
      hour: parseInt(getPart('hour')),
      fullDate: `${getPart('year')}-${getPart('month')}-${getPart('day')}`,
    };
  }, []);

  const runAnalysis = async () => {
    setErrorMessage(null);
    setLoading(true);
    setStatusMessage('KST 실시간 데이터 수집 및 분석 중...');
    
    try {
      const data = await fetchCrisisReport();
      if (data && data.length > 0) {
        setHistory(data);
        setStatusMessage('분석 성공');
      } else {
        throw new Error("서버에서 빈 데이터가 반환되었습니다.");
      }
    } catch (error: any) {
      console.error("Manual Analysis Error:", error);
      let friendlyMsg = error.message;
      if (error.message.includes('429') || error.message.includes('RESOURCE_EXHAUSTED')) {
        friendlyMsg = "현재 분석 요청이 폭주하여 시스템이 잠시 대기 중입니다. 잠시 후 다시 시도해 주세요.";
      }
      setErrorMessage(friendlyMsg);
      setStatusMessage('업데이트 지연');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    const initApp = async () => {
      try {
        const { hour, fullDate } = getKST();
        const savedHistory = await fetchHistory();
        
        if (savedHistory && savedHistory.length > 0) {
          setHistory(savedHistory);
          const lastReportKST = new Intl.DateTimeFormat('en-CA', { 
            timeZone: 'Asia/Seoul' 
          }).format(new Date(savedHistory[0].timestamp));

          if (lastReportKST !== fullDate && hour >= 9) {
            await runAnalysis();
          } else {
            setLoading(false);
            setStatusMessage('관측 데이터 유지 중');
          }
        } else {
          await runAnalysis();
        }
      } catch (e: any) {
        setErrorMessage(`동기화 실패: ${e.message}`);
        setLoading(false);
      }
    };
    initApp();

    const timer = setInterval(() => setCurrentTime(new Date()), 1000);
    return () => clearInterval(timer);
  }, [getKST]);

  const latest = history[0];
  const previous = history[1];
  const scoreChange = previous ? latest?.totalScore - previous.totalScore : 0;

  return (
    <div className="min-h-screen bg-[#F5F5F7] dark:bg-[#0A0A0B] font-sans text-[#1D1D1F] dark:text-[#F5F5F7] transition-colors duration-700">
      <header className="sticky top-0 z-50 bg-white/70 dark:bg-black/70 backdrop-blur-3xl border-b border-gray-100 dark:border-white/5">
        <div className="max-w-3xl mx-auto px-6 h-14 flex justify-between items-center">
          <div className="flex items-center gap-2">
            <div className={`w-2 h-2 rounded-full ${loading ? 'bg-amber-400 animate-pulse' : latest?.totalScore > 70 ? 'bg-red-500 animate-pulse' : 'bg-green-500'}`}></div>
            <h1 className="text-sm font-black tracking-tighter uppercase">Crisis Watch AI</h1>
            <span className="text-[10px] text-gray-400 ml-2 font-bold uppercase hidden md:inline">{statusMessage}</span>
          </div>
          <div className="text-[10px] font-black text-gray-500 dark:text-gray-400 tabular-nums bg-gray-100 dark:bg-white/5 px-2 py-1 rounded-md">
            KST {new Intl.DateTimeFormat('ko-KR', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false, timeZone: 'Asia/Seoul' }).format(currentTime)}
          </div>
        </div>
      </header>

      <main className="max-w-3xl mx-auto px-6 py-10 space-y-8">
        {errorMessage && (
          <div className="p-5 bg-white/50 dark:bg-red-950/20 backdrop-blur-xl border border-red-100 dark:border-red-900/30 rounded-[2rem] shadow-2xl animate-in fade-in slide-in-from-top-4">
            <div className="flex flex-col md:flex-row items-center gap-4 text-center md:text-left">
              <div className="w-12 h-12 rounded-2xl bg-red-100 dark:bg-red-900/40 flex items-center justify-center text-xl shrink-0">⚠️</div>
              <div className="flex-1">
                <h3 className="text-sm font-black text-red-600 dark:text-red-400 mb-1">분석 엔진 일시 정지</h3>
                <p className="text-[11px] leading-relaxed text-red-500/80 dark:text-red-400/60 font-medium">
                  {errorMessage}
                </p>
              </div>
              <button 
                onClick={runAnalysis} 
                className="px-6 py-2.5 bg-red-600 text-white text-[11px] font-black rounded-xl hover:bg-red-700 transition-all active:scale-95 shadow-lg shadow-red-600/20"
              >
                강제 재분석
              </button>
            </div>
          </div>
        )}

        {latest && (
          <div className={`${loading ? 'opacity-40 grayscale pointer-events-none' : 'opacity-100'} transition-all duration-700 space-y-8`}>
            {/* Risk Hero Section */}
            <div className="relative overflow-hidden bg-white dark:bg-[#1C1C1E] rounded-[3rem] p-10 shadow-2xl border border-gray-100 dark:border-white/5 flex flex-col md:flex-row gap-12 items-center">
              <div className="shrink-0 relative">
                 <div className={`absolute inset-0 blur-3xl opacity-20 ${latest.totalScore > 70 ? 'bg-red-500' : 'bg-blue-500'}`}></div>
                <RiskGauge score={latest.totalScore} level={latest.riskLevel} />
                {previous && (
                  <div className="mt-4 flex justify-center">
                    <div className={`text-[10px] font-black px-3 py-1 rounded-full border shadow-sm ${scoreChange > 0 ? 'bg-red-50 text-red-600 border-red-100' : 'bg-blue-50 text-blue-600 border-blue-100'}`}>
                      {scoreChange > 0 ? '▲' : '▼'} {Math.abs(scoreChange)} pts
                    </div>
                  </div>
                )}
              </div>
              <div className="flex-1 space-y-4 text-center md:text-left">
                <div className="flex flex-wrap justify-center md:justify-start gap-2 mb-2">
                  {latest.newsKeywords?.slice(0, 3).map(kw => (
                    <span key={kw} className="text-[9px] font-black px-2 py-0.5 bg-gray-100 dark:bg-white/5 text-gray-500 dark:text-gray-400 rounded-md uppercase tracking-tighter">#{kw}</span>
                  ))}
                </div>
                <h2 className="text-3xl font-black leading-tight tracking-tighter italic">{latest.riskiestIndicator}</h2>
                <p className="text-sm text-gray-600 dark:text-gray-400 leading-relaxed font-medium">{latest.summary}</p>
              </div>
            </div>

            {/* Metrics Grid */}
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
              <MetricCard label="환율 (USD/KRW)" value={`${latest.metrics.exchangeRate.toLocaleString()}원`} thresholdText="1,400원" isWarning={latest.metrics.exchangeRate >= 1400} />
              <MetricCard label="국고채 10년" value={`${latest.metrics.bondYield10Yr}%`} thresholdText="3.5%" isWarning={latest.metrics.bondYield10Yr >= 3.5} />
              <MetricCard label="유동성 (RP)" value={latest.metrics.rpBalance} thresholdText="Trend" />
              <MetricCard label="연기금 채권" value={`${latest.metrics.pensionBondShare}%`} thresholdText="24%+" isWarning={latest.metrics.pensionBondShare >= 24} />
            </div>

            {/* Recommendations & Strategy */}
            {latest.portfolioRecommendation && (
              <div className="grid md:grid-cols-2 gap-6">
                <div className="bg-white dark:bg-[#1C1C1E] rounded-[2.5rem] p-8 border border-gray-100 dark:border-white/5 shadow-sm space-y-6">
                  <h3 className="text-xs font-black uppercase tracking-widest text-blue-500">Defense Strategy</h3>
                  <div className="space-y-4">
                    <div className="flex justify-between items-end">
                      <span className="text-[10px] font-bold text-gray-400 uppercase">자산 배분 권고</span>
                      <span className="text-[10px] font-black text-blue-500">{latest.portfolioRecommendation.actionLog.split('.')[0]}</span>
                    </div>
                    <div className="h-6 flex rounded-full overflow-hidden border border-gray-50 dark:border-white/5">
                      <div className="bg-gray-800 dark:bg-white transition-all duration-1000" style={{ width: `${latest.portfolioRecommendation.krwCashRatio}%` }}></div>
                      <div className="bg-blue-500 transition-all duration-1000" style={{ width: `${latest.portfolioRecommendation.foreignCurrencyRatio}%` }}></div>
                      <div className="bg-amber-400 transition-all duration-1000" style={{ width: `${latest.portfolioRecommendation.investmentRatio}%` }}></div>
                    </div>
                    <div className="flex justify-between text-[9px] font-black text-gray-400 uppercase pt-2">
                      <div className="flex items-center gap-1.5"><div className="w-2 h-2 rounded-full bg-gray-800 dark:bg-white"></div> KRW {latest.portfolioRecommendation.krwCashRatio}%</div>
                      <div className="flex items-center gap-1.5"><div className="w-2 h-2 rounded-full bg-blue-500"></div> Safety {latest.portfolioRecommendation.foreignCurrencyRatio}%</div>
                      <div className="flex items-center gap-1.5"><div className="w-2 h-2 rounded-full bg-amber-400"></div> Risk {latest.portfolioRecommendation.investmentRatio}%</div>
                    </div>
                  </div>
                </div>

                <div className="bg-black text-white dark:bg-white dark:text-black rounded-[2.5rem] p-8 shadow-2xl space-y-4 flex flex-col justify-center">
                  <h3 className="text-[10px] font-black uppercase tracking-widest opacity-60">Critical Triggers</h3>
                  <ul className="space-y-2">
                    {latest.triggers.slice(0, 3).map((t, i) => (
                      <li key={i} className="text-[11px] font-bold flex gap-3 items-start leading-snug">
                        <span className="opacity-40">0{i+1}</span>
                        {t}
                      </li>
                    ))}
                  </ul>
                </div>
              </div>
            )}

            {/* History Section */}
            <div className="bg-white dark:bg-[#1C1C1E] rounded-[3rem] p-10 border border-gray-100 dark:border-white/5 shadow-sm">
              <div className="flex justify-between items-center mb-8">
                <h3 className="text-xs font-black uppercase tracking-widest text-gray-400">Stability Timeline</h3>
                <span className="text-[10px] font-bold text-gray-300">최근 30개 관측치</span>
              </div>
              <HistoryChart history={history} isDark={true} />
            </div>

            {/* Footer Actions */}
            <div className="flex flex-col items-center gap-8 py-10">
              <button 
                onClick={runAnalysis} 
                disabled={loading} 
                className="group relative px-12 py-5 overflow-hidden rounded-full bg-black dark:bg-white text-white dark:text-black transition-all active:scale-95 disabled:opacity-50"
              >
                <div className="absolute inset-0 bg-gradient-to-r from-blue-600 to-indigo-600 opacity-0 group-hover:opacity-100 transition-opacity"></div>
                <span className="relative text-xs font-black tracking-widest uppercase">
                  {loading ? 'Observing Market...' : 'Force Real-time Sync'}
                </span>
              </button>
              <div className="text-center space-y-2 max-w-sm">
                <p className="text-[10px] text-gray-400 font-bold leading-relaxed uppercase tracking-tighter">
                  본 시스템은 Google Search와 Gemini-3 Flash를 결합하여 대한민국 시장의 붕괴 징후를 실시간으로 추적합니다. 모든 데이터는 투자 조언이 아닌 시스템 방어 지표로 활용되어야 합니다.
                </p>
                <p className="text-[10px] font-black text-gray-200 dark:text-white/10 tracking-[0.5em] uppercase">Security Protocol Alpha-01</p>
              </div>
            </div>
          </div>
        )}

        {(!latest && !errorMessage) && (
          <div className="flex flex-col items-center justify-center py-60 space-y-6">
            <div className="relative">
              <div className="w-12 h-12 border-4 border-gray-100 dark:border-white/5 border-t-blue-500 rounded-full animate-spin"></div>
              <div className="absolute inset-0 blur-xl bg-blue-500/20 animate-pulse"></div>
            </div>
            <p className="text-[11px] font-black text-gray-400 uppercase tracking-widest animate-pulse">{statusMessage}</p>
          </div>
        )}
      </main>
    </div>
  );
};

export default App;
