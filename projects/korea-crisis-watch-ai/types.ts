export interface CrisisMetrics {
  bondYield10Yr: number;
  exchangeRate: number;
  rpBalance: number | string; // Allow string for description if exact number is fuzzy
  pensionBondShare: number; // Percentage
  bankDepositChange: number; // In Trillion KRW (change)
}

export interface PortfolioRecommendation {
  krwCashRatio: number; // % (원화 현금)
  foreignCurrencyRatio: number; // % (달러/금 등 안전자산)
  investmentRatio: number; // % (주식/코인 등 공격적 투자)
  actionLog: string; // Specific advice (e.g., "Increase USD holdings immediately")
}

export interface CrisisReport {
  id: string;
  timestamp: string; // ISO String
  totalScore: number; // 0-100
  riskLevel: '정상' | '경고' | '위험' | '임계';
  riskiestIndicator: string;
  summary: string;
  newsKeywords: string[]; // New: AI-extracted news keywords
  advice: string[]; // General advice
  portfolioRecommendation?: PortfolioRecommendation; // Specific numerical advice
  scenarios: {
    base: string;
    warning: string;
    worst: string;
  };
  triggers: string[];
  metrics: CrisisMetrics;
  rawAnalysis: string;
  isMock?: boolean;
}

export interface HistoryPoint {
  date: string;
  score: number;
  exchangeRate: number;
}