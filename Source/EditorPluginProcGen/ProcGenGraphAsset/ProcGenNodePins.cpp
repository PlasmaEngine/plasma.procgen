#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenNodePins.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plProcGenNodePin, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plProcGenNodeInputPin, plProcGenNodePin, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plProcGenNodeOutputPin, plProcGenNodePin, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on
