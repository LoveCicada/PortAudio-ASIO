#include "ui/MainDlg.h"

#include <sstream>

#include "pa_asio.h"

namespace {

CString Utf8ToCString(const std::string& utf8)
{
    if(utf8.empty())
        return CString();

    const int wideLen = MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
        nullptr, 0);
    if(wideLen <= 0)
        return CString();

    CString wide;
    LPWSTR buf = wide.GetBuffer(wideLen);
    MultiByteToWideChar(
        CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
        buf, wideLen);
    wide.ReleaseBuffer(wideLen);
    return wide;
}

std::wstring CStringToWide(const CString& text)
{
#ifdef UNICODE
    return std::wstring(text.GetString());
#else
    const int wideLen = MultiByteToWideChar(
        CP_ACP, 0, text, -1, nullptr, 0);
    if(wideLen <= 0)
        return std::wstring();

    std::wstring wide(static_cast<size_t>(wideLen), L'\0');
    MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], wideLen);
    if(!wide.empty() && wide.back() == L'\0')
        wide.pop_back();
    return wide;
#endif
}

} // namespace

CMainDlg::CMainDlg()
    : CDialogEx(IDD)
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_DEVICE, m_comboDevice);
    DDX_Control(pDX, IDC_COMBO_BUFFER, m_comboBuffer);
    DDX_Control(pDX, IDC_EDIT_CHANNEL, m_editChannel);
    DDX_Control(pDX, IDC_SPIN_CHANNEL, m_spinChannel);
    DDX_Control(pDX, IDC_CHECK_LOOP, m_checkLoop);
    DDX_Control(pDX, IDC_EDIT_FILE, m_editFile);
    DDX_Control(pDX, IDC_EDIT_LOG, m_editLog);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BTN_BROWSE, &CMainDlg::OnBnClickedBrowse)
    ON_BN_CLICKED(IDC_BTN_PLAY, &CMainDlg::OnBnClickedPlay)
    ON_BN_CLICKED(IDC_BTN_STOP, &CMainDlg::OnBnClickedStop)
    ON_BN_CLICKED(IDC_BTN_ASIO_PANEL, &CMainDlg::OnBnClickedAsioPanel)
    ON_CBN_SELCHANGE(IDC_COMBO_DEVICE, &CMainDlg::OnCbnSelchangeDevice)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_CHANNEL, &CMainDlg::OnDeltaposSpinChannel)
    ON_MESSAGE(WM_PLAYBACK_COMPLETE, &CMainDlg::OnPlaybackComplete)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_spinChannel.SetBuddy(&m_editChannel);
    m_spinChannel.SetRange(0, 0);
    m_editChannel.SetWindowText(L"0");

    std::string error;
    if(!m_devices.Initialize(&error))
    {
        AppendLog(CString(L"PortAudio init failed: ") + Utf8ToCString(error));
        SetControlsEnabled(false);
        return TRUE;
    }

    AppendLog(L"PortAudio initialized (ASIO only).");
    RefreshDeviceList();
    SetControlsEnabled(false);
    return TRUE;
}

void CMainDlg::OnDestroy()
{
    m_engine.Stop();
    m_devices.Shutdown();
    CDialogEx::OnDestroy();
}

void CMainDlg::AppendLog(const CString& line)
{
    CString existing;
    m_editLog.GetWindowText(existing);
    if(!existing.IsEmpty())
        existing += L"\r\n";
    existing += line;
    m_editLog.SetWindowText(existing);
    m_editLog.LineScroll(m_editLog.GetLineCount());
}

