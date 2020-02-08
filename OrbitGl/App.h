//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------
#pragma once

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>

#include "../OrbitCore/CoreApp.h"
#include "../OrbitCore/CrashHandler.h"
#include "../OrbitCore/Message.h"
#include "DataViewTypes.h"
#include "Threading.h"

struct CallStack;
struct ContextSwitch;
class Process;
class DataView;
class ProcessesDataView;
class ModulesDataView;
class FunctionsDataView;
class LiveFunctionsDataView;
class CallStackDataView;
class TypesDataView;
class GlobalsDataView;
class SessionsDataView;
class CaptureWindow;
class LogDataView;
class RuleEditor;

//-----------------------------------------------------------------------------
class OrbitApp : public CoreApp {
 public:
  OrbitApp();
  virtual ~OrbitApp();

  static bool Init();
  void PostInit();
  static int OnExit();
  static void MainTick();
  void CheckLicense();
  void SetLicense(const std::wstring& a_License);
  std::string GetVersion();
  void CheckForUpdate();
  void CheckDebugger();

  std::wstring GetCaptureFileName();
  std::wstring GetSessionFileName();
  std::wstring GetSaveFile(const std::wstring& a_Extension);
  void SetClipboard(const std::wstring& a_Text);
  void OnSaveSession(const std::wstring a_FileName);
  void OnLoadSession(const std::wstring a_FileName);
  void OnSaveCapture(const std::wstring a_FileName);
  void OnLoadCapture(const std::wstring a_FileName);
  void OnOpenPdb(const std::wstring a_FileName);
  void OnLaunchProcess(const std::wstring a_ProcessName,
                       const std::wstring a_WorkingDir,
                       const std::wstring a_Args);
  void Inject(const std::wstring a_FileName);
  virtual void StartCapture();
  virtual void StopCapture();
  virtual void StartRemoteCaptureBufferingThread();
  virtual void StopRemoteCaptureBufferingThread();
  void ToggleCapture();
  void OnDisconnect();
  void OnPdbLoaded();
  void LogMsg(const std::wstring& a_Msg) override;
  void SetCallStack(std::shared_ptr<CallStack> a_CallStack);
  void LoadFileMapping();
  void LoadSymbolsFile();
  void LoadSystrace(const std::string& a_FileName);
  void AppendSystrace(const std::string& a_FileName, uint64_t a_TimeOffset);
  void ListSessions();
  void SetRemoteProcess(std::shared_ptr<Process> a_Process);
  void SendRemoteProcess(uint32_t a_PID) override;
  void RefreshCaptureView() override;
  void RequestRemoteModules(const std::vector<std::string> a_Modules);
  void AddWatchedVariable(Variable* a_Variable);
  void UpdateVariable(Variable* a_Variable) override;
  void ClearWatchedVariables();
  void RefreshWatch();
  virtual void Disassemble(const std::string& a_FunctionName,
                           DWORD64 a_VirtualAddress, const char* a_MachineCode,
                           size_t a_Size);
  virtual void ProcessTimer(const Timer& a_Timer,
                            const std::string& a_FunctionName);
  virtual void ProcessSamplingCallStack(LinuxCallstackEvent& a_CallStack);
  virtual void ProcessHashedSamplingCallStack(CallstackEvent& a_CallStack);
  virtual void ProcessCallStack(CallStack& a_CallStack);
  virtual void ProcessContextSwitch(const ContextSwitch& a_ContextSwitch);
  virtual void AddSymbol(uint64_t a_Address, const std::string& a_Module,
                         const std::string& a_Name);
  void ProcessBufferedCaptureData();

  int* GetScreenRes() { return m_ScreenRes; }

  void RegisterProcessesDataView(
      std::shared_ptr<ProcessesDataView> a_Processes);
  void RegisterModulesDataView(std::shared_ptr<ModulesDataView> a_Modules);
  void RegisterFunctionsDataView(
      std::shared_ptr<FunctionsDataView> a_Functions);
  void RegisterLiveFunctionsDataView(
      std::shared_ptr<LiveFunctionsDataView> a_Functions);
  void RegisterCallStackDataView(
      std::shared_ptr<CallStackDataView> a_Callstack);
  void RegisterTypesDataView(std::shared_ptr<TypesDataView> a_Types);
  void RegisterGlobalsDataView(std::shared_ptr<GlobalsDataView> a_Globals);
  void RegisterSessionsDataView(std::shared_ptr<SessionsDataView> a_Sessions);
  void RegisterCaptureWindow(std::shared_ptr<CaptureWindow> a_Capture);
  void RegisterOutputLog(std::shared_ptr<LogDataView> a_Log);
  void RegisterRuleEditor(std::shared_ptr<RuleEditor> a_RuleEditor);

