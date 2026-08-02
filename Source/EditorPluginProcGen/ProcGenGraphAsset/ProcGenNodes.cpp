#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodes.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  plExpressionAST::NodeType::Enum GetOperator(plProcGenBinaryOperator::Enum blendMode)
  {
    switch (blendMode)
    {
      case plProcGenBinaryOperator::Add:
        return plExpressionAST::NodeType::Add;
      case plProcGenBinaryOperator::Subtract:
        return plExpressionAST::NodeType::Subtract;
      case plProcGenBinaryOperator::Multiply:
        return plExpressionAST::NodeType::Multiply;
      case plProcGenBinaryOperator::Divide:
        return plExpressionAST::NodeType::Divide;
      case plProcGenBinaryOperator::Max:
        return plExpressionAST::NodeType::Max;
      case plProcGenBinaryOperator::Min:
        return plExpressionAST::NodeType::Min;

        PL_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return plExpressionAST::NodeType::Invalid;
  }

  plExpressionAST::Node* CreateRandom(plUInt32 uiSeed, plExpressionAST& out_ast, const plProcGenNodeBase::GraphContext& context)
  {
    PL_ASSERT_DEV(context.m_OutputType != plProcGenNodeBase::GraphContext::Unknown, "Unkown output type");

    auto pointIndexDataType = context.m_OutputType == plProcGenNodeBase::GraphContext::Placement ? plProcessingStream::DataType::Short : plProcessingStream::DataType::Int;
    plExpressionAST::Node* pPointIndex = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPointIndex, pointIndexDataType});

    plExpressionAST::Node* pSeed = out_ast.CreateFunctionCall(plProcGenExpressionFunctions::s_GetInstanceSeedFunc.m_Desc, plArrayPtr<plExpressionAST::Node*>());
    pSeed = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pSeed, out_ast.CreateConstant(uiSeed, plExpressionAST::DataType::Int));

    plExpressionAST::Node* arguments[] = {pPointIndex, pSeed};
    return out_ast.CreateFunctionCall(plDefaultExpressionFunctions::s_RandomFunc.m_Desc, arguments);
  }

  plExpressionAST::Node* CreateRemapFrom01(plExpressionAST::Node* pInput, float fMin, float fMax, plExpressionAST& out_ast)
  {
    auto pOffset = out_ast.CreateConstant(fMin);
    auto pScale = out_ast.CreateConstant(fMax - fMin);

    auto pValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pInput, pScale);
    pValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pValue, pOffset);

    return pValue;
  }

  plExpressionAST::Node* CreateRemapTo01WithFadeout(plExpressionAST::Node* pInput, float fMin, float fMax, float fLowerFade, float fUpperFade, plExpressionAST& out_ast)
  {
    // Note that we need to clamp the scale if it is below eps or we would end up with a division by 0.
    // To counter the clamp we move the lower and upper bounds by eps.
    // If no fade out is specified we would get a value of 0 for inputs that are exactly on the bounds otherwise which is not the expected behavior.

    const float eps = plMath::DefaultEpsilon<float>();
    const float fLowerScale = plMath::Max((fMax - fMin), 0.0f) * fLowerFade;
    const float fUpperScale = plMath::Max((fMax - fMin), 0.0f) * fUpperFade;

    if (fLowerScale < eps)
      fMin = fMin - eps;
    if (fUpperScale < eps)
      fMax = fMax + eps;

    auto pLowerOffset = out_ast.CreateConstant(fMin);
    auto pLowerValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Subtract, pInput, pLowerOffset);
    auto pLowerScale = out_ast.CreateConstant(plMath::Max(fLowerScale, eps));
    pLowerValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pLowerValue, pLowerScale);

    auto pUpperOffset = out_ast.CreateConstant(fMax);
    auto pUpperValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Subtract, pUpperOffset, pInput);
    auto pUpperScale = out_ast.CreateConstant(plMath::Max(fUpperScale, eps));
    pUpperValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pUpperValue, pUpperScale);

    auto pValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Min, pLowerValue, pUpperValue);
    return out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pValue);
  }

  void AddDefaultInputs(plExpressionAST& out_ast)
  {
    out_ast.m_InputNodes.Clear();

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionX, plProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionY, plProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float}));

    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalX, plProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalY, plProcessingStream::DataType::Float}));
    out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalZ, plProcessingStream::DataType::Float}));
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenNodeBase, 1, plRTTINoAllocator)
{
  flags.Add(plTypeFlags::Abstract);
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenOutput, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new plDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
  }
  PL_END_PROPERTIES;

  flags.Add(plTypeFlags::Abstract);
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plProcGenOutput::Save(plStreamWriter& inout_stream)
{
  inout_stream << m_sName;
  inout_stream.WriteArray(m_VolumeTagSetIndices).IgnoreResult();
  inout_stream.WriteArray(m_CurveIndices).IgnoreResult();
}

