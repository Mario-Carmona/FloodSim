using DanaSim.Viewer.Domain.Ports;
using DanaSim.Viewer.Domain.ValueObjects;
using Microsoft.AspNetCore.SignalR;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using DanaSim.Viewer.Infrastructure.Mqtt;

namespace DanaSim.Viewer.Infrastructure.SignalR;

/// <summary>
/// Outbound port adapter: translates domain value objects into SignalR messages
/// and broadcasts them to all connected browser clients.
/// </summary>
public sealed class SignalRBroadcaster(
    IHubContext<SimulationHub> hub,
    IOptions<MqttOptions> mqttOptions,
    ILogger<SignalRBroadcaster> logger) : ISimulationBroadcaster
{
    private readonly string _scenario = mqttOptions.Value.Scenario;

    public async Task BroadcastInitialStateAsync(
        GridMeta meta, FrameData frame, CancellationToken ct = default)
    {
        var message = new InitialStateMessage(
            GridMeta: new GridMetaMessage(meta.Rows, meta.Cols, meta.CellSizeM),
            Cells: frame.PaletteGrid.Select(s => (byte)s).ToArray(),
            WaterDepths: frame.WaterDepths,
            Terrain: meta.TerrainHeights,
            SimulationTime: frame.SimulationTime);

        await hub.Clients.Group(_scenario).SendAsync("InitialState", message, ct);
        logger.LogInformation(
            "InitialState broadcast — {Rows}x{Cols}, {Cells} cells",
            meta.Rows, meta.Cols, meta.Rows * meta.Cols);
    }

    public async Task BroadcastFrameUpdateAsync(
        IReadOnlyList<CellChange> changes, string simulationTime, CancellationToken ct = default)
    {
        var message = new FrameUpdateMessage(
            SimulationTime: simulationTime,
            Changes: changes
                .Select(c => new CellUpdateMessage(c.Index, (byte)c.State, c.WaterDepthM))
                .ToArray());

        await hub.Clients.Group(_scenario).SendAsync("FrameUpdate", message, ct);
        logger.LogInformation(
            "FrameUpdate broadcast — {Count} changes, sim_time={Time}",
            changes.Count, simulationTime);
    }

    public async Task BroadcastSimulationEndedAsync(CancellationToken ct = default)
    {
        await hub.Clients.Group(_scenario).SendAsync("SimulationEnded", ct);
        logger.LogInformation("SimulationEnded broadcast");
    }
}
