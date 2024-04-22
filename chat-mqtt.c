#include "./callback_fun.c"

void menu(){
    printf("-- Escolha uma opção --\n");
    printf("1. Iniciar um chat com um usuário\n");
    printf("2. Aceitar uma solicitação de chat\n");    
    printf("3. Enviar mensagem em um chat\n");
    printf("4. Entrar em um grupo\n");
    printf("5. Enviar mensagem em um grupo \n");
	printf("6. Desconectar do broker \n");
    printf("0. Encerrar a aplicação\n");
}

void delete_solicitation(int id){
    strcpy(TOPICS_PENDENTS[id], "");
}

void pub_msg(char * topic, char * payload, MQTTAsync client){
    int rc;

    MQTTAsync_message message = MQTTAsync_message_initializer;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

    opts.onSuccess = onSend;
    opts.onFailure = onSendFailure;
    opts.context = client;

    message.payload = payload;
    message.payloadlen = (int)strlen(payload);
    message.qos = QOS;

    if((rc = MQTTAsync_sendMessage(client, topic, &message, &opts)) != MQTTASYNC_SUCCESS){
        printf("Falha ao publicar a mesangem. Erro: %d\n", rc);
        MQTTAsync_destroy(client);
        exit(0);
    }

}

void sub_topic(char * topic_sub, MQTTAsync client){
    int rc;

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;

    if((rc = MQTTAsync_subscribe(client, topic_sub, QOS, &opts)) != MQTTASYNC_SUCCESS){
        printf("Falha ao assinar o tópico, Erro: %d\n", rc);
        MQTTAsync_destroy(&client);
        exit(0);
    }               
}

void temp_chat(char * rec_user, MQTTAsync client){
    sprintf(TOPICS_PENDENTS[atoi(rec_user)], "%.2s_Solicitation", rec_user);
}

void accept_chat(char * rec_user, MQTTAsync client){
    char topic_rec[20]="", topic_chat[50]="";
    int session_id = rand() % 1000;
    
    sprintf(topic_rec, "%.2s_Client", rec_user);
    sprintf(topic_chat, "%.2s_%.2s_Chat_%d", USER_ID, rec_user, session_id);

    pthread_mutex_lock(&sub_topic_mutex);
    sub_topic(topic_chat, client);
    pthread_mutex_unlock(&sub_topic_mutex);

    pthread_mutex_lock(&topics_online_mutex);
    sprintf(TOPICS_ONLINE[atoi(rec_user)], "%s", topic_chat);
    pthread_mutex_unlock(&topics_online_mutex);

    sprintf(topic_chat, "AC_%.2s_%.2s_%.2s_Chat_%d", USER_ID, USER_ID, rec_user, session_id);

    pthread_mutex_lock(&pub_msg_mutex);
    pub_msg(topic_rec, topic_chat, client);
    pthread_mutex_unlock(&pub_msg_mutex);

}

void deny_chat(char * rec_user, MQTTAsync client){
    char topic_rec[20]="", deny_msg[30]="";
    int session_id = rand() % 1000;
    
    sprintf(topic_rec, "%.2s_Client", rec_user);
    sprintf(deny_msg, "DN_%.2s", USER_ID);
    
    pthread_mutex_lock(&pub_msg_mutex);
    pub_msg(topic_rec, deny_msg, client);
    pthread_mutex_unlock(&pub_msg_mutex);
}

