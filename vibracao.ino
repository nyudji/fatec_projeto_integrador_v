/*************************************************************
  - Captura de dados de vibração de motor eletrico
  - Calculo do valor RMS 
  - Indice ABNT de "saude" do motor (NBR 10082)
    - Nível A: Boa condição, resultado comum após calibração da máquina;
    - Nível B: Aceitável, resultado comum em máquinas com manutenção regular e
operacionalidade dentro do esperado;
    - Nível C: Limite tolerável, resultado não aceitável para operação contínua. Nesse
caso de operação é necessário programar revisão para reajuste o mais rápido
possive;
    - Nível D: Não permissível, resultado inaceitável visto que o tipo de operação é
danosa para a máquina avaliada. Deve-se realizar manutenção corretiva imediamente.

  --- Classe de Mototres ----
  Classe 1 - Máquinas pequenas de potência até 15kW com acionamento por motor elétrico
  Classe 2 - Máquinas médias com potência entre 15 e 75kW, motores montados rigidamente 
             com potência de até 300kW
  Classe 3 - Máquinas motrizes com potência maior que 75kW, com massa rotativa montada 
            em fundação rígida e pesada
  Classe 4 - Máquinas do tipo classe 3 montadas sob fundações relativamente flexíveis
  


***************************************************************/
#include <string.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <arduinoFFT.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_sleep.h>





/*
  - Sugestoes
    - alguns dados de configuração podem virar variaveis configuradas pelo MQTT
    ex:
    SAMPLING_FREQ 1000.0 // Hz (deve ser um double)
    COLETA_TEMPO_MS 60000 // Tempo de coleta em milissegundos (1 minuto)
    SONO_TEMPO_MS 300000 // Tempo de sono em milissegundos (5 minutos)
    ssid = "USUARIO WI FI";
    password = "SENHA WI FI";
    mqtt_server = "HOST MQTT"; //brw.net.br

*/

//**************************************************************
//  Variaveis da placa 
//**************************************************************

// Misselania
#define  bt_boot 0
#define  buzzer  15
#define  led_in  2 
#define  btn     14

// Sensores
#define s01 33
#define s02 32 
#define s03 35
#define s04 34
#define s05 39
#define s06 36
#define s07 4
#define s08 16
#define s09 13

// RS232 TTL
#define RX 3
#define TX 1

// Reles
#define r01 12
#define r02 27
#define r03 26
#define r04 25

// SPI

#define spi_rst  17
#define spi_scs  5
#define pi_clk   18
#define spi_niso 19
#define spi_mosi 23

// I2C
#define i2c_sca  21
#define i2c_scl  22

#define FORMAT_SPIFFS_IF_FAILED true

#define AMOSTRAS 256 // Número de amostras (deve ser uma potência de 2)
#define SAMPLING_FREQ 1000.0 // Hz (deve ser um double)
#define COLETA_TEMPO_MS (1000 * 60) // Tempo de coleta em milissegundos (1 minuto)
#define SONO_TEMPO_MS (1000 * 60 * 5) // Tempo de sono em milissegundos (5 minutos)
#define CLASSE 1 // classe de potencia do motor


Adafruit_MPU6050 mpu;
WiFiClient espClient;
PubSubClient client(espClient);

double acc_x [AMOSTRAS];
double acc_y [AMOSTRAS];
double acc_z [AMOSTRAS];
double acc_t [AMOSTRAS];
int classe = CLASSE; // Classe de potencia do motor




//variaveis de controle do Hardware
 int tempoLed = 1000;
 int bzTempo;
 int bzVezes;
 bool bzAciona;

// Acesso ao WiFi e MQTT
const char* ssid = "realme C55";
const char* password = "d6qfat2m";

// Configuração MQTT
uint64_t chipid; 
uint8_t mac_int[6]; // Mac em formato "inteiro"
char mac_str [20];  // Mac  em formato String (usado para MQTT ClientID
const char* mqttServer = ""; //brw.net.br
const char* mqttUser = "fatec"; 
const char* mqttPassword = "";


