using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class ProcGenPluginModule : ModuleRules
{
    public ProcGenPluginModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/ProcGenPlugin", "ProcGenPluginPCH.h",
            "BUILDSYSTEM_BUILDING_PROCGENPLUGIN_LIB");

        // This package's own shader headers, shared between C++ and HLSL, plus the engine's under
        // the SDK's base data directory. Both have to be reachable by the same name from either side.
        PrivateIncludePaths.Add(Path.Combine(context.ProjectRoot, "Assets"));
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));
    }
}
