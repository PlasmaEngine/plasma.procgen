#pragma once

#include <Core/Interfaces/VegetationWorldModule.h>
#include <Core/World/Declarations.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Declarations.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class plPhysicsWorldModuleInterface;
class plProcPlacementComponent;
struct plMsgExtractRenderData;

namespace plProcGenInternal
{
  struct FlattenedPrefabRenderEntry
  {
    plTransform m_LocalTransform = plTransform::MakeIdentity();
    plMeshResourceHandle m_hMesh;
    plMaterialResourceHandle m_hMaterial;
    plUInt32 m_uiMaterialSlotIndex = 0;
    plUInt32 m_uiSubMeshIndex = 0;
    plColor m_Color = plColor::White;
    plVec4 m_vCustomData = plVec4(0, 1, 0, 1);
    float m_fBoundingSphereRadius = 1.0f;
  };

  struct FlattenedPrefabRenderData
  {
    plDynamicArray<FlattenedPrefabRenderEntry> m_Entries;
    plString m_sFallbackReason;
  };

  class PlacementTile
  {
  public:
    PlacementTile();
    PlacementTile(PlacementTile&& other);
    ~PlacementTile();

    void Initialize(const PlacementTileDesc& desc, plSharedPtr<const PlacementOutput>& ref_pOutput);
    void Deinitialize(plWorld& ref_world);

    bool IsValid() const;

    const PlacementTileDesc& GetDesc() const;
    const PlacementOutput* GetOutput() const;
    plArrayPtr<const plGameObjectHandle> GetPlacedObjects() const;
    plBoundingBox GetBoundingBox() const;
    plColor GetDebugColor() const;

    void PreparePlacementData(const plWorld* pWorld, const plPhysicsWorldModuleInterface* pPhysicsModule, PlacementData& ref_placementData);

    plUInt32 PlaceObjects(plWorld& ref_world, plArrayPtr<const PlacementTransform> objectTransforms);
    plUInt32 PlaceRenderData(plArrayPtr<const PlacementTransform> objectTransforms, plArrayPtr<const FlattenedPrefabRenderData> flattenedPrefabs);
    /// \brief Hands the transforms to the world's vegetation module, one tile per vegetation type. Returns 0 without a module.
    plUInt32 PlaceVegetation(plWorld& ref_world, plArrayPtr<const PlacementTransform> objectTransforms);
    void ExtractRenderData(const plProcPlacementComponent& ownerComponent, plMsgExtractRenderData& ref_msg) const;
    plUInt32 GetRenderInstanceCount() const;
    plUInt32 GetRenderGroupCount() const;

  private:
    PlacementTileDesc m_Desc;
    plSharedPtr<const PlacementOutput> m_pOutput;

    struct State
    {
      enum Enum
      {
        Invalid,
        Initialized,
        Scheduled,
        Finished
      };
    };

    State::Enum m_State = State::Invalid;
    plDynamicArray<plGameObjectHandle> m_PlacedObjects;
    plHybridArray<plVegetationTileHandle, 4> m_VegetationTiles;

    struct RenderInstance
    {
      plTransform m_GlobalTransform = plTransform::MakeIdentity();
      plColor m_Color = plColor::White;
      plVec4 m_vCustomData = plVec4(0, 1, 0, 1);
      float m_fBoundingSphereRadius = 1.0f;
      plUInt32 m_uiRandomSeed = 0;
    };

    struct RenderGroup
    {
      plMeshResourceHandle m_hMesh;
      plMaterialResourceHandle m_hMaterial;
      plUInt32 m_uiMaterialSlotIndex = 0;
      plUInt32 m_uiSubMeshIndex = 0;
      plDynamicArray<RenderInstance> m_Instances;
      mutable plInstanceDataOffset m_InstanceDataOffset;
    };

    plDynamicArray<RenderGroup> m_RenderGroups;
  };
} // namespace plProcGenInternal
