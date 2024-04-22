#include "./chat-mqtt.h"

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf(" Causa: %s\n", cause);

	printf("Reconnecting...\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Falha ao reconectar. Erro %d\n", rc);
		finished = 1;
	}
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response){
	printf("Falha na desconexão. Erro: %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response){
	printf("Desconexão bem sucedida!\n");
	disc_finished = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Falha ao enviar a mensagem %d. Erro %d\n", response->token, response->code);
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;
	// if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	// {
	// 	printf("Failed to start disconnect, return code %d\n", rc);
	// 	exit(EXIT_FAILURE);
	// }
}

void onSend(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	//printf("Message with token value %d delivery confirmed\n", response->token);
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;
	// if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	// {
	// 	printf("Failed to start disconnect, return code %d\n", rc);
	// 	exit(EXIT_FAILURE);
	// }
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
	printf("Assinatura Concluida!\n");
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
	printf("Falha na assinatura. Erro: %d\n", response->code);
}

void onConnect(void* context, MQTTAsync_successData* response) {
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	int rc;

	printf("Conexão bem sucedida!\n");
    
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;

	if ((rc = MQTTAsync_subscribe(client, USER_TOPIC_CONTROL, QOS, &opts)) != MQTTASYNC_SUCCESS) {
		printf("Falha ao iniciar a assinatura no tópico de controle. Erro: %d\n", rc);
		finished = 1;
	}

    if ((rc = MQTTAsync_subscribe(client, USER_TOPIC_CLIENT, QOS, &opts)) != MQTTASYNC_SUCCESS) {
		printf("Falha ao iniciar a assinatura no tópico de cliente. Erro: %d\n", rc);
		finished = 1;
	}
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
	printf("Falha na conexão com o broker. Erro: %d\n", response->code);
	finished = 1;
}
