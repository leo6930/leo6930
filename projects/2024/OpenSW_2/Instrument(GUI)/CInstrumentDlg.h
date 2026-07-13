#pragma once
#include "afxdialogex.h"
#include "Track.h"
#include "Scale.h"
#include "Instrument.h"
#include <string>
#include <afxwin.h>

// CInstrumentDlg 대화 상자

class CInstrumentDlg : public CDialogEx
{
private:
	Track track;
	DECLARE_DYNAMIC(CInstrumentDlg)

	std::thread trackThread; // 재생 스레드를 위한 멤버 변수 선언
	std::atomic<bool> stopFlag; // 정지를 위한 플래그 변수

	Instrument* piano;
	Instrument* drumInstrument;
	Instrument* guitarInstrument;
public:
	CInstrumentDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~CInstrumentDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_INSTRUMENT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditBpm();
	afx_msg void OnClickedButtonPlay();
	afx_msg void OnSelchangeComboScale();
	void AppendOutput(CString message);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnChangeEditStart();
	CWinThread* pTrackThread = nullptr;

	CString CInstrumentDlg::GetDlgItemTextAsCString(int nID) const;

	int GetComboSelection(int nID) const;
	void UpdateScaleOptions(CString startNote);
	afx_msg void OnClickedButtonStop();
};
