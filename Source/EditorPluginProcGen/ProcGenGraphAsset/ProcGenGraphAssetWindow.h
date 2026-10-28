#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plProcGenGraphAssetDocument;

struct plCommandHistoryEvent;

/// The procedural generation graph's window: the graph ("ProcGenGraphView") and the node properties. Every finished
/// edit sends the graph to the engine process, so the scene previews it.
class plProcGenGraphAssetDocumentWindow : public plDocumentWindow
{
public:
  plProcGenGraphAssetDocumentWindow(plProcGenGraphAssetDocument* pDocument);
  ~plProcGenGraphAssetDocumentWindow();

  plProcGenGraphAssetDocument* GetProcGenGraphDocument();

private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void TransationEventHandler(const plCommandHistoryEvent& e);
};
