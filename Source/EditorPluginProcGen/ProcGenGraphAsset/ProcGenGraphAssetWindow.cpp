#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphView.h>
#include <GuiFoundation/EngineUi/EngineGraphView.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

plProcGenGraphAssetDocumentWindow::plProcGenGraphAssetDocumentWindow(plProcGenGraphAssetDocument* pDocument)
  : plDocumentWindow(pDocument)
{
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  SetMenuBarActions("ProcGenAssetMenuBar");
  SetToolBarActions("ProcGenAssetToolBar");

  // the graph, with the node properties on its right
  {
    plUniquePtr<plVisualGraphController> pController = PL_DEFAULT_NEW(plProcGenGraphController);
    pController->Init(static_cast<const plVisualGraphObjectManager*>(pDocument->GetObjectManager()));
    AddPanel<plEditorUiGraphView>({"ProcGenGraphView", "Graph"}, std::move(pController));
  }

  AddPropertiesPanel({"ProcGenAssetDockWidget", "Node Properties", plEditorUiDockRole::Right});

  UpdatePreview();
}

plProcGenGraphAssetDocumentWindow::~plProcGenGraphAssetDocumentWindow()
{
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentWindow::TransationEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

plProcGenGraphAssetDocument* plProcGenGraphAssetDocumentWindow::GetProcGenGraphDocument()
{
  return static_cast<plProcGenGraphAssetDocument*>(GetDocument());
}

void plProcGenGraphAssetDocumentWindow::UpdatePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  plStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plProcGenGraph");
  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetTransformHash(GetDocument()->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetProcGenGraphDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();
  // Write Asset Data
  if (GetProcGenGraphDocument()->WriteAsset(memoryWriter, plAssetCurator::GetSingleton()->GetActiveAssetProfile(), true).Succeeded())
  {
    msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void plProcGenGraphAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "ProcGen Graph";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plProcGenGraphAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  // this event is only needed for changes to the DebugPin
  if (e.m_sProperty == "DebugPin")
  {
    UpdatePreview();
  }
}

void plProcGenGraphAssetDocumentWindow::TransationEventHandler(const plCommandHistoryEvent& e)
{
  if (e.m_Type == plCommandHistoryEvent::Type::TransactionEnded || e.m_Type == plCommandHistoryEvent::Type::UndoEnded || e.m_Type == plCommandHistoryEvent::Type::RedoEnded)
  {
    UpdatePreview();
  }
}
