using DanaSim.Viewer.Web.Logging;
using Microsoft.AspNetCore.Mvc;

namespace DanaSim.Viewer.Web.Controllers;

[ApiController]
[Route("api")]
public sealed class ApiController(InMemoryLogSink logSink) : ControllerBase
{
    /// <summary>GET /api/logs?after=N — returns entries added since index N.</summary>
    [HttpGet("logs")]
    public IActionResult GetLogs([FromQuery] int after = 0)
    {
        var (lastIndex, entries) = logSink.Since(after);
        return Ok(new { lastIndex, entries });
    }
}
