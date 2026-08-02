using System.IO;
using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class EditorPluginProcGenModule : ModuleRules
{
    public EditorPluginProcGenModule(BuildContext context) : base(context)
    {
        PlasmaPackageSdk.ConfigurePackageModule(this, context, "Source/EditorPluginProcGen", "EditorPluginProcGenPCH.h",
            "BUILDSYSTEM_BUILDING_EDITORPLUGINPROCGEN_LIB");

        // Shader headers from the engine's base data directory - in the engine tree this was a
        // ProjectRoot-relative path, here it resolves against the SDK.
        PrivateIncludePaths.Add(Path.Combine(PlasmaPackageSdk.Root, "Data", "Base"));

        PlasmaPackageSdk.ConfigureEditorSdkConsumer(this, context);
        PlasmaPackageSdk.AddPackagePluginImport(this, context, "Source/ProcGenPlugin", "plProcGenPlugin");

        PlasmaPackageQt.ConfigureQtModule(this, context, "EditorPluginProcGenModule",
            "Source/EditorPluginProcGen", "Core", "Gui", "Widgets");
    }
}
