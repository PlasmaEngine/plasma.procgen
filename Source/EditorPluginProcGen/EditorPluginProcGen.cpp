#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginProcGen/Actions/ProcGenActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <EditorUi/Widgets.h>
#include <Foundation/Configuration/Plugin.h>

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

/// The package's icon folder: the icons under their ids (":/AssetIcons/ProcGen_Graph.svg" is
/// "EditorIcons/AssetIcons/ProcGen_Graph.svg"). The plugin binary lives in <package>/Bin/<configuration>.
static plString s_sIconFolder;

static void RegisterIconFolder()
{
  plStringBuilder sPlugin, sCopy;
  plPlugin::GetPluginPaths("plEditorPluginProcGen", sPlugin, sCopy, 0);

  plStringBuilder sFolder = sPlugin.GetFileDirectory();
  sFolder.AppendPath("..", "..", "EditorIcons");
  sFolder.MakeCleanPath();

  s_sIconFolder = sFolder;
  plEditorUi::AddIconFolder(s_sIconFolder);
}

static void UnregisterIconFolder()
{
  plEditorUi::RemoveIconFolder(s_sIconFolder);
  s_sIconFolder.Clear();
}

void OnLoadPlugin()
{
  RegisterIconFolder();

  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "ProcGenAssetMenuBar";

  plActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  plStandardMenus::MapActions(szMenuBar, plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
  plProjectActions::MapActions(szMenuBar);
  plDocumentActions::MapMenuActions(szMenuBar);
  plAssetActions::MapMenuActions(szMenuBar);
  plCommandHistoryActions::MapActions(szMenuBar);

  plEditActions::MapActions("ProcGenAssetMenuBar", false, false);
}

// Tool Bar
{
  const char* szToolBar = "ProcGenAssetToolBar";
  plActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  plDocumentActions::MapToolbarActions(szToolBar);
  plCommandHistoryActions::MapActions(szToolBar, "");
  plAssetActions::MapToolBarActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {
    plProcGenActions::RegisterActions();
    plProcGenActions::MapMenuActions();
  }

  // Tool Bar
  {
  }
}
}

void OnUnloadPlugin()
{
  plProcGenActions::UnregisterActions();

  UnregisterIconFolder();
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
