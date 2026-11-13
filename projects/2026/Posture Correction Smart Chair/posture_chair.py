import cv2
import mediapipe as mp
import math
import serial
import serial.tools.list_ports
import time
import sys
import threading

########### 찐 최종 코드 건들지 말것!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1######################################################
# ==========================================
# 1. 미디어파이프 및 카메라 스레드 설정
# ==========================================
mp_pose = mp.solutions.pose
pose = mp_pose.Pose(min_detection_confidence=0.5, min_tracking_confidence=0.5, model_complexity=0)

class ThreadedCamera:
    def __init__(self, src=1):
        self.cap = cv2.VideoCapture(src)
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
        self.ret, self.frame = self.cap.read()
        self.running = True

        self.thread = threading.Thread(target=self.update, args=())
        self.thread.daemon = True
        self.thread.start()

    def update(self):
        while self.running:
            ret, frame = self.cap.read()
            if ret:
                self.ret = ret
                self.frame = frame
            time.sleep(0.01)

    def read(self):
        if self.frame is not None:
            return self.ret, self.frame.copy()
        return False, None

    def release(self):
        self.running = False
        self.thread.join(timeout=1.0)
        self.cap.release()

    def isOpened(self):
        return self.cap.isOpened()

cap = ThreadedCamera(1)

# ==========================================
# 2. 시리얼 포트 설정
# ==========================================
ports = serial.tools.list_ports.comports()
available_ports = [p.device for p in ports if 'ttyACM' in p.device or 'COM' in p.device]
baud_rate = 115200
ser_list = []

print("🚀 통합 시스템 시작 준비 중...")
for p in available_ports:
    try: ser_list.append(serial.Serial(p, baud_rate, timeout=0.01))
    except: pass

# ==========================================
# 3. 소프트웨어 영점 조절
# ==========================================
def run_calibration(sample_time=3.0):
    print("\n" + "="*50)
    print(" ⚠️ 3초 뒤 영점 조절을 시작합니다.")
    print(" ⚠️ 의자에서 일어나 주시고, 시트에 아무것도 두지 마세요!")
    print("="*50)
    time.sleep(3)

    print("🔄 영점 데이터 수집 중... (움직이지 마세요)")
    start_time = time.time()
    samples_a, samples_b = [], []

    for ser in ser_list: ser.reset_input_buffer()

    while time.time() - start_time < sample_time:
        for ser in ser_list:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    raw = line.split(',')
                    if len(raw) == 17:
                        vals = [int(x) for x in raw[1:]]
                        if raw[0] == 'A': samples_a.append(vals)
                        elif raw[0] == 'B': samples_b.append(vals)
                except: pass

    base_a = [int(sum(s[i] for s in samples_a)/len(samples_a)) for i in range(16)] if samples_a else [0]*16
    base_b = [int(sum(s[i] for s in samples_b)/len(samples_b)) for i in range(16)] if samples_b else [0]*16

    print("✅ 영점 조절 완료!")
    return base_a, base_b

baseline_a, baseline_b = run_calibration()

# ==========================================
# 4. 상태 변수 및 모터 추적 설정
# ==========================================
data_a, data_b = [0] * 16, [0] * 16
states = {'slide': 'STOP', 'tilt': 'STOP', 'lumbar': 'STOP', 'lift': 'STOP'}
last_cmd_time = {'slide': 0.0, 'tilt': 0.0, 'lumbar': 0.0, 'lift': 0.0}

motor_offsets = {'slide': 0.0, 'tilt': 0.0, 'lumbar': 0.0, 'lift': 0.0}

VALID_PRESSURE_THRESHOLD = 27
IS_RIGHT_SIDE = True

def apply_tare_and_filter(raw_data, baseline):
    return [val if (val := max(0, r - b)) >= VALID_PRESSURE_THRESHOLD else 0 for r, b in zip(raw_data, baseline)]

