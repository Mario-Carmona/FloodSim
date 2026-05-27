using DanaSim.Viewer.Application.Extensions;
using DanaSim.Viewer.Application.Services;
using DanaSim.Viewer.Infrastructure.Extensions;
using DanaSim.Viewer.Infrastructure.SignalR;

var builder = WebApplication.CreateBuilder(args);

// ── Services ─────────────────────────────────────────────────────────────────
builder.Services.AddControllersWithViews();
builder.Services.AddSignalR();

builder.Services.Configure<SimulationAppServiceOptions>(
    builder.Configuration.GetSection("Simulation"));

builder.Services.AddApplication();
builder.Services.AddInfrastructure(builder.Configuration);

// ── Pipeline ──────────────────────────────────────────────────────────────────
var app = builder.Build();

if (!app.Environment.IsDevelopment())
    app.UseExceptionHandler("/Home/Error");

app.UseStaticFiles();
app.UseRouting();

app.MapControllerRoute(
    name: "default",
    pattern: "{controller=Home}/{action=Index}/{id?}");

app.MapHub<SimulationHub>("/simulationHub");

app.Run();
