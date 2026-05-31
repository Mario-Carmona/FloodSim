using DanaSim.Viewer.Application.Extensions;
using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Infrastructure.Config;
using DanaSim.Viewer.Infrastructure.Extensions;
using DanaSim.Viewer.Web.Logging;
using Microsoft.Extensions.FileProviders;
using Microsoft.Extensions.FileProviders.Embedded;
using Serilog;
using Serilog.Events;
using Serilog.Formatting.Compact;

// ── Logging bootstrap (before host so startup errors are captured) ────────────
var inMemorySink = new InMemoryLogSink();

Log.Logger = new LoggerConfiguration()
    .MinimumLevel.Debug()
    .MinimumLevel.Override("Microsoft",                  LogEventLevel.Warning)
    .MinimumLevel.Override("Microsoft.Hosting.Lifetime", LogEventLevel.Information)
    .Enrich.FromLogContext()
    .WriteTo.Console(outputTemplate:
        "[{Timestamp:HH:mm:ss} {Level:u3}] {SourceContext:l} — {Message:lj}{NewLine}{Exception}")
    .WriteTo.File(
        formatter: new CompactJsonFormatter(),
        path: Path.Combine(AppContext.BaseDirectory, "logs", "danasim-.log"),
        rollingInterval: RollingInterval.Day,
        retainedFileCountLimit: 7,
        fileSizeLimitBytes: 50 * 1024 * 1024,
        rollOnFileSizeLimit: true)
    .WriteTo.Sink(inMemorySink)
    .CreateLogger();

try
{
    var builder = WebApplication.CreateBuilder(args);

    builder.Host.UseSerilog();

    // ── User config — load before DI so values override appsettings.json ─────
    var userConfigService = new UserConfigService();
    var userCfg = userConfigService.Load();
    if (userCfg is not null)
    {
        builder.Configuration["Mqtt:Host"]            = userCfg.MqttHost;
        builder.Configuration["Mqtt:Port"]            = userCfg.MqttPort.ToString();
        builder.Configuration["Mqtt:Scenario"]        = userCfg.Scenario;
        builder.Configuration["Terrain:BasePath"]     = userCfg.TerrainBasePath;
        builder.Configuration["FileOutput:OutputDir"] = userCfg.OutputDir;
    }

    if (!userConfigService.IsConfigured())
        builder.Configuration["Mqtt:AutoConnect"] = "false";

    // ── Services ─────────────────────────────────────────────────────────────
    builder.Services.AddSingleton(inMemorySink);
    builder.Services.AddSingleton(userConfigService);

    builder.Services.AddControllersWithViews();

    builder.Services.Configure<SimulationAppServiceOptions>(
        builder.Configuration.GetSection("Simulation"));

    builder.Services.AddApplication();
    builder.Services.AddInfrastructure(builder.Configuration);

    // ── Pipeline ──────────────────────────────────────────────────────────────
    var app = builder.Build();

    if (!app.Environment.IsDevelopment())
        app.UseExceptionHandler("/Home/Error");

    app.UseStaticFiles();

    // Serve vendored player JS/CSS from the embedded assembly at /player-assets
    app.UseStaticFiles(new StaticFileOptions
    {
        FileProvider = new ManifestEmbeddedFileProvider(
            typeof(Program).Assembly, "PlayerAssets"),
        RequestPath = "/player-assets",
    });

    // Serve simulation output files (player.html, flood/*.png) under /sim-outputs
    var simOutputDir = builder.Configuration["FileOutput:OutputDir"] ?? "outputs/x3d_heightmap";
    if (!Directory.Exists(simOutputDir))
        Directory.CreateDirectory(simOutputDir);

    app.UseStaticFiles(new StaticFileOptions
    {
        FileProvider          = new PhysicalFileProvider(simOutputDir),
        RequestPath           = "/sim-outputs",
        ServeUnknownFileTypes = true,
    });

    app.UseRouting();

    app.MapControllerRoute(
        name: "default",
        pattern: "{controller=Home}/{action=Index}/{id?}");

    app.Run();
}
catch (Exception ex)
{
    Log.Fatal(ex, "Application terminated unexpectedly");
}
finally
{
    Log.CloseAndFlush();
}
