using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Domain.Ports;
using DanaSim.Viewer.Infrastructure.Config;
using DanaSim.Viewer.Web.Logging;
using Microsoft.AspNetCore.Mvc;

namespace DanaSim.Viewer.Web.Controllers;

[ApiController]
[Route("api")]
public sealed class ApiController(
    InMemoryLogSink logSink,
    SimulationStatusService statusService,
    UserConfigService userConfigService,
    ISimulationController simulationController) : ControllerBase
{
    /// <summary>GET /api/status — live connection and simulation state.</summary>
    [HttpGet("status")]
    public IActionResult GetStatus() => Ok(statusService.Get());

    /// <summary>GET /api/config — current user config plus the log file path.</summary>
    [HttpGet("config")]
    public IActionResult GetConfig()
    {
        var cfg = userConfigService.Load() ?? new UserConfig();
        return Ok(new
        {
            cfg.MqttHost,
            cfg.MqttPort,
            cfg.Scenario,
            cfg.TerrainBasePath,
            cfg.OutputDir,
            logFilePath = AppPaths.LogsDirectory,
        });
    }

    /// <summary>POST /api/config — validate, persist, and reconfigure the MQTT connection.</summary>
    [HttpPost("config")]
    public async Task<IActionResult> PostConfig([FromBody] UserConfig cfg, CancellationToken ct)
    {
        var errors = new Dictionary<string, string>();

        if (string.IsNullOrWhiteSpace(cfg.MqttHost))
            errors["mqttHost"] = "Required";
        if (cfg.MqttPort is < 1 or > 65535)
            errors["mqttPort"] = "Must be between 1 and 65535";
        if (string.IsNullOrWhiteSpace(cfg.Scenario))
            errors["scenario"] = "Required";
        // TerrainBasePath may be left blank — IdrisiTerrainDataReader then resolves
        // it to "{app directory}/data" (the installer's "full" bundled-data variant).
        if (string.IsNullOrWhiteSpace(cfg.OutputDir))
            errors["outputDir"] = "Required";

        if (errors.Count > 0)
            return BadRequest(new { errors });

        userConfigService.Save(cfg);
        await simulationController.ReconfigureAsync(cfg.MqttHost, cfg.MqttPort, cfg.Scenario, ct);
        return Ok();
    }

    /// <summary>
    /// POST /api/control/connect — fire-and-forget connect; retry loop runs in background.
    /// Returns 202 immediately; poll /api/status to observe the outcome.
    /// </summary>
    [HttpPost("control/connect")]
    public IActionResult Connect()
    {
        _ = simulationController.ConnectAsync();
        return Accepted();
    }

    /// <summary>POST /api/control/disconnect — graceful disconnect.</summary>
    [HttpPost("control/disconnect")]
    public async Task<IActionResult> Disconnect(CancellationToken ct)
    {
        await simulationController.DisconnectAsync(ct);
        return Ok();
    }

    /// <summary>GET /api/logs?after=N — returns entries added since index N.</summary>
    [HttpGet("logs")]
    public IActionResult GetLogs([FromQuery] int after = 0)
    {
        var (lastIndex, entries) = logSink.Since(after);
        return Ok(new { lastIndex, entries });
    }
}
