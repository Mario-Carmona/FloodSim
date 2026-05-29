using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Options;
using DanaSim.Viewer.Infrastructure.FileOutput;

namespace DanaSim.Viewer.Web.Controllers;

public sealed class HomeController(IOptions<FileOutputOptions> outputOptions) : Controller
{
    public IActionResult Index()
    {
        var dir  = outputOptions.Value.OutputDir;
        var runs = Directory.Exists(dir)
            ? Directory.GetDirectories(dir)
                .Select(d => Path.GetFileName(d)!)
                .OrderByDescending(n => n)
                .ToList()
            : [];
        return View(runs);
    }
}