def calculate_profile_neck_angle(shoulder_lm, ear_lm, w, h):
    if shoulder_lm.visibility < 0.5 or ear_lm.visibility < 0.5: return 90.0
    dx, dy = abs((ear_lm.x * w) - (shoulder_lm.x * w)), abs((shoulder_lm.y * h) - (ear_lm.y * h))
    return math.degrees(math.atan2(dx, dy)) if dy != 0 else 90.0

def send_motor_command(motor_type, command):
    global last_cmd_time
    curr_time = time.time()

    if states[motor_type] != command or (curr_time - last_cmd_time[motor_type] > 0.3):
        states[motor_type] = command
        last_cmd_time[motor_type] = curr_time

        cmd_bytes = b''
        if motor_type == 'slide': cmd_bytes = b'U' if command == 'FORWARD' else b'D' if command == 'BACKWARD' else b'Y'
        elif motor_type == 'tilt': cmd_bytes = b'B' if command == 'FORWARD' else b'F' if command == 'BACKWARD' else b'S'
        elif motor_type == 'lumbar': cmd_bytes = b'I' if command == 'FORWARD' else b'O' if command == 'BACKWARD' else b'X'
        elif motor_type == 'lift': cmd_bytes = b'H' if command == 'FORWARD' else b'L' if command == 'BACKWARD' else b'Z'

        if cmd_bytes:
            for s in ser_list:
                try:
                    s.write(cmd_bytes)
                    s.flush()
                except: pass

# ==========================================
# 5. 맞춤형 데이터 수집
# ==========================================
def run_personal_calibration(sample_time=5.0):
    while True:
        print("\n" + "="*50)
        print(" 👤 사용자 맞춤 설정 시작")
        print(" ⚠️ 의자에 바른 자세로 깊숙이 앉아주세요!")
        input(" 🟢 준비가 완료되면 터미널에서 [Enter] 키를 누르세요...")
        print("="*50)
        print("🔄 바른 자세 데이터(어깨, 좌판) 수집 중... (5초간 자세를 유지하세요)")

        start_time = time.time()
        shoulder_samples = []
        seat_front_samples = []
        seat_back_samples = []

        for ser in ser_list: ser.reset_input_buffer()

        while time.time() - start_time < sample_time:
            for ser in ser_list:
                # 🚨 버그 수정: 버퍼에 쌓인 데이터를 한 줄씩 안전하게 읽어오도록 while 루프 및 readline 적용
                while ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        if not line: continue
                        raw = line.split(',')

                        if len(raw) == 17:
                            # A 패킷: 방석 (허벅지/엉덩이)
                            if raw[0] == 'A':
                                raw_a = [int(x) for x in raw[1:]]
                                v_data_a = apply_tare_and_filter(raw_a, baseline_a)
                                v_front = sum(v_data_a[i] for i in [0, 1, 2, 3, 8, 9, 10, 11])
                                v_back = sum(v_data_a[i] for i in [4, 5, 6, 7, 12, 13, 14, 15])
                                seat_front_samples.append(v_front)
                                seat_back_samples.append(v_back)
                            # B 패킷: 등받이 (어깨)
                            elif raw[0] == 'B':
                                raw_b = [int(x) for x in raw[1:]]
                                v_data_b = apply_tare_and_filter(raw_b, baseline_b)
                                v_shoulder = v_data_b[6] + v_data_b[7] + v_data_b[14] + v_data_b[15]
                                shoulder_samples.append(v_shoulder)
                    except: pass

        # 수집된 데이터가 있는지 확인 후 계산
        if shoulder_samples and seat_front_samples and seat_back_samples:
            personal_shoulder = int(sum(shoulder_samples) / len(shoulder_samples))
            avg_front = sum(seat_front_samples) / len(seat_front_samples)
            avg_back = sum(seat_back_samples) / len(seat_back_samples)

            # 방어 로직: 엉덩이를 떼고 있었을 경우 0으로 나누는 오류 방지
            personal_seat_ratio = avg_front / avg_back if avg_back > 50 else 1.0
        else:
            personal_shoulder = 0
            personal_seat_ratio = 1.0
            avg_back = 0

        if personal_shoulder < 250:
            print(f"\n❌ [측정 실패] 어깨 압력이 너무 낮습니다! (측정값: {personal_shoulder})")
            print("🔄 2초 뒤 측정을 다시 시작합니다...")
            time.sleep(2)
            continue

        if avg_back < 200:
            print(f"\n❌ [측정 실패] 엉덩이 압력이 너무 낮습니다! 좌판에 끝까지 밀착해 앉아주세요.")
            print("🔄 2초 뒤 측정을 다시 시작합니다...")
            time.sleep(2)
            continue

        print(f"\n✅ 맞춤형 어깨 기준값 설정 완료: {personal_shoulder}")
        print(f"✅ 맞춤형 좌판 비율(허벅지/엉덩이) 설정 완료: {personal_seat_ratio:.2f}")
        return personal_shoulder, personal_seat_ratio

