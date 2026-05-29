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
        var frameData = context.Grid.BuildFrameData(dto.SimulationTime);
        context.Grid.ConsumeData();
        context.StepIndex++;

        await broadcaster.BroadcastFrameUpdateAsync(frameData, context.StepIndex, ct);

        logger.LogInformation(
            "EYE_Frame_Sync — step {Step}, sim_time={Time}, {Changed} changed cells",
            context.StepIndex, dto.SimulationTime, context.LastFrameChanges.Count);
    }
}
