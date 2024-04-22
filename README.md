# Chat-MQTT - Tópicos XIII

## Download e instalação: 

- Para utilizar a aplicação é necessário ter instalado o broker mosquitto e a biblioteca paho-MQTT. Abaixo são demonstradas as instruções de utilização no Ubuntu 20.04.

### Mosquitto

```
sudo apt update
sudo apt install mosquitto
sudo apt install mosquitto-clients
```

### Paho-MQTT-C

```
git clone https://github.com/eclipse/paho.mqtt.c.git

cd paho.mqtt.c

make

sudo make install
```

## Execução:

- Para execução da aplicação deve se compilar o arquivo utilizando a biblioteca do paho mqtt e posteriormente executá-lo, como demonstrado abaixo:


```
gcc chat-mqtt.c -o chat -lpaho-mqtt3as -lpthread

./chat
```

## Utilização:

- Assim que executado, para usar o aplicativo, o usuário deve inserir o seu id único (01 - 99) com dois dígitos. Após a conexão, as funções do menu podem ser utilizadas escrevendo no terminal. A todo momento, independente da chegada de mensagens, os comandos dados no terminal serão direcionados à seleção do menu.

```
Digite seu ID único:
02
Bem-vindo! Agora você está online!
 
-- Escolha uma opção --
1. Iniciar um chat com um usuário
2. Aceitar uma solicitação de chat
3. Enviar mensagem em um chat
4. Entrar em um grupo
5. Enviar mensagem em um grupo
6. Desconectar do broker
0. Encerrar a aplicação
```