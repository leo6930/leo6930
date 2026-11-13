"""
얼굴인식을 활용한 2단계 인증 금고 시스템 (PC 측 메인 프로그램)

1단계: OpenCV + face_recognition 으로 등록된 사용자 얼굴 인증
2단계: 키패드 비밀번호 인증
얼굴 인증 3회 실패 시 침입자 사진 + GPS 위치 지도를 Google Drive 에 업로드

PC <-> Topst D3 보드는 UART(시리얼)로 통신하며,
보드는 LCD / LED / 부저 / 서보모터 / MPU-6050(움직임 감지)를 제어한다.
"""

import cv2, face_recognition, os, time, serial, mimetypes
import folium
from google.oauth2 import service_account
from googleapiclient.discovery import build
from googleapiclient.http import MediaFileUpload

# ───── 설정 (환경변수로 덮어쓰기 가능) ─────
BOARD_PORT   = os.environ.get("SAFE_BOARD_PORT", "/dev/ttyUSB0")   # Topst D3 UART
GPS_PORT     = os.environ.get("SAFE_GPS_PORT", "/dev/ttyACM0")     # u-blox GPS
CAMERA_DEV   = os.environ.get("SAFE_CAMERA", "/dev/video0")
IMAGE_ROOT   = os.environ.get("SAFE_IMAGE_ROOT", "/home/leo/face_recognition/images")
PASSWORD     = os.environ.get("SAFE_PASSWORD", "123")               # 2단계 인증 비밀번호
FOLDER_ID    = os.environ.get("SAFE_DRIVE_FOLDER_ID", "")           # 업로드할 Google Drive 폴더 ID
CREDENTIALS_FILE = os.environ.get("SAFE_CREDENTIALS", "credentials.json")  # 서비스 계정 키 (커밋 금지)
MATCH_TOLERANCE  = 0.45   # 얼굴 거리 임계값 (낮을수록 엄격)
MAX_FAIL         = 3      # 침입자 판정까지 허용하는 얼굴 인증 실패 횟수

# ───── 시리얼 설정 ─────
SER = serial.Serial(BOARD_PORT, 115200, timeout=1)
time.sleep(2)
SER.flushInput()

# ───── 경로 설정 ─────
KNOWN_DIR   = os.path.join(IMAGE_ROOT, "known")
UNKNOWN_DIR = os.path.join(IMAGE_ROOT, "unknown")
INVADER_DIR = os.path.join(IMAGE_ROOT, "invader")
os.makedirs(UNKNOWN_DIR, exist_ok=True)
os.makedirs(INVADER_DIR, exist_ok=True)

# ───── 카메라 열기 ─────
video = cv2.VideoCapture(CAMERA_DEV, cv2.CAP_V4L2)
video.set(cv2.CAP_PROP_BUFFERSIZE, 1)

print("시스템 준비 완료")

# ───── 구글 드라이브 설정 ─────
SCOPES = ['https://www.googleapis.com/auth/drive.file']
credentials = service_account.Credentials.from_service_account_file(CREDENTIALS_FILE, scopes=SCOPES)
drive_service = build('drive', 'v3', credentials=credentials)


def send(cmd: bytes):
    SER.write(cmd + b"\n")
    SER.flush()


def upload_to_drive(files):
    for f in files:
        if not f or not os.path.exists(f):
            continue
        # 같은 이름의 파일이 있으면 삭제 후 재업로드
        results = drive_service.files().list(
            q=f"name='{os.path.basename(f)}' and '{FOLDER_ID}' in parents",
            fields="files(id)").execute()
        for file in results.get('files', []):
            drive_service.files().delete(fileId=file['id']).execute()
        metadata = {
            'name': os.path.basename(f),
            'parents': [FOLDER_ID],
            'mimeType': mimetypes.guess_type(f)[0] or 'application/octet-stream'
        }
        media = MediaFileUpload(f, mimetype=metadata['mimeType'])
        uploaded = drive_service.files().create(body=metadata, media_body=media, fields='id').execute()
        print(f"✅ 업로드 완료: {f} → https://drive.google.com/file/d/{uploaded.get('id')}/view")


def convert_to_degrees(raw, direction, is_latitude=True):
    """NMEA ddmm.mmmm / dddmm.mmmm 형식을 십진수 도(degree)로 변환"""
    try:
        deg = float(raw[:2]) if is_latitude else float(raw[:3])
        minute = float(raw[2:]) if is_latitude else float(raw[3:])
        result = deg + (minute / 60.0)
        if direction in ['S', 'W']:
            result *= -1
        return result
    except Exception as e:
        print("⚠️ 변환 오류:", e)
        return None