void setup() {
  Serial.begin(115200); //921600

  iniciaHardware();

  conectarWiFi();

  client.setServer(mqttServer, 1883);

  iniciaMPU6050();
  
}

void loop() {

  if (!client.connected()) {
        reconnect();
    }
  
  coletaDados();

  calculaRMS();

  //vTaskDelay(pdMS_TO_TICKS(SAMPLING_FREQ));

}

//
// COLETA DE DADOS a guarda dados brutos para psteriormente calcular RMS
//
void coletaDados() {
    int n = 0;
    sensors_event_t a, g, temp;
    char buf [12];

    

    zeraMatrizes();
    Serial.println("----- Coletando dados: " + String (AMOSTRAS) + " amostras");
    for (int i = 0; i < AMOSTRAS; i++) {
      mpu.getEvent(&a, &g, &temp);
      acc_x [i] = a.acceleration.x;
      acc_y [i] = a.acceleration.y;
      acc_z [i] = a.acceleration.z;
      acc_t [i] = temp.temperature;
      
      dtostrf(acc_x [i], 0, 6, buf);
      client.publish("vibration/vx", buf);
      dtostrf(acc_y [i], 0, 6, buf);
      client.publish("vibration/vy", buf);
      dtostrf(acc_z [i], 0, 6, buf);
      client.publish("vibration/vz", buf);
      dtostrf(acc_t [i], 0, 6, buf);
      client.publish("vibration/tp", buf);
      
      Serial.printf("%f %f %f %f\n", acc_x[i], acc_y[i], acc_z[i], acc_t[i]);
          
      //vTaskDelay(pdMS_TO_TICKS(100));    
    }

    
    Serial.println("----- Amostra coletada ....");
}


//
// ZERA OS VALORES CONTIDOS NAS MATRIZES DE LEITURA
//
void zeraMatrizes() {
  int i;
  for (i = 0; i < AMOSTRAS; i++) {
    acc_x [i] = 0;
    acc_y [i] = 0;
    acc_z [i] = 0;
    acc_t [i] = 0;
  }
}

//
// SOMA DOAS DADOS DE LEITURA AO QUADRADO
//
double somaAcc(double acc [AMOSTRAS]) {
  double sm = 0.0;
  int i;

  for (i = 0; i < AMOSTRAS; i++) {
    sm = sm + pow (acc[i],2);
  }

  return (sm);
}


//
// MEDIA DOS DADOS DE LEITURA
//
double media(double acc [AMOSTRAS]) {
  int i, s;

  s = 0;  
  for (i = 0; i < AMOSTRAS; i++) {
    s = s + acc [i];
  }

  return (s/AMOSTRAS);

}



//
//  SOMA DOS DADOS DE LEITURA MENOS A MEDIA
//
void mediaAcc(double acc [AMOSTRAS]) {
  double med; // media da matriz
  int i;

  med = media(acc);

  for (i = 0; i < AMOSTRAS; i++) {
    acc [i]= acc [i] - med; 
  }


}
// ******************************************************
// ******      Coleta dados   ***************************
// --------    Calcula o valor RMS dos dados
// ******************************************************

void calculaRMS () {

  char leu [255];
  char *vx;
  char *vy;
  char *vz;
  char *tp;
  char* d = ";";
  double rmsX, rmsY, rmsZ, medTemperatura;
  char cv; //retorno da classe vibração
  char buf [12];
  
  
  
  mediaAcc (acc_x);
  mediaAcc (acc_y);
  mediaAcc (acc_z);
  medTemperatura = media (acc_t);
  rmsX = sqrt (somaAcc (acc_x)/AMOSTRAS);
  rmsY = sqrt (somaAcc (acc_y)/AMOSTRAS);
  rmsZ = sqrt (somaAcc (acc_z)/AMOSTRAS);
  cv = classeVibracao (rmsX, rmsY, rmsZ);

  dtostrf (rmsX, 0, 2, buf);
  client.publish("vibration/rms_x", buf);
  dtostrf (rmsY, 0, 2, buf);
  client.publish("vibration/rms_y", buf);
  dtostrf (rmsZ, 0, 2, buf);
  client.publish("vibration/rms_z", buf);
  dtostrf (medTemperatura, 0, 2, buf);
  client.publish("vibration/rms_t", buf);
  
  client.publish("vibration/classe", String(cv).c_str());
  Serial.print(rmsX,6);
  Serial.print(" ");
  Serial.print(rmsY,6);
  Serial.print(" ");
  Serial.print(rmsZ,6);
  Serial.print(" ");
  Serial.print(medTemperatura);
  Serial.print(" ");
  Serial.print(cv);
  Serial.println("");
   
}



