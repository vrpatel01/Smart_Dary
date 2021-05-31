#include <ESP8266WiFi.h>

const char *ssid = "WIFI_SSID";
const char *password = "WIFI_PASSWD";

WiFiServer server(80);

String header;

String output15State = "off";
String fan_status = "OFF";
const float optimum_temp = 24;
float curr_temp = 10.0;
float threshold = optimum_temp;
float temp_auto_flag = false;
float minimum = 16;
float maximum = 40;



const int fan_sprinkler = 16; //Relay pin output that would control the fan and the motor

// LM35 temperature sensor
const int tempin = A0;

// LED Fan Pin
#define fled D1 //To indicate the sprinkler motor

void setup()
{
    Serial.begin(9600);
    Serial.println();

    // Initialize the output variables as outputs
    pinMode(fan_sprinkler, OUTPUT);
    pinMode(fled, OUTPUT);
    turn_off_fan();

    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" connected");

    server.begin();
    Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());
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
                    Serial.print("1:" + line);
                if (line[1] == 'R')
                {
                    threshold = parseln(line);
                    if (threshold > maximum )
                    {
                        threshold = maximum;
                    }
                    if (threshold < minimum )
                    {
                        threshold = minimum;
                    }
                    Serial.print("Threshld 1 is :" + (String)threshold);
                }
                // wait for end of client's request, that is marked with an empty line
                if (line.length() == 1 && line[0] == '\n')
                {
                    update_temprature();
                    if (curr_temp > threshold)
                        turn_on_fan();
                    else if (fan_status = "ON")
                        turn_off_fan();
                    client.println(prepareHtmlPage());
                    break;
                }
            }
        }

        while (client.available())
        {
            // but first, let client finish its request
            // that's diplomatic compliance to protocols
            // (and otherwise some clients may complain, like curl)
            // (that is an example, prefer using a proper webserver library)
            client.read();
        }

        // close the connection:
        client.stop();
        Serial.println("[Client disconnected]");
    }
}

// prepare a web page to be send to a client (web browser)
String prepareHtmlPage()
{
    String htmlPage;
    htmlPage.reserve(1024); // prevent ram fragmentation
    htmlPage = F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n" // the connection will be closed after completion of the response
                 "Refresh: 5\r\n"        // refresh the page automatically every 5 sec
                 "\r\n"
                 "<!doctype html>"
                 "<html lang=en>"
                 "<head>"
                 "<meta charset=utf-8>"
                 "<meta name=viewport content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
                 "<link rel=stylesheet href=https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css "
                 "integrity=sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm crossorigin=anonymous>"
                 "<style>.cnt{text-align:center}.bhk{text-align:center;font-size:40px;font-family:Times New Roman}"
                 ".bhl{text-align:center;font-size:15pt}.txt1{font-size:60px;text-align:center;background-color:orange}"
                 ".txt2{font-size:50px;text-align:center;background-color:green}.textbox{-webkit-border-radius:5px;"
                 "-moz-border-radius:5px;border-radius:5px;border:1px solid #848484;outline:0;height:40px;width:75px;"
                 "text-align:center;font-size:32px}</style>"
                 "<title>Temprature System</title>"
                 "</head>"
                 "<BODY>"
                 "<div class=txt1>"
                 "<p>Smart Dairy Farming</p>"
                 "</div>"
                 "<br>"
                 "<div class=txt2>"
                 "<p>Temprature System</p>"
                 "</div>"
                 "<br>"
                 "<form method=GET>"
                 "<div class=bhk>"
                 "<p>Current Temprature (°C)</p>"
                 "</div>"
                 "<div class=bhl>"
                 "<input class=textbox placeholder=");
    htmlPage += (String)curr_temp;

    htmlPage += F("disabled > "
                  "</div>"
                  "<br>"
                  "<div class=bhk>"
                  "<p> Enter new threshold</p>"
                  "</div>"
                  "<div class=cnt>"
                  "<input class=textbox type=number name=new_temp placeholder=");
    htmlPage += (String)threshold;

    htmlPage += F(">"
                  "</div>"
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
                  "<button type=button class=\"btn btn-secondary btn-lg\" disabled>Cooling System : </button>");

    if (fan_status == "ON")
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

void update_temprature()
{

    int analogValue = analogRead(tempin);
    float millivolts = (analogValue * 800) / 1023;
    float celsius = millivolts / 10;
    curr_temp = (int)celsius;

    /////
    Serial.print("in DegreeC=   ");
    Serial.println(celsius);
    /////

    delay(200);
}

void turn_on_fan()
{
    digitalWrite(fan_sprinkler, LOW);
    digitalWrite(fled, HIGH);          // change to low if opposite
    delay(100);
    fan_status = "ON";
}

void turn_off_fan()
{
    digitalWrite(fan_sprinkler, HIGH);
    digitalWrite(fled, LOW);         // change to low if opposite
    delay(100);
    fan_status = "OFF";
}

float parseln(String ln)
{
    char c = 'a';
    char temp[4] = "";
    int i = 39;
    int j = 0;
    while (c != '\0')
    {
        i++;
        c = ln[i];
        if (isdigit(c))
        {
            temp[j] = c;
            j++;
        }
        else
            break;
    }
    if (temp != "")
        return atoi(temp);
    else
        return threshold;
}
