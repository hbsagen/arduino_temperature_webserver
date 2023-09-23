//#include <Wire.h>
//#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

WiFiUDP udp;

void connectToWiFi(); // Function prototype
void checkWiFiConnection(); // Function prototype
void handleRoot(); // Function prototype
void handleNewPage(); // Function prototype


const IPAddress serverIP(192, 168, 10, 101); // The IP address of the server to fetch the word from
const int udpPort = 6666; // The UDP port to fetch the word from

const char* ssid = "SSID";
const char* password = "password";
const int maxReadings = 144;
float pressures[maxReadings];
const int readingInterval = 10 * 60 * 1000; //skal være 600k for hvert tiende minutt
String ord = "";

ESP8266WebServer server(80);
ESP8266WebServer server2(8888);
Adafruit_BMP085 bmp;
unsigned long lastReadingTime = 0;
float temperatures[maxReadings];
int timestamps[maxReadings];
int currentIndex = 1;
bool hasReading = false;

#include <ESP8266HTTPClient.h>

String get_random_word() {
  const unsigned long timeout = 2000; // Set the desired timeout in milliseconds
  

  udp.beginPacket(serverIP, udpPort);
  udp.write("Request random word");
  udp.endPacket();

  //delay(500); // Give the server some time to send the response, increase if needed

  unsigned long startTime = millis();

  while (millis() - startTime < timeout) {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      char word[packetSize + 1];
      udp.read(word, packetSize);
      word[packetSize] = '\0';
      Serial.print("Random word: ");
      Serial.println(word);
      ord = word;
      return word;
    }
  }
  
  Serial.println("Error: No UDP response received");
  return ord;
}



void setup() {
  Serial.begin(115200);
  udp.begin(6666);

  // Initialize the BMP085 sensor
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }

  connectToWiFi();

  server.on("/", handleRoot);
  server2.on("/", handleNewPage);
  server.begin();
  server2.begin();
  Serial.println("HTTP server started");

  for (int i = 0; i <= maxReadings; i++) {
    temperatures[i] = 0;
    timestamps[i] = 0;
  }

      // Get the current temperature and pressure reading
    float currentTemperature = bmp.readTemperature();
    float currentPressure = bmp.readPressure() / 100.0;

    // Add the current readings to the arrays
    temperatures[currentIndex] = currentTemperature;
    pressures[currentIndex] = currentPressure;
    timestamps[currentIndex] = currentIndex;
    currentIndex = (currentIndex + 1);
    
    temperatures[currentIndex] = currentTemperature;
    pressures[currentIndex] = currentPressure;
    timestamps[currentIndex] = currentIndex;
    currentIndex = (currentIndex + 1);
}

void loop() {
  server.handleClient();
  server2.handleClient();

  unsigned long currentTime = millis();
  if (currentTime - lastReadingTime >= readingInterval) {
    lastReadingTime = currentTime;
    checkWiFiConnection();

    // Get the current temperature and pressure reading
    float currentTemperature = bmp.readTemperature();
    float currentPressure = bmp.readPressure() / 100.0;

    // Add the current readings to the arrays
    
    if (currentIndex < maxReadings){
      temperatures[currentIndex] = currentTemperature;
      pressures[currentIndex] = currentPressure;
      timestamps[currentIndex] = currentIndex;
      currentIndex = (currentIndex + 1);
    } else {
      for (int i = 0; i < maxReadings - 1; i++){
        temperatures[i] = temperatures[i+1];
        pressures[i] = pressures[i+1];
      }
      temperatures[maxReadings - 1] = currentTemperature;
      pressures[maxReadings - 1] = currentPressure;
    }
  }
}

