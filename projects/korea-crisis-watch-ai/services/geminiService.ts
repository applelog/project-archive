import { GoogleGenAI, Type } from "@google/genai";
import { CrisisReport } from "../types";

const GEMINI_API_KEY = process.env.GEMINI_API_KEY || (import.meta as any).env?.VITE_GEMINI_API_KEY || "";
const ai = new GoogleGenAI({ apiKey: GEMINI_API_KEY });

const HISTORY_KEY = 'crisis_history_v1';

export const fetchCrisisReport = async (): Promise<CrisisReport[]> => {
  if (!GEMINI_API_KEY) {
    throw new Error("Gemini API Key가 설정되지 않았습니다. 설정 메뉴에서 API 키를 확인해 주세요.");
  }

  const todayKST = new Intl.DateTimeFormat('ko-KR', { 
    timeZone: 'Asia/Seoul', 
    year: 'numeric', month: '2-digit', day: '2-digit' 
  }).format(new Date());

  const SYSTEM_INSTRUCTION = `
[Role] 대한민국 국가 시스템 붕괴 조기 경보 및 경제 안정성 수호 AI.
[Goal] 실시간 지표를 분석하여 대한민국 경제가 '관리 가능한 위기' 수준에 머물도록 가이드를 제공함.
[Instruction] Google Search 도구를 사용하여 '${todayKST}' 기준 최신 한국 시장 지표(환율, 국채금리, 한국은행 RP 유동성)를 조회하고 분석하십시오.
[Output] 분석 결과를 엄격한 JSON 형식으로만 출력하십시오.

필수 JSON 필드:
{
  "totalScore": 0-100 사이의 위험 점수,
  "riskLevel": "정상" | "경고" | "위험" | "임계",
  "riskiestIndicator": 현재 가장 위협적인 지표 이름 (예: 원달러 환율 폭등),
  "summary": 지표를 종합한 2-3문장의 날카로운 분석 총평,
  "newsKeywords": 현재 시장을 지배하는 키워드 3-5개,
  "advice": [행동 지침 리스트],
  "portfolioRecommendation": { 
    "krwCashRatio": 숫자(%), 
    "foreignCurrencyRatio": 숫자(%), 
    "investmentRatio": 숫자(%), 
    "actionLog": "현시점 가장 시급한 자산 이동 전략" 
  },
  "scenarios": { "base": "기본 전망", "warning": "우려 상황", "worst": "최악의 상황" },
  "triggers": ["주의 깊게 봐야 할 트리거 이벤트 3가지"],
  "metrics": { "bondYield10Yr": 숫자, "exchangeRate": 숫자, "rpBalance": "조 단위 문자열", "pensionBondShare": 숫자, "bankDepositChange": 숫자 }
}`;

  try {
    const response = await ai.models.generateContent({
      model: "gemini-3-flash-preview",
      contents: `${todayKST} 기준 대한민국 금융 시장의 위험 수치를 구체적으로 검색해서 분석해줘.`,
      config: {
        systemInstruction: SYSTEM_INSTRUCTION,
        tools: [{ googleSearch: {} }],
        responseMimeType: "application/json",
        temperature: 0.2,
      },
    });

    const text = response.text;
    if (!text) throw new Error("AI 응답이 비어 있습니다.");
    
    const data = JSON.parse(text);
    const newReport: CrisisReport = {
      id: Date.now().toString(),
      timestamp: new Date().toISOString(),
      rawAnalysis: text,
      ...data
    };

    // Save to localStorage
    const history = await fetchHistory();
    const newHistory = [newReport, ...history].slice(0, 30);
    localStorage.setItem(HISTORY_KEY, JSON.stringify(newHistory));

    return newHistory;
  } catch (error: any) {
    console.error("Gemini Analysis Failed:", error);
    if (error.message?.includes('429') || error.message?.includes('quota')) {
      throw new Error("API 할당량이 초과되었습니다. 잠시 후 다시 시도해 주세요.");
    }
    throw new Error(error.message || "분석 중 오류가 발생했습니다.");
  }
};

export const fetchHistory = async (): Promise<CrisisReport[]> => {
  try {
    const stored = localStorage.getItem(HISTORY_KEY);
    if (!stored) return [];
    return JSON.parse(stored);
  } catch (error) {
    console.warn("Failed to fetch history from localStorage:", error);
    return [];
  }
};

export const resetHistory = async (): Promise<void> => {
  localStorage.removeItem(HISTORY_KEY);
};
