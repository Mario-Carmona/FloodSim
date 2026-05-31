namespace DanaSim.Viewer.Domain.ValueObjects;

/// <summary>
/// Immutable grid dimensions and spatial metadata, sent to the renderer on Init_EOF.
/// TerrainHeights is a flat array of length Rows*Cols (null until InitAgent_Layer is processed).
/// </summary>
public sealed record GridMeta(
    int Rows,
    int Cols,
    float CellSizeM,
    float[]? TerrainHeights);
