#include <UIPEthernet.h>//ENC28J60
#include <dht.h>//DHT11
/*******************************/
byte mac[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05}; //um endereço mac diferente para cada tomada
IPAddress ip(192, 168, 0, 6); //seta o ip fixo um ip para cada tomada
EthernetServer server(1000); //porta de acesso a tomada
/*******************************/
int VQ;
dht DHT;
/*******************************/
void setup()
{
  Ethernet.begin(mac, ip);
  server.begin();
  VQ = determineVQ(A1); //Quiscent output voltage - the average voltage ACS712 shows with no load (0 A)
  pinMode(7, OUTPUT);//Define o pino 7 do rele como saida
  digitalWrite(7, LOW);//mantem o rele desligado, pois quem vai aciona-lo é o servidor mandando um GET
  pinMode(8, OUTPUT);//define o pino 8 como saida, LED VERDE
  pinMode(4, OUTPUT);//define o pino 4 como saida, LED VERMELHO
  digitalWrite(8, HIGH);
  digitalWrite(4, HIGH);
  delay(2000);
  digitalWrite(8, LOW);//desliga o led verde, liga se o rele estiver HIGH (ligado)
  digitalWrite(4, LOW);//desliga o led vermelho, ele liga se o rele estiver em LOW (desligado
}
/******************************/
void loop()
{
  if (digitalRead(7) == HIGH) {
    digitalWrite(8, HIGH);
  } else {
    digitalWrite(8, LOW);
  }
  EthernetClient client = server.available();
  if (client)
  {
    digitalWrite(4, HIGH);
    boolean currentLineIsBlank = true;
    String linha;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        linha.concat(c);
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/javascript");
          client.println("Access-Control-Allow-Origin: *");
          client.println();
          int posicao = 5;
          boolean X = true;
          while (X == true) {//pega o comando recebido
            if (linha.substring(posicao, posicao + 1) == " ") {
              X = false;
            }
            posicao++;
          }
          String comando = linha.substring(5, posicao - 1);
          if (comando != "") {
            int v;
            float c;
            DHT.read11(A5);
            if (comando == "voltagem" or comando == "corrente" or comando == "potencia") {
              c = readCurrent();
              v = readVoltage();
            }
            if (comando == "umidade") {
              client.println(int(DHT.humidity));
            } else if (comando == "temperatura") {
              client.println(int(DHT.temperature));
            } else if (comando == "hello") {
              client.println("OKDSV2ENCPH@NTOM");
            } else if (comando == "voltagem") {
              client.println(v);
            } else if (comando == "corrente") {
              client.println(c);
            } else if (comando == "potencia") {
              client.println(v * c);
            } else if (comando == "ligar") {
              digitalWrite(7, HIGH);
            } else if (comando == "desligar") {
              digitalWrite(7, LOW);
            } else if (comando == "status") {
              if (digitalRead(7) == HIGH) {
                client.println("on");
              } else {
                client.println("off");
              }
            } else {
              //client.println("E404");
            }
          } else {
            //client.println("IoT By Diovane Soligo");
          }
          break; // fim da transmissão
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(480);
    client.stop();
    digitalWrite(4, LOW);
  }
  client.flush();
}
/**************************************/
//SENSOR TENSAO
int readVoltage() {
  float val = 0.00;
  float media = 0.00;
  for (int i = 1; i <= 1000; i++) {
    int valor = analogRead(A3);
    if ((valor > 100) & (valor < 650)) {
      val = ((analogRead(A3) * 5.000) / 1024) * 53;
    }
    if ((valor > 651) & (valor < 1023)) {
      val = ((analogRead(A3) * 5.000) / 1024) * 49;
    }
    media += val;
    delay(1);
  }
  media /= 1000;
  return int(media);
}
/*************************************/
//SENSOR CORRENTE
int determineVQ(int PIN) {
  long VQ = 0;
  //read 1000 samples to stabilise value
  for (int i = 0; i < 1000; i++) {
    VQ += abs(analogRead(PIN));
    delay(1);//depends on sampling (on filter capacitor), can be 1/80000 (80kHz) max.
  }
  VQ /= 1000;
  return int(VQ);
}
float readCurrent() {
  int current = 0;
  int sensitivity = 52;//change this to 100 for ACS712-20A or to 66 for ACS712-30A
  //read 5 samples to stabilise value
  for (int i = 0; i < 200; i++) {
    current += abs(analogRead(A1) - VQ);
    delay(1);
  }
  current = map(current / 200, 0, 1023, 0, 5000);
  float corrente = float(current) / sensitivity;
  if (corrente <= 0.08) {
    corrente = 0.0;
  }
  return corrente;
}
