
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Replace with your network credentials
const char *ssid = "WIFI_SSID";
const char *password = "WIFI_PASSWD";

// Set web server port number to 80
WiFiServer server(80);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// pins
#define hopper D1
#define motor D2
#define water_supply D8
#define water_sensor A0

// Variable to store the HTTP request
String header;
int food_amount = 40;
int last_food_amount = food_amount;
float water_level = 0;
int hr_c = 0, mn_c = 0, se_c = 0;
int hr_r = 0, mn_r = 4;
unsigned long epochTime;
unsigned long new_time;
String food_system_status = "OFF";
String water_system_status = "OFF";

// food system flag
bool food_flag = true;

void setup()
{
    Serial.begin(9600);
    Serial.println();

    // initilalize pins

    pinMode(hopper, OUTPUT);
    pinMode(motor, OUTPUT);
    pinMode(water_supply, OUTPUT);
    pinMode(water_sensor, INPUT);

    //.. start with off state food
    digitalWrite(motor, HIGH);
    digitalWrite(hopper, LOW);
    food_system_status = "OFF";

    //.. start with off state water
    digitalWrite(water_supply, LOW);
    water_system_status = "OFF";

    // Connect to Wi-Fi network with SSID and password
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    // Print local IP address and start web server
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    server.begin();

    //Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

    // Initialize a NTPClient to get time
    timeClient.begin();
    // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    timeClient.setTimeOffset(19800);
}

void loop()
{
    WiFiClient client = server.available();
    // wait for a client (web browser) to connect
    if (client)
    {
        Serial.println("\n[Client connected]");
        while (client.connected())
        {
            // read line by line what the client (web browser) is requesting
            if (client.available())
            {
                String line = client.readStringUntil('\r');
                if (line[1] == 'R')
                {
                    Serial.print(line);
                    parseln(line);
                }

                // wait for end of client's request, that is marked with an empty line
                if (line.length() == 1 && line[0] == '\n')
                {
                    update_water_system();
                    update_food_system();
                    client.println(prepareHtmlPage());
                    delay(7000); // custoom delay
                    break;
                }
            }
        }

        while (client.available())
        {
            client.read();
        }

        // close the connection:
        client.stop();
        Serial.println("[Client disconnected]");

        // remove if website lag
        update_water_system();
        update_food_system();
    }
    update_water_system();
    update_food_system();
}

String prepareHtmlPage()
{
    String htmlPage;
    htmlPage.reserve(1024); // prevent ram fragmentation
    htmlPage = F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n" // the connection will be closed after completion of the response
                 // "Refresh: 5\r\n"        // refresh the page automatically every 5 sec
                 "\r\n"
                 "<!doctype html>"
                 "<html lang=en>"
                 "<head>"
                 "<meta charset=utf-8>"
                 "<meta name=viewport content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
                 "<link rel=stylesheet href=https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css"
                 " integrity=sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm crossorigin=anonymous>"
                 "<style>.cnt{text-align:center}.bhk{text-align:center;font-size:40px;font-family:Times New Roman}"
                 ".bhl{text-align:center;font-size:15pt}.txt1{text-align:center;font-size:50px;background-color:orange}"
                 ".txt2{font-size:40px;text-align:center;background-color:green}.textbox{-webkit-border-radius:5px;"
                 "-moz-border-radius:5px;border-radius:5px;border:1px solid #848484;outline:0;height:40px;width:75px;"
                 "text-align:center;font-size:32px}</style>"
                 "<title>Feeding System</title>"
                 "</head>"
                 "<BODY>"
                 "<div class=txt1>"
                 "<H1>"
                 "<p>Smart Dairy Farming</p>"
                 "</H1>"
                 "</div>"
                 "<div class=txt2>"
                 "<H1>"
                 "<p>Feeding System </p>"
                 "</H1>"
                 "</div>"
                 "<form method=GET>"
                 "<div class=bhk>"
                 "<p> Enter Amount in (KG)</p>"
                 "</div>"
                 "<div class=bhl>"
                 "<input class=textbox type=number name=food_amount  placeholder=");
    htmlPage += (String)last_food_amount;

    htmlPage += F(">"
                  "</div>"
                  "<div class=bhk>"
                  "<p> Enter Time When food is to be given</p>"
                  "</div>"
                  "<div class=bhl><input type=time id=time name=time_val ></div>"
                  "<br>"
                  "<br>"
                  "<div class=cnt>"
                  "<button type=submit class=\"btn btn-success btn-lg\"> OK </button>"
                  "</div>"
                  "</form>"
                  "<br>"
                  "<br>"
                  "<div class=cnt>"
                  "<div class=btn-group role=group aria-label=\"Basic example\">"
                  "<button type=button class=\"btn btn-secondary btn-lg\" disabled>Food System : </button>");

    if (food_system_status == "ON")
        htmlPage += F("<button type=button class=\"btn btn-secondary\">ON</button>");
    else
        htmlPage += F("<button type=button class=\"btn btn-secondary\">OFF</button>");

    htmlPage += F("</div>"
                  "</div>"
                  "<br>"
                  "<div class=cnt>"
                  "<div class=btn-group role=group aria-label=\"Basic example\">"
                  "<button type=button class=\"btn btn-secondary btn-lg\" disabled>Water System : </button>");

    if (water_system_status == "ON")
        htmlPage += F("<button type=button class=\"btn btn-secondary\">ON</button>");
    else
        htmlPage += F("<button type=button class=\"btn btn-secondary\">OFF</button>");

    htmlPage += F("</div>"
                  "</div>"
                  "</BODY>"
                  "</HTML"
                  "\r\n");
    return htmlPage;
}

