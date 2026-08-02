#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <ProcGenPlugin/Components/ProcVertexColorComponent.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

#include <RendererCore/../../../Data/Plugins/ProcGen/ProcGenVertexColor.h>

namespace
{
  static plCpuMeshResourceHandle ExtractCpuMeshResource(const plMeshComponentBase& meshComponent)
  {
    plWorldGeoExtractionUtil::MeshObjectList meshObjects(plFrameAllocator::GetCurrentAllocator());

    plMsgExtractGeometry msg;
    msg.m_pMeshObjects = &meshObjects;

    meshComponent.SendMessage(msg);

    if (meshObjects.IsEmpty())
      return plCpuMeshResourceHandle();

    return meshObjects[0].m_hMeshResource;
  }

  PL_ALWAYS_INLINE static plCustomInstanceDataOffset EncodeOffset(plCustomInstanceDataOffset offset, plUInt32 uiNumOutputs)
  {
    plCustomInstanceDataOffset res;
    res.m_uiOffset = uiNumOutputs << VERTEX_COLOR_ACCESS_OFFSET_BITS | (offset.m_uiOffset & VERTEX_COLOR_ACCESS_OFFSET_MASK);
    return res;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

using namespace plProcGenInternal;

plProcVertexColorComponentManager::plProcVertexColorComponentManager(plWorld* pWorld)
  : plComponentManager<plProcVertexColorComponent, plBlockStorageType::Compact>(pWorld)
{
}

plProcVertexColorComponentManager::~plProcVertexColorComponentManager() = default;

void plProcVertexColorComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    constexpr plUInt32 uiInitialBufferSize = 1024 * 16;

    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(plColorLinearUB);
    desc.m_uiTotalSize = uiInitialBufferSize * desc.m_uiStructSize;
    desc.m_ResourceAccess.m_bImmutable = false;

    if (plGALDevice::GetDefaultDevice()->GetCapabilities().m_bSupportsTexelBuffer)
    {
      desc.m_BufferFlags = plGALBufferUsageFlags::TexelBuffer | plGALBufferUsageFlags::ShaderResource;
      desc.m_Format = plGALResourceFormat::RGBAUByteNormalized;
    }
    else
    {
      desc.m_BufferFlags = plGALBufferUsageFlags::StructuredBuffer | plGALBufferUsageFlags::ShaderResource;
    }

    plRenderDataManager* pRenderDataManager = GetWorld()->GetOrCreateModule<plRenderDataManager>();
    m_uiCustomDataIndex = pRenderDataManager->RegisterCustomInstanceData(desc, "ProcVertexColors",
      // Before upload callback
      [this]()
      {
        plTaskSystem::WaitForGroup(m_UpdateTaskGroupID);
        m_UpdateTaskGroupID.Invalidate();
        m_uiNextTaskIndex = 0;
      });
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plProcVertexColorComponentManager::UpdateVertexColors, this);
    desc.m_Phase = plWorldUpdatePhase::PreAsync;
    desc.m_fPriority = 10000.0f;

    this->RegisterUpdateFunction(desc);
  }

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnResourceEvent, this));

  plProcVolumeComponent::GetAreaInvalidatedEvent().AddEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnAreaInvalidated, this));
}

void plProcVertexColorComponentManager::Deinitialize()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnResourceEvent, this));

  plProcVolumeComponent::GetAreaInvalidatedEvent().RemoveEventHandler(plMakeDelegate(&plProcVertexColorComponentManager::OnAreaInvalidated, this));

  SUPER::Deinitialize();
}