String buildChartData() {
  DynamicJsonDocument json(2048);
  JsonArray temperaturesJson = json.createNestedArray("temperatures");
  JsonArray pressuresJson = json.createNestedArray("pressures");
  JsonArray timestampsJson = json.createNestedArray("timestamps");

  float minTemp = temperatures[0];
  float maxTemp = temperatures[0];
  float minPres = pressures[0];
  float maxPres = pressures[0];

  for (int i = 0; i < maxReadings; i++) {
    if (timestamps[i] != 0) {
      temperaturesJson.add(temperatures[i]);
      pressuresJson.add(pressures[i]);
      timestampsJson.add(timestamps[i]);

      if (temperatures[i] < minTemp) {
        minTemp = temperatures[i];
      }
      if (temperatures[i] > maxTemp) {
        maxTemp = temperatures[i];
      }
      if (pressures[i] < minPres) {
        minPres = pressures[i];
      }
      if (pressures[i] > maxPres) {
        maxPres = pressures[i];
      }
    }
  }

  String chartData;
  serializeJson(json, chartData);

  return chartData;
}

void handleRoot() {

  DynamicJsonDocument json(8192);

  JsonArray temperaturesJson = json.createNestedArray("temperatures");
  JsonArray pressuresJson = json.createNestedArray("pressures");
  JsonArray timestampsJson = json.createNestedArray("timestamps");

  for (int i = 0; i < maxReadings; i++) {
    if (timestamps[i] != 0) {
      temperaturesJson.add(temperatures[i]);
      pressuresJson.add(pressures[i]);
      timestampsJson.add(timestamps[i]);
    }
  }

  String chartData;
  serializeJson(json, chartData);

  String html = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@300;400;500&display=swap" rel="stylesheet">
    <link rel="icon" type="image/png" sizes="16x16" href="https://sagened.com/skyaware/images/favicon-16x16.png">
    <link rel="icon" type="image/png" sizes="32x32" href="https://sagened.com/skyaware/images/favicon-32x32.png">
    <link rel="apple-touch-icon" sizes="180x180" href="https://sagened.com/skyaware/images/apple-touch-icon.png">
    <title>Temperatur og lufttrykk</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
      body {
        font-family: 'Roboto', sans-serif;
        margin: 0;
        padding: 0;
        background: #f5f5f5;
        color: #333;
      }
      .container {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        max-width: 800px;
        margin: 0 auto;
        padding: 2rem;
      }
      h1 {
        font-weight: 500;
        font-size: 2.5rem;
        margin-bottom: 1rem;
      }
      p {
        font-weight: 300;
        font-size: 1.25rem;
        margin-bottom: 2rem;
      }
      canvas {
        max-width: 100%;
      }
    </style>
  </head>
  <body>
    <div class="container">

        <h1>)=====" + String(temperatures[currentIndex - 1],1) + R"=====( °C</h1>

      <canvas id="tempPressureChart"></canvas>
      <div id="minMaxValues" style="display: flex; justify-content: space-between; width: 100%; margin-top: 1rem;"></div>
    </div>

  <script>
    const chartData = )=====";
  html += chartData;
  html += R"=====(
    const ctx = document.getElementById('tempPressureChart').getContext('2d');
    const data = {
      labels: chartData.timestamps,
      datasets: [{
        label: 'Temperatur (°C)',
        data: chartData.temperatures,
        yAxisID: 'y1',
        backgroundColor: 'rgba(75, 192, 192, 0.2)',
        borderColor: 'rgba(75, 192, 192, 1)',
        borderWidth: 3,
        pointRadius: 0,
        tension: 0.4 //smooth from hard 0 to max 1
      }, {
        label: 'Lufttrykk [hPa]',
        data: chartData.pressures,
        yAxisID: 'y2',
        backgroundColor: 'rgba(255, 99, 132, 0.2)',
        borderColor: 'rgba(255, 99, 132, 1)',
        borderWidth: 3,
        pointRadius: 0,
        tension: 0.4
      }]
    };
    const config = {
    type: 'line',
    data: data,
    options: {
          plugins: {
        legend: {
          display: false // Hide the default legend
        }
      },
      scales: {
        x: {
          type: 'linear',
          title: {
            display: false,
            text: 'Time (s)'
          },
          min: 1,
          max:)=====" + String(currentIndex - 1) + R"=====(, //slett om ikke fungerer
          ticks: {
            display: false // Hide x-axis tick labels
          },
          grid: { 
            display: false,
            borderWidth: 0 
          }
        },
  y1: {
    title: {
      display: false,
      text: 'Temperature (°C)'
    },
    min: Math.min(...chartData.temperatures) - 5,
    max: Math.max(...chartData.temperatures) + 1,
    ticks: {
      stepSize: 5
    },
    grid: { display: false }
  },
  y2: {
    title: {
      display: false,
      text: 'Pressure (hPa)'
    },
    position: 'right',
    min: Math.min(...chartData.pressures) - 1,
    max: Math.max(...chartData.pressures) + 5,
    ticks: {
      stepSize: 5
    },
    grid: { display: false }
  }
      }
    },
  plugins: [{
      id: 'customLegend',
      afterDraw: function(chart) {
        const ctx = chart.ctx;
        const datasets = chart.data.datasets;

        // Custom label for Temperature
        ctx.strokeStyle = datasets[0].borderColor;
        ctx.lineWidth = datasets[0].borderWidth;
        ctx.beginPath();
        ctx.moveTo(chart.chartArea.left + 200, chart.chartArea.top + 10);
        ctx.lineTo(chart.chartArea.left + 250, chart.chartArea.top + 10);
        ctx.stroke();

        ctx.fillStyle = '#333';
        ctx.font = '18px Roboto';
        ctx.fillText('Temperatur [°C]', chart.chartArea.left + 60, chart.chartArea.top + 15);

        // Custom label for Pressure
        ctx.strokeStyle = datasets[1].borderColor;
        ctx.lineWidth = datasets[1].borderWidth;
        ctx.beginPath();
        ctx.moveTo(chart.chartArea.left + 600, chart.chartArea.top + 10);
        ctx.lineTo(chart.chartArea.left + 650, chart.chartArea.top + 10);
        ctx.stroke();

        ctx.fillStyle = '#333';
        ctx.font = '18px Roboto';
        ctx.fillText('Lufttrykk (hPa)', chart.chartArea.left + 460, chart.chartArea.top + 15);
      }
    }]
  };
    const tempPressureChart = new Chart(ctx, config);

    // Display min/max values below the graph
  document.getElementById('minMaxValues').innerHTML = `
    <div>
      <strong>Min:</strong> ${Math.min(...chartData.temperatures).toFixed(2)} °C
    </div>
    <div>
      <strong>Max:</strong> ${Math.max(...chartData.temperatures).toFixed(2)} °C
    </div>
    <div>
      <strong>Min:</strong> ${Math.min(...chartData.pressures).toFixed(2)} hPa
    </div>
    <div>
      <strong>Max:</strong> ${Math.max(...chartData.pressures).toFixed(2)} hPa
    </div>
  `;

    function refreshPage() {
    location.reload();
  }

  // Refresh the page every minute (60000 milliseconds)
  setInterval(refreshPage, 600000);
  </script>
  )=====";

  server.send(200, "text/html", html);
}

void handleNewPage() {
  String wordOfTheDay = get_random_word();
  String link = "https://google.com/search?q=define:" + wordOfTheDay;

  String html = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <title>Temperatur</title>
    <meta http-equiv="refresh" content="600"> <!-- Refresh the page every 10 minutes -->
    <title>New Page</title>
    <style>
      body {
        background-color: black;
        color: white;
        font-size: 6rem; /* Adjust this value to change the font size */
        font-family: Arial, sans-serif;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
      }
      #wordOfTheDay {
        font-size: 2rem;
        margin-top: 1rem;
      }
      a {
        color: white;
        text-decoration: none;
      }
    </style>
  </head>
  <body>
    <h1>)=====" + String(temperatures[currentIndex - 1], 1) + R"=====( °C</h1>
    <div id="wordOfTheDay" style="font-size: 48px;">Word of the Day: <a href=")=====" + link + R"=====(">)=====" + wordOfTheDay + R"=====(</a>
    </div>
  </body>
  </html>
  )=====";
  server2.send(200, "text/html", html);
}

void connectToWiFi() {
   WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}  

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
}
