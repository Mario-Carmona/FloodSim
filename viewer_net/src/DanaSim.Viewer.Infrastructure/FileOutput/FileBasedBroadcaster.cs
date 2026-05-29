using System.Text.Json;
using DanaSim.Viewer.Domain.Enums;
using DanaSim.Viewer.Domain.Ports;
using DanaSim.Viewer.Domain.ValueObjects;
using DanaSim.Viewer.Infrastructure.Mqtt;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.PixelFormats;

namespace DanaSim.Viewer.Infrastructure.FileOutput;

/// <summary>
/// Output adapter that writes flood PNGs + manifest.json + player.html to disk,
/// mirroring the output of python/visualizer/renderers/x3d/x3d_renderer.py.
/// </summary>
public sealed class FileBasedBroadcaster(
    IOptions<FileOutputOptions> options,
    IOptions<MqttOptions> mqttOptions,
    ILogger<FileBasedBroadcaster> logger) : ISimulationBroadcaster
{
    private const float Nodata = -9999f;

    private readonly string _outputBaseDir = options.Value.OutputDir;
    private readonly string _jsAssetsSourcePath = options.Value.JsAssetsSourcePath;
    private readonly string _scenario = mqttOptions.Value.Scenario;

    private string _scenarioDir = "";
    private string _floodDir = "";
    private GridMeta? _meta;
    private float _minH;
    private float _maxH;
    private readonly List<string> _frameNames = [];

    // ── ISimulationBroadcaster ────────────────────────────────────────────────

    public async Task BroadcastInitialStateAsync(GridMeta meta, FrameData frame, CancellationToken ct)
    {
        _meta = meta;
        _frameNames.Clear();
        _scenarioDir = Path.Combine(_outputBaseDir, _scenario);
        _floodDir    = Path.Combine(_scenarioDir, "flood");
        Directory.CreateDirectory(_scenarioDir);
        Directory.CreateDirectory(_floodDir);

        var (terrainPng, minH, maxH) = EncodeTerrain(meta);
        _minH = minH;
        _maxH = maxH;

        CopyJsAssets(_scenarioDir);

        var cssPath = Path.Combine(_jsAssetsSourcePath, "css", "style.css");
        var css     = File.Exists(cssPath) ? await File.ReadAllTextAsync(cssPath, ct) : "";

        var playerHtml = GeneratePlayerHtml(meta, minH, maxH,
            Convert.ToBase64String(terrainPng), css, []);
        await File.WriteAllTextAsync(Path.Combine(_scenarioDir, "player.html"), playerHtml, ct);

        await WriteManifestAsync(live: true, ct);

        logger.LogInformation(
            "FileBasedBroadcaster: output ready at '{Dir}' (heights {MinH:F1}–{MaxH:F1} m)",
            _scenarioDir, minH, maxH);
    }

    public async Task BroadcastFrameUpdateAsync(FrameData frame, int stepIndex, CancellationToken ct)
    {
        if (_meta is null) return;

        var stepName = $"step_{stepIndex:D5}";
        var floodPng = EncodeFlood(frame.PaletteGrid, _meta.Rows, _meta.Cols);
        await File.WriteAllBytesAsync(Path.Combine(_floodDir, $"{stepName}.png"), floodPng, ct);

        _frameNames.Add(stepName);
        await WriteManifestAsync(live: true, ct);

        logger.LogDebug("Flood PNG saved: {Step} ({Kb:F1} KB)", stepName, floodPng.Length / 1024.0);
    }

    public async Task BroadcastSimulationEndedAsync(CancellationToken ct)
    {
        await WriteManifestAsync(live: false, ct);
        logger.LogInformation(
            "Simulation ended — {Count} frames written to '{Dir}'", _frameNames.Count, _scenarioDir);
    }

    // ── PNG encoding ──────────────────────────────────────────────────────────

    private static (byte[] png, float minH, float maxH) EncodeTerrain(GridMeta meta)
    {
        var heights = meta.TerrainHeights ?? new float[meta.Rows * meta.Cols];

        float minH = float.MaxValue, maxH = float.MinValue;
        foreach (var h in heights)
        {
            if (h > Nodata) { minH = MathF.Min(minH, h); maxH = MathF.Max(maxH, h); }
        }
        if (minH >= maxH || minH == float.MaxValue) { minH = 0f; maxH = 1f; }

        using var img = new Image<L8>(meta.Cols, meta.Rows);
        for (int r = 0; r < meta.Rows; r++)
            for (int c = 0; c < meta.Cols; c++)
            {
                var h = heights[r * meta.Cols + c];
                float norm = h > Nodata
                    ? Math.Clamp((h - minH) / (maxH - minH), 0f, 1f)
                    : 0f;
                img[c, r] = new L8((byte)(norm * 255));
            }

        using var ms = new MemoryStream();
        img.SaveAsPng(ms);
        return (ms.ToArray(), minH, maxH);
    }

    private static byte[] EncodeFlood(FloodState[] paletteGrid, int rows, int cols)
    {
        using var img = new Image<L8>(cols, rows);
        for (int r = 0; r < rows; r++)
            for (int c = 0; c < cols; c++)
                img[c, r] = new L8((byte)paletteGrid[r * cols + c]);

        using var ms = new MemoryStream();
        img.SaveAsPng(ms);
        return ms.ToArray();
    }

    // ── Manifest ──────────────────────────────────────────────────────────────

    private async Task WriteManifestAsync(bool live, CancellationToken ct)
    {
        var obj  = new { frames = _frameNames.ToArray(), live };
        var json = JsonSerializer.Serialize(obj, new JsonSerializerOptions { WriteIndented = true });
        await File.WriteAllTextAsync(Path.Combine(_floodDir, "manifest.json"), json, ct);
    }

    // ── JS / CSS assets ───────────────────────────────────────────────────────

    private void CopyJsAssets(string outputDir)
    {
        if (string.IsNullOrWhiteSpace(_jsAssetsSourcePath)) return;

        var src = Path.Combine(_jsAssetsSourcePath, "js");
        var dst = Path.Combine(outputDir, "js");

        if (!Directory.Exists(src)) { logger.LogWarning("JS assets not found: {Path}", src); return; }
        if (Directory.Exists(dst))  Directory.Delete(dst, recursive: true);

        CopyDirectoryRecursive(src, dst);
    }

    private static void CopyDirectoryRecursive(string src, string dst)
    {
        Directory.CreateDirectory(dst);
        foreach (var f in Directory.GetFiles(src))
            File.Copy(f, Path.Combine(dst, Path.GetFileName(f)), overwrite: true);
        foreach (var d in Directory.GetDirectories(src))
            CopyDirectoryRecursive(d, Path.Combine(dst, Path.GetFileName(d)));
    }

    // ── player.html generation ────────────────────────────────────────────────

    private static string GeneratePlayerHtml(
        GridMeta meta, float minH, float maxH,
        string terrainB64, string css, string[] frameNames)
    {
        // Matches python/visualizer/renderers/x3d/colors.py defaults
        string[] stateColors =
        [
            "0.96 0.87 0.70",  // 0 Dry
            "0.60 0.80 1.00",  // 1 Risk1 very shallow
            "0.39 0.59 0.86",  // 2 Risk2 low depth
            "0.12 0.39 0.78",  // 3 Risk3 medium depth
            "0.00 0.20 0.71",  // 4 Risk4 high depth
            "0.29 0.00 0.51",  // 5 Risk5 extreme depth
        ];

        float mapW = meta.Cols * meta.CellSizeM;
        float mapD = meta.Rows * meta.CellSizeM;
        float camH = MathF.Max(maxH * 3.0f, mapD * 0.3f);

        string ovPos  = $"{mapW / 2:F0} {camH:F0} {mapD + mapD * 0.4f:F0}";
        string cenPos = $"{mapW / 2:F0} {camH * 2:F0} {mapD / 2:F0}";
        string latPos = $"{mapW / 2:F0} {camH * 0.35f:F0} {mapD * 2.2f:F0}";

        var configObj = new
        {
            pngCols    = meta.Cols,
            pngRows    = meta.Rows,
            pngRes     = meta.CellSizeM,
            mapW,
            mapD,
            minH,
            maxH,
            stateColors,
            floodFrames = frameNames,
            terrainSrc  = $"data:image/png;base64,{terrainB64}",
        };
        var configJson = JsonSerializer.Serialize(configObj);

        int nFrames   = frameNames.Length;
        int sliderMax = Math.Max(0, nFrames - 1);

        // $$""" = C# raw string where {{ }} is interpolation, { } is literal
        return $$"""
            <!DOCTYPE html>
            <html lang="en">
            <head>
              <meta charset="UTF-8"/>
              <title>DanaSim Heightmap Viewer</title>
              <script src="https://cdn.jsdelivr.net/npm/x_ite@12.1.4/dist/x_ite.min.js"></script>
              <style>
            {{css}}
              </style>
            </head>
            <body>
              <!-- rows={{meta.Rows}} cols={{meta.Cols}} cell_size={{meta.CellSizeM:F4}}m frames={{nFrames}} -->
              <div id="loading">Decoding heightmap and building 3D terrain&#8230;</div>
              <div id="viewport">
                <x3d-canvas>
                  <x3d>
                    <scene>
                      <background skyColor="0.53 0.81 0.98"></background>
                      <Viewpoint DEF="VP_Overview" position="{{ovPos}}"  orientation="1 0 0 -0.5"    description="Overview"></Viewpoint>
                      <Viewpoint DEF="VP_Cenital"  position="{{cenPos}}" orientation="1 0 0 -1.5708" description="Cenital"></Viewpoint>
                      <Viewpoint DEF="VP_Lateral"  position="{{latPos}}" orientation="0 0 0 1"       description="Lateral"></Viewpoint>
                      <Viewpoint DEF="VP_Zone" position="0 1 1" orientation="1 0 0 -0.5" description="Zona seleccionada"></Viewpoint>
                      <NavigationInfo type='"EXAMINE" "ANY"' speed="500"></NavigationInfo>
                      <Transform DEF="ZScale" scale="1 1 1">
                        <Group id="terrain_container"></Group>
                      </Transform>
                    </scene>
                  </x3d>
                </x3d-canvas>

                <aside id="sidebar" aria-label="Panel de control">
                  <button id="sidebarToggle" type="button" aria-label="Mostrar/ocultar panel">&#9776;</button>
                  <div class="sidebar-content">
                    <h2>Capas</h2>
                    <label class="layer-row"><input id="layerTerrain" type="checkbox" checked/><span>Terreno</span></label>
                    <label class="layer-row"><input id="layerWater"   type="checkbox" checked/><span>Agua</span></label>
                    <fieldset class="state-group">
                      <legend>Profundidad</legend>
                      <label class="layer-row"><input id="layerState1" type="checkbox" checked/><span class="swatch" data-state="1"></span><span>Muy somero</span></label>
                      <label class="layer-row"><input id="layerState2" type="checkbox" checked/><span class="swatch" data-state="2"></span><span>Profundidad baja</span></label>
                      <label class="layer-row"><input id="layerState3" type="checkbox" checked/><span class="swatch" data-state="3"></span><span>Profundidad media</span></label>
                      <label class="layer-row"><input id="layerState4" type="checkbox" checked/><span class="swatch" data-state="4"></span><span>Profundidad alta</span></label>
                      <label class="layer-row"><input id="layerState5" type="checkbox" checked/><span class="swatch" data-state="5"></span><span>Profundidad extrema</span></label>
                    </fieldset>
                    <h2>C&#225;mara</h2>
                    <div class="camera-presets">
                      <button class="cam-btn" data-vp="VP_Overview" title="Vista perspectiva (P)">Perspectiva</button>
                      <button class="cam-btn" data-vp="VP_Cenital"  title="Vista cenital (C)">Cenital</button>
                      <button class="cam-btn" data-vp="VP_Lateral"  title="Vista lateral (L)">Lateral</button>
                      <button class="cam-btn" id="camReset"         title="Resetear c&#225;mara (R)">&#8635; Reset</button>
                    </div>
                    <div class="slider-row">
                      <label for="zScale">Exageraci&#243;n Z</label>
                      <input id="zScale" type="range" min="1" max="10" value="1" step="0.5"/>
                      <span id="zScaleVal">1&#215;</span>
                    </div>
                    <p class="shortcuts-hint">
                      Atajos: <kbd>C</kbd> cenital &nbsp;&#183;&nbsp; <kbd>P</kbd> perspectiva
                      &nbsp;&#183;&nbsp; <kbd>L</kbd> lateral &nbsp;&#183;&nbsp; <kbd>R</kbd> reset
                      &nbsp;&#183;&nbsp; <kbd>Space</kbd> play &nbsp;&#183;&nbsp; <kbd>&#8592;</kbd><kbd>&#8594;</kbd> frames
                    </p>
                    <h2>Minimapa</h2>
                    <div class="minimap-wrap">
                      <canvas id="minimapCanvas" title="Arrastra para seleccionar zona"></canvas>
                      <button id="minimapReset" class="cam-btn" style="margin-top:6px;width:100%">&#8635; Vista completa</button>
                    </div>
                    <h2>Leyenda</h2>
                    <div id="legend"></div>
                  </div>
                </aside>
              </div>

              <div id="controls">
                <button id="prevBtn">&#9664;</button>
                <button id="playBtn">&#9654; Play</button>
                <button id="nextBtn">&#9654;&#9654;</button>
                <input id="slider" type="range" min="0" max="{{sliderMax}}" value="0"/>
                <span id="frame-label">Frame 0 / {{sliderMax}}</span>
                <label>Speed <input id="speed" type="range" min="100" max="2000" value="800" step="100"/></label>
                <button id="followBtn" title="Saltar al &#250;ltimo frame al llegar uno nuevo">&#128205; Seguir</button>
                <span id="status">Loading terrain&#8230;</span>
              </div>

              <script>
            window.__CONFIG__ = {{configJson}};
              </script>
              <script type="module" src="js/app.js"></script>
            </body>
            </html>
            """;
    }
}
