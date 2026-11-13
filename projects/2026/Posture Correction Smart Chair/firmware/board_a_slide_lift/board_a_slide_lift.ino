// --- 보드 식별자: A보드 ---
char BOARD_ID = 'A';

// --- 3번 모터 (슬라이드용) - 핀 번호를 A보드와 동일하게 설정 가능 ---
int RPWM_Pin3 = 2; int LPWM_Pin3 = 3;
int R_EN_Pin3 = 4; int L_EN_Pin3 = 5;

// --- 4번 모터 (높낮이용) ---
int RPWM_Pin4 = 10; int LPWM_Pin4 = 11;
int R_EN_Pin4 = 12; int L_EN_Pin4 = 13;

int motorSpeed = 180;

void setup() {
  pinMode(RPWM_Pin3, OUTPUT); pinMode(LPWM_Pin3, OUTPUT);
  pinMode(R_EN_Pin3, OUTPUT); pinMode(L_EN_Pin3, OUTPUT);
  digitalWrite(R_EN_Pin3, HIGH); digitalWrite(L_EN_Pin3, HIGH);

  pinMode(RPWM_Pin4, OUTPUT); pinMode(LPWM_Pin4, OUTPUT);
  pinMode(R_EN_Pin4, OUTPUT); pinMode(L_EN_Pin4, OUTPUT);
  digitalWrite(R_EN_Pin4, HIGH); digitalWrite(L_EN_Pin4, HIGH);

  Serial.begin(115200);
}

void loop() {
  // 1. 인식표 출력
  Serial.print(BOARD_ID);
  Serial.print(",");

  // 2. 16개 센서 데이터 송신 (A0 ~ A15)
  for (int i = 0; i < 16; i++) {
    int val = analogRead(A0 + i);
    Serial.print(val);
    if (i < 15) Serial.print(",");
  }
  Serial.println();

  // 3. 파이썬 명령 수신
  if (Serial.available() > 0) {
    char command = Serial.read();

    if (command == 'U') {        // 슬라이드 전진
      analogWrite(LPWM_Pin3, 0); analogWrite(RPWM_Pin3, motorSpeed);
    } else if (command == 'D') { // 슬라이드 후진
      analogWrite(RPWM_Pin3, 0); analogWrite(LPWM_Pin3, motorSpeed);
    } else if (command == 'Y') { // 슬라이드 정지
      analogWrite(RPWM_Pin3, 0); analogWrite(LPWM_Pin3, 0);
    }
    else if (command == 'H') {   // 높낮이 상승
      analogWrite(LPWM_Pin4, 0); analogWrite(RPWM_Pin4, motorSpeed);
    } else if (command == 'L') { // 높낮이 하강
      analogWrite(RPWM_Pin4, 0); analogWrite(LPWM_Pin4, motorSpeed);
    } else if (command == 'Z') { // 높낮이 정지
      analogWrite(RPWM_Pin4, 0); analogWrite(LPWM_Pin4, 0);
    }
  }
  delay(50);
}
