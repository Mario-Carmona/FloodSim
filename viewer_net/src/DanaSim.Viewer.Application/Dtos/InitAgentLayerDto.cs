using System.Text.Json.Serialization;

namespace DanaSim.Viewer.Application.Dtos;

public sealed class InitAgentLayerDto
{
    [JsonPropertyName("id")] public string Id { get; set; } = string.Empty;
    [JsonPropertyName("data_path")] public string DataPath { get; set; } = string.Empty;
    [JsonPropertyName("data_filename")] public string DataFilename { get; set; } = string.Empty;
}