  void Unregister(std::shared_ptr<DataView> a_DataView);
  bool SelectProcess(const std::wstring& a_Process);
  bool SelectProcess(uint32_t a_ProcessID);
  bool Inject(unsigned long a_ProcessId);
  static void AddSamplingReport(
      std::shared_ptr<class SamplingProfiler>& a_SamplingProfiler);
  static void AddSelectionReport(
      std::shared_ptr<SamplingProfiler>& a_SamplingProfiler);
  void GoToCode(DWORD64 a_Address);
  void GoToCallstack();
  void GoToCapture();
  void GetDisassembly(DWORD64 a_Address, DWORD a_NumBytesBelow,
                      DWORD a_NumBytes);

  // Callbacks
  typedef std::function<void(DataViewType a_Type)> RefreshCallback;
  void AddRefreshCallback(RefreshCallback a_Callback) {
    m_RefreshCallbacks.push_back(a_Callback);
  }
  typedef std::function<void(std::shared_ptr<class SamplingReport>)>
      SamplingReportCallback;
  void AddSamplingReoprtCallback(SamplingReportCallback a_Callback) {
    m_SamplingReportsCallbacks.push_back(a_Callback);
  }
  void AddSelectionReportCallback(SamplingReportCallback a_Callback) {
    m_SelectionReportCallbacks.push_back(a_Callback);
  }
  typedef std::function<void(Variable* a_Variable)> WatchCallback;
  void AddWatchCallback(WatchCallback a_Callback) {
    m_AddToWatchCallbacks.push_back(a_Callback);
  }
  typedef std::function<void(const std::wstring& a_Extension,
                             std::wstring& o_Variable)>
      SaveFileCallback;
  void SetSaveFileCallback(SaveFileCallback a_Callback) {
    m_SaveFileCallback = a_Callback;
  }
  void AddUpdateWatchCallback(WatchCallback a_Callback) {
    m_UpdateWatchCallbacks.push_back(a_Callback);
  }
  void FireRefreshCallbacks(DataViewType a_Type = DataViewType::ALL);
  void Refresh(DataViewType a_Type = DataViewType::ALL) {
    FireRefreshCallbacks(a_Type);
  }
  void AddUiMessageCallback(
      std::function<void(const std::wstring&)> a_Callback);
  typedef std::function<std::wstring(const std::wstring& a_Caption,
                                     const std::wstring& a_Dir,
                                     const std::wstring& a_Filter)>
      FindFileCallback;
  void SetFindFileCallback(FindFileCallback a_Callback) {
    m_FindFileCallback = a_Callback;
  }
  std::wstring FindFile(const std::wstring& a_Caption,
                        const std::wstring& a_Dir,
                        const std::wstring& a_Filter);
  typedef std::function<void(const std::wstring&)> ClipboardCallback;
  void SetClipboardCallback(ClipboardCallback a_Callback) {
    m_ClipboardCallback = a_Callback;
  }

  void SetCommandLineArguments(const std::vector<std::string>& a_Args);
  const std::vector<std::string>& GetCommandLineArguments() {
    return m_Arguments;
  }

  void SendToUiAsync(const std::wstring& a_Msg) override;
  void SendToUiNow(const std::wstring& a_Msg) override;
  void NeedsRedraw();

  const std::map<std::wstring, std::wstring>& GetFileMapping() {
    return m_FileMapping;
  }

  void EnqueueModuleToLoad(const std::shared_ptr<struct Module>& a_Module);
  void LoadModules();
  void LoadRemoteModules();
  bool LoadRemoteModuleLocally(std::shared_ptr<struct Module>& a_Module);
  bool IsLoading();
  void SetTrackContextSwitches(bool a_Value);
  bool GetTrackContextSwitches();

  void EnableUnrealSupport(bool a_Value);
  virtual bool GetUnrealSupportEnabled() override;

  void EnableSampling(bool a_Value);
  virtual bool GetSamplingEnabled() override;

  void EnableUnsafeHooking(bool a_Value);
  virtual bool GetUnsafeHookingEnabled() override;

