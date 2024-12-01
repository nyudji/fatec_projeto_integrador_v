import streamlit as st
import paho.mqtt.client as mqtt
import time
import pandas as pd
from save import save

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
    st.title('Monitoramento de vibração de motores')
    with st.expander("Sumário dos Diagnósticos"):
        st.markdown("""
        - **Nível A**: Boa condição, resultado comum após calibração da máquina;
        - **Nível B**: Aceitável, resultado comum em máquinas com manutenção regular e operacionalidade dentro do esperado;
        - **Nível C**: Limite tolerável, resultado não aceitável para operação contínua. Nesse caso de operação é necessário programar revisão para reajuste o mais rápido possível;
        - **Nível D**: Não permissível, resultado inaceitável visto que o tipo de operação é danosa para a máquina avaliada. Deve-se realizar manutenção corretiva imediatamente.
        """)

    # Placeholder para gráficos e dados
    placeholder = st.empty()

    while True:
        with placeholder.container():
            # Exibe a recomendação com base na classe de vibração
            if classe:
                st.subheader('Diagnóstico')
                if classe[-1] == 'A':
                    col1, col2 = st.columns([1, 2])
                    with col1:
                        st.markdown("<h2 style='text-align: center;'>Estado</h2>", unsafe_allow_html=True)
                    with col2:
                        st.markdown("<h2 style='color: green; text-align: left;'>[A]</h2>", unsafe_allow_html=True)
                        st.write("**O motor está em boas condições.**")
                elif classe[-1] == 'B':        
                    col1, col2 = st.columns([1, 2])
                    with col1:
                        st.markdown("<h2 style='text-align: center;'>Estado</h2>", unsafe_allow_html=True) 
                    with col2:
                        st.markdown("<h2 style='color: lightblue; text-align: left;'>[B]</h2>", unsafe_allow_html=True)
                        st.write("**O motor está em condições aceitáveis.**")
                elif classe[-1] == 'C':
                    col1, col2 = st.columns([1, 2])
                    with col1:
                        st.markdown("<h2 style='text-align: center;'>Estado</h2>", unsafe_allow_html=True)
                    with col2:
                        st.markdown("<h2 style='color: orange; text-align: left;'>[C]</h2>", unsafe_allow_html=True)
                        st.write("**A manutenção é recomendada.**")
                elif classe[-1] == 'D':
                    col1, col2 = st.columns([1, 2])
                    with col1:
                        st.markdown("<h2 style='text-align: center;'>Estado</h2>", unsafe_allow_html=True)
                    with col2:
                        st.markdown("<h2 style='color: red; text-align: left;'>[D]</h2>", unsafe_allow_html=True)
                        st.write("**Manutenção corretiva necessária imediatamente.**")


            # Gráfico combinado de vibrações
            st.subheader('Gráfico Combinado de Vibrações (X, Y, Z)')            
            if rms_x and rms_y and rms_z:
                # Cria um DataFrame para combinar os eixos
                data = {
                    "Vibração X": rms_x[-100:],  # Últimos 100 pontos
                    "Vibração Y": rms_y[-100:],  # Últimos 100 pontos
                    "Vibração Z": rms_z[-100:]   # Últimos 100 pontos
                }
                df = pd.DataFrame(data)
                st.line_chart(df)

            # Gráficos individuais menores
            st.subheader('X, Y, Z')
            col1, col2, col3 = st.columns(3)  # Divide em 3 colunas
            
            with col1:
                st.caption("Vibração X")
                if rms_x:
                    st.line_chart(rms_x[-50:])  # Exibe apenas os últimos 50 pontos
                
            with col2:
                st.caption("Vibração Y")
                if rms_y:
                    st.line_chart(rms_y[-50:])  # Exibe apenas os últimos 50 pontos
                
            with col3:
                st.caption("Vibração Z")
                if rms_z:
                    st.line_chart(rms_z[-50:])  # Exibe apenas os últimos 50 pontos

            # Exibe a temperatura
            if temperatura:
                st.subheader('Temperatura')
                st.write(f"**{temperatura[-1]} °C**")  # Exibe a última temperatura recebida


        # Aguarda 1 segundo antes de atualizar novamente
        time.sleep(1)

if __name__ == "__main__":
    # Inicia o loop MQTT em segundo plano
    mqtt_loop()

    # Roda o Streamlit
    main()
  