void plProcVertexColorComponentManager::UpdateVertexColors(const plWorldModule::UpdateContext& context)
{
  plRenderDataManager* pRenderDataManager = GetWorld()->GetModule<plRenderDataManager>();
  pRenderDataManager->CompactCustomInstanceDataBuffer(m_uiCustomDataIndex);

  m_UpdateContexts.SetCount(m_ComponentsToUpdate.GetCount());

  // New allocations first to ensure that the buffer is large enough and the memory is not invalidated by reallocations
  {
    for (plUInt32 i = 0; i < m_ComponentsToUpdate.GetCount(); ++i)
    {
      plProcVertexColorComponent* pComponent = nullptr;
      if (!TryGetComponent(m_ComponentsToUpdate[i], pComponent))
        continue;

      if (!UpdateComponentOutputs(*pComponent))
        continue;

      auto pMeshComponent = pComponent->GetMeshComponent();
      if (pMeshComponent == nullptr || pMeshComponent->GetCustomInstanceDataOffset().IsInvalidated() == false)
        continue;

      auto hCpuMesh = ExtractCpuMeshResource(*pMeshComponent);
      if (hCpuMesh.IsValid() == false)
        continue;

      plResourceLock<plCpuMeshResource> pCpuMesh(hCpuMesh, plResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
        continue;

      const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
      const plUInt32 uiNumOutputs = pComponent->m_Outputs.GetCount();
      const plUInt32 uiVertexColorCount = meshBufferDescriptor.GetVertexCount() * uiNumOutputs;

      plCustomInstanceDataOffset offset = pMeshComponent->GetCustomInstanceDataOffset();
      plGALDynamicBufferHandle hVertexColorsBuffer;
      pRenderDataManager->GetOrCreateCustomInstanceData<plColorLinearUB>(m_uiCustomDataIndex, pComponent, hVertexColorsBuffer, offset, uiVertexColorCount);
      pComponent->m_CustomInstanceDataOffset = offset;
      pMeshComponent->SetCustomInstanceData(EncodeOffset(offset, uiNumOutputs), hVertexColorsBuffer);

      auto& updateContext = m_UpdateContexts[i];
      updateContext.m_pComponent = pComponent;
      updateContext.m_hCpuMesh = hCpuMesh;
      updateContext.m_uiVertexColorOffset = offset.m_uiOffset;
    }
  }

  // Update
  if (m_ComponentsToUpdate.IsEmpty() == false)
  {
    m_UpdateTaskGroupID = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);

    plGALDynamicBuffer* pBuffer = plGALDevice::GetDefaultDevice()->GetDynamicBuffer(pRenderDataManager->GetCustomInstanceDataBuffer(m_uiCustomDataIndex));
    PL_ASSERT_DEV(pBuffer != nullptr, "Vertex color buffer not found.");

    for (const auto& updateContext : m_UpdateContexts)
    {
      if (updateContext.m_pComponent == nullptr)
        continue;

      UpdateComponentVertexColors(updateContext, *pBuffer);
    }

    m_ComponentsToUpdate.Clear();

    plTaskSystem::StartTaskGroup(m_UpdateTaskGroupID);
  }
}

bool plProcVertexColorComponentManager::UpdateComponentOutputs(plProcVertexColorComponent& component)
{
  component.m_Outputs.Clear();

  {
    plResourceLock<plProcGenGraphResource> pResource(component.m_hResource, plResourceAcquireMode::BlockTillLoaded);
    auto outputs = pResource->GetVertexColorOutputs();

    for (auto& outputDesc : component.m_OutputDescs)
    {
      if (!outputDesc.m_sName.IsEmpty())
      {
        bool bOutputFound = false;
        for (auto& pOutput : outputs)
        {
          if (pOutput->m_sName == outputDesc.m_sName)
          {
            component.m_Outputs.PushBack(pOutput);
            bOutputFound = true;
            break;
          }
        }

        if (!bOutputFound)
        {
          component.m_Outputs.PushBack(nullptr);
          plLog::Error("Vertex Color Output with name '{}' not found in Proc Gen Graph '{}'", outputDesc.m_sName, pResource->GetResourceID());
        }
      }
      else
      {
        component.m_Outputs.PushBack(nullptr);
      }
    }
  }

  return component.HasValidOutputs();
}