void plProcGenOutput::CopyValuesFromContext(const GraphContext& context)
{
  m_VolumeTagSetIndices = context.m_VolumeTagSetIndices;
  m_CurveIndices = context.m_CurveIndices;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_PlacementOutput, 2, plRTTIDefaultAllocator<plProcGen_PlacementOutput>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectsToPlace)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab")),
    PL_MEMBER_PROPERTY("Footprint", m_fFootprint)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("MinOffset", m_vMinOffset),
    PL_MEMBER_PROPERTY("MaxOffset", m_vMaxOffset),
    PL_MEMBER_PROPERTY("YawRotationSnap", m_YawRotationSnap)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromRadian(0.0f), plVariant())),
    PL_MEMBER_PROPERTY("AlignToNormal", m_fAlignToNormal)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_MEMBER_PROPERTY("MinScale", m_vMinScale)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f)), new plClampValueAttribute(plVec3(0.0f), plVariant())),
    PL_MEMBER_PROPERTY("MaxScale", m_vMaxScale)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f)), new plClampValueAttribute(plVec3(0.0f), plVariant())),
    PL_MEMBER_PROPERTY("ColorGradient", m_sColorGradient)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    PL_MEMBER_PROPERTY("CullDistance", m_fCullDistance)->AddAttributes(new plDefaultValueAttribute(30.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ENUM_MEMBER_PROPERTY("PlacementMode", plProcPlacementMode, m_PlacementMode),
    PL_ENUM_MEMBER_PROPERTY("RenderMode", plProcPlacementRenderMode, m_RenderMode)->AddAttributes(new plDefaultValueAttribute(plProcPlacementRenderMode::SpawnPrefabs)),
    PL_MEMBER_PROPERTY("NumAdditionalRays", m_uiNumAdditionalRays)->AddAttributes(new plDefaultValueAttribute(4), new plClampValueAttribute(3, 20)),
    PL_MEMBER_PROPERTY("RaySpread", m_fRaySpread)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_ENUM_MEMBER_PROPERTY("PlacementPattern", plProcPlacementPattern, m_PlacementPattern),
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),

    PL_MEMBER_PROPERTY("Density", m_DensityPin),
    PL_MEMBER_PROPERTY("Scale", m_ScalePin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Pink))),
    PL_MEMBER_PROPERTY("ColorIndex", m_ColorIndexPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Violet))),
    PL_MEMBER_PROPERTY("ObjectIndex", m_ObjectIndexPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Cyan)))
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Active} Placement Output: {Name}"),
    new plCategoryAttribute("Output"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_PlacementOutput::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPointIndex, plProcessingStream::DataType::Short}));

  out_ast.m_OutputNodes.Clear();

  // density
  {
    auto pDensity = inputs[0];
    if (pDensity == nullptr)
    {
      pDensity = out_ast.CreateConstant(1.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutDensity, plProcessingStream::DataType::Float}, pDensity));
  }

  // scale
  {
    auto pScale = inputs[1];
    if (pScale == nullptr)
    {
      pScale = CreateRandom(11.0f, out_ast, ref_context);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutScale, plProcessingStream::DataType::Float}, pScale));
  }

  // color index
  {
    auto pColorIndex = inputs[2];
    if (pColorIndex == nullptr)
    {
      pColorIndex = CreateRandom(13.0f, out_ast, ref_context);
    }

    pColorIndex = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pColorIndex);
    pColorIndex = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pColorIndex, out_ast.CreateConstant(255.0f));
    pColorIndex = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pColorIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutColorIndex, plProcessingStream::DataType::Byte}, pColorIndex));
  }

  // object index
  {
    auto pObjectIndex = inputs[3];
    if (pObjectIndex == nullptr)
    {
      pObjectIndex = CreateRandom(17.0f, out_ast, ref_context);
    }

    pObjectIndex = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pObjectIndex);
    pObjectIndex = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pObjectIndex, out_ast.CreateConstant(m_ObjectsToPlace.GetCount() - 1));
    pObjectIndex = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pObjectIndex, out_ast.CreateConstant(0.5f));

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({plProcGenInternal::ExpressionOutputs::s_sOutObjectIndex, plProcessingStream::DataType::Byte}, pObjectIndex));
  }

  return nullptr;
}

