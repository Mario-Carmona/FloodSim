using DanaSim.Viewer.Domain.ValueObjects;

namespace DanaSim.Viewer.Domain.Ports;

/// <summary>
/// Outbound port: sends simulation state to all connected browser clients.
/// Implemented by SignalRBroadcaster in the Infrastructure layer.
/// </summary>
public interface ISimulationBroadcaster
{
    Task BroadcastInitialStateAsync(GridMeta meta, FrameData frame, CancellationToken ct = default);
    Task BroadcastFrameUpdateAsync(IReadOnlyList<ValueObjects.CellChange> changes, string simulationTime, CancellationToken ct = default);
    Task BroadcastSimulationEndedAsync(CancellationToken ct = default);
}
