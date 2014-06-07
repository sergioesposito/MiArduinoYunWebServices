/*
  Arduino Yun Bridge example

 This example for the Arduino Yun shows how to use the
 Bridge library to access the digital and analog pins
 on the board through REST calls. It demonstrates how
 you can create your own API when using REST style
 calls through the browser.

 Possible commands created in this shetch:

 * "/arduino/digital/13"     -> digitalRead(13)
 * "/arduino/digital/13/1"   -> digitalWrite(13, HIGH)
 

 This example code is part of the public domain

 http://arduino.cc/en/Tutorial/Bridge

 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <Process.h>
#include <FileIO.h>


// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

void setup() {
  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  
  // using A0 and A2 as vcc and gnd
  pinMode(A0, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A0, HIGH);
  digitalWrite(A2, LOW);

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
  FileSystem.begin();
}

void loop() {
  // Get clients coming from server
  YunClient client = server.accept();

  // There is a new client?
  if (client) {
    // Process request
    process(client);

    // Close connection and free resources.
    client.stop();
  }

  delay(50); // Poll every 50ms
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

  
  if (command == "controlar") {
    controlar(client);
  }
  
  if (command == "servicios") {
    servicios(client);
  }
  
 
}

void servicios(YunClient client) {
  // read the command
  String command = client.readStringUntil('!');

   
  if (command == "email") {
    emailCommand(client);
  }
  
  
  if (command == "twitter") {
    twitterCommand(client);
  }
}  

void controlar(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');
  
  if (command == "led") {
    digitalCommand(client);
  }
  
  if (command == "wifi") {
    wifiStatusCommand(client);
  }
  
  if (command == "temperatura") {
    leeTemperaturaCommand(client);
  }
  
}

void leeTemperaturaCommand(YunClient client) {
  const float CENTIVOLTIOS = 333.0;
  const float BITSANALOGICODIGITAL = 1023.0;
  const float FACTORFAHRENHEIT = 1.8;
  const int   SUMAFAHRENHEIT = 32;
  const char CELSIUS = 'C';
  const char FAHRENHEIT = 'F';
  int valorSensor;
  float temperaturaCelsius;
  float temperartura;
  
  char escala = client.read();
  valorSensor = analogRead(A1);
  temperaturaCelsius = valorSensor * CENTIVOLTIOS / BITSANALOGICODIGITAL;
  
  if(escala == CELSIUS){
    temperartura = temperaturaCelsius;
  } else {
    temperartura =  temperaturaCelsius * FACTORFAHRENHEIT + SUMAFAHRENHEIT;
  }  
   
  client.print("[{\"temperatura\":");
  client.print("\"");
  client.print(temperartura);
  client.print("\"");
  client.print("}]");
}  


void emailCommand(YunClient client) {
  Process p;
  String dataString;
  String de;
  String para;
  String asunto;
  String texto;
  String comandoShell;
  int resultado;
  String dummy;
  
  
  dataString+="From:"; 
  dummy = client.readStringUntil('=');
  
  de = client.readStringUntil('&');
  
  dataString+=de;
  dataString+="\n";
  
  dataString+="To:";
  dummy = client.readStringUntil('=');
  
  para = client.readStringUntil('&');
  
  dataString+=para;
  dataString+="\n";
  
  dataString+="Subject:";
  dummy = client.readStringUntil('=');
    
  asunto = client.readStringUntil('&');
  
  dataString+=asunto;
  dataString+="\n\n";
  
  dummy = client.readStringUntil('=');
    
  texto = client.readStringUntil('|');
  
  dataString+=texto;
  
  
  File dataFile = FileSystem.open("/mnt/sda1/mail.txt", FILE_WRITE);
  if (dataFile) {

    dataFile.println(dataString);
    dataFile.close();
    
  }  
  // if the file isn't open, pop up an error:
  else {
    client.println("error opening /mnt/sda1/mail.txt");
  } 
   comandoShell = "cat /mnt/sda1/mail.txt | ssmtp " + para + " 2>&1";
   resultado = p.runShellCommand(comandoShell);

   while(p.running());  
   
   while (p.available()>0) {
    char c = p.read();
    client.print(c);
  }
  
  client.print("[{\"Resultado\":");
  client.print("\"");
  client.print(resultado);
  client.print("\"");
  client.print("}]");
}  

void redSocialCommand(YunClient client, String redSocial) {
  Process p;   
  int resultado;
  String command = client.readStringUntil('=');
  if (command == "texto"){
      String texto =  client.readStringUntil('|');
      p.begin("/mnt/sda1/yun" + redSocial + ".py");      
      p.addParameter(texto); 
      resultado = p.run();
      
      while(p.running());  
      
      while (p.available()>0) {
    char c = p.read();
    client.print(c);
  }
   
      client.print("[{\"Resultado\":");
      client.print("\"");
      client.print(resultado);
      client.print("\"");
      client.print("}]");
  }  
}  

void wifiStatusCommand(YunClient client) {
  Process wifiCheck;  // initialize a new process
  String wifiCheckString;
  wifiCheckString = "\"";
  client.print("[{\"statuswifi\":");
  wifiCheck.runShellCommand("/usr/bin/pretty-wifi-info.lua");  // command you want to run
  
  // while there's any characters coming back from the
  // process, print them to the serial monitor:
  while (wifiCheck.available() > 0) {
    char c = wifiCheck.read();
    wifiCheckString += c;
  }
  wifiCheckString +=("\"");
  client.print(wifiCheckString);
  client.print("}]");
}  



void digitalCommand(YunClient client) {
  int pin, value;

  
  pin = 13;
  char caracter = client.read();
  
  if (caracter == '1' || caracter == '0') {
    value = caracter - '0';
    digitalWrite(pin, value);
  }
  else {
    value = digitalRead(pin);
  }
  
  client.print("[{\"statusled\":");
  client.print("\"");
  client.print(value);
  client.print("\"");
  client.print("}]");

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}





