<div align="center">
  <h1>Projeto IOT</h1>
  <img src="https://cdn-icons-png.flaticon.com/512/6119/6119533.png" alt="Icon" width="120">
</div>
<br>

Este projeto aborda o diagnóstico de motores de bombas d'água, utilizando tecnologias de IoT e análise de dados. A solução propõe sensores de vibração conectados via ESP32 para coleta contínua de dados, analisados em tempo real e por meio do RMS fazer diagnóstico referente ao motor. Permitindo assim, detecção de falhas iminentes, como desalinhamentos ou desgastes mecânicos.

A solução permite identificar falhas mecânicas iminentes, reduzindo custos, evitando paradas inesperadas e aumentando a vida útil dos equipamentos. Com isso, promovemos maior eficiência, confiabilidade e sustentabilidade na gestão de sistemas de bombeamento.

## Ferramentas
- Python
- Arduino
- Streamlit
- MQTTClient(Mobile)
- Eclipse Mosquitto
- PostgreSQL

## Funções
- Leitura de sensores MPU(giroscópio e temperatura)
- Calculo RMS(Root Mean Square)
- Monitoramento de dados
- Log em .txt e SQL

<div align="center">
    <img src="https://github.com/user-attachments/assets/92732e5f-a921-4a87-9471-585020ceded3" alt="dash1" width="450"/>
    <img src="https://github.com/user-attachments/assets/4560195f-544d-4d65-8a6e-2bacc8f864d8" alt="dash2" width="450"/>
</div>

## Requirements
- Python 3.12
- `streamlit` - plataforma web
- `pandas` - manipulação de dados
- `psycopg2` - conexao com postgre
- `schedule` - agendamento de tarefas