def get_gps_coordinates():
    """GPGGA/GNGGA 문장에서 유효한 위·경도를 받을 때까지 읽는다"""
    try:
        ser = serial.Serial(GPS_PORT, 9600, timeout=1)
    except Exception as e:
        print("❌ GPS 포트 열기 실패:", e)
        return None, None
    print("📡 GPS 수신 시작...")
    lat, lon = None, None
    while True:
        try:
            line = ser.readline().decode(errors='ignore').strip()
            if line.startswith("$GPGGA") or line.startswith("$GNGGA"):
                parts = line.split(',')
                print("🔄 수신 데이터:", line)
                if len(parts) > 5 and parts[2] and parts[3] and parts[4] and parts[5]:
                    lat = convert_to_degrees(parts[2], parts[3], is_latitude=True)
                    lon = convert_to_degrees(parts[4], parts[5], is_latitude=False)
                    if lat and lon:
                        print(f"📍 현재 좌표: 위도 {lat}, 경도 {lon}")
                        break
        except Exception as e:
            print("❌ 오류 발생:", e)
            break
    ser.close()
    return lat, lon


def generate_map(lat, lon, ts):
    """folium 으로 금고 위치를 표시한 HTML 지도 생성"""
    file_name = f"gps_map_{ts}.html"
    if lat and lon:
        m = folium.Map(location=[lat, lon], zoom_start=17)
        folium.Marker([lat, lon], tooltip="현재 위치").add_to(m)
        m.save(file_name)
        print(f"🗺️ 위치 정보를 {file_name}에 저장했습니다.")
        return file_name
    else:
        print("⚠️ 유효한 위치 정보를 수신하지 못했습니다.")
        return None


def load_known_faces():
    known_enc, known_names = [], []
    for f in os.listdir(KNOWN_DIR):
        if f.lower().endswith((".jpg", ".jpeg", ".png")):
            img  = face_recognition.load_image_file(os.path.join(KNOWN_DIR, f))
            encs = face_recognition.face_encodings(img)
            if encs:
                known_enc.append(encs[0])
                known_names.append(os.path.splitext(f)[0])
    print(f"[DEBUG] known loaded: {len(known_enc)}")
    return known_enc, known_names


# ───── 메인 루프 ─────
while True:
    cmd = input(">> 시작하려면 [1] 입력: ")
    if cmd != '1':
        continue
    for f in os.listdir(UNKNOWN_DIR):
        os.remove(os.path.join(UNKNOWN_DIR, f))

    known_enc, known_names = load_known_faces()

    send(b"FACE")
    fail_count = 0

    while True:
        print("📷 얼굴 인식 중...")
        for _ in range(5):          # 버퍼에 쌓인 오래된 프레임 버리기
            video.grab()
        ok, frame = video.read()
        if not ok:
            continue

        rgb  = frame[:, :, ::-1]
        locs = face_recognition.face_locations(rgb)
        if not locs:
            print("얼굴 없음")
            time.sleep(1)
            continue

        ts   = time.strftime("%Y%m%d_%H%M%S")
        path = os.path.join(UNKNOWN_DIR, f"{ts}.jpg")
        cv2.imwrite(path, frame)

        encs = face_recognition.face_encodings(face_recognition.load_image_file(path))
        if not encs:
            os.remove(path)
            print("인코딩 실패")
            continue

        match = face_recognition.compare_faces(known_enc, encs[0], MATCH_TOLERANCE)

        if True in match:
            # ── 1단계 통과 → 2단계 비밀번호 ──
            print("✅ 사용자 인증 성공")
            send(b"REQ_PW")
            os.remove(path)
            fail_count = 0

            while True:
                pw = input("비밀번호 입력: ")
                if pw == PASSWORD:
                    send(b"OK")
                    time.sleep(1)
                    SER.flushInput()
                    input("잠그려면 1을 입력하고 Enter: ")
                    send(b"1")
                    break
                else:
                    send(b"FAIL"); time.sleep(1)
            break

        else:
            fail_count += 1
            print(f"❌ 사용자 인증 실패 ({fail_count}/{MAX_FAIL})")
            send(b"FAIL"); time.sleep(1)
            os.remove(path)

            if fail_count >= MAX_FAIL:
                # ── 침입자 판정: 사진 + 위치를 주인에게 공유 ──
                print("🚨 침입 시도 감지! 얼굴 사진 및 위치 기록")
                new_path = os.path.join(INVADER_DIR, f"invader_{ts}.jpg")
                cv2.imwrite(new_path, frame)

                lat, lon = get_gps_coordinates()
                gps_file = generate_map(lat, lon, ts)
                upload_to_drive([new_path, gps_file])
                break

    print("🔐 보드 응답 대기 중...")
    start = time.time()
    while time.time() - start < 5:
        if SER.in_waiting:
            resp = SER.readline().decode(errors='ignore').strip()
            print(f"[DEBUG] 수신: {resp}")
            if resp == "LOCKED":
                print("🔐 금고 잠금 완료")
                break
    else:
        print("⚠️ 보드로부터 'LOCKED' 응답 없음 (타임아웃)")
