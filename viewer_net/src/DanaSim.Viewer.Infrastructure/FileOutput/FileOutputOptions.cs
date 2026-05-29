namespace DanaSim.Viewer.Infrastructure.FileOutput;

public sealed class FileOutputOptions
{
    /// <summary>Root directory where per-scenario output folders are created.</summary>
    public string OutputDir { get; set; } = "outputs/x3d_heightmap";

    /// <summary>
    /// Path to python/visualizer/tools/x3d_player/static — the JS/CSS assets
    /// that are copied next to player.html.  Leave empty to skip the copy step.
    /// </summary>
    public string JsAssetsSourcePath { get; set; } = "";
}