int msgarrvd(void *context, char *topic_name, int topic_len, MQTTAsync_message *message){
    int rc;
    char user_id[5];
    MQTTAsync client = (MQTTAsync)context;

    if(!strcmp(topic_name, USER_TOPIC_CONTROL)){
        strncpy(user_id, (char *)message->payload, 2);
        printf("User %.2s Enviou uma solicição de chat!\n", (char *)message->payload);
        
        temp_chat(user_id, client);
    }
    else if(!strcmp(topic_name, USER_TOPIC_CLIENT)){
        
        char * rep = strtok((char *)message->payload, "_");
        char * user_rec = strtok(NULL, "_");

        if(!strcmp(rep, "AC")){
            printf("Chat do user %s aceito!\n", user_rec);

            char * new_topic = strtok(NULL, ";");
            
            pthread_mutex_lock(&sub_topic_mutex);
            sub_topic(new_topic, client);
            pthread_mutex_unlock(&sub_topic_mutex);

            pthread_mutex_lock(&topics_online_mutex);
            sprintf(TOPICS_ONLINE[atoi(user_rec)], "%s", new_topic);
            pthread_mutex_unlock(&topics_online_mutex);
        }
        else if(!strcmp(rep, "DN")){
            printf("Chat do user %s negado!\n", user_rec);
        }
 
    }
    else{
        //printf("   Topic: %s\n", topic_name);
        char * sender = strtok((char *)message->payload, ";");
        char * user_rec = strtok(NULL, ";");

        if(strcmp(user_rec, USER_ID_ID)){

            if(!strcmp(sender, "US")){

                char * message_rec = strtok(NULL, ";");
                
                printf("\nOlha a mensagem!\n");
                printf("User %s: %s\n", user_rec, message_rec);

            }

            else if(!strcmp(sender, "GP")){

                char * rec_group = strtok(NULL, ";");
                char * message_rec = strtok(NULL, ";");

                printf("\nOlha a mensagem! - %s\n", rec_group);
                printf("User %s: %s\n", user_rec, message_rec);
                
            }

            sleep(4);
            menu();
        }

    }
    return 1;
}

void handle_new_chat(MQTTAsync client){
    int sel, def, vrf = 0; 
    char user_id[4];

    for(int i = 0; i < 99; i++){
        if(strlen(TOPICS_PENDENTS[i]) > 6){
            printf("%d - User %2d\n", i, i);
            vrf++;
        }
    }

    if(!vrf){
        printf("Você não tem nenhuma solicitação de chat aberta\n");
        return;
    }
    printf("Selecione um dos chats acima para aceitar ou recusar:\n");

    scanf("%d", &sel);

    printf("Você deseja aceitar ou recusar (1-Sim / 2-Não)?\n");
    
    scanf("%d", &def);

    strncat(user_id, TOPICS_PENDENTS[sel], 2);

    delete_solicitation(sel);

    if(def == 1){
        printf("Enviando confirmação...\n");
        accept_chat(user_id, client);
    }
    else{
        printf("Excluindo solicitação\n");
        deny_chat(user_id, client);
    }
}

void send_msg_chat(MQTTAsync client){
    int sel, vrf = 0; 
    char msg_topic[20]="", message[64], payload[90];

    for(int i = 0; i < 99; i++){
        if(strlen(TOPICS_ONLINE[i]) > 5){
            printf("%d - User %2d\n", i, i);
            vrf++;
        }
    }

    if(!vrf){
        printf("Você não tem nenhuma sessão de chat aberta\n");
        return;
    }
    printf("Selecione um dos chats acima para mandar mensagem:\n");

    scanf("%d", &sel);
    strcpy(msg_topic, TOPICS_ONLINE[sel]);

    printf("Digite a mensagem que deseja enviar:\n");

    __fpurge(stdin);
    
    fgets(message, sizeof(message), stdin);

    sprintf(payload, "US;%s;%s;", USER_ID_ID, message);

    pthread_mutex_lock(&pub_msg_mutex);
    pub_msg(msg_topic, payload, client);
    pthread_mutex_unlock(&pub_msg_mutex);

}

