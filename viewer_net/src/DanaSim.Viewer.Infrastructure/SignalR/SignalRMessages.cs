namespace DanaSim.Viewer.Infrastructure.SignalR;

/// <summary>Sent once on Init_EOF — full grid state for the initial render.</summary>
public sealed record InitialStateMessage(
    GridMetaMessage GridMeta,
    byte[] Cells,        // FloodState values, flat (rows*cols)
    float[] WaterDepths, // metres, flat (rows*cols)
    float[]? Terrain,    // elevation metres, flat — null if not loaded
    string SimulationTime);

public sealed record GridMetaMessage(int Rows, int Cols, float CellSizeM);

/// <summary>Sent on every EYE_Frame_Sync — only the cells that changed.</summary>
public sealed record FrameUpdateMessage(
    string SimulationTime,
    CellUpdateMessage[] Changes);

public sealed record CellUpdateMessage(int Index, byte State, float Height);