PERSONAL_SHOULDER_THRESHOLD, PERSONAL_SEAT_RATIO = run_personal_calibration()

# ==========================================
# 5.5. 카메라 동기화 대기
# ==========================================
print("\n" + "="*50)
print(" 📷 카메라 동기화 및 위치 조정 (모터 대기 중)")
print(" ⚠️ 화면 창이 뜨면, 내 몸에 초록색 선이 안정적으로 그려지는지 확인하세요.")
print(" 🟢 화면 창을 클릭한 후 [Enter] 또는 [S] 키를 누르면 제어가 시작됩니다!")
print("="*50)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret: break

    results = pose.process(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))

    if results.pose_landmarks:
        lm = results.pose_landmarks.landmark
        target_shoulder = lm[12] if IS_RIGHT_SIDE else lm[11]
        target_ear = lm[8] if IS_RIGHT_SIDE else lm[7]

        if target_shoulder.visibility > 0.5 and target_ear.visibility > 0.5:
            h, w, _ = frame.shape
            s_px = (int(target_shoulder.x * w), int(target_shoulder.y * h))
            e_px = (int(target_ear.x * w), int(target_ear.y * h))
            cv2.circle(frame, s_px, 5, (0, 0, 255), -1)
            cv2.circle(frame, e_px, 5, (0, 0, 255), -1)
            cv2.line(frame, s_px, e_px, (0, 255, 0), 3)
            cv2.putText(frame, "TRACKING OK!", (20, 130), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
        else:
            cv2.putText(frame, "WAITING FOR TRACKING...", (20, 130), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

    cv2.putText(frame, "[ PREVIEW MODE : MOTOR OFF ]", (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 255), 3)
    cv2.putText(frame, "Click here & Press 'ENTER' to Start", (20, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    cv2.imshow('System Debug View', frame)

    key = cv2.waitKey(1) & 0xFF
    if key == 13 or key == ord('s') or key == ord('S'):
        print("\n✅ 카메라 동기화 완료! 실시간 모터 제어를 시작합니다.")
        break
    elif key == ord('q') or key == ord('Q'):
        print("\n🛑 시스템을 종료합니다.")
        cap.release()
        cv2.destroyAllWindows()
        for s in ser_list: s.close()
        sys.exit()

# ==========================================
# 6. 메인 루프
# ==========================================
try:
    last_print_time = time.time()
    last_update_time = time.time()
    smooth_turtle_score = 0.0

    # 💡 틸트만 일어날 때(Forward)와 누울 때(Backward)의 가중치를 분리했습니다.
    RETURN_WEIGHTS = {
        'slide': 1.2,
        'tilt_forward_return': 1.2,   # 🚨 [수정됨] 누웠다가 세워질 때 (0.8 -> 1.2으로 높여서 시간을 더 짧게 끊어줌)
        'tilt_backward_return': 1.3,  # [유지] 숙였다가 누울 때 (기존 0.8로 잘 된다고 하셨으므로 건드리지 않음)
        'lumbar': 1.3,
        'lift': 1.3
    }

    for ser in ser_list: ser.reset_input_buffer()

    while cap.isOpened():
        current_time = time.time()
        dt = current_time - last_update_time
        last_update_time = current_time

        # 🚨 [수정 완료] 틸트만 방향을 구별하여 가중치 적용
        for m in ['slide', 'tilt', 'lumbar', 'lift']:
            # 현재 모터가 0점(원점)을 향해 돌아가고 있는지 판별
            is_returning = False
            if (motor_offsets[m] > 0 and states[m] == 'BACKWARD') or (motor_offsets[m] < 0 and states[m] == 'FORWARD'):
                is_returning = True

            applied_dt = dt
            if is_returning:
                if m == 'tilt':
                    if states[m] == 'FORWARD':  # 뒤로 누웠다가 세워지는 복귀
                        applied_dt = dt * RETURN_WEIGHTS['tilt_forward_return']
                    else:                       # 앞으로 숙였다가 눕는 복귀
                        applied_dt = dt * RETURN_WEIGHTS['tilt_backward_return']
                else:
                    applied_dt = dt * RETURN_WEIGHTS[m]

            if states[m] == 'FORWARD': motor_offsets[m] += applied_dt
            elif states[m] == 'BACKWARD': motor_offsets[m] -= applied_dt

        for ser in ser_list:
            if ser.in_waiting > 500: ser.reset_input_buffer()
            while ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    raw = line.split(',')
                    if len(raw) == 17:
                        if raw[0] == 'A': data_a = [int(x) for x in raw[1:]]
                        elif raw[0] == 'B': data_b = [int(x) for x in raw[1:]]
                except: pass

        v_data_a = apply_tare_and_filter(data_a, baseline_a)
        v_data_b = apply_tare_and_filter(data_b, baseline_b)

        ret, frame = cap.read()
        if not ret: break
        results = pose.process(cv2.cvtColor(frame, cv2.COLOR_BGR2RGB))

        neck_angle = 90.0
        sh_vis, ear_vis = 0.0, 0.0
        h, w, _ = frame.shape

        if results.pose_landmarks:
            lm = results.pose_landmarks.landmark
            target_shoulder = lm[12] if IS_RIGHT_SIDE else lm[11]
            target_ear = lm[8] if IS_RIGHT_SIDE else lm[7]
            sh_vis, ear_vis = target_shoulder.visibility, target_ear.visibility
            neck_angle = calculate_profile_neck_angle(target_shoulder, target_ear, w, h)

            if sh_vis > 0.5 and ear_vis > 0.5:
                s_px = (int(target_shoulder.x * w), int(target_shoulder.y * h))
                e_px = (int(target_ear.x * w), int(target_ear.y * h))
                cv2.circle(frame, s_px, 5, (0, 0, 255), -1)
                cv2.circle(frame, e_px, 5, (0, 0, 255), -1)
                cv2.line(frame, s_px, e_px, (0, 255, 0), 3)

        v_seat_front = sum(v_data_a[i] for i in [0, 1, 2, 3, 8, 9, 10, 11])
        v_seat_back = sum(v_data_a[i] for i in [4, 5, 6, 7, 12, 13, 14, 15])

        v_lumbar = sum(v_data_b[0:6])
        v_shoulder = v_data_b[6] + v_data_b[7] + v_data_b[14] + v_data_b[15]

        is_empty_chair = (sum(v_data_a) < 800)

        # ==================================================
        # D. 모터 제어 결정 로직
        # ==================================================
        desired_cmds = {'slide': 'STOP', 'tilt': 'STOP', 'lumbar': 'STOP', 'lift': 'STOP'}
        is_returning_ui = False

        if is_empty_chair:
            status_text = "GOOD POSTURE"
            color = (0, 255, 0)
            is_returning_ui = True
            smooth_turtle_score = 0.0

            # 상체(틸트, 럼버) 먼저 원점 복귀 -> 이후 하체 복귀
            need_return_upper = (motor_offsets['tilt'] > 0.2 or motor_offsets['tilt'] < -0.2 or
                                 motor_offsets['lumbar'] > 0.2 or motor_offsets['lumbar'] < -0.2)

            if need_return_upper:
                desired_cmds['slide'] = 'STOP'
                desired_cmds['lift'] = 'STOP'

                if motor_offsets['tilt'] > 0.2: desired_cmds['tilt'] = 'BACKWARD'
                elif motor_offsets['tilt'] < -0.2: desired_cmds['tilt'] = 'FORWARD'
                else: desired_cmds['tilt'] = 'STOP'; motor_offsets['tilt'] = 0.0

                if motor_offsets['lumbar'] > 0.2: desired_cmds['lumbar'] = 'BACKWARD'
                elif motor_offsets['lumbar'] < -0.2: desired_cmds['lumbar'] = 'FORWARD'
                else: desired_cmds['lumbar'] = 'STOP'; motor_offsets['lumbar'] = 0.0
            else:
                need_return_lower = (motor_offsets['slide'] > 0.2 or motor_offsets['slide'] < -0.2 or
                                     motor_offsets['lift'] > 0.2 or motor_offsets['lift'] < -0.2)

                if need_return_lower:
                    if motor_offsets['slide'] > 0.2: desired_cmds['slide'] = 'BACKWARD'
                    elif motor_offsets['slide'] < -0.2: desired_cmds['slide'] = 'FORWARD'
                    else: desired_cmds['slide'] = 'STOP'; motor_offsets['slide'] = 0.0

                    if motor_offsets['lift'] > 0.2: desired_cmds['lift'] = 'BACKWARD'
                    elif motor_offsets['lift'] < -0.2: desired_cmds['lift'] = 'FORWARD'
                    else: desired_cmds['lift'] = 'STOP'; motor_offsets['lift'] = 0.0

                    desired_cmds['tilt'] = 'STOP'
                    desired_cmds['lumbar'] = 'STOP'
                else:
                    desired_cmds['slide'] = 'STOP'
                    desired_cmds['lift'] = 'STOP'
                    desired_cmds['tilt'] = 'STOP'
                    desired_cmds['lumbar'] = 'STOP'

        else:
            # ── 판정 ──────────────────
            base_ratio_limit = PERSONAL_SEAT_RATIO + 0.35
            current_ratio_limit = (base_ratio_limit + 0.25) if motor_offsets['lift'] > 1.0 else base_ratio_limit

            is_slide_bad = (v_seat_front > (v_seat_back * current_ratio_limit))

            tilt_threshold = PERSONAL_SHOULDER_THRESHOLD * 0.3
            is_tilt_bad = v_shoulder < tilt_threshold

            if is_slide_bad:
                is_tilt_bad = False

            TURTLE_ANGLE_LIMIT = 40.0
            if neck_angle > TURTLE_ANGLE_LIMIT:
                is_neck_bad = True
                smooth_turtle_score = 1.0
            else:
                norm_angle = neck_angle / TURTLE_ANGLE_LIMIT
                press_loss = 0.0 if v_shoulder > (PERSONAL_SHOULDER_THRESHOLD * 0.7) else max(0, (PERSONAL_SHOULDER_THRESHOLD - v_shoulder) / float(PERSONAL_SHOULDER_THRESHOLD))
                current_turtle_score = (0.8 * norm_angle) + (0.4 * press_loss)
                smooth_turtle_score = (smooth_turtle_score * 0.8) + (current_turtle_score * 0.2)
                is_neck_bad = smooth_turtle_score > 0.70

            # ── 독립 제어 ────────────────────────────────────
            active_issues = []

            # 1. 좌판 슬라이딩 감지 시
            if is_slide_bad:
                # 🚨 [확인 완료] 슬라이드와 틸트 무조건 동시 명령
                desired_cmds['tilt'] = 'BACKWARD'
                desired_cmds['slide'] = 'FORWARD'
                active_issues.append("SLIDING")

            # 2. 어깨 떠짐 감지 시
            if is_tilt_bad:
                desired_cmds['tilt'] = 'BACKWARD'
                desired_cmds['lift'] = 'BACKWARD'
                if "TILT" not in active_issues: active_issues.append("TILT")

            # 3. 거북목 감지 시
            if is_neck_bad:
                # 다른 자세 때문에 틸트가 이미 작동 중이 아니라면 구동
                if desired_cmds['tilt'] == 'STOP':
                    desired_cmds['tilt'] = 'FORWARD'
                desired_cmds['lumbar'] = 'FORWARD'
                if "NECK" not in active_issues: active_issues.append("NECK")

            if active_issues:
                status_text = f"BAD ({', '.join(active_issues)})"
                color = (0, 0, 255)

                active_motors = [m for m in ['slide', 'tilt', 'lift', 'lumbar'] if desired_cmds[m] != 'STOP']

                if len(active_motors) > 2:
                    for m in active_motors[2:]:
                        desired_cmds[m] = 'STOP'
                    status_text += " (PWR LIMIT)"

            # 4. 바른 자세 → 순차 복귀 (상체 먼저 -> 하체 나중에)
            else:
                need_return_upper = (motor_offsets['tilt'] > 0.2 or motor_offsets['tilt'] < -0.2 or
                                     motor_offsets['lumbar'] > 0.2 or motor_offsets['lumbar'] < -0.2)

                if need_return_upper:
                    desired_cmds['slide'] = 'STOP'
                    desired_cmds['lift'] = 'STOP'

                    if motor_offsets['tilt'] > 0.2: desired_cmds['tilt'] = 'BACKWARD'
                    elif motor_offsets['tilt'] < -0.2: desired_cmds['tilt'] = 'FORWARD'
                    else: desired_cmds['tilt'] = 'STOP'; motor_offsets['tilt'] = 0.0

                    if motor_offsets['lumbar'] > 0.2: desired_cmds['lumbar'] = 'BACKWARD'
                    elif motor_offsets['lumbar'] < -0.2: desired_cmds['lumbar'] = 'FORWARD'
                    else: desired_cmds['lumbar'] = 'STOP'; motor_offsets['lumbar'] = 0.0

                    status_text = "RETURNING (UPPER)"
                    color = (255, 255, 0)

                else:
                    need_return_lower = (motor_offsets['slide'] > 0.2 or motor_offsets['slide'] < -0.2 or
                                         motor_offsets['lift'] > 0.2 or motor_offsets['lift'] < -0.2)

                    if need_return_lower:
                        if motor_offsets['slide'] > 0.2: desired_cmds['slide'] = 'BACKWARD'
                        elif motor_offsets['slide'] < -0.2: desired_cmds['slide'] = 'FORWARD'
                        else: desired_cmds['slide'] = 'STOP'; motor_offsets['slide'] = 0.0

                        if motor_offsets['lift'] > 0.2: desired_cmds['lift'] = 'BACKWARD'
                        elif motor_offsets['lift'] < -0.2: desired_cmds['lift'] = 'FORWARD'
                        else: desired_cmds['lift'] = 'STOP'; motor_offsets['lift'] = 0.0

                        desired_cmds['tilt'] = 'STOP'
                        desired_cmds['lumbar'] = 'STOP'

                        status_text = "RETURNING (LOWER)"
                        color = (255, 255, 0)
                    else:
                        desired_cmds['slide'] = 'STOP'
                        desired_cmds['lift'] = 'STOP'
                        desired_cmds['tilt'] = 'STOP'
                        desired_cmds['lumbar'] = 'STOP'

                        status_text = "GOOD POSTURE"
                        color = (0, 255, 0)

        # 🚨 [수정 완료] 절대 안전 한계치 증가 (4.0 -> 6.0초)
        # 틸트가 작동하다가 한계치에 막혀서 멈추는 현상 방지
        max_times = {'slide': 5.0, 'tilt': 6.0, 'lumbar': 5.0, 'lift': 5.0}
        for m in ['slide', 'tilt', 'lumbar', 'lift']:
            target_cmd = desired_cmds[m]
            limit = max_times[m]
            if target_cmd == 'FORWARD' and motor_offsets[m] >= limit:
                target_cmd = 'STOP'
            elif target_cmd == 'BACKWARD' and motor_offsets[m] <= -limit:
                target_cmd = 'STOP'
            desired_cmds[m] = target_cmd

        for m in ['slide', 'tilt', 'lumbar', 'lift']:
            send_motor_command(m, desired_cmds[m])

        # ==================================================
        # E. 화면 출력 및 디버깅 텍스트
        # ==================================================
        cv2.putText(frame, f"STATUS: {status_text}", (20, 50), cv2.FONT_HERSHEY_SIMPLEX, 1.2, color, 3)
        if is_returning_ui:
            cv2.putText(frame, "-> Auto Returning", (20, 80), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)

        cv2.putText(frame, f"[CAM] Vis: {sh_vis:.2f}/{ear_vis:.2f}", (20, 105), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0) if (sh_vis>0.5 and ear_vis>0.5) else (0,0,255), 2)
        cv2.putText(frame, f"Neck Angle : {neck_angle:.1f} deg", (20, 130), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        cv2.putText(frame, "--- MOTOR OFFSETS (sec) ---", (20, 175), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
        cv2.putText(frame, f"Slide : {motor_offsets['slide']:+.1f}s", (20, 200), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.putText(frame, f"Tilt  : {motor_offsets['tilt']:+.1f}s", (20, 225), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.putText(frame, f"Lumbar: {motor_offsets['lumbar']:+.1f}s", (20, 250), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        cv2.putText(frame, f"Lift  : {motor_offsets['lift']:+.1f}s", (20, 275), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

        cv2.putText(frame, "[FUSION DEBUG]", (400, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)
        score_color = (0, 0, 255) if smooth_turtle_score > 0.70 else (0, 255, 0)
        cv2.putText(frame, f"TOTAL : {smooth_turtle_score:.2f} / 0.70", (400, 55), cv2.FONT_HERSHEY_SIMPLEX, 0.6, score_color, 2)

        if current_time - last_print_time >= 1.0:
            if is_empty_chair:
                print(f"🟢 [GOOD POSTURE] (빈 의자) | Slide:{motor_offsets['slide']:+.1f}s | Tilt:{motor_offsets['tilt']:+.1f}s | Lumbar:{motor_offsets['lumbar']:+.1f}s | Lift:{motor_offsets['lift']:+.1f}s")
            else:
                status_icon = "🟢" if status_text == "GOOD POSTURE" else "🔴"
                print(f"{status_icon} [{status_text}] 목: {neck_angle:5.1f}° | 어깨: {v_shoulder:4d} | Slide:{motor_offsets['slide']:+.1f}s | Tilt:{motor_offsets['tilt']:+.1f}s | Lumbar:{motor_offsets['lumbar']:+.1f}s | Lift:{motor_offsets['lift']:+.1f}s")
            last_print_time = current_time

        cv2.imshow('System Debug View', frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord('q') or key == ord('Q'):
            break
        elif key == ord('r') or key == ord('R'):
            print("\n🔄 수동 영점 조절 모든 모터 타이머를 0.0으로 리셋합니다.")
            for m in ['slide', 'tilt', 'lumbar', 'lift']:
                motor_offsets[m] = 0.0

finally:
    print("\n🛑 시스템 종료: 모터 정지 중...")
    for m in ['slide', 'tilt', 'lumbar', 'lift']: send_motor_command(m, 'STOP')
    cap.release()
    cv2.destroyAllWindows()
    for s in ser_list: s.close()
