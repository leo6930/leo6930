#include "pch.h"
#include "Instrument(GUI).h"
#include "CInstrumentDlg.h"
#include "Track.h"
#include "Scale.h"
#include "stdexcept"
#include <algorithm>
#include <thread>   
#include <chrono>   
#include <iomanip>

using namespace std;

// CInstrumentDlg 클래스 구현

IMPLEMENT_DYNAMIC(CInstrumentDlg, CDialogEx)

CInstrumentDlg::CInstrumentDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIALOG_INSTRUMENT, pParent), track(120, "C",4) 
{
    piano = new Instrument(""); 
    if (piano == nullptr) {
        AfxMessageBox(_T("piano 객체 생성에 실패했습니다."));
    }

    drumInstrument = new Instrument(""); 
    if (drumInstrument == nullptr) {
        AfxMessageBox(_T("drum 객체 생성에 실패했습니다."));
    }

    guitarInstrument = new Instrument(""); 
    if (guitarInstrument == nullptr) {
        AfxMessageBox(_T("guitar 객체 생성에 실패했습니다."));
    }

}

CInstrumentDlg::~CInstrumentDlg() {
    if (piano) {
        delete piano; 
        piano = nullptr;
    }
    if (drumInstrument) {
        delete drumInstrument;
        drumInstrument = nullptr;
    }

    if (guitarInstrument) {
        delete guitarInstrument; // 기타 객체 해제
        guitarInstrument = nullptr;
    }
}

void CInstrumentDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CInstrumentDlg, CDialogEx)
    ON_EN_CHANGE(IDC_EDIT_BPM, &CInstrumentDlg::OnChangeEditBpm)
    ON_BN_CLICKED(IDC_BUTTON_PLAY, &CInstrumentDlg::OnClickedButtonPlay)
    ON_CBN_SELCHANGE(IDC_COMBO_SCALE, &CInstrumentDlg::OnSelchangeComboScale)
    ON_WM_TIMER()
    ON_EN_CHANGE(IDC_EDIT_START, &CInstrumentDlg::OnChangeEditStart)
    ON_BN_CLICKED(IDC_BUTTON_STOP, &CInstrumentDlg::OnClickedButtonStop)
END_MESSAGE_MAP()

BOOL CInstrumentDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    



    SetDlgItemText(IDC_EDIT_START, _T("C"));
    SetDlgItemText(IDC_EDIT_BPM, _T("120"));

   
    CComboBox* pScaleCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCALE);
    pScaleCombo->AddString(_T("Major"));
    pScaleCombo->AddString(_T("Minor"));
    pScaleCombo->AddString(_T("Pentatonic Major"));
    pScaleCombo->AddString(_T("Pentatonic Minor"));
    pScaleCombo->SetCurSel(0);

    CComboBox* pBeatCombo = (CComboBox*)GetDlgItem(IDC_COMBO_BEAT);
    pBeatCombo->AddString(_T("4/4"));
    pBeatCombo->AddString(_T("3/4"));
    pBeatCombo->SetCurSel(1);

    
    track.setOutputFunction([this](const CString& message) {
        AppendOutput(message);
        });

    
    return TRUE;
}

void CInstrumentDlg::OnChangeEditBpm()
{
    CString bpmStr;
    GetDlgItemText(IDC_EDIT_BPM, bpmStr);

    int bpm = _ttoi(bpmStr);
    if (bpm > 0 && bpm <= 300) {
        track.setBPM(bpm); 
    }
    else {
        AfxMessageBox(_T("BPM 값은 1에서 300 사이여야 합니다."));
    }
}


void CInstrumentDlg::OnClickedButtonPlay() {
    AppendOutput(_T("재생 버튼 클릭됨: 음악 재생을 시작합니다.\r\n"));

    try {
        
        CString scaleType;
        CComboBox* pScaleCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCALE);
        int selIndex = pScaleCombo->GetCurSel();
        pScaleCombo->GetLBText(selIndex, scaleType);

        string scaleTypeStr = CT2A(scaleType.GetString());
        transform(scaleTypeStr.begin(), scaleTypeStr.end(), scaleTypeStr.begin(), ::tolower);

        CString startNote;
        GetDlgItemText(IDC_EDIT_START, startNote);
        string startNoteStr = CT2A(startNote.GetString());

        CString timeSignatureStr;
        CComboBox* pBeatCombo = (CComboBox*)GetDlgItem(IDC_COMBO_BEAT);
        int beatSelIndex = pBeatCombo->GetCurSel();
        pBeatCombo->GetLBText(beatSelIndex, timeSignatureStr);

        int timeSignature = (timeSignatureStr == _T("4/4")) ? 4 : 3;

        
        Scale scale(startNoteStr, scaleTypeStr);
        track.setScale(scale);
        track.setTimeSignature(timeSignature);

        track.generateDrumPattern();
        track.generateMainTrackNotes(scale);
        track.generateBackingTrack(scale, selIndex);

        
        AppendOutput(_T("메인 기타 트랙 단음 진행: "));
        for (const auto& note : track.getMainTrackNotes()) {
            CString outputNote;
            outputNote.Format(_T("%s "), CString(note.c_str()));
            AppendOutput(outputNote);
        }
        AppendOutput(_T("\r\n"));

       
        Instrument guitarInstrument("");

        
        drumInstrument->setFilePath("sounds/drum/Kick.wav");

       
        track.playSynchronizedTrack(*piano, *drumInstrument, guitarInstrument, scale);
        AppendOutput(_T("디버깅: 피아노, 드럼, 기타 동기화 재생 시작.\r\n"));
    }
    catch (const std::exception& e) {
        AfxMessageBox(_T("오류 발생: ") + CString(e.what()));
    }
}











 







