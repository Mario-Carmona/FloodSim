using DanaSim.Viewer.Application.Ports;
using DanaSim.Viewer.Domain.Ports;
using DanaSim.Viewer.Infrastructure.Mqtt;
using DanaSim.Viewer.Infrastructure.Terrain;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using MQTTnet;

namespace DanaSim.Viewer.Infrastructure.Extensions;

public static class InfrastructureServiceExtensions
{
    public static IServiceCollection AddInfrastructure(
        this IServiceCollection services, IConfiguration configuration)
    {
        // MQTT
        services.Configure<MqttOptions>(configuration.GetSection("Mqtt"));
        services.AddSingleton<IMqttClient>(_ => new MqttClientFactory().CreateMqttClient());
        services.AddSingleton<IControlPublisher, MqttControlPublisher>();
        services.AddHostedService<MqttClientAdapter>();

        // Terrain data
        services.Configure<TerrainOptions>(configuration.GetSection("Terrain"));
        services.AddSingleton<ITerrainDataReader, IdrisiTerrainDataReader>();

        return services;
    }
}
