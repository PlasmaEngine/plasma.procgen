using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class ProcGenPluginTarget : TargetRules
{
    public ProcGenPluginTarget(BuildContext context) : base(context)
    {
        Type = TargetType.SharedLibrary;
        OutputName = "plProcGenPlugin";
        OutputDirectory = PlasmaPackageSdk.PackageBinaryDirectory(context);
        UsePCHFiles = true;
        UseUnityBuild = true;
        UseAdaptiveUnityBuild = true;
        UseIncrementalLinking = true;
        ExtraModules.Add("ProcGenPluginModule");
    }
}
