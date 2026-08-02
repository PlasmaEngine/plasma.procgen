#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Components/ProcPlacementComponent.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

using namespace plProcGenInternal;

PlacementTile::PlacementTile()
  : m_pOutput(nullptr)

{
}

PlacementTile::PlacementTile(PlacementTile&& other)
{
  m_Desc = other.m_Desc;
  m_pOutput = other.m_pOutput;

  m_State = other.m_State;
  other.m_State = State::Invalid;

  m_PlacedObjects = std::move(other.m_PlacedObjects);
  m_RenderGroups = std::move(other.m_RenderGroups);
}

PlacementTile::~PlacementTile()
{
  PL_ASSERT_DEV(m_State == State::Invalid, "Implementation error");
}

void PlacementTile::Initialize(const PlacementTileDesc& desc, plSharedPtr<const PlacementOutput>& ref_pOutput)
{
  m_Desc = desc;
  m_pOutput = ref_pOutput;

  m_State = State::Initialized;
}

void PlacementTile::Deinitialize(plWorld& ref_world)
{
  for (auto hObject : m_PlacedObjects)
  {
    ref_world.DeleteObjectDelayed(hObject);
  }
  m_PlacedObjects.Clear();

  if (plRenderDataManager* pRenderDataManager = ref_world.GetModule<plRenderDataManager>())
  {
    for (auto& group : m_RenderGroups)
    {
      pRenderDataManager->DeleteInstanceData(group.m_InstanceDataOffset);
    }
  }
  m_RenderGroups.Clear();

  m_Desc.m_hComponent.Invalidate();
  m_pOutput = nullptr;
  m_State = State::Invalid;
}

bool PlacementTile::IsValid() const
{
  return !m_Desc.m_hComponent.IsInvalidated() && m_pOutput != nullptr;
}

const PlacementTileDesc& PlacementTile::GetDesc() const
{
  return m_Desc;
}

const PlacementOutput* PlacementTile::GetOutput() const
{
  return m_pOutput;
}

plArrayPtr<const plGameObjectHandle> PlacementTile::GetPlacedObjects() const
{
  return m_PlacedObjects;
}

plBoundingBox PlacementTile::GetBoundingBox() const
{
  return m_Desc.GetBoundingBox();
}

plColor PlacementTile::GetDebugColor() const
{
  switch (m_State)
  {
    case State::Initialized:
      return plColor::Orange;
    case State::Scheduled:
      return plColor::Yellow;
    case State::Finished:
      return plColor::Green;
    default:
      return plColor::DarkRed;
  }
}

void PlacementTile::PreparePlacementData(const plWorld* pWorld, const plPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& ref_placementData)
{
  const plUInt64 uiOutputNameHash = m_pOutput->m_sName.GetHash();
  plUInt32 hashData[] = {
    static_cast<plUInt32>(m_Desc.m_iPosX),
    static_cast<plUInt32>(m_Desc.m_iPosY),
    static_cast<plUInt32>(uiOutputNameHash),
    static_cast<plUInt32>(uiOutputNameHash >> 32),
  };

  ref_placementData.m_pPhysicsModule = pPhysicsModule;
  ref_placementData.m_pWorld = pWorld;
  ref_placementData.m_pOutput = m_pOutput;
  ref_placementData.m_uiTileSeed = plHashingUtils::xxHash32(hashData, sizeof(hashData));
  ref_placementData.m_TileBoundingBox = GetBoundingBox();
  ref_placementData.m_GlobalToLocalBoxTransforms = m_Desc.m_GlobalToLocalBoxTransforms;

  m_State = State::Scheduled;
}

plUInt32 PlacementTile::PlaceObjects(plWorld& ref_world, plArrayPtr<const PlacementTransform> objectTransforms)
{
  PL_PROFILE_SCOPE("PlacementTile::PlaceObjects");

  plGameObjectDesc desc;
  auto& objectsToPlace = m_pOutput->m_ObjectsToPlace;

  plHybridArray<plPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());



  for (auto& objectTransform : objectTransforms)
  {
    const plUInt32 uiObjectIndex = objectTransform.m_uiObjectIndex;
    plPrefabResource* pPrefab = prefabs[uiObjectIndex];

    if (pPrefab == nullptr)
    {
      pPrefab = plResourceManager::BeginAcquireResource(objectsToPlace[uiObjectIndex], plResourceAcquireMode::BlockTillLoaded);
      prefabs[uiObjectIndex] = pPrefab;
    }

    plTransform transform = plSimdConversion::ToTransform(objectTransform.m_Transform);
    plHybridArray<plGameObject*, 8> rootObjects;

    plPrefabInstantiationOptions options;
    options.m_pCreatedRootObjectsOut = &rootObjects;

    pPrefab->InstantiatePrefab(ref_world, transform, options);

    // only send the color message, if we actually have a custom color
    if (objectTransform.m_bHasValidColor)
    {
      for (auto pRootObject : rootObjects)
      {
        // Set the color
        plMsgSetColor msg;
        msg.m_Color = objectTransform.m_ObjectColor.ToLinearFloat();
        pRootObject->PostMessageRecursive(msg, plTime::MakeZero(), plObjectMsgQueueType::AfterInitialized);
      }
    }

    for (auto pRootObject : rootObjects)
    {
      m_PlacedObjects.PushBack(pRootObject->GetHandle());
    }
  }

  for (auto pPrefab : prefabs)
  {
    if (pPrefab != nullptr)
    {
      plResourceManager::EndAcquireResource(pPrefab);
    }
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}

