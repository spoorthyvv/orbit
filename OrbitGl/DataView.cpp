//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------

#include "DataView.h"

#include <fstream>

#include "App.h"
#include "CallStackDataView.h"
#include "Core.h"
#include "DataView.h"
#include "FunctionDataView.h"
#include "GlobalDataView.h"
#include "LiveFunctionDataView.h"
#include "Log.h"
#include "LogDataView.h"
#include "ModuleDataView.h"
#include "OrbitType.h"
#include "Params.h"
#include "Pdb.h"
#include "ProcessDataView.h"
#include "SamplingReportDataView.h"
#include "SessionsDataView.h"
#include "TypeDataView.h"

//-----------------------------------------------------------------------------
DataView::~DataView() {}

//-----------------------------------------------------------------------------
std::shared_ptr<DataView> DataView::Create(DataViewType a_Type) {
  switch (a_Type) {
    case DataViewType::FUNCTIONS: {
      auto dataView = std::make_shared<FunctionsDataView>();
      GOrbitApp->RegisterFunctionsDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::TYPES: {
      auto dataView = std::make_shared<TypesDataView>();
      GOrbitApp->RegisterTypesDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::LIVEFUNCTIONS: {
      auto dataView = std::make_shared<LiveFunctionsDataView>();
      GOrbitApp->RegisterLiveFunctionsDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::CALLSTACK: {
      auto dataView = std::make_shared<CallStackDataView>();
      GOrbitApp->RegisterCallStackDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::GLOBALS: {
      auto dataView = std::make_shared<GlobalsDataView>();
      GOrbitApp->RegisterGlobalsDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::MODULES: {
      auto dataView = std::make_shared<ModulesDataView>();
      GOrbitApp->RegisterModulesDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::SAMPLING: {
      auto dataView = std::make_shared<SamplingReportDataView>();
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::PROCESSES: {
      auto dataView = std::make_shared<ProcessesDataView>();
      GOrbitApp->RegisterProcessesDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::SESSIONS: {
      auto dataView = std::make_shared<SessionsDataView>();
      GOrbitApp->RegisterSessionsDataView(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    case DataViewType::LOG: {
      auto dataView = std::make_shared<LogDataView>();
      GOrbitApp->RegisterOutputLog(dataView);
      return std::static_pointer_cast<DataView>(dataView);
    }
    default:
      break;
  }

  return nullptr;

  /*GOrbitApp->RegisterCallStackDataView(this);
  GOrbitApp->RegisterCaptureWindow(this);
  GOrbitApp->RegisterFunctionsDataView(this);
  GOrbitApp->RegisterGlobalsDataView(this);
  GOrbitApp->RegisterOutputLog(this);
  GOrbitApp->RegisterProcessesDataView(this);
  GOrbitApp->RegisterRuleEditor(this);
  GOrbitApp->RegisterSessionsDataView(this);
  GOrbitApp->RegisterTypesDataView(this);*/
}

//-----------------------------------------------------------------------------
const std::vector<std::wstring>& DataView::GetColumnHeaders() {
  static std::vector<std::wstring> columns = {L"Invalid Header"};
  return columns;
}

//-----------------------------------------------------------------------------
const std::vector<float>& DataView::GetColumnHeadersRatios() {
  static std::vector<float> empty;
  return empty;
}

//-----------------------------------------------------------------------------
const std::wstring DV_COPY_SELECTION = L"Copy Selection";
const std::wstring DV_EXPORT_TO_CSV = L"Export to CSV";

//-----------------------------------------------------------------------------
std::vector<std::wstring> DataView::GetContextMenu(int a_Index) {
  static std::vector<std::wstring> menu = {DV_COPY_SELECTION, DV_EXPORT_TO_CSV};
  return menu;
}

//-----------------------------------------------------------------------------
void DataView::OnContextMenu(const std::wstring& a_Action, int a_MenuIndex,
                             std::vector<int>& a_ItemIndices) {
  UNUSED(a_MenuIndex);

  if (a_Action == DV_EXPORT_TO_CSV) {
    ExportCSV(GOrbitApp->GetSaveFile(L".csv"));
  } else if (a_Action == DV_COPY_SELECTION) {
    CopySelection(a_ItemIndices);
  }
}

//-----------------------------------------------------------------------------
void DataView::ExportCSV(const std::wstring& a_FileName) {
  std::ofstream out(ws2s(a_FileName));
  if (out.fail()) return;

  const std::vector<std::wstring>& headers = GetColumnHeaders();

  for (size_t i = 0; i < headers.size(); ++i) {
    out << ws2s(headers[i]);
    if (i < headers.size() - 1) out << ", ";
  }

  out << "\n";

  size_t numColumns = headers.size();
  size_t numElements = GetNumElements();
  for (size_t i = 0; i < numElements; ++i) {
    for (size_t j = 0; j < numColumns; ++j) {
      out << ws2s(GetValue((int)i, (int)j));
      if (j < numColumns - 1) out << ", ";
    }

    out << "\n";
  }

  out.close();
}

//-----------------------------------------------------------------------------
void DataView::CopySelection(std::vector<int>& selection) {
  std::wstring clipboard;
  const std::vector<std::wstring>& headers = GetColumnHeaders();

  for (size_t i = 0; i < headers.size(); ++i) {
    clipboard += headers[i];
    if (i < headers.size() - 1) clipboard += L", ";
  }

  clipboard += L"\n";

  size_t numColumns = headers.size();
  size_t numElements = GetNumElements();
  for (size_t i : selection) {
    if (i < numElements) {
      for (size_t j = 0; j < numColumns; ++j) {
        clipboard += GetValue((int)i, (int)j);
        if (j < numColumns - 1) clipboard += L", ";
      }

      clipboard += L"\n";
    }
  }

  GOrbitApp->SetClipboard(clipboard);
}