CString CInstrumentDlg::GetDlgItemTextAsCString(int nID) const {
    CString text;
    GetDlgItemText(nID, text);
    return text;
}


void CInstrumentDlg::OnSelchangeComboScale() {
    CString scaleType;
    CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCALE);
    int selIndex = pCombo->GetCurSel();
    pCombo->GetLBText(selIndex, scaleType);

    string scaleTypeStr = CT2A(scaleType.GetString());

    
    transform(scaleTypeStr.begin(), scaleTypeStr.end(), scaleTypeStr.begin(), ::tolower);

    try {
        Scale scale("C", scaleTypeStr);
        track.setScale(scale);
        track.generateBackingTrack(scale,selIndex);
        track.generateMainTrackNotes(scale);
        track.generateDrumPattern();
    }
    catch (const std::invalid_argument& e) {
        CString errorMsg;
        errorMsg.Format(_T("트랙 데이터를 생성하는 중 오류가 발생했습니다: %s"), CString(e.what()));
        AfxMessageBox(errorMsg);
    }
}

void CInstrumentDlg::AppendOutput(CString message) {
    CEdit* pEdit = (CEdit*)GetDlgItem(IDC_OUTPUT);
    if (!pEdit) return;

    CString existingText;
    pEdit->GetWindowText(existingText);

    existingText += message + _T("\r\n");
    pEdit->SetWindowText(existingText);

   
    pEdit->LineScroll(pEdit->GetLineCount() - 1);
}


void CInstrumentDlg::OnTimer(UINT_PTR nIDEvent) {
    if (nIDEvent == 1) {
        static int beatIndex = 0;

       
        if (track.getMainTrackNotes().empty() || track.getDrumPatternAsString().empty()) {
            AfxMessageBox(_T("트랙 데이터가 유효하지 않습니다."));
            KillTimer(1);
            return;
        }

        if (beatIndex < track.getMainTrackNotes().size()) {
            CString output;
            output.Format(_T("메인 트랙: %s, 드럼: %s"),
                CString(track.getMainTrackNotes()[beatIndex].c_str()),
                CString(track.getDrumPatternAsString().c_str()));
            AppendOutput(output);
            beatIndex++;
        }
        else {
            KillTimer(1); 
        }
    }

    CDialogEx::OnTimer(nIDEvent);
}

void CInstrumentDlg::OnChangeEditStart()
{
    // TODO:  RICHEDIT 컨트롤인 경우, 이 컨트롤은
    // CDialogEx::OnInitDialog() 함수를 재지정 
    //하고 마스크에 OR 연산하여 설정된 ENM_CHANGE 플래그를 지정하여 CRichEditCtrl().SetEventMask()를 호출하지 않으면
    // 이 알림 메시지를 보내지 않습니다.

    // TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
    CString startNote;
    GetDlgItemText(IDC_EDIT_START, startNote);

    if (startNote.IsEmpty()) {
        return; // 빈 입력일 경우 처리하지 않음
    }

    startNote.MakeUpper(); // 입력값을 대문자로 변환

    // 유효한 음인지 확인
    std::vector<CString> validNotes = {
        _T("C"), _T("C#"), _T("D"), _T("D#"), _T("E"),
        _T("F"), _T("F#"), _T("G"), _T("G#"), _T("A"),
        _T("A#"), _T("B")
    };

    if (std::find(validNotes.begin(), validNotes.end(), startNote) == validNotes.end()) {
        AfxMessageBox(_T("유효하지 않은 음계입니다. 다시 입력해주세요."));
        SetDlgItemText(IDC_EDIT_START, _T("")); 
        return;
    }

    
    string startNoteStr = CT2A(startNote.GetString());
    track.setScale(Scale(startNoteStr, "major"));
}

void CInstrumentDlg::UpdateScaleOptions(CString startNote)
{
    
    CComboBox* pScaleCombo = (CComboBox*)GetDlgItem(IDC_COMBO_SCALE);
    pScaleCombo->ResetContent(); 

    
    pScaleCombo->AddString(_T("Major"));
    pScaleCombo->AddString(_T("Minor"));
    pScaleCombo->AddString(_T("Pentatonic Major"));
    pScaleCombo->AddString(_T("Pentatonic Minor"));

    pScaleCombo->SetCurSel(0); 
}


void CInstrumentDlg::OnClickedButtonStop()
{
    // TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
    AppendOutput(_T("정지 버튼 클릭됨: 모든 재생을 중단합니다.\r\n"));

    track.stopAllPlayback(); 

}

