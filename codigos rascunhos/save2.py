import time
import paho.mqtt.client as mqtt
from datetime import datetime
import schedule
import os

# Função para salvar log no arquivo de texto
def save_log_to_txt(log_data, filename='logs.txt'):
    timestamp = log_data['timestamp']
    diagnostic = log_data['diagnostic']
    message = log_data['message']
    
    # Cria a pasta 'logs' se não existir
    if not os.path.exists('logs'):
        os.makedirs('logs')
    
    # Abre o arquivo no modo append para adicionar novas entradas
    with open(os.path.join('logs', filename), 'a') as file:
        file.write(f"{timestamp} - {diagnostic} - {message}\n")

# Função para gerar log de erro
def log_error(diagnostic, cv, avg_x, avg_y, avg_z, avg_temp):
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    message = f"Erro: Classe {cv}, Média X: {avg_x:.2f}, Média Y: {avg_y:.2f}, Média Z: {avg_z:.2f}, Média Temp: {avg_temp:.2f}"
    log_data = {"timestamp": timestamp, "diagnostic": diagnostic, "message": message}
    
    # Salva o log de erro no arquivo de texto
    save_log_to_txt(log_data)
    print(f"Log de erro gerado: {message}")

# Função para configurar a conexão MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado com sucesso!")
        # Inscreve-se nos tópicos após a conexão bem-sucedida
        client.subscribe("vibration/#")
    else:
        print(f"Falha na conexão. Código de erro: {rc}")

# Função para tratar a recepção de mensagens MQTT
def on_message(client, userdata, msg):
    global rms_x_values, rms_y_values, rms_z_values, rms_temp_values, cv

    # Aqui, assumimos que a mensagem recebida é o valor que foi publicado
    if msg.topic == "vibration/rms_x":
        rms_x_values.append(float(msg.payload.decode()))
    elif msg.topic == "vibration/rms_y":
        rms_y_values.append(float(msg.payload.decode()))
    elif msg.topic == "vibration/rms_z":
        rms_z_values.append(float(msg.payload.decode()))
    elif msg.topic == "vibration/rms_t":
        rms_temp_values.append(float(msg.payload.decode()))
    elif msg.topic == "vibration/classe":
        cv = msg.payload.decode()

# Função para calcular a média dos valores
def calculate_average():
    global rms_x_values, rms_y_values, rms_z_values, rms_temp_values, cv
    
    # Verifica se existem valores para calcular a média
    if rms_x_values and rms_y_values and rms_z_values and rms_temp_values:
        avg_x = sum(rms_x_values) / len(rms_x_values)
        avg_y = sum(rms_y_values) / len(rms_y_values)
        avg_z = sum(rms_z_values) / len(rms_z_values)
        avg_temp = sum(rms_temp_values) / len(rms_temp_values)

        # Verifica a classe antes de gerar o log
        if cv == "A" or cv == "B":
            diagnostic = "OK"
            # Log de OK (quando A ou B)
            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            message = f"Classe {cv}, Média X: {avg_x:.2f}, Média Y: {avg_y:.2f}, Média Z: {avg_z:.2f}, Média Temp: {avg_temp:.2f}"
            log_data = {"timestamp": timestamp, "diagnostic": diagnostic, "message": message}
            # Salva o log no arquivo de texto
            save_log_to_txt(log_data)
        else:
            diagnostic = "Erro"
            log_error(diagnostic, cv, avg_x, avg_y, avg_z, avg_temp)

        # Limpa as listas após salvar os logs para calcular a média de novos valores
        rms_x_values.clear()
        rms_y_values.clear()
        rms_z_values.clear()
        rms_temp_values.clear()

# Função de reconexão caso a conexão MQTT seja perdida
def on_disconnect(client, userdata, rc):
    print(f"Desconectado do servidor com código {rc}")
    if rc != 0:
        print("Tentando reconectar...")
        client.reconnect()

# Função principal para iniciar o monitoramento
def save():
    # Definindo variáveis para o MQTT
    MQTT_BROKER = "brw.net.br"  # Host do MQTT
    MQTT_PORT = 1883
    MQTT_TOPIC = "vibration/#"
    MQTT_CLIENT_ID = "streamlit_client"
    MQTT_USERNAME = "fatec"  # Insira seu nome de usuário aqui
    MQTT_PASSWORD = "SQRT(e)!=172"  # Insira sua senha aqui

    # Variáveis globais
    global rms_x_values, rms_y_values, rms_z_values, rms_temp_values, cv
    rms_x_values = []
    rms_y_values = []
    rms_z_values = []
    rms_temp_values = []
    cv = ""

    # Configura o cliente MQTT
    client = mqtt.Client(MQTT_CLIENT_ID)
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)

    # Define as funções de conexão, desconexão e mensagem
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect
    client.on_message = on_message

    # Conectar ao broker MQTT
    client.connect(MQTT_BROKER, MQTT_PORT, 60)

    # Iniciar o loop para aguardar as mensagens MQTT
    client.loop_start()

    # Agendamento para coletar dados e calcular a média a cada 1 minuto
    schedule.every(1).minutes.do(calculate_average)

    # Loop principal
    while True:
        schedule.run_pending()
        time.sleep(1)

# Chama a função principal para iniciar o monitoramento
if __name__ == "__main__":
    save()