void update_water_system()
{
    water_level = analogRead(water_sensor);
    Serial.println("water_level : " + (String)water_level);
    if (water_level < 130)
    {
        digitalWrite(water_supply, HIGH);
        water_system_status = "ON";
    }
    else if (water_level > 220)
    {
        digitalWrite(water_supply, LOW);
        water_system_status = "OFF";
    }
    delay(100);
}

void update_food_system()
{
    timeClient.update();

    hr_c = timeClient.getHours();
    Serial.print("Hour: ");
    Serial.print(hr_c);

    if (hr_c == hr_r && food_flag)
    {
        mn_c = timeClient.getMinutes();
        Serial.print(" Minutes: ");
        Serial.print(mn_c);
        if (mn_c == mn_r)
        {
            se_c = timeClient.getSeconds();
            Serial.print(" Seconds: ");
            Serial.println(se_c);
            if (se_c <= 7)
                start_food_system();
        }
    }

    if (food_flag == false)
    {
        timeClient.update();
        epochTime = timeClient.getEpochTime();
        if (epochTime > new_time)
            stop_food_system();
    }
    delay(200);
}

void start_food_system()
{
    food_flag = false;
    int duration = calc_time();
    timeClient.update();
    epochTime = timeClient.getEpochTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);
    new_time = epochTime + duration;

    digitalWrite(motor, LOW);
    digitalWrite(hopper, HIGH);
    food_system_status = "ON";
    delay(100);
}

void stop_food_system()
{
    food_flag = true;
    digitalWrite(motor, HIGH);
    digitalWrite(hopper, LOW);
    food_system_status = "OFF";
    delay(100);
}

int calc_time()
{
    // duration will depend on hopper size belt size (length,width)
    return food_amount;
}

void parseln(String ln)
{
    char c = 'a';
    char temp[3] = "";
    char tim[2] = "";
    int i = 43;
    int h = 54;
    int j = 0;
    int offset = 0;

    while (c != '\0')
    {
        c = ln[j];
        j++;
    }

    if (j > 62)
    {
        i++;
        j = 0;
        c = ln[i];
        if (isdigit(c))
        {
            temp[j] = c;
            i++;
            j++;
            c = ln[i];
            if (isdigit(c))
            {
                temp[j] = c;
                i++;
                j++;
                c = ln[i];
                if (isdigit(c))
                {
                    temp[j] = c;
                    i++;
                    j++;
                    c = ln[i];
                    offset += 1;
                }
                offset += 1;
            }
            offset += 1;
        }

        food_amount = atoi(temp);
        h += offset;
        tim[0] = ln[h];
        tim[1] = ln[h + 1];
        hr_r = atoi(tim);
        h += 5; // %3A
        tim[0] = ln[h];
        tim[1] = ln[h + 1];
        mn_r = atoi(tim);

        Serial.println("Val food_amount = " + (String)food_amount);
        Serial.println("Val hr_r = " + (String)hr_r);
        Serial.println("Val mn_r = " + (String)mn_r);
        return;
    }
    return;
}