  void EnableOutputDebugString(bool a_Value);
  virtual bool GetOutputDebugStringEnabled() override;

  void RequestThaw() { m_NeedsThawing = true; }
  void OnMiniDump(const Message& a_Message);
  void OnRemoteProcess(const Message& a_Message);
  void OnRemoteProcessList(const Message& a_Message);
  void OnRemoteModuleDebugInfo(const Message& a_Message);
  void LaunchRuleEditor(class Function* a_Function);
  void SetHeadless(bool a_Headless) { m_Headless = a_Headless; }
  bool GetHeadless() const { return m_Headless; }
  void SetIsRemote(bool a_IsRemote) { m_IsRemote = a_IsRemote; }
  bool IsRemote() const { return m_IsRemote; }
  bool HasTcpServer() const { return !IsRemote(); }

  std::shared_ptr<RuleEditor> GetRuleEditor() { return m_RuleEditor; }
  virtual const std::unordered_map<DWORD64, std::shared_ptr<class Rule>>*
  GetRules();

 private:
  std::vector<std::string> m_Arguments;
  std::vector<RefreshCallback> m_RefreshCallbacks;
  std::vector<WatchCallback> m_AddToWatchCallbacks;
  std::vector<WatchCallback> m_UpdateWatchCallbacks;
  std::vector<SamplingReportCallback> m_SamplingReportsCallbacks;
  std::vector<SamplingReportCallback> m_SelectionReportCallbacks;
  std::vector<std::shared_ptr<DataView>> m_Panels;
  FindFileCallback m_FindFileCallback;
  SaveFileCallback m_SaveFileCallback;
  ClipboardCallback m_ClipboardCallback;
  bool m_Headless = false;
  bool m_IsRemote = false;

  std::shared_ptr<ProcessesDataView> m_ProcessesDataView = nullptr;
  std::shared_ptr<ModulesDataView> m_ModulesDataView = nullptr;
  std::shared_ptr<FunctionsDataView> m_FunctionsDataView = nullptr;
  std::shared_ptr<LiveFunctionsDataView> m_LiveFunctionsDataView = nullptr;
  std::shared_ptr<CallStackDataView> m_CallStackDataView = nullptr;
  std::shared_ptr<TypesDataView> m_TypesDataView = nullptr;
  std::shared_ptr<GlobalsDataView> m_GlobalsDataView = nullptr;
  std::shared_ptr<SessionsDataView> m_SessionsDataView = nullptr;
  std::shared_ptr<CaptureWindow> m_CaptureWindow = nullptr;
  std::shared_ptr<LogDataView> m_Log = nullptr;
  std::shared_ptr<RuleEditor> m_RuleEditor = nullptr;
  int m_ScreenRes[2];
  bool m_HasPromptedForUpdate = false;
  bool m_NeedsThawing = false;
  bool m_UnrealEnabled = false;

  std::vector<std::shared_ptr<class SamplingReport>> m_SamplingReports;
  std::map<std::wstring, std::wstring> m_FileMapping;
  std::vector<std::string> m_SymbolDirectories;
  std::function<void(const std::wstring&)> m_UiCallback;

  // buffering data to send large messages instead of small ones:
  std::shared_ptr<std::thread> m_MessageBufferThread = nullptr;
  std::vector<ContextSwitch> m_ContextSwitchBuffer;
  Mutex m_ContextSwitchMutex;
  std::vector<Timer> m_TimerBuffer;
  Mutex m_TimerMutex;
  std::vector<LinuxCallstackEvent> m_SamplingCallstackBuffer;
  Mutex m_SamplingCallstackMutex;
  std::vector<CallstackEvent> m_HashedSamplingCallstackBuffer;
  Mutex m_HashedSamplingCallstackMutex;

  std::wstring m_User;
  std::wstring m_License;

  std::queue<std::shared_ptr<struct Module>> m_ModulesToLoad;
  std::vector<std::string> m_PostInitArguments;

  class EventTracer* m_EventTracer = nullptr;
  class Debugger* m_Debugger = nullptr;
  int m_NumTicks = 0;
#ifdef _WIN32
  CrashHandler m_CrashHandler;
#else
  std::shared_ptr<class BpfTrace> m_BpfTrace;
#endif
};

//-----------------------------------------------------------------------------
extern class OrbitApp* GOrbitApp;
