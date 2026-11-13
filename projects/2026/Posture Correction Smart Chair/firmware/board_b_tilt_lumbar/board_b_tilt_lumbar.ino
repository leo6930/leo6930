// --- 보드 식별자 (A보드는 'A', B보드는 'B'로 설정) ---
char BOARD_ID = 'B';

// --- 1번 모터 (틸트) ---
int RPWM_Pin1 = 2; int LPWM_Pin1 = 3;
int R_EN_Pin1 = 4; int L_EN_Pin1 = 5;

// --- 2번 모터 (럼버 서포트) ---
int RPWM_Pin2 = 6; int LPWM_Pin2 = 7;
int R_EN_Pin2 = 8; int L_EN_Pin2 = 9;

int motorSpeed = 180;

void setup() {
  pinMode(RPWM_Pin1, OUTPUT); pinMode(LPWM_Pin1, OUTPUT);
  pinMode(R_EN_Pin1, OUTPUT); pinMode(L_EN_Pin1, OUTPUT);
  digitalWrite(R_EN_Pin1, HIGH); digitalWrite(L_EN_Pin1, HIGH);

  pinMode(RPWM_Pin2, OUTPUT); pinMode(LPWM_Pin2, OUTPUT);
  pinMode(R_EN_Pin2, OUTPUT); pinMode(L_EN_Pin2, OUTPUT);
  digitalWrite(R_EN_Pin2, HIGH); digitalWrite(L_EN_Pin2, HIGH);

  Serial.begin(115200); // 16개 센서 데이터를 빠르게 보내기 위해 속도 상향
}

void loop() {
  // 1. 인식표 출력 (예: "B,")
  Serial.print(BOARD_ID);
  Serial.print(",");

  // 2. 센서 16개 값 파이썬으로 전송 (A0 ~ A15)
  for (int i = 0; i < 16; i++) {
    int val = analogRead(A0 + i);
    Serial.print(val);
    if (i < 15) Serial.print(",");
  }
  Serial.println();

  // 3. 파이썬 명령 수신 및 모터 제어
  if (Serial.available() > 0) {
    char command = Serial.read();

    // --- 틸트 제어 ---
    if (command == 'B') {
      analogWrite(LPWM_Pin1, 0); analogWrite(RPWM_Pin1, motorSpeed);
    } else if (command == 'F') {
      analogWrite(RPWM_Pin1, 0); analogWrite(LPWM_Pin1, motorSpeed);
    } else if (command == 'S') {
      analogWrite(RPWM_Pin1, 0); analogWrite(LPWM_Pin1, 0);
    }

    // --- 등받이 제어 ---
    else if (command == 'I') {
      analogWrite(LPWM_Pin2, 0); analogWrite(RPWM_Pin2, motorSpeed);
    } else if (command == 'O') {
      analogWrite(RPWM_Pin2, 0); analogWrite(LPWM_Pin2, motorSpeed);
    } else if (command == 'X') {
      analogWrite(RPWM_Pin2, 0); analogWrite(LPWM_Pin2, 0);
    }
  }
  delay(50);
}
