#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plProcGenGraphAssetDocument;

class plQtVisualGraphScene;
class plQtVisualGraphView;
struct plCommandHistoryEvent;

class plProcGenGraphAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plProcGenGraphAssetDocumentWindow(plProcGenGraphAssetDocument* pDocument);
  ~plProcGenGraphAssetDocumentWindow();

  plProcGenGraphAssetDocument* GetProcGenGraphDocument();

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void TransationEventHandler(const plCommandHistoryEvent& e);

  plQtVisualGraphScene* m_pScene;
  plQtVisualGraphView* m_pView;
};