void send_msg_group(MQTTAsync client){
    int sel; 
    char msg_topic[20]="", message[64], payload[120];

    for(int i = 1; i < group_control; i++){
        printf("%d - %s\n", i, GP_TOPICS_ONLINE[i]);
    }

    if(group_control == 1){
        printf("Você não está cadastrado em nenhum grupo!\n");
        return;
    }

    printf("Selecione um dos chats acima para mandar mensagem:\n");


    scanf("%d", &sel);
    strcpy(msg_topic, GP_TOPICS_ONLINE[sel]);

    printf("Digite a mensagem que deseja enviar:\n");

    __fpurge(stdin);
    
    fgets(message, sizeof(message), stdin);

    sprintf(payload, "GP;%s;%s;%s;", USER_ID_ID, msg_topic, message);

    pthread_mutex_lock(&pub_msg_mutex);
    pub_msg(msg_topic, payload, client);
    pthread_mutex_unlock(&pub_msg_mutex);

}

void sub_group(MQTTAsync client){
    int rc;
    char group[20], group_in_file[2], topic[30]="";

    printf("Qual o nome do grupo que você deseja entrar?\n");
    
    __fpurge(stdin);
    fgets(group, sizeof(group), stdin);

    size_t ln = strlen(group) - 1;
        if (*group && group[ln] == '\n') 
            group[ln] = '\0';

    sprintf(topic, "%s_Group", group);

    //printf("%s", topic);

    pthread_mutex_lock(&topics_online_mutex);
    strcpy(GP_TOPICS_ONLINE[group_control], topic);
    pthread_mutex_unlock(&topics_online_mutex);

    group_control++;

    pthread_mutex_lock(&sub_topic_mutex);
    sub_topic(topic, client);
    pthread_mutex_lock(&sub_topic_mutex);
}

void ini_chat(MQTTAsync client){

    char rec_id[10], rec_topic[20] = "";

    printf("Qual ID do usuario que você deseja mandar mensagem?\n");
    
    __fpurge(stdin);
    fgets(rec_id, sizeof(rec_id), stdin);

    strncat(rec_topic, rec_id, 2);
    strcat(rec_topic, "_Control");

    printf("Requisitando inicio de sessão...\n");

    pthread_mutex_lock(&pub_msg_mutex);
    pub_msg(rec_topic, USER_ID, client);
    pthread_mutex_unlock(&pub_msg_mutex);
}

int main(){
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int sel, rc;    

    disc_opts.onSuccess = onDisconnect;
    disc_opts.onFailure = onDisconnectFailure;

    printf("Digite seu ID único:\n");
    
    __fpurge(stdin);
    fgets(USER_ID, sizeof(USER_ID), stdin);

    strncat(USER_ID_ID, USER_ID, 2);

    strncat(USER_TOPIC_CONTROL, USER_ID, 2);
    strcat(USER_TOPIC_CONTROL, "_Control");

    strncat(USER_TOPIC_CLIENT, USER_ID, 2);
    strcat(USER_TOPIC_CLIENT, "_Client");

    if ((rc = MQTTAsync_create(&client, ADDRESS, USER_ID_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Falha ao criar o client. Erro: %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Falha ao definir os callbacks. Erro: %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS){
		printf("Falha ao conectar. Erro: %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

    //system("clear");
    printf("Bem-vindo! Agora você está online!\n\n");

    do{

        //system("clear");
        menu();
        scanf("%d", &sel);

        switch (sel){
            case 1:
                ini_chat(client);
                break;
            
            case 2:
                handle_new_chat(client);
                break;

            case 3:
                send_msg_chat(client);
                break;

            case 4:
                sub_group(client);
                break;

            case 5:
                send_msg_group(client);
                break;

            case 6:
                if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS){
                    printf("Failed to start disconnect, return code %d\n", rc);
                    rc = EXIT_FAILURE;
                    goto destroy_exit;
                }
                break;
                
            case 0:
                printf("Saindo...\n");
                break;    
            default:
                printf("Opção inválida!\n");
                break;
        }

    }while(sel);

    destroy_exit:
        MQTTAsync_destroy(&client);
    exit:
        return rc;
}