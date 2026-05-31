using DanaSim.Viewer.Application.Extensions;
using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Infrastructure.Extensions;
using Microsoft.Extensions.FileProviders;

var builder = WebApplication.CreateBuilder(args);

// ── Services ─────────────────────────────────────────────────────────────────
builder.Services.AddControllersWithViews();

builder.Services.Configure<SimulationAppServiceOptions>(
    builder.Configuration.GetSection("Simulation"));

builder.Services.AddApplication();
builder.Services.AddInfrastructure(builder.Configuration);

// ── Pipeline ──────────────────────────────────────────────────────────────────
var app = builder.Build();

if (!app.Environment.IsDevelopment())
    app.UseExceptionHandler("/Home/Error");

app.UseStaticFiles();

// Serve simulation output files (player.html, js/, flood/*.png) under /sim-outputs
var simOutputDir = builder.Configuration["FileOutput:OutputDir"] ?? "outputs/x3d_heightmap";
if (!Directory.Exists(simOutputDir))
    Directory.CreateDirectory(simOutputDir);

app.UseStaticFiles(new StaticFileOptions
{
    FileProvider   = new PhysicalFileProvider(simOutputDir),
    RequestPath    = "/sim-outputs",
    ServeUnknownFileTypes = true,  // serve .x3d, .json, etc.
});

app.UseRouting();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

app.Run();
