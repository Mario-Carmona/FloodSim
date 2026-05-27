using System.Text.Json;
using DanaSim.Viewer.Application.Dtos;
using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Application.StateMachine;
using DanaSim.Viewer.Domain.Ports;
using Microsoft.Extensions.Logging;

namespace DanaSim.Viewer.Application.Handlers;

public sealed class EyeFrameSyncHandler(ILogger<EyeFrameSyncHandler> logger) : IMqttEventHandler
{
    public async Task HandleAsync(
        JsonElement payload, SimulationContext context, SimulationStateMachine stateMachine,
        IControlPublisher control, ISimulationBroadcaster broadcaster, CancellationToken ct)
    {
        if (!context.Grid.HasNewData)
            return;

        var dto = payload.Deserialize<EyeFrameSyncDto>() ?? new EyeFrameSyncDto();
        var frame = context.Grid.BuildFrameData(dto.SimulationTime);
        context.Grid.ConsumeData();
        context.StepIndex++;

        // Send only the changed cells to avoid pushing full grid every frame
        var changes = frame.PaletteGrid
            .Select((state, i) => new Domain.ValueObjects.CellChange(i, state, frame.WaterDepths[i]))
            .Where(c => c.State != Domain.Enums.FloodState.Dry)
            .ToList();

        await broadcaster.BroadcastFrameUpdateAsync(changes, dto.SimulationTime, ct);

        logger.LogInformation(
            "EYE_Frame_Sync — step {Step}, sim_time={Time}, {Changed} active cells",
            context.StepIndex, dto.SimulationTime, changes.Count);
    }
}