//
// CLASSE DO VALOR DA VIBRAÇÃO
//
char classeVibracao (double x, double y, double z) {

  double f_x, f_y, f_z, faixa;

  f_x = faixaVibracao (x);
  f_y = faixaVibracao (y);
  f_z = faixaVibracao (z); 

  // seleciona o maior eixo de vibração
  faixa = f_x;

  if (f_x < f_y) 
    faixa = f_y; 

  if (faixa < f_z)
    faixa = f_z;

  // classifica pelas faixas de vibração
  if (classe = 1) { 
    if (faixa <= 0.71)
      return ('A');
    else if (faixa <= 1.8)
      return ('B');  
    else if (faixa <= 4.5)
      return ('C');  
    else 
      return ('D');
  }

  if (classe = 2) {
    if (faixa <= 1.12)
      return ('A');
    else if (faixa <= 2.8)
      return ('B');  
    else if (faixa <= 7.1)
      return ('C');  
    else 
      return ('D');      
  }

  if (classe = 3) {
    if (faixa <= 1.8)
      return ('A');
    else if (faixa <= 4.5)
      return ('B');  
    else if (faixa <= 11.2)
      return ('C');  
    else 
      return ('D');      
  }

  if (classe = 4) { 
    if (faixa <= 2.8)
      return ('A');
    else if (faixa <= 7.1)
      return ('B');  
    else if (faixa <= 18.0)
      return ('C');  
    else 
      return ('D');      
  }

} 

//
// FAIXA DOS VALORES DE VIBRAÇAO
//
double faixaVibracao (double valor) {

  if (valor < 0.112)
    return (0.11);
  else if (valor <= 0.18)
    return (0.18);  
  else if (valor <= 0.45)
    return (0.45);  
  else if (valor <= 0.71)
    return (0.71);
  else if (valor <= 1.12)
    return (1.12);
  else if (valor <= 1.8)
    return (1.8);
  else if (valor <= 2.8)
    return (2.8);              
  else if (valor <= 4.5)
    return (4.5);
  else if (valor <= 7.1)
    return (7.1); 
  else if (valor <= 11.2)
    return (11.2); 
  else if (valor <= 18.0)
    return (18.0);                                               
  else if (valor <= 28.0)
    return (28.0);
  else if (valor <= 45.0)
    return (45.0);
  else if (valor <= 71.0)
    return (71.0);
  else
    return (100.0);        

}




// ------------------------------------------------------------
// Configura Hardware
// ------------------------------------------------------------
void iniciaHardware () {

  pinMode(buzzer, OUTPUT); //
  pinMode(led_in, OUTPUT); //
  pinMode(btn, INPUT_PULLUP); //
  
  digitalWrite (buzzer, LOW);

  pinMode(s01, INPUT_PULLUP); // 
  pinMode(s02, INPUT_PULLUP); // 
  pinMode(s03, INPUT_PULLUP); // 
  pinMode(s05, INPUT_PULLUP); // 
  pinMode(s06, INPUT_PULLUP); // 
  pinMode(s07, INPUT_PULLUP); //
  pinMode(s08, INPUT_PULLUP); //
  pinMode(s09, INPUT_PULLUP); //

  pinMode (r01, OUTPUT); // 
  pinMode (r02, OUTPUT); // 
  pinMode (r03, OUTPUT); // 
  pinMode (r04, OUTPUT); // 

  xTaskCreate(PiscaLed, "PiscaLed", 1024 * 2, NULL, 4, NULL); 
  vTaskDelay(pdMS_TO_TICKS(300));

  xTaskCreate(Beep, "Beep", 1024 * 2, NULL, 4, NULL);
  vTaskDelay(pdMS_TO_TICKS(300));

  xTaskCreate(tkBotao, "tkBotao", 1024 * 3, NULL, 4, NULL);
  vTaskDelay(pdMS_TO_TICKS(300));

}

