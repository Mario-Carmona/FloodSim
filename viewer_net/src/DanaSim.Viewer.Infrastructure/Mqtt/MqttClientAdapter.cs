using System.Threading.Channels;
using DanaSim.Viewer.Application.Ports;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using MQTTnet;
using MQTTnet.Protocol;

namespace DanaSim.Viewer.Infrastructure.Mqtt;

/// <summary>
/// Subscribes to the MQTT broker, receives simulation events, and forwards them
/// sequentially to ISimulationEventHandler via a Channel (preserving event order).
/// Reconnects automatically on disconnection.
/// </summary>
public sealed class MqttClientAdapter : IHostedService, IAsyncDisposable
{
    private readonly IMqttClient _client;
    private readonly ISimulationEventHandler _handler;
    private readonly MqttOptions _opts;
    private readonly ILogger<MqttClientAdapter> _logger;

    private readonly Channel<string> _messageChannel =
        Channel.CreateUnbounded<string>(new UnboundedChannelOptions { SingleReader = true });

    private MqttClientOptions? _connectOptions;
    private Task? _processingLoop;
    private CancellationTokenSource _cts = new();

    public MqttClientAdapter(
        IMqttClient client,
        ISimulationEventHandler handler,
        IOptions<MqttOptions> options,
        ILogger<MqttClientAdapter> logger)
    {
        _client = client;
        _handler = handler;
        _opts = options.Value;
        _logger = logger;

        _client.ApplicationMessageReceivedAsync += OnMessageReceived;
        _client.DisconnectedAsync += OnDisconnected;
    }

    public async Task StartAsync(CancellationToken ct)
    {
        _cts = CancellationTokenSource.CreateLinkedTokenSource(ct);
        _connectOptions = BuildConnectOptions();

        await ConnectAsync(_cts.Token);
        _processingLoop = ProcessChannelAsync(_cts.Token);
    }

    public async Task StopAsync(CancellationToken ct)
    {
        _messageChannel.Writer.TryComplete();
        await _cts.CancelAsync();

        if (_processingLoop is not null)
            await _processingLoop;

        if (_client.IsConnected)
            await _client.DisconnectAsync(cancellationToken: ct);
    }

    public async ValueTask DisposeAsync()
    {
        _cts.Dispose();
        _client.ApplicationMessageReceivedAsync -= OnMessageReceived;
        _client.DisconnectedAsync -= OnDisconnected;
        _client.Dispose();
    }

    // ── Private ──────────────────────────────────────────────────────────────

    private MqttClientOptions BuildConnectOptions() =>
        new MqttClientOptionsBuilder()
            .WithTcpServer(_opts.Host, _opts.Port)
            .WithClientId("DanaSim_NetViewer")
            .WithKeepAlivePeriod(TimeSpan.FromSeconds(_opts.KeepAliveSeconds))
            .WithWillTopic($"FloodSim/{_opts.Scenario}/system/control")
            .WithWillPayload("""{"process":"System_Disconnected","source":"DanaSim_NetViewer"}""")
            .WithWillQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce)
            .WithWillRetain(true)
            .Build();

    private async Task ConnectAsync(CancellationToken ct)
    {
        try
        {
            await _client.ConnectAsync(_connectOptions!, ct);
            _logger.LogInformation("Connected to MQTT broker at {Host}:{Port}", _opts.Host, _opts.Port);

            await _client.SubscribeAsync(new MqttClientSubscribeOptionsBuilder()
                .WithTopicFilter(f => f
                    .WithTopic(MqttTopics.Events(_opts.Scenario))
                    .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce))
                .WithTopicFilter(f => f
                    .WithTopic(MqttTopics.PingIn(_opts.Scenario))
                    .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce))
                .Build(), ct);

            _logger.LogInformation(
                "Subscribed to scenario '{Scenario}'", _opts.Scenario);
        }
        catch (Exception ex) when (ex is not OperationCanceledException)
        {
            _logger.LogWarning(ex, "Could not connect to broker — will retry");
        }
    }

    private async Task OnDisconnected(MqttClientDisconnectedEventArgs e)
    {
        if (_cts.IsCancellationRequested)
            return;

        _logger.LogWarning("Disconnected from broker. Reconnecting in {Ms}ms...", _opts.ReconnectDelayMs);

        try
        {
            await Task.Delay(_opts.ReconnectDelayMs, _cts.Token);
            await ConnectAsync(_cts.Token);
        }
        catch (OperationCanceledException) { }
    }

    private Task OnMessageReceived(MqttApplicationMessageReceivedEventArgs e)
    {
        var payload = e.ApplicationMessage.ConvertPayloadToString();
        _messageChannel.Writer.TryWrite(payload);
        return Task.CompletedTask;
    }

    /// <summary>
    /// Sequential processing loop: ensures events are handled one at a time,
    /// preserving protocol order (critical for state machine transitions).
    /// </summary>
    private async Task ProcessChannelAsync(CancellationToken ct)
    {
        await foreach (var rawJson in _messageChannel.Reader.ReadAllAsync(ct))
        {
            try
            {
                await _handler.HandleEventAsync(rawJson, ct);
            }
            catch (Exception ex)
            {
                _logger.LogError(ex, "Unhandled error processing MQTT message");
            }
        }
    }
}
