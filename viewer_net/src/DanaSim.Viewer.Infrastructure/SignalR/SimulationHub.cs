using Microsoft.AspNetCore.SignalR;

namespace DanaSim.Viewer.Infrastructure.SignalR;

/// <summary>
/// SignalR hub: browser clients connect here to receive real-time simulation updates.
/// Server-to-client methods: InitialState, FrameUpdate, SimulationEnded.
/// </summary>
public sealed class SimulationHub : Hub
{
    public async Task JoinSimulation(string scenario)
    {
        await Groups.AddToGroupAsync(Context.ConnectionId, scenario);
    }
}
