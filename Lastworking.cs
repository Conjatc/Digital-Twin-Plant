//Last working

using System;
using System.IO;
using System.IO.Compression;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using MQTTnet;
using MQTTnet.Client;
using MQTTnet.Client.Options;
using UnityEngine;
using UnityEngine.UI;

public class AWSIoTReceiver : MonoBehaviour
{
    [Header("AWS IoT Settings")]
    public string endpoint = "a2qxwnrprho4b8-ats.iot.us-east-1.amazonaws.com";
    public string topicImageRealtime = "plants/rt/images";
    public string topicSensorRealtime = "plants/rt/status";
    public string topicCommand = "plants/commands";
    public string clientId = "Computer";

    [Header("Cert Paths (StreamingAssets)")]
    public string clientCertPath = "deviceCert.pfx";
    public string clientCertPassword = "123789";

    [Header("UI")]
    public RawImage imageDisplay;
    public Text sensorText;
    public Button toggleRealtimeButton;
    private Text toggleButtonText;

    private IMqttClient mqttClient;
    private bool realtimeEnabled = false;

    async void Start()
    {
        if (toggleRealtimeButton != null)
        {
            toggleRealtimeButton.onClick.AddListener(ToggleRealtime);
            toggleButtonText = toggleRealtimeButton.GetComponentInChildren<Text>();
            toggleButtonText.text = "Start Real Time Monitoring";
        }

        await ConnectToAwsIot();
    }

async Task ConnectToAwsIot()
    {
        var factory = new MqttFactory();
        mqttClient = factory.CreateMqttClient();

        string certFullPath = Path.Combine(Application.streamingAssetsPath, clientCertPath);

        if (!File.Exists(certFullPath))
        {
            Debug.LogError($"Certificate not found at: {certFullPath}");
            return;
        }

        X509Certificate2 clientCert;
        try
        {
            clientCert = new X509Certificate2(certFullPath, clientCertPassword);
        }
        catch (Exception ex)
        {
            Debug.LogError("Failed to load certificate: " + ex.Message);
            return;
        }

        var tlsOptions = new MqttClientOptionsBuilderTlsParameters
        {
            UseTls = true,
            Certificates = new[] { clientCert },
            SslProtocol = System.Security.Authentication.SslProtocols.Tls12,
            AllowUntrustedCertificates = false,
            IgnoreCertificateChainErrors = false,
            IgnoreCertificateRevocationErrors = false,
            CertificateValidationHandler = context => true
        };

        var options = new MqttClientOptionsBuilder()
            .WithClientId(clientId)
            .WithTcpServer(endpoint, 8883)
            .WithCleanSession()
            .WithTls(tlsOptions)
            .Build();

        mqttClient.UseConnectedHandler(async e =>
        {
            Debug.Log(" Connected to AWS IoT!");

            await mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(topicImageRealtime).Build());
            await mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic(topicSensorRealtime).Build());
        });

        mqttClient.UseDisconnectedHandler(e =>
        {
            Debug.LogWarning(" Disconnected from AWS IoT.");
            if (e.Exception != null)
                Debug.LogError("Disconnection reason: " + e.Exception.Message);
        });

        mqttClient.UseApplicationMessageReceivedHandler(e =>
        {
            string topic = e.ApplicationMessage.Topic;
            string payload = Encoding.UTF8.GetString(e.ApplicationMessage.Payload);

            if (topic == topicImageRealtime)
                ProcessImagePayload(payload);
            else if (topic == topicSensorRealtime)
                ProcessSensorPayload(payload);
        });

        try
        {
            await mqttClient.ConnectAsync(options, CancellationToken.None);
        }
        catch (Exception ex)
        {
            Debug.LogError(" MQTT connection failed: " + ex.Message);
            if (ex.InnerException != null)
                Debug.LogError("Inner: " + ex.InnerException.Message);
        }
    }

    void ProcessSensorPayload(string json)
    {
        try
        {
            var data = JsonUtility.FromJson<PlantStatus>(json);
            sensorText.text = $"Time: {data.timestamp}\nTemp: {data.temp:F1} °C\nMoisture: {data.moist:F1}%\nBrightness: {data.brightness:F1}\nGreen: {data.green:F1}";
        }
        catch
        {
            sensorText.text = json;
        }
    }

    [Serializable]
    public class ImagePayload
    {
        public string t; // timestamp
        public string i; // base64 image
    }

    void ProcessImagePayload(string json)
    {
        try
        {
            var imagePayload = JsonUtility.FromJson<ImagePayload>(json);
            byte[] imageBytes = Convert.FromBase64String(imagePayload.i);
            LoadImage(imageBytes);
        }
        catch (Exception e)
        {
            Debug.LogError("Image decoding error: " + e.Message);
        }
    }

    void LoadImage(byte[] imageData)
    {
        Texture2D tex = new Texture2D(2, 2);
        tex.LoadImage(imageData);
        imageDisplay.texture = tex;
    }

    public async void ToggleRealtime()
    {
        realtimeEnabled = !realtimeEnabled;
        string state = realtimeEnabled ? "on" : "off";
        string message = $"{{\"realtime\":\"{state}\"}}";

        toggleButtonText.text = realtimeEnabled ? "Realtime: ON" : "Realtime: OFF";

        // Change button color (green for ON, red for OFF)
        ColorBlock colors = toggleRealtimeButton.colors;
        colors.selectedColor = realtimeEnabled ? Color.green : Color.red;
        colors.highlightedColor = colors.selectedColor;
        colors.pressedColor = colors.selectedColor * 0.9f; // darker shade
        toggleRealtimeButton.colors = colors;


        if (mqttClient != null && mqttClient.IsConnected)
        {
            var mqttMsg = new MqttApplicationMessageBuilder()
                .WithTopic(topicCommand)
                .WithPayload(Encoding.UTF8.GetBytes(message))
                .Build();

            await mqttClient.PublishAsync(mqttMsg);
            Debug.Log("Published realtime toggle: " + message);
        }
    }

    private async void OnApplicationQuit()
    {
        if (mqttClient != null && mqttClient.IsConnected)
            await mqttClient.DisconnectAsync();
    }

    [Serializable]
    public class PlantStatus
    {
        public string timestamp;
        public float temp;
        public float moist;
        public float brightness;
        public float green;
    }
}