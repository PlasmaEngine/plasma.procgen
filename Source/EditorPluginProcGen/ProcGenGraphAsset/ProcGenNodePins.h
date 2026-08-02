#pragma once

#include <Foundation/Reflection/Reflection.h>

/// Pin for connecting procedural generation nodes.
///
/// Pins represent input or output connections on ProcGen nodes. They are used to pass
/// values like density, position, color, or other data between nodes in the procedural generation graph.
struct plProcGenNodePin
{
  PL_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Input = PL_BIT(0),  ///< Pin accepts input from other nodes.
      Output = PL_BIT(1), ///< Pin provides output to other nodes.

      Default = 0
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
    };
  };

  plBitflags<Type> m_Type;
  plUInt8 m_uiInputIndex = 0xFF;
  plUInt8 m_uiOutputIndex = 0xFF;
  class plProcGenNodeBase* m_pParent = nullptr;
};
PL_DECLARE_FLAGS_OPERATORS(plProcGenNodePin::Type);

/// Input pin for receiving data from other procedural generation nodes.
struct plProcGenNodeInputPin : public plProcGenNodePin
{
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plProcGenNodeInputPin() { m_Type = Type::Input; }
};

/// Output pin for sending data to other procedural generation nodes.
struct plProcGenNodeOutputPin : public plProcGenNodePin
{
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plProcGenNodeOutputPin() { m_Type = Type::Output; }
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plProcGenNodePin);
PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plProcGenNodeInputPin);
PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plProcGenNodeOutputPin);
