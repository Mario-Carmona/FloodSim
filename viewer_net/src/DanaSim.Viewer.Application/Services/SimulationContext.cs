using DanaSim.Viewer.Domain.Entities;
using DanaSim.Viewer.Domain.ValueObjects;

namespace DanaSim.Viewer.Application.Services;

/// <summary>
/// Mutable runtime state shared across all MQTT event handlers within a session.
/// Mirrors the fields owned by SimulationApp in the Python visualizer.
/// </summary>
public sealed class SimulationContext
{
    public SimulationGrid Grid { get; } = new();
    public SimulationConfig Config { get; } = new();

    // Backpressure (ChunkAck)
    public List<CellChange> PendingChanges { get; } = [];
    public int ChunksPerBatch { get; set; }
    public int ChunksSinceAck { get; set; }

    // Frame timeout detection
    public long? FrameStartTick { get; set; }

    public bool IsRunning { get; set; } = true;
    public int StepIndex { get; set; }
}
