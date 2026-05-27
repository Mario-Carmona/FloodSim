using System.Text.Json;
using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Application.StateMachine;
using DanaSim.Viewer.Domain.Ports;
using Microsoft.Extensions.Logging;

namespace DanaSim.Viewer.Application.Handlers;

public sealed class FrameEndHandler(ILogger<FrameEndHandler> logger) : IMqttEventHandler
{
    public Task HandleAsync(
        JsonElement payload, SimulationContext context, SimulationStateMachine stateMachine,
        IControlPublisher control, ISimulationBroadcaster broadcaster, CancellationToken ct)
    {
        var count = context.PendingChanges.Count;
        context.Grid.ApplyBulkChanges(context.PendingChanges);
        context.PendingChanges.Clear();
        context.FrameStartTick = null;

        logger.LogInformation("FrameEnd — {Count} changes applied to grid", count);
        return Task.CompletedTask;
    }
}
