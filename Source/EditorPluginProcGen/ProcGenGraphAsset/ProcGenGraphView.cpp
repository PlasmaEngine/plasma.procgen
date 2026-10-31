#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphView.h>

namespace
{
  /// {Pin}: the pin name when connected, else "{InputPin}", which the property pass fills with the literal.
  void FillInputPlaceholders(const plVisualGraphObjectManager* pManager, const plDocumentObject* pNode, plStringBuilder& ref_sTitle)
  {
    plStringBuilder sPlaceholder;
    plStringBuilder sInput;
    for (const auto& pPin : pManager->GetInputPins(pNode))
    {
      sPlaceholder.Set("{", pPin->GetName(), "}");

      if (pManager->HasConnections(*pPin))
      {
        ref_sTitle.ReplaceAll(sPlaceholder, pPin->GetName());
      }
      else
      {
        sInput.Set("{Input", pPin->GetName(), "}");
        ref_sTitle.ReplaceAll(sPlaceholder, sInput);
      }
    }
  }

  void FormatPropertyValue(const plDocumentObject* pNode, const plAbstractProperty* pProperty, plStringBuilder& out_sValue, bool& ref_bActive)
  {
    const auto& typeAccessor = pNode->GetTypeAccessor();

    if (pProperty->GetCategory() == plPropertyCategory::Set)
    {
      plHybridArray<plVariant, 16> values;
      typeAccessor.GetValues(pProperty->GetPropertyName(), values);

      out_sValue = "{";
      for (const plVariant& value : values)
      {
        if (out_sValue.GetElementCount() > 1)
          out_sValue.Append(", ");

        out_sValue.Append(value.ConvertTo<plString>().GetView());
      }
      out_sValue.Append("}");
      return;
    }

    const plVariant value = typeAccessor.GetValue(pProperty->GetPropertyName());
    if (pProperty->GetSpecificType()->IsDerivedFrom<plEnumBase>() || pProperty->GetSpecificType()->IsDerivedFrom<plBitflagsBase>())
    {
      plStringBuilder sEnum;
      plReflectionUtils::EnumerationToString(pProperty->GetSpecificType(), value.ConvertTo<plInt64>(), sEnum);
      out_sValue = plTranslate(sEnum);
    }
    else if (pProperty->GetSpecificType() == plGetStaticRTTI<bool>())
    {
      out_sValue = value.Get<bool>() ? "[x]" : "[ ]";

      if (plStringUtils::IsEqual(pProperty->GetPropertyName(), "Active"))
        ref_bActive = value.Get<bool>();
    }
    else if (value.CanConvertTo<plString>())
    {
      out_sValue = value.ConvertTo<plString>();
    }
  }
} // namespace

void plProcGenGraphController::ComputeNodeTitle(const plVisualGraphObjectManager* pManager, const plDocumentObject* pNode, plStringBuilder& out_sTitle, bool& out_bActive)
{
  out_bActive = true;
  const plRTTI* pRtti = pNode->GetType();

  const plTitleAttribute* pAttr = pRtti->GetAttributeByType<plTitleAttribute>();
  if (pAttr == nullptr)
  {
    out_sTitle = pRtti->GetTypeName();
    if (out_sTitle.StartsWith_NoCase("plProcGen"))
      out_sTitle.Shrink(9, 0);

    out_sTitle.TrimLeft("_");
    return;
  }

  out_sTitle = pAttr->GetTitle();
  FillInputPlaceholders(pManager, pNode, out_sTitle);

  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  plStringBuilder sValue;
  plStringBuilder sPlaceholder;
  for (const plAbstractProperty* pProperty : properties)
  {
    FormatPropertyValue(pNode, pProperty, sValue, out_bActive);

    sPlaceholder.Set("{", pProperty->GetPropertyName(), "}");
    out_sTitle.ReplaceAll(sPlaceholder, sValue);
  }
}

plColorGammaUB plProcGenGraphController::GetCategoryColor(plStringView sCategory)
{
  plColorScheme::Enum color = plColorScheme::Green;
  if (sCategory == "Input")
    color = plColorScheme::Lime;
  else if (sCategory == "Output")
    color = plColorScheme::Cyan;
  else if (sCategory == "Math")
    color = plColorScheme::Blue;

  return plColorScheme::ToPastel(plColorScheme::DarkUI(color));
}

plProcGenGraphAssetDocument* plProcGenGraphController::GetProcGenDocument() const
{
  // the debug pin is view state the document keeps for the preview, not an edit
  return const_cast<plProcGenGraphAssetDocument*>(static_cast<const plProcGenGraphAssetDocument*>(m_pManager->GetDocument()));
}

void plProcGenGraphController::GetNodeLook(const plDocumentObject* pNode, plVisualGraphNodeLook& out_look) const
{
  plVisualGraphController::GetNodeLook(pNode, out_look);
  if (out_look.m_Kind != plVisualGraphNodeLook::Kind::Box)
    return;

  plStringBuilder sTitle;
  ComputeNodeTitle(m_pManager, pNode, sTitle, out_look.m_bActive);
  out_look.m_sTitle = sTitle;
  out_look.m_sSubtitle.Clear();

  if (const plCategoryAttribute* pAttr = pNode->GetType()->GetAttributeByType<plCategoryAttribute>())
    out_look.m_HeaderColor = GetCategoryColor(pAttr->GetCategory());
}

void plProcGenGraphController::GetPinLook(const plVisualGraphPin& pin, plVisualGraphPinLook& out_look) const
{
  plVisualGraphController::GetPinLook(pin, out_look);
  out_look.m_bDebug = &pin == m_pDebugPin;
}

void plProcGenGraphController::ExtendPinMenu(const plVisualGraphPin& pin, plVisualGraphMenu& ref_menu)
{
  const plVisualGraphPin* pPin = &pin;
  plEditorUiMenuItem& item = ref_menu.AddAction("Debug", [this, pPin]()
    { SetDebugPin(m_pDebugPin == pPin ? nullptr : pPin); });

  item.m_bCheckable = true;
  item.m_bChecked = m_pDebugPin == pPin;
  item.m_sShortcut = "D";
}

bool plProcGenGraphController::OnPinKey(const plVisualGraphPin& pin, plUInt32 uiCharacter)
{
  if (uiCharacter != 'd' && uiCharacter != 'D')
    return false;

  SetDebugPin(m_pDebugPin == &pin ? nullptr : &pin);
  return true;
}

void plProcGenGraphController::SetDebugPin(const plVisualGraphPin* pPin)
{
  if (m_pDebugPin == pPin)
    return;

  m_pDebugPin = pPin;
  GetProcGenDocument()->SetDebugPin(pPin);
  InvalidateLook();
}

plStatus plProcGenGraphController::RemoveNode(const plDocumentObject* pNode)
{
  // the debug pin goes with its node
  if (m_pDebugPin != nullptr && m_pDebugPin->GetParent() == pNode)
    SetDebugPin(nullptr);

  return plVisualGraphController::RemoveNode(pNode);
}