void plProcVertexColorComponentManager::UpdateComponentVertexColors(const UpdateContext& context, plGALDynamicBuffer& buffer)
{
  auto pComponent = context.m_pComponent;
  if (pComponent->HasValidOutputs() == false)
    return;

  plResourceLock<plCpuMeshResource> pCpuMesh(context.m_hCpuMesh, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  if (m_uiNextTaskIndex >= m_UpdateTasks.GetCount())
  {
    m_UpdateTasks.PushBack(PL_DEFAULT_NEW(plProcGenInternal::VertexColorTask));
  }

  auto& pUpdateTask = m_UpdateTasks[m_uiNextTaskIndex];

  plStringBuilder taskName = "VertexColor ";
  taskName.Append(pCpuMesh->GetResourceIdOrDescription());
  pUpdateTask->ConfigureTask(taskName, plTaskNesting::Never);

  const auto& meshBufferDescriptor = pCpuMesh->GetDescriptor().MeshBufferDesc();
  const auto meshBBox = pCpuMesh->GetDescriptor().GetBounds().GetBox();
  auto vertexColorData = buffer.MapForWriting<plColorLinearUB>(context.m_uiVertexColorOffset);

  plHybridArray<plProcVertexColorMapping, 2> outputMappings;
  for (auto& outputDesc : pComponent->m_OutputDescs)
  {
    outputMappings.PushBack(outputDesc.m_Mapping);
  }

  pUpdateTask->Prepare(*GetWorld(), meshBufferDescriptor, pComponent->GetOwner()->GetGlobalTransform(), meshBBox, pComponent->m_Outputs, outputMappings, vertexColorData);

  plTaskSystem::AddTaskToGroup(m_UpdateTaskGroupID, pUpdateTask);

  ++m_uiNextTaskIndex;
}

void plProcVertexColorComponentManager::EnqueueUpdate(plProcVertexColorComponent& component)
{
  auto& hResource = component.GetResource();
  if (!hResource.IsValid())
  {
    return;
  }

  if (!m_ComponentsToUpdate.Contains(component.GetHandle()))
  {
    m_ComponentsToUpdate.PushBack(component.GetHandle());
  }
}

void plProcVertexColorComponentManager::RemoveComponent(plProcVertexColorComponent& component)
{
  m_ComponentsToUpdate.RemoveAndSwap(component.GetHandle());

  plRenderDataManager* pRenderDataManager = GetWorld()->GetModule<plRenderDataManager>();
  pRenderDataManager->DeleteCustomInstanceData(m_uiCustomDataIndex, component.m_CustomInstanceDataOffset);
}

void plProcVertexColorComponentManager::OnResourceEvent(const plResourceEvent& resourceEvent)
{
  if (resourceEvent.m_Type != plResourceEvent::Type::ResourceContentUnloading || resourceEvent.m_pResource->GetReferenceCount() == 0)
    return;

  if (auto pResource = plDynamicCast<plProcGenGraphResource*>(resourceEvent.m_pResource))
  {
    plProcGenGraphResourceHandle hResource(pResource);

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        EnqueueUpdate(*it);
      }
    }
  }
}

void plProcVertexColorComponentManager::OnAreaInvalidated(const plProcGenInternal::InvalidatedArea& area)
{
  if (area.m_pWorld != GetWorld())
    return;

  plSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = plDefaultSpatialDataCategories::RenderStatic.GetBitmask() | plDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInBox(area.m_Box, queryParams,
    [this](plGameObject* pObject)
    {
      plHybridArray<plProcVertexColorComponent*, 4> components;
      pObject->TryGetComponentsOfBaseType(components);

      for (auto pComponent : components)
      {
        EnqueueUpdate(*pComponent);
      }

      return plVisitorExecution::Continue;
    });
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plProcVertexColorOutputDesc, plNoBase, 1, plRTTIDefaultAllocator<plProcVertexColorOutputDesc>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new plDynamicStringEnumAttribute("ProcGenOutputNameEnum")),
    PL_MEMBER_PROPERTY("Mapping", m_Mapping),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

static plTypeVersion s_ProcVertexColorOutputDescVersion = 1;
plResult plProcVertexColorOutputDesc::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream << m_sName;
  PL_SUCCEED_OR_RETURN(m_Mapping.Serialize(inout_stream));

  return PL_SUCCESS;
}

