#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>

class plVisualGraphPin;

class plProcGenGraphAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenGraphAssetProperties, plReflectedClass);

public:
  plString m_sDebugPrefab;
  plString m_sDebugColorGradient;
  plString m_sDebugSurface;
  float m_fDebugFootprint = 1.0f;
  float m_fDebugAlignToNormal = 1.0f;
  plEnum<plProcPlacementPattern> m_DebugPlacementPattern = plProcPlacementPattern::RegularGrid;
};

class plProcGenGraphAssetDocument : public plAssetDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenGraphAssetDocument, plAssetDocument);

public:
  plProcGenGraphAssetDocument(plStringView sDocumentPath);

  void SetDebugPin(const plVisualGraphPin* pDebugPin);
  void UpdateDebugNode();

  plStatus WriteAsset(plStreamWriter& inout_stream, const plPlatformProfile* pAssetProfile, bool bAllowDebug) const;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual void GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const override;
  virtual bool CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const override;
  virtual bool Paste(
    const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType) override;

  virtual void AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable) override;

  void GetAllOutputNodes(plDynamicArray<const plDocumentObject*>& placementNodes, plDynamicArray<const plDocumentObject*>& vertexColorNodes) const;

private:
  friend class plProcGenAction;

  virtual void InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const override;

  struct GenerateContext;

  plExpressionAST::Node* GenerateExpressionAST(const plDocumentObject* outputNode, const char* szOutputName, GenerateContext& context, plExpressionAST& out_Ast) const;
  plExpressionAST::Node* GenerateDebugExpressionAST(GenerateContext& context, plExpressionAST& out_Ast) const;

  void DumpSelectedOutput(bool bAst, bool bDisassembly) const;

  void CreateDebugNode();

  const plVisualGraphPin* m_pDebugPin = nullptr;
  plUniquePtr<plProcGen_PlacementOutput> m_pDebugNode;
};
