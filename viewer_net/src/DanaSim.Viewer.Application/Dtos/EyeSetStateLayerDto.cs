using System.Text.Json.Serialization;

namespace DanaSim.Viewer.Application.Dtos;

public sealed class EyeSetStateLayerDto
{
    [JsonPropertyName("id")] public string Id { get; set; } = string.Empty;
    [JsonPropertyName("changes")] public ChangesSection Changes { get; set; } = new();

    public sealed class ChangesSection
    {
        // Keys are flat cell indices as strings: "142560", "142561", ...
        [JsonPropertyName("cells")]
        public Dictionary<string, CellStateDto> Cells { get; set; } = new();
    }

    public sealed class CellStateDto
    {
        [JsonPropertyName("state")] public int State { get; set; }
        [JsonPropertyName("height")] public float Height { get; set; }
    }
}
