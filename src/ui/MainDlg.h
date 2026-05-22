#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <afxwin.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxdialogex.h>

#include "audio/AudioEngine.h"
#include "audio/DeviceEnumerator.h"
#include "audio/WavFileReader.h"

#include "resource.h"

class CMainDlg : public CDialogEx
{
public:
    enum { IDD = IDD_MAIN_DIALOG };

    CMainDlg();

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    afx_msg void OnDestroy();

    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedPlay();
    afx_msg void OnBnClickedStop();
    afx_msg void OnBnClickedAsioPanel();
    afx_msg void OnCbnSelchangeDevice();
    afx_msg void OnDeltaposSpinChannel(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnPlaybackComplete(WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()

private:
    void AppendLog(const CString& line);
    void SetControlsEnabled(bool playbackActive);
    void RefreshDeviceList();
    void RefreshBufferList();
    void UpdateChannelSpinRange();

    bool GetSelectedDevice(PaDeviceIndex* outDevice, int* outMaxChannels);
    long GetSelectedBufferFrames() const;
    int GetOutputChannel() const;

    DeviceEnumerator m_devices;
    AudioEngine m_engine;
    WavFileReader m_wav;

    CComboBox m_comboDevice;
    CComboBox m_comboBuffer;
    CEdit m_editChannel;
    CSpinButtonCtrl m_spinChannel;
    CButton m_checkLoop;
    CEdit m_editFile;
    CEdit m_editLog;
};
