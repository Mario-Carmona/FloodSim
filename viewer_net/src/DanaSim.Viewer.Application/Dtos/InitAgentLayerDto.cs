using System.Text.Json.Serialization;

namespace DanaSim.Viewer.Application.Dtos;

public sealed class InitAgentLayerDto
{
    [JsonPropertyName("id")] public string Id { get; set; } = string.Empty;
    [JsonPropertyName("data_path")] public string DataPath { get; set; } = string.Empty;
    [JsonPropertyName("data_filename")] public string DataFilename { get; set; } = string.Empty;
    [JsonPropertyName("color_palette")] public List<ColorPaletteEntryDto>? ColorPalette { get; set; }
}

public sealed class ColorPaletteEntryDto
{
    [JsonPropertyName("value")] public int Value { get; set; }
    [JsonPropertyName("label")] public string Label { get; set; } = string.Empty;
    [JsonPropertyName("hex")] public string Hex { get; set; } = string.Empty;
    [JsonPropertyName("rgba")] public byte[] Rgba { get; set; } = [];
}