void plProcGen_PlacementOutput::Save(plStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);

  inout_stream.WriteArray(m_ObjectsToPlace).IgnoreResult();

  inout_stream << m_fFootprint;

  inout_stream << m_vMinOffset;
  inout_stream << m_vMaxOffset;

  // chunk version 6
  inout_stream << m_YawRotationSnap;
  inout_stream << m_fAlignToNormal;

  inout_stream << m_vMinScale;
  inout_stream << m_vMaxScale;

  inout_stream << m_fCullDistance;

  inout_stream << m_uiCollisionLayer;

  inout_stream << m_sColorGradient;

  // chunk version 3
  inout_stream << m_sSurface;

  // chunk version 5
  inout_stream << m_PlacementMode;

  // chunk version 10
  inout_stream << m_RenderMode;

  // chunk version 8
  inout_stream << m_uiNumAdditionalRays;
  inout_stream << m_fRaySpread;

  // chunk version 7
  inout_stream << m_PlacementPattern;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_VertexColorOutput, 2, plRTTIDefaultAllocator<plProcGen_VertexColorOutput>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PL_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PL_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
    PL_MEMBER_PROPERTY("A", m_APin),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Active} Vertex Color Output: {Name}"),
    new plCategoryAttribute("Output"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_VertexColorOutput::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "", "Implementation error");

  AddDefaultInputs(out_ast);

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorR, plProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorG, plProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorB, plProcessingStream::DataType::Float}));
  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorA, plProcessingStream::DataType::Float}));

  out_ast.m_InputNodes.PushBack(out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPointIndex, plProcessingStream::DataType::Int}));

  out_ast.m_OutputNodes.Clear();

  plHashedString sOutputNames[4] = {
    plProcGenInternal::ExpressionOutputs::s_sOutColorR,
    plProcGenInternal::ExpressionOutputs::s_sOutColorG,
    plProcGenInternal::ExpressionOutputs::s_sOutColorB,
    plProcGenInternal::ExpressionOutputs::s_sOutColorA,
  };

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(sOutputNames); ++i)
  {
    auto pInput = inputs[i];
    if (pInput == nullptr)
    {
      pInput = out_ast.CreateConstant(0.0f);
    }

    out_ast.m_OutputNodes.PushBack(out_ast.CreateOutput({sOutputNames[i], plProcessingStream::DataType::Float}, pInput));
  }

  return nullptr;
}

void plProcGen_VertexColorOutput::Save(plStreamWriter& inout_stream)
{
  SUPER::Save(inout_stream);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Random, 2, plRTTIDefaultAllocator<plProcGen_Random>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Seed", m_iSeed)->AddAttributes(new plClampValueAttribute(-1, plVariant()), new plDefaultValueAttribute(-1), new plMinValueTextAttribute("Auto")),
    PL_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    PL_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),

    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_FUNCTION_PROPERTY(OnObjectCreated),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Random: {Seed}"),
    new plCategoryAttribute("Math"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Random::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  float fSeed = m_iSeed < 0 ? m_uiAutoSeed : m_iSeed;

  auto pRandom = CreateRandom(fSeed, out_ast, ref_context);
  return CreateRemapFrom01(pRandom, m_fOutputMin, m_fOutputMax, out_ast);
}

