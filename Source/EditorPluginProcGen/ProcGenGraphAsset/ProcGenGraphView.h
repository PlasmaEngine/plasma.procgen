#pragma once

#include <GuiFoundation/VisualGraph/VisualGraphController.h>

class plProcGenGraphAssetDocument;

/// \brief The procedural generation graph in the engine graph view: headers coloured by category, titles with
/// their inputs and properties filled in, inactive nodes dimmed, and one debug pin whose result the preview
/// shows (pin menu "Debug", or D over a pin).
class plProcGenGraphController : public plVisualGraphController
{
public:
  void GetNodeLook(const plDocumentObject* pNode, plVisualGraphNodeLook& out_look) const override;
  void GetPinLook(const plVisualGraphPin& pin, plVisualGraphPinLook& out_look) const override;
  void ExtendPinMenu(const plVisualGraphPin& pin, plVisualGraphMenu& ref_menu) override;
  bool OnPinKey(const plVisualGraphPin& pin, plUInt32 uiCharacter) override;

  /// The pin whose result the preview shows; nullptr for the graph's output.
  void SetDebugPin(const plVisualGraphPin* pPin);
  const plVisualGraphPin* GetDebugPin() const { return m_pDebugPin; }

  /// A node's title: its plTitleAttribute with inputs ({Pin}: the pin name when connected, else {InputPin})
  /// and property values filled in, or its type name without "plProcGen". out_bActive receives its "Active"
  /// property (true without one).
  static void ComputeNodeTitle(const plVisualGraphObjectManager* pManager, const plDocumentObject* pNode, plStringBuilder& out_sTitle, bool& out_bActive);

  /// The header colour of a node category.
  static plColorGammaUB GetCategoryColor(plStringView sCategory);

protected:
  plStatus RemoveNode(const plDocumentObject* pNode) override;

private:
  plProcGenGraphAssetDocument* GetProcGenDocument() const;

  const plVisualGraphPin* m_pDebugPin = nullptr;
};
