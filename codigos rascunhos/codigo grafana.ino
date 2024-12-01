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
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <MPU6050.h>

// Configurações do Wi-Fi e MQTT
const char* ssid = "yourSSID";
const char* password = "yourPassword";

// Configurações do MQTT
uint64_t chipid = ESP.getEfuseMac(); // Pega o MAC do ESP32
uint8_t mac_int[6];  // MAC em formato "inteiro"
char mac_str[20];    // MAC em formato String (usado para MQTT ClientID)
const char* mqttServer = "brw.net.br"; // Endereço do servidor MQTT
const char* mqttUser = "fatec"; 
const char* mqttPassword = "SQRT(e)!=172"; 

WiFiClient espClient;
PubSubClient client(espClient);
MPU6050 mpu;

// Função para conectar ao Wi-Fi
void conectarWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando em ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Wi-Fi conectado!");
  Serial.println("IP: " + WiFi.localIP());
}

// Função para reconectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect(mac_str, mqttUser, mqttPassword)) {
      Serial.println("conectado");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentaremos novamente em 5 segundos");
      delay(5000);
    }
  }
}

// Função para inicializar o MPU6050
void iniciaMPU6050() {
  if (!mpu.begin()) {
    Serial.println("Erro ao inicializar MPU6050");
    while (1);
  }
  Serial.println("MPU6050 inicializado com sucesso");
}

// Função para calcular a faixa de vibração
double faixaVibracao(double valor) {
  if (valor < 0.112) return 0.11;
  else if (valor <= 0.18) return 0.18;  
  else if (valor <= 0.45) return 0.45;  
  else if (valor <= 0.71) return 0.71;
  else if (valor <= 1.12) return 1.12;
  else if (valor <= 1.8) return 1.8;
  else if (valor <= 2.8) return 2.8;              
  else if (valor <= 4.5) return 4.5;
  else if (valor <= 7.1) return 7.1; 
  else if (valor <= 11.2) return 11.2; 
  else if (valor <= 18.0) return 18.0;                                               
  else if (valor <= 28.0) return 28.0;
  else if (valor <= 45.0) return 45.0;
  else if (valor <= 71.0) return 71.0;
  else return 100.0;
}

// Função para calcular a classe de vibração
char classeVibracao(double x, double y, double z, int classe) {
  double f_x, f_y, f_z, faixa;

  f_x = faixaVibracao(x);
  f_y = faixaVibracao(y);
  f_z = faixaVibracao(z);

  // seleciona o maior eixo de vibração
  faixa = f_x;
  if (f_x < f_y) faixa = f_y;
  if (faixa < f_z) faixa = f_z;

  // classifica pelas faixas de vibração de acordo com a classe
  if (classe == 1) {
    if (faixa <= 0.71) return 'A';
    else if (faixa <= 1.8) return 'B';  
    else if (faixa <= 4.5) return 'C';  
    else return 'D';
  }
  if (classe == 2) {
    if (faixa <= 1.12) return 'A';
    else if (faixa <= 2.8) return 'B';  
    else if (faixa <= 7.1) return 'C';  
    else return 'D';      
  }
  if (classe == 3) {
    if (faixa <= 1.8) return 'A';
    else if (faixa <= 4.5) return 'B';  
    else if (faixa <= 11.2) return 'C';  
    else return 'D';      
  }
  if (classe == 4) {
    if (faixa <= 2.8) return 'A';
    else if (faixa <= 7.1) return 'B';  
    else if (faixa <= 18.0) return 'C';  
    else return 'D';      
  }

  return 'A';  // valor padrão caso não se encaixe em nenhuma classe
}

// Função para calcular o RMS total
double calculaRMS(double x, double y, double z) {
  return sqrt((x * x + y * y + z * z) / 3);
}

// Função para diagnóstico do motor
String diagnosticoCondicao(char c) {
  switch (c) {
    case 'A': return "A (Ótimo estado)";
    case 'B': return "B (Estado aceitável)";
    case 'C': return "C (Limite tolerável)";
    case 'D': return "D (Não permissível)";
    default: return "Desconhecido";
  }
}

void setup() {
  Serial.begin(115200);
  conectarWiFi();
  client.setServer(mqttServer, 1883);

  // Converte MAC para string para usar como ClientID do MQTT
  sprintf(mac_str, "%02X%02X%02X%02X%02X%02X", mac_int[0], mac_int[1], mac_int[2], mac_int[3], mac_int[4], mac_int[5]);
  
  reconnect();
  iniciaMPU6050();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Leitura dos dados do MPU6050
  Vector rawData = mpu.readAccelerometer();
  double x = rawData.x;
  double y = rawData.y;
  double z = rawData.z;

  // Calcular a classe de vibração
  int classe = 2;  // Pode ser alterado conforme a classe desejada
  char classeDiagnostico = classeVibracao(x, y, z, classe);

  // Calcular RMS total
  double rmsTotal = calculaRMS(x, y, z);

  // Diagnóstico de condição do motor
  String diagnostico = diagnosticoCondicao(classeDiagnostico);

  // Publicar os dados no MQTT
  char buf[10];
  
  // Enviar RMS por eixo
  dtostrf(x, 0, 2, buf);
  client.publish("vibration/rms_x", buf);

  dtostrf(y, 0, 2, buf);
  client.publish("vibration/rms_y", buf);

  dtostrf(z, 0, 2, buf);
  client.publish("vibration/rms_z", buf);

  // Enviar RMS total
  dtostrf(rmsTotal, 0, 2, buf);
  client.publish("vibration/rms_t", buf);

  // Enviar classe de vibração
  client.publish("vibration/classe", String(classeDiagnostico).c_str());

  // Enviar diagnóstico do motor
  client.publish("vibration/diagnostico", diagnostico.c_str());
  
  // Imprimir os valores no Serial Monitor
  Serial.print("RMS X: "); Serial.println(x, 2);
  Serial.print("RMS Y: "); Serial.println(y, 2);
  Serial.print("RMS Z: "); Serial.println(z, 2);
  Serial.print("RMS Total: "); Serial.println(rmsTotal, 2);
  Serial.print("Temperatura Média: "); Serial.println(0.0, 2);  // Aqui você pode colocar a leitura real de temperatura se estiver disponível
  Serial.print("Diagnóstico: "); Serial.println(diagnostico);
  
  delay(2000);  // Delay para o próximo ciclo
}