void CMainDlg::SetControlsEnabled(bool playbackActive)
{
    const BOOL enableUi = playbackActive ? FALSE : TRUE;
    m_comboDevice.EnableWindow(enableUi);
    m_comboBuffer.EnableWindow(enableUi);
    m_editChannel.EnableWindow(enableUi);
    m_spinChannel.EnableWindow(enableUi);
    m_checkLoop.EnableWindow(enableUi);
    m_editFile.EnableWindow(enableUi);
    GetDlgItem(IDC_BTN_BROWSE)->EnableWindow(enableUi);
    GetDlgItem(IDC_BTN_PLAY)->EnableWindow(enableUi);
    GetDlgItem(IDC_BTN_ASIO_PANEL)->EnableWindow(enableUi);
    GetDlgItem(IDC_BTN_STOP)->EnableWindow(playbackActive);
}

void CMainDlg::RefreshDeviceList()
{
    m_comboDevice.ResetContent();

    const std::vector<AsioDeviceInfo>& list = m_devices.devices();
    for(size_t i = 0; i < list.size(); ++i)
    {
        const CString item = Utf8ToCString(list[i].name);
        const int idx = m_comboDevice.AddString(item);
        if(idx >= 0)
            m_comboDevice.SetItemData(idx, static_cast<DWORD_PTR>(list[i].deviceIndex));
    }

    const int def = m_devices.defaultDeviceListIndex();
    if(def >= 0 && def < m_comboDevice.GetCount())
        m_comboDevice.SetCurSel(def);

    RefreshBufferList();
    UpdateChannelSpinRange();
}

void CMainDlg::RefreshBufferList()
{
    m_comboBuffer.ResetContent();

    PaDeviceIndex device = paNoDevice;
    int maxChannels = 0;
    if(!GetSelectedDevice(&device, &maxChannels))
        return;

    std::vector<AsioBufferSizeOption> options;
    std::string error;
    if(!m_devices.GetBufferSizes(device, &options, &error))
    {
        AppendLog(CString(L"Buffer sizes: ") + Utf8ToCString(error));
        return;
    }

    int preferredIndex = 0;
    for(size_t i = 0; i < options.size(); ++i)
    {
        CString label;
        label.Format(L"%ld", options[i].frames);
        if(options[i].isPreferred)
            label += L" (preferred)";

        const int idx = m_comboBuffer.AddString(label);
        if(idx >= 0)
        {
            m_comboBuffer.SetItemData(idx, static_cast<DWORD_PTR>(options[i].frames));
            if(options[i].isPreferred)
                preferredIndex = idx;
        }
    }

    if(m_comboBuffer.GetCount() > 0)
        m_comboBuffer.SetCurSel(preferredIndex);
}

void CMainDlg::UpdateChannelSpinRange()
{
    PaDeviceIndex device = paNoDevice;
    int maxChannels = 0;
    if(!GetSelectedDevice(&device, &maxChannels))
        return;

    const int maxChannel = (maxChannels > 0) ? (maxChannels - 1) : 0;
    m_spinChannel.SetRange(0, maxChannel);

    int current = GetOutputChannel();
    if(current < 0)
        current = 0;
    if(current > maxChannel)
        current = maxChannel;

    CString text;
    text.Format(L"%d", current);
    m_editChannel.SetWindowText(text);
}

bool CMainDlg::GetSelectedDevice(PaDeviceIndex* outDevice, int* outMaxChannels)
{
    if(!outDevice)
        return false;

    const int sel = m_comboDevice.GetCurSel();
    if(sel < 0)
        return false;

    *outDevice = static_cast<PaDeviceIndex>(m_comboDevice.GetItemData(sel));

    if(outMaxChannels)
    {
        *outMaxChannels = 0;
        const std::vector<AsioDeviceInfo>& list = m_devices.devices();
        for(size_t i = 0; i < list.size(); ++i)
        {
            if(list[i].deviceIndex == *outDevice)
            {
                *outMaxChannels = list[i].maxOutputChannels;
                break;
            }
        }
    }
    return true;
}

long CMainDlg::GetSelectedBufferFrames() const
{
    const int sel = m_comboBuffer.GetCurSel();
    if(sel < 0)
        return 512;

    return static_cast<long>(m_comboBuffer.GetItemData(sel));
}

