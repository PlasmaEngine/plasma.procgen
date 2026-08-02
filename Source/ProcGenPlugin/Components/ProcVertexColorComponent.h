#pragma once

#include <Core/World/World.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <RendererCore/Pipeline/RenderData.h>

class plMeshComponentBase;
class plProcVertexColorComponent;
using plCpuMeshResourceHandle = plTypedResourceHandle<class plCpuMeshResource>;

class PL_PROCGENPLUGIN_DLL plProcVertexColorComponentManager : public plComponentManager<plProcVertexColorComponent, plBlockStorageType::Compact>
{
  PL_DISALLOW_COPY_AND_ASSIGN(plProcVertexColorComponentManager);

public:
  plProcVertexColorComponentManager(plWorld* pWorld);
  ~plProcVertexColorComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class plProcVertexColorComponent;

  struct UpdateContext
  {
    plProcVertexColorComponent* m_pComponent = nullptr;
    plCpuMeshResourceHandle m_hCpuMesh;
    plUInt32 m_uiVertexColorOffset = 0;
  };

  void UpdateVertexColors(const plWorldModule::UpdateContext& context);
  bool UpdateComponentOutputs(plProcVertexColorComponent& component);
  void UpdateComponentVertexColors(const UpdateContext& context, plGALDynamicBuffer& buffer);

  void EnqueueUpdate(plProcVertexColorComponent& component);
  void RemoveComponent(plProcVertexColorComponent& component);

  void OnResourceEvent(const plResourceEvent& resourceEvent);

  void OnAreaInvalidated(const plProcGenInternal::InvalidatedArea& area);

  plDynamicArray<plComponentHandle> m_ComponentsToUpdate;
  plDynamicArray<UpdateContext> m_UpdateContexts;

  plDynamicArray<plSharedPtr<plProcGenInternal::VertexColorTask>> m_UpdateTasks;
  plTaskGroupID m_UpdateTaskGroupID;
  plUInt32 m_uiNextTaskIndex = 0;

  plUInt32 m_uiCustomDataIndex = 0;
};

//////////////////////////////////////////////////////////////////////////

struct plProcVertexColorOutputDesc
{
  plHashedString m_sName;
  plProcVertexColorMapping m_Mapping;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PROCGENPLUGIN_DLL, plProcVertexColorOutputDesc);

//////////////////////////////////////////////////////////////////////////

struct plMsgTransformChanged;

class PL_PROCGENPLUGIN_DLL plProcVertexColorComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProcVertexColorComponent, plComponent, plProcVertexColorComponentManager);

public:
  plProcVertexColorComponent();
  ~plProcVertexColorComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(plStringView sFile);
  plStringView GetResourceFile() const;

  void SetResource(const plProcGenGraphResourceHandle& hResource);
  const plProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

  const plProcVertexColorOutputDesc& GetOutputDesc(plUInt32 uiIndex) const;
  void SetOutputDesc(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnTransformChanged(plMsgTransformChanged& ref_msg);

private:
  plUInt32 OutputDescs_GetCount() const;
  void OutputDescs_Insert(plUInt32 uiIndex, const plProcVertexColorOutputDesc& outputDesc);
  void OutputDescs_Remove(plUInt32 uiIndex);

  bool HasValidOutputs() const;

  plMeshComponentBase* GetMeshComponent();

  plProcGenGraphResourceHandle m_hResource;
  plSmallArray<plProcVertexColorOutputDesc, 1> m_OutputDescs;

  plSmallArray<plSharedPtr<const plProcGenInternal::VertexColorOutput>, 1> m_Outputs;

  plCustomInstanceDataOffset m_CustomInstanceDataOffset;
};