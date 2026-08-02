#pragma once

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodePins.h>
#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

class plProcGenNodeBase : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenNodeBase, plReflectedClass);

public:
  struct GraphContext
  {
    enum OutputType
    {
      Unknown,
      Placement,
      Color,
    };

    plProcGenInternal::GraphSharedData m_SharedData;
    plSmallArray<plUInt8, 4> m_VolumeTagSetIndices;
    plSmallArray<plUInt8, 4> m_CurveIndices;
    OutputType m_OutputType = OutputType::Unknown;
  };

  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) = 0;
};

//////////////////////////////////////////////////////////////////////////

class plProcGenOutput : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenOutput, plProcGenNodeBase);

public:
  bool m_bActive = true;

  void Save(plStreamWriter& inout_stream);
  void CopyValuesFromContext(const GraphContext& context);

  plString m_sName;

  plSmallArray<plUInt8, 4> m_VolumeTagSetIndices;
  plSmallArray<plUInt8, 4> m_CurveIndices;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_PlacementOutput : public plProcGenOutput
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_PlacementOutput, plProcGenOutput);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  void Save(plStreamWriter& inout_stream);

  plHybridArray<plString, 4> m_ObjectsToPlace;

  float m_fFootprint = 1.0f;

  plVec3 m_vMinOffset = plVec3(0);
  plVec3 m_vMaxOffset = plVec3(0);

  plAngle m_YawRotationSnap = plAngle::MakeFromRadian(0.0f);
  float m_fAlignToNormal = 1.0f;

  plVec3 m_vMinScale = plVec3(1);
  plVec3 m_vMaxScale = plVec3(1);

  float m_fCullDistance = 30.0f;

  plUInt32 m_uiCollisionLayer = 0;

  plString m_sSurface;

  plString m_sColorGradient;

  plEnum<plProcPlacementMode> m_PlacementMode;
  plEnum<plProcPlacementRenderMode> m_RenderMode;
  plUInt8 m_uiNumAdditionalRays = 4;
  float m_fRaySpread = 1.0f;

  plEnum<plProcPlacementPattern> m_PlacementPattern;

  plProcGenNodeInputPin m_DensityPin;
  plProcGenNodeInputPin m_ScalePin;
  plProcGenNodeInputPin m_ColorIndexPin;
  plProcGenNodeInputPin m_ObjectIndexPin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_VertexColorOutput : public plProcGenOutput
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_VertexColorOutput, plProcGenOutput);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  void Save(plStreamWriter& inout_stream);

  plProcGenNodeInputPin m_RPin;
  plProcGenNodeInputPin m_GPin;
  plProcGenNodeInputPin m_BPin;
  plProcGenNodeInputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Random : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Random, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plInt32 m_iSeed = -1;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  plProcGenNodeOutputPin m_OutputValuePin;

private:
  void OnObjectCreated(const plAbstractObjectNode& node);

  plUInt32 m_uiAutoSeed;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_PerlinNoise : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_PerlinNoise, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plVec3 m_Scale = plVec3(10);
  plVec3 m_Offset = plVec3::MakeZero();
  plUInt32 m_uiNumOctaves = 3;

  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;

  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Blend : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Blend, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plEnum<plProcGenBinaryOperator> m_Operator;
  float m_fInputValueA = 1.0f;
  float m_fInputValueB = 1.0f;
  bool m_bClampOutput = false;

  plProcGenNodeInputPin m_InputValueAPin;
  plProcGenNodeInputPin m_InputValueBPin;
  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Remap : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Remap, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputMin = 0.0f;
  float m_fInputMax = 1.0f;
  float m_fOutputMin = 0.0f;
  float m_fOutputMax = 1.0f;
  bool m_bClampIntermediate = false;

  plProcGenNodeInputPin m_InputValuePin;
  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Contrast : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Contrast, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputValue = 0.5f;
  float m_fContrast = 0.0f;

  plProcGenNodeInputPin m_InputValuePin;
  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Height : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Height, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fMinHeight = 0.0f;
  float m_fMaxHeight = 1000.0f;
  float m_fLowerFade = 0.2f;
  float m_fUpperFade = 0.2f;

  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Slope : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Slope, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plAngle m_MinSlope = plAngle::MakeFromDegree(0.0f);
  plAngle m_MaxSlope = plAngle::MakeFromDegree(30.0f);
  float m_fLowerFade = 0.0f;
  float m_fUpperFade = 0.2f;

  plProcGenNodeOutputPin m_OutputValuePin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Position : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Position, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plProcGenNodeOutputPin m_XPin;
  plProcGenNodeOutputPin m_YPin;
  plProcGenNodeOutputPin m_ZPin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_Normal : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_Normal, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plProcGenNodeOutputPin m_XPin;
  plProcGenNodeOutputPin m_YPin;
  plProcGenNodeOutputPin m_ZPin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_MeshVertexColor : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_MeshVertexColor, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  plProcGenNodeOutputPin m_RPin;
  plProcGenNodeOutputPin m_GPin;
  plProcGenNodeOutputPin m_BPin;
  plProcGenNodeOutputPin m_APin;
};

//////////////////////////////////////////////////////////////////////////

class plProcGen_ApplyVolumes : public plProcGenNodeBase
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGen_ApplyVolumes, plProcGenNodeBase);

public:
  virtual plExpressionAST::Node* GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context) override;

  float m_fInputValue = 0.0f;

  plTagSet m_IncludeTags;

  plEnum<plProcVolumeImageMode> m_ImageVolumeMode;
  plColorGammaUB m_RefColor;

  plProcGenNodeInputPin m_InputValuePin;
  plProcGenNodeOutputPin m_OutputValuePin;
};
