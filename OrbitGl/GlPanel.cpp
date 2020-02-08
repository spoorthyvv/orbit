//-----------------------------------
// Copyright Pierric Gimmig 2013-2017
//-----------------------------------

#include "GlPanel.h"

#include "App.h"
#include "BlackBoard.h"
#include "CaptureWindow.h"
#include "GlCanvas.h"
#include "HomeWindow.h"
#include "ImmediateWindow.h"
#include "PluginCanvas.h"
#include "RuleEditor.h"

//-----------------------------------------------------------------------------
std::shared_ptr<GlPanel> GlPanel::Create(Type a_Type, void* a_UserData) {
  GlPanel* panel = nullptr;

  switch (a_Type) {
    case CAPTURE: {
      auto panel = std::make_shared<CaptureWindow>();
      GOrbitApp->RegisterCaptureWindow(panel);
      return std::static_pointer_cast<GlPanel>(panel);
    }
    case IMMEDIATE: {
      auto panel = std::make_shared<ImmediateWindow>();
      return std::static_pointer_cast<GlPanel>(panel);
    }
    case VISUALIZE: {
      auto panel = std::make_shared<BlackBoard>();
      return std::static_pointer_cast<GlPanel>(panel);
    }
    case RULE_EDITOR: {
      auto panel = std::make_shared<RuleEditor>();
      GOrbitApp->RegisterRuleEditor(panel);
      return std::static_pointer_cast<GlPanel>(panel);
    }
    case DEBUG: {
      auto panel = std::make_shared<HomeWindow>();
      return std::static_pointer_cast<GlPanel>(panel);
    }
    case PLUGIN: {
      auto panel = std::make_shared<PluginCanvas>((Orbit::Plugin*)a_UserData);
      return std::static_pointer_cast<GlPanel>(panel);
    }
    default:
      break;
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
GlPanel::GlPanel() {
  m_WindowOffset[0] = 0;
  m_WindowOffset[1] = 0;
  m_MainWindowWidth = 0;
  m_MainWindowHeight = 0;
  m_NeedsRedraw = true;
}

//-----------------------------------------------------------------------------
GlPanel::~GlPanel() {}

//-----------------------------------------------------------------------------
void GlPanel::Initialize() {}

//-----------------------------------------------------------------------------
void GlPanel::Resize(int a_Width, int a_Height) {}

//-----------------------------------------------------------------------------
void GlPanel::Render(int a_Width, int a_Height) {}
