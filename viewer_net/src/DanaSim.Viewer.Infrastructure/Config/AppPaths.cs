namespace DanaSim.Viewer.Infrastructure.Config;

/// <summary>
/// User-writable paths for the app's data and config files.
/// On Linux: ~/.config/danasim-viewer/
/// On Windows: %APPDATA%\danasim-viewer\
/// </summary>
public static class AppPaths
{
    private static readonly string Base = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "danasim-viewer");

    public static string UserConfigFile => Path.Combine(Base, "user-config.json");
    public static string LogsDirectory  => Path.Combine(Base, "logs");

    public static void EnsureBaseExists() => Directory.CreateDirectory(Base);
}