plUInt32 PlacementTile::PlaceRenderData(plArrayPtr<const PlacementTransform> objectTransforms, plArrayPtr<const FlattenedPrefabRenderData> flattenedPrefabs)
{
  PL_PROFILE_SCOPE("PlacementTile::PlaceRenderData");

  PL_ASSERT_DEV(m_RenderGroups.IsEmpty(), "Render data was already placed for this tile");

  auto FindOrAddGroup = [this](const FlattenedPrefabRenderEntry& entry) -> RenderGroup& {
    for (auto& group : m_RenderGroups)
    {
      if (group.m_hMesh == entry.m_hMesh && group.m_hMaterial == entry.m_hMaterial && group.m_uiSubMeshIndex == entry.m_uiSubMeshIndex && group.m_uiMaterialSlotIndex == entry.m_uiMaterialSlotIndex)
      {
        return group;
      }
    }

    auto& group = m_RenderGroups.ExpandAndGetRef();
    group.m_hMesh = entry.m_hMesh;
    group.m_hMaterial = entry.m_hMaterial;
    group.m_uiMaterialSlotIndex = entry.m_uiMaterialSlotIndex;
    group.m_uiSubMeshIndex = entry.m_uiSubMeshIndex;
    return group;
  };

  for (const auto& objectTransform : objectTransforms)
  {
    const plUInt32 uiObjectIndex = objectTransform.m_uiObjectIndex;
    if (uiObjectIndex >= flattenedPrefabs.GetCount())
      continue;

    const auto& prefabRenderData = flattenedPrefabs[uiObjectIndex];
    const plTransform placementTransform = plSimdConversion::ToTransform(objectTransform.m_Transform);

    for (const auto& entry : prefabRenderData.m_Entries)
    {
      auto& group = FindOrAddGroup(entry);
      auto& instance = group.m_Instances.ExpandAndGetRef();

      instance.m_GlobalTransform = plTransform::MakeGlobalTransform(placementTransform, entry.m_LocalTransform);
      instance.m_Color = objectTransform.m_bHasValidColor ? objectTransform.m_ObjectColor.ToLinearFloat() : entry.m_Color;
      instance.m_vCustomData = entry.m_vCustomData;

      instance.m_fBoundingSphereRadius = entry.m_fBoundingSphereRadius * plMath::Max(instance.m_GlobalTransform.GetMaxScale(), 1.0f);

      plUInt32 hashData[] = {
        static_cast<plUInt32>(m_Desc.m_iPosX),
        static_cast<plUInt32>(m_Desc.m_iPosY),
        objectTransform.m_uiPointIndex,
        uiObjectIndex,
        group.m_uiSubMeshIndex,
      };
      instance.m_uiRandomSeed = plHashingUtils::xxHash32(hashData, sizeof(hashData));
    }
  }

  m_State = State::Finished;

  plUInt32 uiNumInstances = 0;
  for (const auto& group : m_RenderGroups)
  {
    uiNumInstances += group.m_Instances.GetCount();
  }

  return uiNumInstances;
}

void PlacementTile::ExtractRenderData(const plProcPlacementComponent& ownerComponent, plMsgExtractRenderData& ref_msg) const
{
  if (m_RenderGroups.IsEmpty())
    return;

  const plUInt32 uiOwnerUniqueId = plRenderComponent::GetUniqueIdForRendering(ownerComponent);
  const bool bDynamic = true;

  plUInt32 uiVisibleInstances = 0;
  plBoundingBoxSphere tileBounds = plBoundingBoxSphere::MakeFromBox(GetBoundingBox());

  for (const auto& group : m_RenderGroups)
  {
    if (group.m_Instances.IsEmpty())
      continue;

    plGALDynamicBufferHandle hInstanceDataBuffer;
    auto instanceData = ref_msg.m_pRenderDataManager->GetOrCreateInstanceData(&ownerComponent, bDynamic, hInstanceDataBuffer, group.m_InstanceDataOffset, group.m_Instances.GetCount());

    for (plUInt32 i = 0; i < group.m_Instances.GetCount(); ++i)
    {
      const auto& instance = group.m_Instances[i];
      plRenderDataManager::FillPerInstanceData(instanceData[i], nullptr, instance.m_GlobalTransform, uiOwnerUniqueId, instance.m_Color, instance.m_vCustomData, instance.m_fBoundingSphereRadius, instance.m_uiRandomSeed);
    }

    plMeshRenderData* pRenderData = ref_msg.m_pRenderDataManager->CreateRenderDataForThisFrame<plMeshRenderData>(ownerComponent.GetOwner());
    pRenderData->m_Flags.Add(plRenderData::Flags::Dynamic);
    pRenderData->m_vGlobalPosition = GetBoundingBox().GetCenter();
    pRenderData->SetFallbackGlobalBounds(tileBounds);
    pRenderData->Fill(group.m_InstanceDataOffset, hInstanceDataBuffer, group.m_hMaterial, group.m_hMesh, group.m_uiMaterialSlotIndex, group.m_uiSubMeshIndex, group.m_Instances.GetCount());

    bool bDontCacheYet = false;
    plRenderData::Category category = plMaterialResource::GetRenderDataCategory(group.m_hMaterial, &bDontCacheYet);
    ref_msg.AddRenderData(pRenderData, category, plRenderData::Caching::Never);

    uiVisibleInstances += group.m_Instances.GetCount();
  }

  PL_IGNORE_UNUSED(uiVisibleInstances);
}

plUInt32 PlacementTile::GetRenderInstanceCount() const
{
  plUInt32 uiNumInstances = 0;
  for (const auto& group : m_RenderGroups)
  {
    uiNumInstances += group.m_Instances.GetCount();
  }

  return uiNumInstances;
}

plUInt32 PlacementTile::GetRenderGroupCount() const
{
  return m_RenderGroups.GetCount();
}
