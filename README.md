# Projeto IOT
![7082598](https://github.com/user-attachments/assets/daebd76d-198c-4f1e-8faa-ee9dadd08548)

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

## Requirements
- Python 3.9+
- `streamlit` - plataforma web
- `pandas` - manipulação de dados
- `psycopg2` - conexao com postgre
- `schedule` - agendamento de tarefas
