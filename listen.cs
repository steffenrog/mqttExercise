using System.Text;
using MQTTnet;
using MQTTnet.Client;

namespace MqttClientApp;

internal class Program
{
    private static async Task Main(string[] args)
    {
        // Create a new MQTT client.
        var factory = new MqttFactory();
        var mqttClient = factory.CreateMqttClient();

        // Configure options
        var options = new MqttClientOptionsBuilder()
            .WithClientId("ClientId")
            .WithTcpServer("SD.local", 1883) // Replace with your MQTT broker IP and port
            .Build();

        // Connect to the MQTT broker
        await mqttClient.ConnectAsync(options);

        Console.WriteLine("Connected successfully with MQTT Brokers.");
        // Subscribe to the topic
        var mqttFactory = new MqttFactory();
        var mqttSubscribeOptions = mqttFactory.CreateSubscribeOptionsBuilder()
                .WithTopicFilter(
                    f => { f.WithTopic("canbus/#"); }).Build()
            ;
        await mqttClient.SubscribeAsync(mqttSubscribeOptions);
        Console.WriteLine("Subscribed");

        mqttClient.ApplicationMessageReceivedAsync += e =>
        {
            Console.WriteLine($"Id = {e.ApplicationMessage.Topic.Split('/')[1]}");
            Console.WriteLine($"Payload = {Encoding.UTF8.GetString(e.ApplicationMessage.Payload)}");
            Console.WriteLine();
            return Task.CompletedTask;
        };

        Console.WriteLine("Press any key to exit.");
        Console.ReadLine();

        // Disconnect from the MQTT broker
        await mqttClient.DisconnectAsync();
    }
}