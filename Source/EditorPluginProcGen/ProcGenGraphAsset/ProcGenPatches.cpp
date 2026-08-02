#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

/// Migrates ProcGen nodes from using RenderPipeline pin types to ProcGen-specific pin types.
///
/// This patch processes all ProcGen node types and updates their pin property types:
/// - plRenderPipelineNodeInputPin -> plProcGenNodeInputPin
/// - plRenderPipelineNodeOutputPin -> plProcGenNodeOutputPin
class plProcGenPinTypePatch_1_2 : public plGraphPatch
{
public:
  plProcGenPinTypePatch_1_2()
    : plGraphPatch("", 2, PatchType::GraphPatch)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // Iterate through all nodes in the graph
    for (auto it = pGraph->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      plAbstractObjectNode* pCurrentNode = it.Value();
      const plStringView sType = pCurrentNode->GetType();

      // Only process ProcGen node types
      if (sType.StartsWith("plProcGen"))
      {
        // Iterate through all properties of this node
        auto& properties = pCurrentNode->GetProperties();
        for (plUInt32 i = 0; i < properties.GetCount(); ++i)
        {
          auto& prop = properties[i];

          // Check if this property is a struct with a RenderPipeline pin type
          if (prop.m_Value.IsA<plUuid>())
          {
            plUuid propertyObjectGuid = prop.m_Value.Get<plUuid>();
            plAbstractObjectNode* pPropertyNode = pGraph->GetNode(propertyObjectGuid);

            if (pPropertyNode)
            {
              const plStringView sPropertyType = pPropertyNode->GetType();

              // Rename RenderPipeline pin types to ProcGen pin types
              if (sPropertyType == "plRenderPipelineNodeInputPin")
              {
                pPropertyNode->SetType("plProcGenNodeInputPin");
              }
              else if (sPropertyType == "plRenderPipelineNodeOutputPin")
              {
                pPropertyNode->SetType("plProcGenNodeOutputPin");
              }
              else if (sPropertyType == "plRenderPipelineNodePin")
              {
                pPropertyNode->SetType("plProcGenNodePin");
              }
            }
          }
        }
      }
    }
  }
};

plProcGenPinTypePatch_1_2 g_plProcGenPinTypePatch_1_2;