int CMainDlg::GetOutputChannel() const
{
    CString text;
    m_editChannel.GetWindowText(text);
    return _ttoi(text);
}

void CMainDlg::OnCbnSelchangeDevice()
{
    RefreshBufferList();
    UpdateChannelSpinRange();
}

void CMainDlg::OnDeltaposSpinChannel(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
    UpdateChannelSpinRange();
    if(pResult)
        *pResult = 0;
}

void CMainDlg::OnBnClickedBrowse()
{
    CFileDialog dlg(
        TRUE,
        L"wav",
        nullptr,
        OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        L"WAV Files (*.wav)|*.wav|All Files (*.*)|*.*||",
        this);

    if(dlg.DoModal() == IDOK)
        m_editFile.SetWindowText(dlg.GetPathName());
}

void CMainDlg::OnBnClickedPlay()
{
    CString path;
    m_editFile.GetWindowText(path);
    path.Trim();
    if(path.IsEmpty())
    {
        AppendLog(L"Select a WAV file first.");
        return;
    }

    std::string error;
    if(!m_wav.Load(CStringToWide(path), &error))
    {
        AppendLog(CString(L"WAV load failed: ") + Utf8ToCString(error));
        return;
    }

    PaDeviceIndex device = paNoDevice;
    int maxChannels = 0;
    if(!GetSelectedDevice(&device, &maxChannels))
    {
        AppendLog(L"No ASIO device selected.");
        return;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(device);
    if(devInfo &&
       static_cast<int>(devInfo->defaultSampleRate) !=
           static_cast<int>(m_wav.sampleRate()))
    {
        CString warn;
        warn.Format(
            L"Warning: WAV rate %.0f Hz, device default %.0f Hz.",
            m_wav.sampleRate(),
            devInfo->defaultSampleRate);
        AppendLog(warn);
    }

    AudioEngineConfig config;
    config.deviceIndex = device;
    config.outputChannel = GetOutputChannel();
    config.framesPerBuffer = GetSelectedBufferFrames();
    config.loopPlayback = (m_checkLoop.GetCheck() == BST_CHECKED);

    CMainDlg* self = this;
    if(!m_engine.Start(
           m_wav,
           config,
           [self]() {
               if(self && ::IsWindow(self->m_hWnd))
                   self->PostMessage(WM_PLAYBACK_COMPLETE, 0, 0);
           },
           &error))
    {
        AppendLog(CString(L"Playback start failed: ") + Utf8ToCString(error));
        return;
    }

    CString info;
    info.Format(
        L"Playing: %s | %d ch | %.0f Hz | buffer %ld | out ch %d | loop %s",
        static_cast<LPCTSTR>(path),
        m_wav.channelCount(),
        m_wav.sampleRate(),
        config.framesPerBuffer,
        config.outputChannel,
        config.loopPlayback ? L"on" : L"off");
    AppendLog(info);

    SetControlsEnabled(true);
}

void CMainDlg::OnBnClickedStop()
{
    m_engine.Stop();
    AppendLog(L"Playback stopped.");
    SetControlsEnabled(false);
}

LRESULT CMainDlg::OnPlaybackComplete(WPARAM, LPARAM)
{
    m_engine.Stop();
    AppendLog(L"Playback finished.");
    SetControlsEnabled(false);
    return 0;
}

void CMainDlg::OnBnClickedAsioPanel()
{
    PaDeviceIndex device = paNoDevice;
    if(!GetSelectedDevice(&device, nullptr))
    {
        AppendLog(L"No ASIO device selected.");
        return;
    }

    const PaError err = PaAsio_ShowControlPanel(device, m_hWnd);
    if(err != paNoError)
        AppendLog(CString(L"ASIO control panel: ") + Utf8ToCString(Pa_GetErrorText(err)));
    else
        AppendLog(L"ASIO control panel closed.");
}
