#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>

/// Qt graphics item for procedural generation nodes.
///
/// Visual representation of procedural generation nodes, such as noise generators, modifiers, or output nodes.
class plQtProcGenNode : public plQtVisualGraphNode
{
public:
  plQtProcGenNode();

  virtual void InitNode(const plVisualGraphObjectManager* pManager, const plDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

/// Qt graphics item for procedural generation pins.
///
/// Extends the base pin with debugging support. Pins can be marked for debug visualization,
/// allowing users to inspect intermediate results in the procedural generation pipeline.
class plQtProcGenPin : public plQtVisualGraphPin
{
public:
  plQtProcGenPin();
  ~plQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& ref_menu) override;

  virtual void keyPressEvent(QKeyEvent* pEvent) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

/// Qt scene for procedural generation graphs.
///
/// Manages the visual scene for procedural generation graph editing, including debug pin tracking
/// for visualizing intermediate generation results.
class plQtProcGenScene : public plQtVisualGraphScene
{
public:
  plQtProcGenScene(QObject* pParent = nullptr);
  ~plQtProcGenScene();

  void SetDebugPin(plQtProcGenPin* pDebugPin);

private:
  virtual plStatus RemoveNode(plQtVisualGraphNode* pNode) override;

  bool m_bUpdatingDebugPin = false;
  plQtProcGenPin* m_pDebugPin = nullptr;
};
