/*
  Universidad Politécnica de Madrid
  Máster Universitario en Ingeniería Web
  
  Implementación de Servicios Web para el Arduino Yun
  Elaborado por Sergio Antonio Espósito Pérez
  para el Proyecto de Fin de Máster.
  Junio 2014
  
  
  Este sketch implementa servicios de dos categorías
  
  1.- Control y acceso a recursos del Arduino Yun (pin digital, 
  sensor de temperatura conectado a pin analógico, consulta
  del estado de la red wifi ejecutando el comando Linux correspondiente
  2.- Interfaz con el mundo exterior (envío de email vía ssmtp, envío de tweet 
  y actualziación de status en facbook vía llamadas a scripts Python)
  
  Los servicios son consumidos  a través de llamadas estilo REST. 
  La respuesta de los servicios es en fromato JSON
  
  Catálogo de servicios
  =====================
  http://ip_arduino/arduino/controlar/led/leer 
    -->Obtiene el estado del led asociado al pin digital 13 del Arduino Yun
    -->Respuesta tipo [{"statusled":"x"}] Donde x puede ser 0 o 1 (apagado o encendido)
  http://ip_arduino/arduino/controlar/led/x
    -->Apaga/enciende el led asociado al pin digital 13 del Arduino Yun (x=0 apaga el led, x=1 enciende el led)
    -->Respuesta tipo [{"statusled":"x"}] Donde x es el estado del led luego de procesar la petición
  http://ip_arduino/arduino/controlar/temperatura/escala
    -->Obtiene la temperatura ambiente medida por un sensor LM35 conectado al pin analógico 1 del Arduino Yun, y la convierte a la escala indicada (escala=C Celsius, escala=F Fahrenheit)
    -->Respuesta tipo [{"temperatura":"nn.nn"}] nn.nn=Temperatura expresada en grados de la escala solictada
  http://ip_arduino/arduino/controlar/wifi/status
    -->Obtiene información acerca de la red wifi a la que está conectado el Arduino Yun, ejecutando el comando /usr/bin/pretty-wifi-info.lua y capturando su salida
    -->Respuesta tipo [{"statuswifi":"cadena"}] cadena=cadena de caracteres con información sobre la wifi
  http://ip_arduino/arduino/servicios/email!de=origen&para=destino&asunto=cadenaasunto&texto=cadenatexto|
    -->Envía un email desde la cuenta origen hasta la cuenta destino con el asunto y texto indicados, ejecutando el servicio ssmtp de Linux
    -->Respuesta tipo [{Resultado":"n"}] n=0 ejecutado correctamente, n<>0 ha habido algún error
  http://ip_arduino/arduino/servicios/twitter!texto=texto|
    -->Escribe un tweet en el  timeline de una cuenta preconfigurada con el texto indicado, ejecutando un script Python que usa la librería de código abierto Twython
    -->Respuesta tipo [{Resultado":"n"}] n=0 ejecutado correctamente, n<>0 ha habido algún error  
  http://ip_arduino/arduino/servicios/facebook!texto=texto|
    -->Actualiza el status de una cuenta Facebook preconfigurada con el texto indicado, ejecutando un script Python que usa la librería de código abierto Facebook Python SDK
    -->Respuesta tipo [{Resultado":"n"}] n=0 ejecutado correctamente, n<>0 ha habido algún error  
 
 Para consruir este skecth se ha tomado  ha tomado parcialmente y porteriormente modificado y adaptado código publicado en 

 http://arduino.cc/en/Tutorial/
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
  String command = client.readStringUntil('?');

   
  if (command == "email") {
    emailCommand(client);
  }
  
  
  if (command == "twitter" || command == "facebook") {
    redSocialCommand(client,  command);
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
  YunClient auxClient;
  
  
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
    
  texto = client.readStringUntil('\r');
  
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
  auxClient = client;
  
  comandoShell = "cat /mnt/sda1/mail.txt | ssmtp " + para + " 2>&1";
  resultado = p.runShellCommand(comandoShell);
  while(p.running());  
   
  client = auxClient;
  client.print("[{\"Resultado\":");
  client.print("\"");
  client.print(resultado);
  client.print("\"");
  client.print("}]");
}  

void redSocialCommand(YunClient client, String redSocial) {
  Process p;   
  int resultado;
  YunClient auxClient;
  String command = client.readStringUntil('=');
  if (command == "texto"){
      String texto =  client.readStringUntil('\r');
      p.begin("/mnt/sda1/yun" + redSocial + ".py");      
      p.addParameter(texto); 
      auxClient = client;
      
      resultado = p.run();
      while(p.running());  
      
      client = auxClient;
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





