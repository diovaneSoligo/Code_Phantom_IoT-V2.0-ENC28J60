#include <UIPEthernet.h>
#include <SPI.h>
#include <dht.h>

#define dht_dpin A5 //Pino DATA do Sensor ligado na porta Analogica A1

dht DHT; //Inicializa o sensor temperatura e umidade

byte mac[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06 };//endereço mac

IPAddress ip(192, 168, 0, 6); //seta o ip fixo
EthernetServer server(80); //porta do servidor, pois ele vai ser o server
EthernetClient cliente;

int val;
int VQ;
int ACSPin = A1;//entrada do sinal do sensor de corrente
void setup()
{
  Serial.begin(9600);


  VQ = determineVQ(A1); //Quiscent output voltage - the average voltage ACS712 shows with no load (0 A)
  //pinMode(A3, INPUT);//setar porta tensao

  //iniciar o servidor, passa o mac e o ip fixo
  Ethernet.begin(mac, ip);
  server.begin();

  //visualizar info da placa
  Serial.begin(9600);
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Mascara : ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS-ServerIP: ");
  Serial.println(Ethernet.dnsServerIP());

  pinMode(7, OUTPUT);//Define o pino 7 do rele como saida
  digitalWrite(7, LOW);//mantem o rele desligado, pois quem vai aciona-lo é o servidor mandando um GET

  pinMode(8, OUTPUT);//define o pino 8 como saida, LED VERDE
  digitalWrite(8, LOW);//desliga o led verde, liga se o rele estiver HIGH (ligado)

  pinMode(4, OUTPUT);//define o pino 4 como saida, LED VERMELHO
  digitalWrite(4, LOW);//desliga o led vermelho, ele liga se o rele estiver em LOW (desligado)

  digitalWrite(8, HIGH);
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(8, LOW);
  digitalWrite(4, LOW);
  delay(500);
  digitalWrite(8, HIGH);
  digitalWrite(4, HIGH);
  delay(500);
  digitalWrite(8, LOW);//desliga o led verde, liga se o rele estiver HIGH (ligado)
  digitalWrite(4, LOW);//desliga o led vermelho, ele liga se o rele estiver em LOW (desligado)
  delay(500);


  //sensor de corrente
  VQ = determineVQ(ACSPin); //Quiscent output voltage - the average voltage ACS712 shows with no load (0 A)
  delay(1000);

}

void loop()
{
  
  /*****verifica rele para os leds dianteiros da tomada Vermelho e Verde****/
  if (digitalRead(7) == HIGH) { //se o rele estiver ligado
    digitalWrite(8, HIGH); //liga LED VERDE
  } else {
    digitalWrite(8, LOW); //desliga LED VERDE
  }
  EthernetClient client = server.available();

  if (client) // se tiver cliente conectado
  {
    digitalWrite(4, HIGH); //liga LED VERMELHO
    boolean currentLineIsBlank = true;
    String linha;
    while (client.connected()) // em quanto o cliente estiver conectado
    {
      if (client.available())
      {
        char c = client.read(); //Variável para armazenar os caracteres que forem recebidos
        linha.concat(c); // Pega os valor após o IP do navegador ex: 192.168.1.2/1000?

        // Recebeu dois caracteres de fim de linha (\ n). O cliente terminou
        // envio do pedido.
        if (c == '\n' && currentLineIsBlank)
        {
          // Enviar um cabeçalho de resposta HTTP padrão
          // IMPORTANTE, ISSO FAZ O ARDUINO RECEBER REQUISIÇÃO AJAX DE OUTRO SERVIDOR E NÃO APENAS LOCAL.
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/javascript");
          client.println("Access-Control-Allow-Origin: *");
          client.println();

          int iniciofrente = linha.indexOf("?");

          if (iniciofrente > -1) { //verifica se existe comando

            iniciofrente  = iniciofrente + 6; // pega o caracter seguinte
            int fimfrente = iniciofrente + 3; //espera 3 caracteres

            String acao   = linha.substring(iniciofrente, fimfrente); //pega o valor do comando

            if (acao == "001") {//liga rele
              digitalWrite(7, HIGH);
              val = 1;
            }
            else if (acao == "000") {//desliga rele
              digitalWrite(7, LOW);
              val = 0;
            } else {
              //nao mostra nada
            }

            //mostra JSON pro cliente
            client.print("dados({ RELE : ");
            client.print(val);
            client.print(" , TENSAO : ");
            client.print(calcTensao());
            client.print(" , CORRENTE : ");
            client.print(readCurrent(ACSPin));
            client.print(" , TEMPERATURA : ");
            client.print(DHT.temperature);
            client.print(" , UMIDADE : ");
            client.print(DHT.humidity);
            client.print(" })");

          }
          break;

          break; // fim da transmissão
        }

        // Se o cliente enviou um caractere de fim de linha (\ n),
        // eleve a bandeira e veja se o próximo caractere é outro
        // caractere de fim de linha
        if (c == '\n')
        {
          currentLineIsBlank = true;
        } else if (c != '\r')
        {
          currentLineIsBlank = false;
        }


        // Se o cliente envia um caractere que não é um caractere de fim de linha,
        // redefinir o sinalizador e aguarde o caractere de fim de linha.
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }
    // tempo pro navegador receber os dados
    delay(400);

    // fexa conexão
    client.stop();
    digitalWrite(4, LOW);
  }
}


//Daqui pra baixo Sensor de Corrente
int determineVQ(int PIN) {
  long VQ = 0;
  //read 1000 samples to stabilise value
  for (int i = 0; i < 1000; i++) {
    VQ += abs(analogRead(PIN));
    delay(1);//depends on sampling (on filter capacitor), can be 1/80000 (80kHz) max.
  }
  VQ /= 1000;
  Serial.print(map(VQ, 0, 1023, 0, 5000));
  Serial.println(" mV");
  return int(VQ);
}

float readCurrent(int PIN) {
  int current = 0;
  int sensitivity = 52;//change this to 100 for ACS712-20A or to 66 for ACS712-30A
  //read 5 samples to stabilise value
  for (int i = 0; i < 200; i++) {
    current += abs(analogRead(PIN) - VQ);
    //delay(1);
  }
  current = map(current / 200, 0, 1023, 0, 5000);
  float corrente =    float(current) / sensitivity;
  /*
  if (corrente <= 0.08) {
    corrente = 0;
  }
  */
  Serial.print("Corrente: ");
  Serial.println(corrente);

  return corrente;
}
//******************************//
//Daqui pra baixo sensor tensão
int calcTensao() {

  float valor    = 0;
  int i = 0;
  float media = 0;
  float tensao = 0;

  while (i < 100) {// le X valores para calcular uma media
    valor = (5.0 * analogRead(A3)) / 1023;// regra de 3 5V —– 1023 X —– Valor Obtido
    media += valor;//armazena media
    i++;//incremento while
  }
  media /= 100;//faz a media

  tensao = (245 * media) / 5;//calcula tensao maxima(V) * media leitura sensor / 5V

  if (tensao < 90)//se indentificar tensao abaixo de 90v entende-se por desligada
  {
    tensao = 0;
  }
  Serial.print("Tensao: ");
  Serial.println (tensao);
  return tensao;
}
