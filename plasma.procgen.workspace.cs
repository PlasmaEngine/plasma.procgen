// #pl-version 1

using PlasmaBuild.Core.Configuration;
using PlasmaBuild.Core.Rules;

public class PlasmaProcGenWorkspace : WorkspaceRules
{
    public PlasmaProcGenWorkspace(BuildContext context) : base(context)
    {
        TargetNames.Add("ProcGenPlugin");
        TargetNames.Add("EditorPluginProcGen");
    }
}