//------------------------------------------------------------------------------------------
//
// -----   Pisca LED 
//
//------------------------------------------------------------------------------------------

void PiscaLed (void *parameter) {
   
  while (true) {   
     digitalWrite(led_in, !digitalRead(led_in));
     vTaskDelay(pdMS_TO_TICKS(tempoLed));
  }
 
}


//*****************************************************************************
//
//  Funcoes de Buzzer
//
//*****************************************************************************

void actBeep (int msTempo, int xVezes) {
  
    bzTempo = msTempo;
    bzVezes = xVezes;
    bzAciona = true;

  
}

//*****************************************************************************
//
//  Beep task
//
//*****************************************************************************

void Beep (void * parameter) {
    int i = 0, tempo;


  vTaskDelay(pdMS_TO_TICKS(100)); 
  while (true) {

      
    vTaskDelay(pdMS_TO_TICKS(10)); 
    if (bzAciona) { 
      while (i < bzVezes) {
        digitalWrite (buzzer,HIGH);
        vTaskDelay(pdMS_TO_TICKS(bzTempo/2)); 
        digitalWrite (buzzer,LOW);
        vTaskDelay(pdMS_TO_TICKS(bzTempo/2)); 
        i++; 
      }
      bzAciona = 0;
      i = 0;
      
    }
  }
  
}

//------------------------------------------------------------------------------------------
// -----------------  task Botão de Configuração
//------------------------------------------------------------------------------------------

void tkBotao (void *parameter) {
  
int cnt1 = 0, cnt2; 
 
   
//Serial.println ("- Monitora Botão de CFG");  
vTaskDelay(pdMS_TO_TICKS(10000)); // tempo para task inicia
while (true)  { 
   
  vTaskDelay(pdMS_TO_TICKS(100)); // tempo para task inicia  
      
   
  while (digitalRead (btn) == LOW) {
   
    Serial.print ("Botão apertado ");
    Serial.print (cnt1);
    Serial.print (" - " );
    Serial.println (cnt2);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (cnt1 == 10) {
      actBeep (100, 1);
      cnt2++;  
     }
        
    if (cnt1 == 20) {
      actBeep (100, 1); 
      cnt2++;
     }
    if (cnt1 == 30) {
      actBeep (100, 1);
      cnt2++;
     } 
    if (cnt1 == 40) {
      actBeep (100, 1);
      cnt2++;
     }  
    cnt1++;  
    if (cnt1 > 40)
      cnt1  = 0;
    
  }
   // ve quantos segundos apertou
  if (cnt2 == 1) {
    //Serial.println (cnt2); 
    ESP.restart();
    //displayCfg ();// mostra config de rede no Display
  }
  if (cnt2 == 2) {
     //Serial.println (cnt2); 
     ESP.restart();
  }
  if (cnt2 == 3) {
    ESP.restart();
  }
  if (cnt2 == 4) {
    ESP.restart();
  }
  cnt2 = 0;
  cnt1 = 0;      
  }

}



// ------------------------------------------------------------
// Conecta ao WiFi
// ------------------------------------------------------------
void conectarWiFi() {
    Serial.print("Conectando ao WiFi: ");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    setMacAddress ();
    Serial.println("\nWiFi conectado!");

    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address: ");
    Serial.println(mac_str);
    
}

//------------------------------------------------------------------------------------------
//
// ------    pega MAC ADDREES do ESP
//
//------------------------------------------------------------------------------------------

void setMacAddress () {
  
 char mac01[5];
 char mac02[15];   
 char mca [50]; 
 

  chipid = ESP. getEfuseMac (); // O ID do chip é essencialmente seu endereço MAC (comprimento: 6 bytes).
  sprintf (mac01, "%04X" , ( uint16_t ) (chipid >> 32 )); // print High 2 bytes
  sprintf (mac02,"%08X" , ( uint32_t ) chipid); // imprime Low 4bytes.
  strncpy (mca, mac01,sizeof(mca));
  strcat (mca, mac02);
 

  strncpy (mac_str,mca,sizeof(mac_str));
  
  atoh(mac_int, (char *)mac_str); // mac para ETH
  
  
}

