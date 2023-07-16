using System.Text;
using MQTTnet;
using MQTTnet.Client;

namespace MqttClientApp;

internal class Program
{
    private static IMqttClient _mqttClient;
    private static MqttClientOptions _options;
    const string ClientId = "ClientId";
    const string Server = "SD.local";
    const int ServerPort = 1883;

    const string Topic = "canbus/#";

    private static async Task Main(string[] args)
    {
        ConfigureClient();
        await ConnectToServer();
        await SubscribeToTopic();
        SubscribeToMessagesRecievedEvent();
        WaitForExit();
        await PerformDisconnect();
    }

    private static void ConfigureClient()
    {
        var factory = new MqttFactory();
        _mqttClient = factory.CreateMqttClient();

        // Configure options
        _options = new MqttClientOptionsBuilder()
            .WithClientId(ClientId)
            .WithTcpServer(Server, ServerPort) // Replace with your MQTT broker IP and port
            .Build();
    }

    private static async Task ConnectToServer()
    {
        // Connect to the MQTT broker
        var connectResult = await _mqttClient.ConnectAsync(_options);
        if (connectResult.ResultCode == MqttClientConnectResultCode.Success)
            Console.WriteLine("Connected successfully with MQTT Brokers.");
        else
            throw new Exception(connectResult.ReasonString);
    }

    private static async Task SubscribeToTopic()
    {
        var subscribeResult = await _mqttClient.SubscribeAsync(Topic);
        if (subscribeResult.ReasonString != null)
            throw new Exception(subscribeResult.ReasonString);
        Console.WriteLine("Subscribed");
    }

    private static void SubscribeToMessagesRecievedEvent()
    {
        _mqttClient.ApplicationMessageReceivedAsync += e =>
        {
            Console.WriteLine($"Id = {e.ApplicationMessage.Topic.Split('/')[1]}");
            Console.WriteLine($"Payload = {Encoding.UTF8.GetString(e.ApplicationMessage.Payload)}");
            Console.WriteLine();
            return Task.CompletedTask;
        };
    }

    private static void WaitForExit()
    {
        Console.WriteLine("Press any key to exit.");
        Console.ReadLine();
    }

    private static async Task PerformDisconnect()
    {
        // Disconnect from the MQTT broker
        await _mqttClient.DisconnectAsync();
    }
}