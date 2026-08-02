#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

/// Visual graph pin for procedural generation nodes.
///
/// Basic pin implementation for procedural generation graphs without additional metadata.
class plProcGenPin : public plVisualGraphPin
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenPin, plVisualGraphPin);

public:
  using plVisualGraphPin::plVisualGraphPin;
};

/// Object manager for procedural generation graphs.
///
/// Manages nodes and connections for procedural generation systems, such as terrain generation or placement rules.
/// Validates connections between different types of procedural generation nodes.
class plProcGenNodeManager : public plVisualGraphObjectManager
{
public:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node) override;
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override;

  virtual plStatus InternalCanConnect(const plVisualGraphPin& source, const plVisualGraphPin& target, CanConnectResult& out_result) const override;
};