void plProcGen_Random::OnObjectCreated(const plAbstractObjectNode& node)
{
  m_uiAutoSeed = plHashHelper<plUuid>::Hash(node.GetGuid());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_PerlinNoise, 2, plRTTIDefaultAllocator<plProcGen_PerlinNoise>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Scale", m_Scale)->AddAttributes(new plDefaultValueAttribute(plVec3(10))),
    PL_MEMBER_PROPERTY("Offset", m_Offset),
    PL_MEMBER_PROPERTY("NumOctaves", m_uiNumOctaves)->AddAttributes(new plClampValueAttribute(1, 6), new plDefaultValueAttribute(3)),
    PL_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    PL_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),

    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Perlin Noise"),
    new plCategoryAttribute("Math"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_PerlinNoise::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  plExpressionAST::Node* pPos = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPosition, plProcessingStream::DataType::Float3});
  pPos = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Divide, pPos, out_ast.CreateConstant(m_Scale, plExpressionAST::DataType::Float3));
  pPos = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Add, pPos, out_ast.CreateConstant(m_Offset, plExpressionAST::DataType::Float3));

  auto pPosX = out_ast.CreateSwizzle(plExpressionAST::VectorComponent::X, pPos);
  auto pPosY = out_ast.CreateSwizzle(plExpressionAST::VectorComponent::Y, pPos);
  auto pPosZ = out_ast.CreateSwizzle(plExpressionAST::VectorComponent::Z, pPos);

  auto pNumOctaves = out_ast.CreateConstant(m_uiNumOctaves, plExpressionAST::DataType::Int);

  plExpressionAST::Node* arguments[] = {pPosX, pPosY, pPosZ, pNumOctaves};

  auto pNoiseFunc = out_ast.CreateFunctionCall(plDefaultExpressionFunctions::s_PerlinNoiseFunc.m_Desc, arguments);

  return CreateRemapFrom01(pNoiseFunc, m_fOutputMin, m_fOutputMax, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Blend, 3, plRTTIDefaultAllocator<plProcGen_Blend>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Operator", plProcGenBinaryOperator, m_Operator),
    PL_MEMBER_PROPERTY("InputA", m_fInputValueA)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("InputB", m_fInputValueB)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("ClampOutput", m_bClampOutput),

    PL_MEMBER_PROPERTY("A", m_InputValueAPin),
    PL_MEMBER_PROPERTY("B", m_InputValueBPin),
    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("{Operator}({A}, {B})"),
    new plCategoryAttribute("Math"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Blend::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInputA = inputs[0];
  if (pInputA == nullptr)
  {
    pInputA = out_ast.CreateConstant(m_fInputValueA);
  }

  auto pInputB = inputs[1];
  if (pInputB == nullptr)
  {
    pInputB = out_ast.CreateConstant(m_fInputValueB);
  }

  plExpressionAST::Node* pBlend = out_ast.CreateBinaryOperator(GetOperator(m_Operator), pInputA, pInputB);

  if (m_bClampOutput)
  {
    pBlend = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pBlend);
  }

  return pBlend;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Remap, 2, plRTTIDefaultAllocator<plProcGen_Remap>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InputMin", m_fInputMin),
    PL_MEMBER_PROPERTY("InputMax", m_fInputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("ClampIntermediate", m_bClampIntermediate),
    PL_MEMBER_PROPERTY("OutputMin", m_fOutputMin),
    PL_MEMBER_PROPERTY("OutputMax", m_fOutputMax)->AddAttributes(new plDefaultValueAttribute(1.0f)),

    PL_MEMBER_PROPERTY("X", m_InputValuePin),
    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Remap: [{InputMin}, {InputMax}] -> [{OutputMin}, {OutputMax}]"),
    new plCategoryAttribute("Math"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Remap::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(0.0f);
  }

  plExpressionAST::Node* p01Value = nullptr;
  if (m_fInputMin == 0.0f && m_fInputMax == 1.0f)
  {
    p01Value = pInput;
  }
  else if (plMath::IsEqual(m_fInputMin, m_fInputMax, plMath::DefaultEpsilon<float>()))
  {
    p01Value = out_ast.CreateConstant(1.0f);
  }
  else
  {
    auto pOffset = out_ast.CreateConstant(m_fInputMin);
    auto pValue = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Subtract, pInput, pOffset);
    auto pScale = out_ast.CreateConstant(1.0f / (m_fInputMax - m_fInputMin));
    p01Value = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Multiply, pValue, pScale);
  }

  if (m_bClampIntermediate)
  {
    p01Value = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, p01Value);
  }

  if (m_fOutputMin == 0.0f && m_fOutputMax == 1.0f)
  {
    return p01Value;
  }

  auto remapFrom01 = CreateRemapFrom01(p01Value, m_fOutputMin, m_fOutputMax, out_ast);

  return remapFrom01;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Contrast, 2, plRTTIDefaultAllocator<plProcGen_Contrast>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Input", m_fInputValue)->AddAttributes(new plDefaultValueAttribute(0.5f)),
    PL_MEMBER_PROPERTY("Contrast", m_fContrast)->AddAttributes(new plClampValueAttribute(-10.0f, 10.0f)),

    PL_MEMBER_PROPERTY("X", m_InputValuePin),
    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Contrast: {Contrast}"),
    new plCategoryAttribute("Math"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Contrast::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(m_fInputValue);
  }

  auto pA = out_ast.CreateConstant(-m_fContrast);
  auto pB = out_ast.CreateConstant(1.0f + m_fContrast);
  auto pLerp = out_ast.CreateTernaryOperator(plExpressionAST::NodeType::Lerp, pA, pB, pInput);
  auto pSaturate = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::Saturate, pLerp);

  return pSaturate;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Height, 2, plRTTIDefaultAllocator<plProcGen_Height>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MinHeight", m_fMinHeight)->AddAttributes(new plDefaultValueAttribute(0.0f)),
    PL_MEMBER_PROPERTY("MaxHeight", m_fMaxHeight)->AddAttributes(new plDefaultValueAttribute(1000.0f)),
    PL_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),

    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Height: [{MinHeight}, {MaxHeight}]"),
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Height::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pHeight = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float});
  return CreateRemapTo01WithFadeout(pHeight, m_fMinHeight, m_fMaxHeight, m_fLowerFade, m_fUpperFade, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Slope, 2, plRTTIDefaultAllocator<plProcGen_Slope>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MinSlope", m_MinSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(0.0f))),
    PL_MEMBER_PROPERTY("MaxSlope", m_MaxSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(60.0f))),
    PL_MEMBER_PROPERTY("LowerFade", m_fLowerFade)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_MEMBER_PROPERTY("UpperFade", m_fUpperFade)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1.0f)),


    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Slope: [{MinSlope}, {MaxSlope}]"),
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Slope::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  auto pNormalZ = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalZ, plProcessingStream::DataType::Float});
  // acos explodes for values slightly larger than 1 so make sure to clamp before
  auto pClampedNormalZ = out_ast.CreateBinaryOperator(plExpressionAST::NodeType::Min, out_ast.CreateConstant(1.0f), pNormalZ);
  auto pAngle = out_ast.CreateUnaryOperator(plExpressionAST::NodeType::ACos, pClampedNormalZ);
  return CreateRemapTo01WithFadeout(pAngle, m_MinSlope.GetRadian(), m_MaxSlope.GetRadian(), m_fLowerFade, m_fUpperFade, out_ast);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Position, 2, plRTTIDefaultAllocator<plProcGen_Position>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("X", m_XPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PL_MEMBER_PROPERTY("Y", m_YPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PL_MEMBER_PROPERTY("Z", m_ZPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Position"),
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Position::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "X")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionX, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "Y")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionY, plProcessingStream::DataType::Float});
  }
  else
  {
    PL_ASSERT_DEBUG(sOutputName == "Z", "Implementation error");
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_Normal, 2, plRTTIDefaultAllocator<plProcGen_Normal>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("X", m_XPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PL_MEMBER_PROPERTY("Y", m_YPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PL_MEMBER_PROPERTY("Z", m_ZPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Normal"),
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_Normal::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "X")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalX, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "Y")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalY, plProcessingStream::DataType::Float});
  }
  else
  {
    PL_ASSERT_DEBUG(sOutputName == "Z", "Implementation error");
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sNormalZ, plProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_MeshVertexColor, 2, plRTTIDefaultAllocator<plProcGen_MeshVertexColor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("R", m_RPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red))),
    PL_MEMBER_PROPERTY("G", m_GPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Green))),
    PL_MEMBER_PROPERTY("B", m_BPin)->AddAttributes(new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Blue))),
    PL_MEMBER_PROPERTY("A", m_APin),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Mesh Vertex Color"),
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_MeshVertexColor::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  if (sOutputName == "R")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorR, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "G")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorG, plProcessingStream::DataType::Float});
  }
  else if (sOutputName == "B")
  {
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorB, plProcessingStream::DataType::Float});
  }
  else
  {
    PL_ASSERT_DEBUG(sOutputName == "A", "Implementation error");
    return out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sColorA, plProcessingStream::DataType::Float});
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGen_ApplyVolumes, 2, plRTTIDefaultAllocator<plProcGen_ApplyVolumes>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new plTagSetWidgetAttribute("Default")),

    PL_MEMBER_PROPERTY("InputValue", m_fInputValue),

    PL_ENUM_MEMBER_PROPERTY("ImageVolumeMode", plProcVolumeImageMode, m_ImageVolumeMode),
    PL_MEMBER_PROPERTY("RefColor", m_RefColor)->AddAttributes(new plExposeColorAlphaAttribute()),

    PL_MEMBER_PROPERTY("In", m_InputValuePin),
    PL_MEMBER_PROPERTY("Value", m_OutputValuePin)
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Volumes: {IncludeTags}"),
    new plCategoryAttribute("Modifiers"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plExpressionAST::Node* plProcGen_ApplyVolumes::GenerateExpressionASTNode(plTempHashedString sOutputName, plArrayPtr<plExpressionAST::Node*> inputs, plExpressionAST& out_ast, GraphContext& ref_context)
{
  PL_ASSERT_DEBUG(sOutputName == "Value", "Implementation error");

  plUInt32 tagSetIndex = ref_context.m_SharedData.AddTagSet(m_IncludeTags);
  PL_ASSERT_DEV(tagSetIndex <= 255, "Too many tag sets");
  if (!ref_context.m_VolumeTagSetIndices.Contains(tagSetIndex))
  {
    ref_context.m_VolumeTagSetIndices.PushBack(tagSetIndex);
  }

  auto pPosX = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionX, plProcessingStream::DataType::Float});
  auto pPosY = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionY, plProcessingStream::DataType::Float});
  auto pPosZ = out_ast.CreateInput({plProcGenInternal::ExpressionInputs::s_sPositionZ, plProcessingStream::DataType::Float});

  auto pInput = inputs[0];
  if (pInput == nullptr)
  {
    pInput = out_ast.CreateConstant(m_fInputValue);
  }

  plExpressionAST::Node* arguments[] = {
    pPosX,
    pPosY,
    pPosZ,
    pInput,
    out_ast.CreateConstant(tagSetIndex, plExpressionAST::DataType::Int),
    out_ast.CreateConstant(m_ImageVolumeMode.GetValue(), plExpressionAST::DataType::Int),
    out_ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.r)),
    out_ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.g)),
    out_ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.b)),
    out_ast.CreateConstant(plMath::ColorByteToFloat(m_RefColor.a)),
  };

  return out_ast.CreateFunctionCall(plProcGenExpressionFunctions::s_ApplyVolumesFunc.m_Desc, arguments);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plProcGen_Blend_1_2 : public plGraphPatch
{
public:
  plProcGen_Blend_1_2()
    : plGraphPatch("plProcGen_Blend", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pMode = pNode->FindProperty("Mode");
    if (pMode && pMode->m_Value.IsA<plString>())
    {
      plStringBuilder val = pMode->m_Value.Get<plString>();
      val.ReplaceAll("plProcGenBlendMode", "plProcGenBinaryOperator");

      pNode->AddProperty("Operator", val.GetData());
    }
  }
};

plProcGen_Blend_1_2 g_plProcGen_Blend_1_2;