plResult plProcVertexColorOutputDesc::Deserialize(plStreamReader& inout_stream)
{
  /*plTypeVersion version =*/inout_stream.ReadVersion(s_ProcVertexColorOutputDescVersion);
  inout_stream >> m_sName;
  PL_SUCCEED_OR_RETURN(m_Mapping.Deserialize(inout_stream));

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plProcVertexColorComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_ProcGen_Graph")),
    PL_ARRAY_ACCESSOR_PROPERTY("OutputDescs", OutputDescs_GetCount, GetOutputDesc, SetOutputDesc, OutputDescs_Insert, OutputDescs_Remove),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Construction/Procedural Generation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plProcVertexColorComponent::plProcVertexColorComponent() = default;
plProcVertexColorComponent::~plProcVertexColorComponent() = default;

void plProcVertexColorComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(*this);

  if (GetUniqueID() != plInvalidIndex)
  {
    GetOwner()->EnableStaticTransformChangesNotifications();
  }
}

void plProcVertexColorComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->RemoveComponent(*this);

  // Don't disable notifications as other components attached to the owner game object might need them too.
  // GetOwner()->DisableStaticTransformChangesNotifications();
}

void plProcVertexColorComponent::SetResourceFile(plStringView sFile)
{
  plProcGenGraphResourceHandle hResource;

  if (!sFile.IsEmpty())
  {
    hResource = plResourceManager::LoadResource<plProcGenGraphResource>(sFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

plStringView plProcVertexColorComponent::GetResourceFile() const
{
  return m_hResource.GetResourceID();
}

void plProcVertexColorComponent::SetResource(const plProcGenGraphResourceHandle& hResource)
{
  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

const plProcVertexColorOutputDesc& plProcVertexColorComponent::GetOutputDesc(plUInt32 uiIndex) const
{
  return m_OutputDescs[uiIndex];
}

void plProcVertexColorComponent::SetOutputDesc(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.EnsureCount(uiIndex + 1);
  m_OutputDescs[uiIndex] = outputDesc;

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

void plProcVertexColorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_hResource;
  s.WriteArray(m_OutputDescs).IgnoreResult();
}

void plProcVertexColorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_hResource;
  if (uiVersion >= 2)
  {
    s.ReadArray(m_OutputDescs).IgnoreResult();
  }
  else
  {
    plHybridArray<plHashedString, 2> outputNames;
    s.ReadArray(outputNames).IgnoreResult();

    for (auto& outputName : outputNames)
    {
      auto& outputDesc = m_OutputDescs.ExpandAndGetRef();
      outputDesc.m_sName = outputName;
    }
  }
}

void plProcVertexColorComponent::OnTransformChanged(plMsgTransformChanged& ref_msg)
{
  auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
  pManager->EnqueueUpdate(*this);
}

plUInt32 plProcVertexColorComponent::OutputDescs_GetCount() const
{
  return m_OutputDescs.GetCount();
}

void plProcVertexColorComponent::OutputDescs_Insert(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc)
{
  m_OutputDescs.InsertAt(uiIndex, outputDesc);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

void plProcVertexColorComponent::OutputDescs_Remove(plUInt32 uiIndex)
{
  m_OutputDescs.RemoveAtAndCopy(uiIndex);

  if (IsActiveAndInitialized())
  {
    auto pManager = static_cast<plProcVertexColorComponentManager*>(GetOwningManager());
    pManager->EnqueueUpdate(*this);
  }
}

bool plProcVertexColorComponent::HasValidOutputs() const
{
  for (auto& pOutput : m_Outputs)
  {
    if (pOutput != nullptr && pOutput->m_pByteCode != nullptr)
    {
      return true;
    }
  }

  return false;
}

plMeshComponentBase* plProcVertexColorComponent::GetMeshComponent()
{
  plMeshComponentBase* pMeshComponent = nullptr;
  bool _ = GetOwner()->TryGetComponentOfBaseType(pMeshComponent);
  return pMeshComponent;
}


PL_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVertexColorComponent);