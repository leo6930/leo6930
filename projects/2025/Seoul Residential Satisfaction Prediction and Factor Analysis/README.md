
# 🏢 서울 주거 만족도 예측 및 요인 분석
> **Seoul Residential Satisfaction Prediction and Factor Analysis**
> 
> 서울서베이 데이터를 활용하여 서울시민의 주거 종합만족도를 예측하고, 만족도에 영향을 미치는 다각적 요인을 인공지능 다층 퍼셉트론(MLP) 모델로 분석하는 인공지능 프로젝트입니다.

---

## 📌 프로젝트 개요 (Overview)
본 프로젝트는 서울시 자치구별 주거 형태, 주거 점유 형태, 월평균 가구 소득, 교통수단 등의 환경 및 인구통계학적 데이터를 기반으로 **주거 종합만족도(0~5점 척도)를 예측**합니다.
초기 설계한 회귀 모델에서 인공물(Artifact)로 인한 데이터 유출(Data Leakage) 문제를 진단하고, 이를 개선하여 **중간값 기준 만족/불만족을 분류하는 강건한(Robust) 이진 분류 모델로 빌드업**한 과정을 포함하고 있습니다.

---

## 🛠 기술 스택 (Tech Stack)
* **Language:** Python 3
* **Framework:** PyTorch (Deep Learning)
* **Libraries:** Pandas, NumPy, Scikit-Learn, Matplotlib, Joblib
* **Environment:** Google Colab / Jupyter Notebook

---

## ⚙️ 데이터 파이프라인 및 핵심 구현 내용 (Pipeline)

### 1. 데이터 전처리 및 피처 엔지니어링 (Data Preprocessing)
* **결측치 및 예외 처리:** 무응답 혹은 잘못된 시스템 값(`-1`, `99999`)을 `NaN`으로 변환한 뒤 결측치를 처리했습니다. (연속형은 `0`, 범주형은 `'없음'`으로 대체)
* **피처 스케일링 (Scaling):** 연속형 변수는 학습 데이터셋의 평균과 표준편차를 기준으로 **Z-score 표준화(Standardization)**를 수행했습니다.
* **범주형 인코딩 (Encoding):** 자치구 코드, 직업, 가구 소득 등 범주형 변수들은 `pd.get_dummies()`를 활용해 **원-핫 인코딩(One-Hot Encoding)**을 적용했습니다.

### 2. 모델 아키텍처 (Model Architecture)
PyTorch 기반의 **다층 퍼셉트론(MLP)** 구조를 채택했습니다. 과적합(Overfitting)을 방지하기 위해 Dropout 레이어를 레이어 사이에 배치했습니다.
* **Input Layer:** 데이터 전처리 후 매핑된 고차원 피처 (Z-score 피처 + 원-핫 인코딩 피처)
* **Hidden Layer 1:** 256 Nodes + ReLU Activation + Dropout (0.2~0.3)
* **Hidden Layer 2:** 128 Nodes + ReLU Activation (+ Dropout)
* **Output Layer:** 
  * Phase 1 (회귀): 1 Node (Linear) -> 종합만족도 점수 예측
  * Phase 2 (분류): 1 Node (Sigmoid) -> 만족 여부 확률 예측

### 3. 학습 및 최적화 기법 (Training & Optimization)
* **Loss Function:** `MSELoss` (회귀) / `BCELoss` (이진 분류)
* **Optimizer:** Adam Optimizer (Learning Rate = 1e-4)
* **정규화 기법:** Gradient Clipping (Max Norm = 5.0)을 적용해 그래디언트 폭주를 방지하고 학습 안정성을 높였습니다.
* **Learning Rate Scheduler:** `ReduceLROnPlateau`를 도입하여 Validation Loss가 정체될 때 학습률을 자동으로 감소시켰습니다.

---

## 📈 모델 고도화 과정 및 결과 (Model Refinement)

### 📊 Phase 1: 파이토치 회귀(MLP Regression) 모델
* **방법:** 종합만족도 점수(0~5점)를 직접 예측하는 회귀 모델을 설계했습니다.
* **결과:** $R^2$ (결정계수) 점수가 **0.9913**이라는 비정상적으로 높은 성능을 기록했습니다.
* **문제 진단:** 데이터 내에 포함된 '통근 환경 만족도', '녹지 환경 만족도', '야간 보행 안전도' 등 세부 만족도 피처들이 타겟 변수인 '종합만족도'와 직접적인 인과 및 높은 상관관계를 지니고 있어 **데이터 유출(Data Leakage)**이 발생했음을 확인했습니다.

### 🛡️ Phase 2: 데이터 유출 제거 후 이진 분류(MLP Binary Classification) 모델
* **개선 작업:** 타겟 변수의 힌트가 될 수 있는 모든 '만족도' 및 '안전도' 계열 피처(21개)를 피처셋에서 완전히 배제했습니다.
* **문제 재정의:** 종합만족도의 중간값(Median)인 `3.0196`을 기준으로 **초과(1: 만족), 이하(0: 불만족)**로 타겟을 이진화했습니다.
* **최종 성능 결과:**
  * **Accuracy:** 69.33%
  * **Precision:** 68.56%
  * **Recall:** 66.95%
  * **F1 Score:** 67.74%
* **결론:** 유출 피처를 제거함으로써 실변수(인구통계 정보, 주거 특성 등) 기반의 현실적이고 왜곡 없는 예측 모델을 완성했습니다.

---

## 🗺️ 자치구별 만족도 추론 및 시각화 (Inference & Visualization)
* 학습된 전처리 메타 데이터(`preprocess_meta.pkl`)와 최적 가중치 파일(`satisfaction_model.pth`, `best_binary_model.pth`)을 저장하여 인프라 독립적인 추론 파이프라인을 구축했습니다.
* 자치구별(`구코드`) 대표 행 데이터를 추출하여 각 구별 만족도 지수를 예측하고 텍스트 리포트 및 성능 곡선(ROC Curve, PR Curve, Loss Curve) 시각화를 지원합니다.
