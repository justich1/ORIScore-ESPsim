namespace ORIScore.ESPsim.App;

public static class AppPaths
{
    public const string DefaultRepositoryUrl = "https://www.oris-core.cz/repo-sim/";

    public static string BaseDir => AppContext.BaseDirectory;
    public static string BoardsDir => Path.Combine(BaseDir, "boards");
    public static string LibrariesDir => Path.Combine(BaseDir, "libraries");
    public static string LibraryRepoDir => Path.Combine(LibrariesDir, "repo");
    public static string LibraryRepoOnlineDir => Path.Combine(LibraryRepoDir, "online");
    public static string LibraryInstalledDir => Path.Combine(LibrariesDir, "installed");
    public static string LibraryStubsDir => Path.Combine(LibrariesDir, "stubs");
    public static string DevicesDir => Path.Combine(BaseDir, "devices");
    public static string ProjectsDir => Path.Combine(BaseDir, "projects");
    public static string BuildsDir => Path.Combine(BaseDir, "builds");
    public static string CurrentBuildDir => Path.Combine(BuildsDir, "Current");
    public static string ConfigsDir => Path.Combine(BaseDir, "configs");
    public static string LogsDir => Path.Combine(BaseDir, "logs");
    public static string RunnerDir => Path.Combine(BaseDir, "runner");
    public static string DeviceFsDir => Path.Combine(BaseDir, "devicefs");

    public static void EnsurePortableLayout()
    {
        foreach (string dir in new[]
        {
            BoardsDir,
            LibraryRepoDir,
            LibraryRepoOnlineDir,
            LibraryInstalledDir,
            LibraryStubsDir,
            DevicesDir,
            ProjectsDir,
            BuildsDir,
            ConfigsDir,
            LogsDir,
            RunnerDir,
            DeviceFsDir
        })
        {
            Directory.CreateDirectory(dir);
        }
    }

    public static string FindDirectory(string preferred, params string[] legacyRelativeCandidates)
    {
        if (Directory.Exists(preferred))
            return preferred;

        var dir = new DirectoryInfo(BaseDir);
        for (int up = 0; up < 10 && dir != null; up++, dir = dir.Parent)
        {
            foreach (string rel in legacyRelativeCandidates)
            {
                string candidate = Path.GetFullPath(Path.Combine(dir.FullName, rel));
                if (Directory.Exists(candidate))
                    return candidate;
            }
        }

        return preferred;
    }
}
