import streamlit as st
import paho.mqtt.client as mqtt
import time
import json

# Definindo variáveis para o MQTT
MQTT_BROKER = "brw.net.br"  # Host do MQTT
MQTT_PORT = 1883
MQTT_TOPIC = "vibration/#"
MQTT_CLIENT_ID = "streamlit_client"
MQTT_USERNAME = "fatec"  # Insira seu nome de usuário aqui
MQTT_PASSWORD = "SQRT(e)!=172"    # Insira sua senha aqui

# Inicializando os dados de vibração
rms_x = []
rms_y = []
rms_z = []
classe = []
temperatura = []

# Função para conectar ao broker MQTT
def on_connect(client, userdata, flags, rc):
    print("Conectado com o código: " + str(rc))
    client.subscribe(MQTT_TOPIC)

# Função para processar as mensagens MQTT
def on_message(client, userdata, msg):
    global rms_x, rms_y, rms_z, classe, temperatura
    payload = msg.payload.decode('utf-8')

    # Processa os dados recebidos (assumindo que sejam valores numéricos)
    if msg.topic == "vibration/rms_x":
        rms_x.append(float(payload))
    elif msg.topic == "vibration/rms_y":
        rms_y.append(float(payload))
    elif msg.topic == "vibration/rms_z":
        rms_z.append(float(payload))
    elif msg.topic == "vibration/classe":
        classe.append(payload)
    elif msg.topic == "vibration/tp":
        temperatura.append(float(payload))

# Função para conectar e rodar o MQTT
def mqtt_loop():
    # Conectar ao MQTT
    client = mqtt.Client(MQTT_CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message

    # Configuração de autenticação
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)

    # Inicia o loop do MQTT em segundo plano
    client.loop_start()

# Função para iniciar a interface Streamlit
def main():
    st.title('Monitoramento de Vibração de Motor Elétrico')

    # Placeholder para gráficos e dados
    placeholder = st.empty()

    while True:
        with placeholder.container():
            # Exibe gráficos de vibração
            if rms_x:
                st.subheader('Vibração X')
                st.line_chart(rms_x)

            if rms_y:
                st.subheader('Vibração Y')
                st.line_chart(rms_y)

            if rms_z:
                st.subheader('Vibração Z')
                st.line_chart(rms_z)

            # Exibe a classe de vibração
            if classe:
                st.subheader('Classe de Vibração')
                st.write(classe[-1])  # Exibe a última classe recebida

            # Exibe a temperatura
            if temperatura:
                st.subheader('Temperatura')
                st.write(f"{temperatura[-1]} °C")  # Exibe a última temperatura recebida

            # Exibe a recomendação com base na classe de vibração
            if classe:
                if classe[-1] == 'A':
                    st.write("Estado [A]: Excelente. O motor está em boas condições.")
                elif classe[-1] == 'B':
                    st.write("Estado [B]: Bom. O motor está em condições aceitáveis.")
                elif classe[-1] == 'C':
                    st.write("Estado [C]: Preocupante. A manutenção é recomendada.")
                elif classe[-1] == 'D':
                    st.write("Estado [D]: Crítico. Manutenção corretiva necessária imediatamente.")

        # Aguarda 1 segundo antes de atualizar novamente
        time.sleep(1)

if __name__ == "__main__":
    # Inicia o loop MQTT em segundo plano
    mqtt_loop()

    # Roda o Streamlit
    main()
