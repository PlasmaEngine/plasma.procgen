using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class ProcGenPluginModule : ModuleRules
{
    public ProcGenPluginModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/ProcGenPlugin", "ProcGenPluginPCH.h",
            "BUILDSYSTEM_BUILDING_PROCGENPLUGIN_LIB");

        // Shader headers from the engine's base data directory - in the engine tree this was a
        // ProjectRoot-relative path, here it resolves against the SDK.
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));
    }
}