//------------------------------------------------------------------------------------------
//
// Função de char para (decimal) Hexa
//
//------------------------------------------------------------------------------------------

bool atoh(uint8_t * myuint,  char * mystring) {   
  int i, j;
  for (i = 0; i < (strlen(mystring) / 2); i++) {
          myuint[i] = 0;
          for (j = 0; j < 2; j++) {
              char firstchar = mystring[(i*2)+j];
              if (firstchar >= '0' && firstchar <= '9') {
                  myuint[i] = myuint[i]*16 + firstchar - '0';
              } else if (firstchar >= 'A' && firstchar <= 'F') {
                  myuint[i] = myuint[i]*16 + firstchar - 'A' + 10;
              } else if (firstchar >= 'a' && firstchar <= 'f') {
                  myuint[i] = myuint[i]*16 + firstchar - 'a' + 10;
              } else {
                  return (false);
              }
              
          }
      }
      return (true);
}


// --------------------------------------------------------
// INICIA MPU 6050
// --------------------------------------------------------
bool iniciaMPU6050 () {

  // Inicializa MPU 6050!
  if (!mpu.begin()) {
    Serial.println("Não encontrei o chip MPU6050");
    tempoLed = 200;
    actBeep (150, 3);
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP.restart ();
  }

  Serial.println("MPU6050 Encontrado!");

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);

  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  mpu.setGyroRange(MPU6050_RANGE_500_DEG);

  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);

  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  return (true);

}


// ----------------------------------------------
// Reconexão ao broker MQTT
// ----------------------------------------------
void reconnect() {

  while (!client.connected()) {
      Serial.print("Conectando ao servidor MQTT...");

      IPAddress serverIP;
      if (WiFi.hostByName(mqttServer, serverIP)) {
          Serial.print("\nServidor resolvido: ");
          Serial.println(serverIP);
      } else {
          Serial.println("Falha ao resolver o servidor MQTT.");
          delay(5000);
          continue; // Tenta novamente
      }

      // Tentar conectar ao broker MQTT
      if (client.connect(mac_str, mqttUser, mqttPassword)) {
          Serial.println("Conectado!");
          delay(2000);
      } else {
          Serial.print("Falha, rc=");
          Serial.print(client.state());
          Serial.println(" Tentando novamente em 5 segundos...");
          delay(5000);
      }
  }
}


// ---------------------------------------------------
// Inicializa arquivo de coleta de dados
// ---------------------------------------------------

void iniciaArquivo () {
  // Arquivo para gurdar dados
    if (!SPIFFS.begin(true)) {
        Serial.println("Falha ao inicializar SPIFFS.");
        return;
    }

    File file = SPIFFS.open("/vibration_data.csv", FILE_WRITE);
    if (file) {
        file.close();
    } else {
        Serial.println("Erro ao abrir o arquivo para escrita.");
        SPIFFS.format();
        ESP.restart ();
    }
}


// ******************************************************
// ******      Envia dados MQTT de arquivo  *************
// ******************************************************

void enviaDadosMQTT() {
    if (!client.connected()) {
        reconnect();
    }
    

    File file = SPIFFS.open("/vibration_data.csv", FILE_READ);
    if (file) {
        
        while (file.available()) {
            String line = file.readStringUntil('\n');
            client.publish("vibration/data", line.c_str());
            Serial.println(String(line.c_str()));
            client.loop(); // o loop é quem efetivamente envia então é bom estar aqui
            vTaskDelay(pdMS_TO_TICKS(1000));

        }
        file.close();
    } else {
        Serial.println("Falha ao abrir o arquivo para leitura.");
    }

    file = SPIFFS.open("/vibration_data.csv", FILE_WRITE); // Limpa o arquivo após envio
    file.close();
}
