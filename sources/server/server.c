/**
 * Server.
 * @author Jan Kubice
 * @version 1.0
 * @date 11.10.2022
 */ 

#include<stdio.h>
#include<string.h>    
#include<stdlib.h>    
#include<sys/socket.h>
#include<arpa/inet.h> 
#include<unistd.h> 
#include<pthread.h>
#include<time.h>

/** kody zprav */
#define CLIENT_MSG_SIZE 512
#define REQ_ID "1"
#define CONNECT_TO_GAME "2"
#define RECONNECT_TO_GAME "3"
#define CREATE_NEW_ROOM "4"
#define START_GAME "5"
#define SEND_QUIZ_ANSWER "6"
#define PAUSE_GAME "7"
#define LEAVE_GAME "8"
#define NEXT_QUESTION "9"
#define BACK_TO_MENU "10"
#define SHOW_TABLE "11"
#define ERR "-1"

/** stavy hry */
#define STATE_IN_LOBBY 1
#define STATE_IN_GAME 2
#define STATE_IN_PAUSE 3
#define STATE_ENDED 4

/** nastaveni serveru */
#define NUMBER_OF_PLAYERS 300
#define NUMBER_OF_ROOMS 100
#define NUMBER_OF_QUESTIONS 3
#define NUMBER_PLAYERS_IN_ONE_GAME 3

/** Struktura hrace */
typedef struct Player
{
    int id; 
    int points;
    int socket;
    char name[255];
    int game_ended;
} player;


/** Struktura hry */
typedef struct Game
{
    int id;
    int availability[3];
    player array_p[3];
    int state;
    int number_players;
    int q_ids[NUMBER_OF_QUESTIONS];
    int round;
} game;

/**
 * Struktura, která symbolizuje jednu otázku.
*/
typedef struct Question
{
    char *question;
    char *ans1;
    char *ans2;
    char *ans3;
    char *ans4;
    int correct;

} question;


void *connection_handler(void *);

player* players = NULL;
game* games = NULL;
question* questions = NULL;

/**
 * @brief Uspi hru, vyuziva se pred zacatkem dalsiho kola
 * 
 * @param game_id id hry, ktera se ma uspat
*/
void* sleep_time(int game_id){
    sleep(5);
    next_round(game_id);
}

/**
 * @brief Spocita pocet otazek v souboru
 * 
 * @param path_to_questions cesta k otazkam
 * @return int pocet otazek
 */
int count_questions(char* path_to_questions){
    FILE *fp;
    int count = 0;
    char c;

    fp = fopen(path_to_questions, "r");

    if (fp == NULL){
        printf("Soubor s otazkama nelze otevrit");
        exit(0);
    }

    for (c = getc(fp); c != EOF; c = getc(fp)){
        if (c == '\n'){
            count = count + 1;
        }
    }

    return (count+1)/6;
}

/**
 * @brief Nacte soubor s otazkama 
 * 
 * @param path_to_questions cesta k souboru s otazkama
 */
void load_questions(char* path_to_questions, question* questions){
    questions = calloc(count_questions(path_to_questions), sizeof(question));
}

/**
 * @brief Initializuje misto pro hrace
 */
void init_players(player* players){
    int i;
    for(i = 0; i < NUMBER_OF_PLAYERS; i++){
        players[i].id= -1;
        players[i].game_ended = 0;
    }
}

/**
 * @brief Vytvori novou mistnost
 * 
 * @param sock socket
 * @param params parametry od klienta
 */
void create_room(int sock, char* num_of_players){
    int free_found = 0;
    int i;
    for(i = 0; i < NUMBER_OF_ROOMS; i++){
        if(games[i].id == -1){
            games[i].id = i;
            games[i].state = STATE_IN_LOBBY;
            games[i].array_p[0] = players[atoi(num_of_players)];
            games[i].number_players = 1;
            games[i].availability[0] = 1;
            free_found = 1;
            break;
        }
    }

    if (free_found == 0){
        send_error("Nelze založit místnost. Serve je plný.", sock);
    }

    char text_number[8];
    sprintf(text_number, "%d", i);

    char *msg = malloc(strlen("4,") + strlen(text_number) + 1);
    memcpy(msg, "4,", strlen("4,"));
    memcpy(msg + strlen("4,"), text_number, strlen(text_number));
    memcpy(msg + strlen("4,") + strlen(text_number), ";1", strlen(";1") + 1);
    write(sock, msg, sizeof(msg));
    free(msg);
}

/**
 * @brief Pripoji do loby a aktualizuje stav loby
 * 
 * @param socket socket od kud jde dotaz  
 * @param game_id_str string id hry kam se chci pripojit
 * @param num_of_players pocet hracu
 */
void connect_to_game(int socket, char* game_id_str, char* num_of_players){
    int game_id = atoi(game_id_str);

    if (game_id < 0 || game_id > NUMBER_OF_ROOMS){
        send_error("Neplatné ID místnosti", socket);
        return;
    }
    else if(games[game_id].id == -1){
        send_error("Špatné ID", socket);
        return;
    }
    else if(games[game_id].number_players == 3){
        send_error("Plná místnost", socket);
        return;
    } 
    else if(games[game_id].state == STATE_IN_GAME) {
        send_error("Hra už jede", socket);
        return;
    }
    
    int i;
    /**od 1, admin je vzdy na pozici 0 **/
    for (i = 1; i < NUMBER_PLAYERS_IN_ONE_GAME; i++){
        if (games[game_id].availability[i] == 0){
            games[game_id].array_p[i] = players[atoi(num_of_players)];
            games[game_id].number_players++;
            games[game_id].availability[i] = 1;
            break;
        }  
    } 

    char text_number[8];
    sprintf(text_number, "%d", games[game_id].number_players);

    char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
    memcpy(msg, "2,", strlen("2,"));
    memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);

    /**odeslani zpravy vsem hracum v mistnosti**/
    int p;
    for (p = 1; p < 3; p++){
        if (games[game_id].availability[p] == 1){
            write(games[game_id].array_p[p].socket, msg, sizeof(msg));
        }       
    }
    free(msg);

    /**zprava pro admina, vidi ID mistnosti**/
    char game_id_text[8];
    sprintf(game_id_text, "%d", game_id);

    char *msg_for_admin = malloc(strlen("4,") + strlen(game_id_text) + strlen(text_number) + 1);
    memcpy(msg_for_admin, "4,", strlen("4,"));
    memcpy(msg_for_admin + strlen("4,"), game_id_text, strlen(game_id_text));
    memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text), ";", strlen(";"));
    memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text) + strlen(";"), text_number, strlen(text_number)+1);
    write(games[game_id].array_p[0].socket, msg_for_admin, sizeof(msg_for_admin));
    free(msg_for_admin);
}

/**
 * @brief Zacne hru a odesle prvni otazku
 * 
 * @param player_id_str id admina hry, ktery zacina hru
 */
void start_game(char* player_id_str){
    int player_id = atoi(player_id_str);
    int free_game_id;
    int game_id;
    char text_number[8];
    sprintf(text_number, "%d", player_id);

    /** najde volnou hru k zapnuti**/
    for (free_game_id = 0; free_game_id < NUMBER_OF_ROOMS; free_game_id++){
        if(games[free_game_id].array_p[0].id == player_id){
            game_id = free_game_id;
            break;
        }
    }

    games[free_game_id].state = STATE_IN_GAME;

    /** prvni otazka**/
    printf("player id: %d", player_id);
    printf("game id: %d\n", game_id);

    question q = questions[games[game_id].q_ids[games[game_id].round]]; /**TODO chybicka blba**/
    send_question_to_all(q, game_id);
}

/**
 * @brief Prijme odpoved na otazku od hrace
 * 
 * @param player_id_str id hrace ktery odpovidal
 * @param answer_str odpoved od hrace
 * @param sock socked hrace
 */
void quiz_answer(char* player_id_str, char* answer_str, int sock){
    int player_id = atoi(player_id_str);
    int answer = atoi(answer_str);
    int game_id = -1;
    int g,p;

    for (g = 0; g < NUMBER_OF_ROOMS;g++){
        for (p = 0; p < NUMBER_PLAYERS_IN_ONE_GAME; p++) {
            if(games[g].availability[p]= 1) {
                if(games[g].array_p[p].id == player_id) {
                game_id = g;
                break;
                }
            }
        }  
    }

    if (game_id == -1){
        send_error("Nepovedlo se odeslat odpověď", sock);
        return;
    }


    if (answer == questions[games[game_id].q_ids[games[game_id].round]].correct){
        players[player_id].points += 1;

        char correct[8];
        sprintf(correct, "%d", questions[games[game_id].q_ids[games[game_id].round]].correct);

        int p;
        for (p = 0; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
            if (games[game_id].availability[p] == 1){
                char points[8];
                sprintf(points, "%d", players[games[game_id].array_p[p].id].points);
                
                
                char *msg = malloc(strlen("6,") + strlen(points) + strlen(";") + strlen(correct) + 1);
                memcpy(msg, "6,", strlen("6,"));
                memcpy(msg + strlen("6,"), points, strlen(points));
                memcpy(msg + strlen("6,") + strlen(points), ";", strlen(";"));
                memcpy(msg + strlen("6,") + strlen(points) + strlen(";"), correct, strlen(correct) + 1);
                write(games[game_id].array_p[p].socket, msg, sizeof(msg));
                free(msg);
            }       
        }
        pthread_t thread;
        pthread_create(&thread, NULL, sleep_time, game_id);    
    }
    else{
        send_error("Nespravná odpověď", sock);
    }
            
}

void leave_game(char* player_id_str){
    int player_id = atoi(player_id_str);
    int g, i, game_id;
    int is_admin = 0;

    for (g = 0; g < NUMBER_OF_ROOMS;g++){
        for(i = 0; i < 3; i++){
            if(games[g].array_p[i].id == player_id && games[g].array_p[i].game_ended == 1){
                game_id = g;
                games[game_id].availability[i] = 0;
                games[game_id].number_players -= 1;

                if (i == 0){
                    is_admin = 1;
                }
                break;
            }
        }   
    }
    
    /** pokud hru opusti admin ukonci se cela hra **/
    if (is_admin == 1){
        games[game_id].array_p[0].game_ended = 0;
        char* msg = malloc(strlen(BACK_TO_MENU) + strlen(",") + strlen("x") + 1);
        memcpy(msg, BACK_TO_MENU, strlen(BACK_TO_MENU));
        memcpy(msg + strlen(BACK_TO_MENU), ",", strlen(","));
        memcpy(msg + strlen(BACK_TO_MENU) + strlen(","), "x", strlen("x") + 1);

        int p;
        for (p = 1; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
            if (games[game_id].availability[p] == 1){
                games[game_id].array_p[p].game_ended = 0;
                write(games[game_id].array_p[p].socket, msg, strlen(msg));
            }       
        }
        free(msg);
        clean_room(game_id);
    }
    /**pokud se odpoji nekdo v lobby aktualizuje se pocet hracu**/
    if(games[game_id].state == STATE_IN_LOBBY){
        char text_number[8];
        sprintf(text_number, "%d", games[game_id].number_players);

        char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
        memcpy(msg, "2,", strlen("2,"));
        memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);

        int p;
        for (p = 1; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
            if (games[game_id].availability[p] == 1){
                write(games[game_id].array_p[p].socket, msg, sizeof(msg));
            }       
        }
        free(msg);
        
        /**tvorba zprávy**/
        char text_number_2[8];
        sprintf(text_number_2, "%d", games[game_id].number_players);

        /**tvorba zprávy pro admina, vidí id místnosti navíc**/
        char game_id_text[8];
        sprintf(game_id_text, "%d", game_id);

        char *msg_for_admin = malloc(strlen("4,") + strlen(game_id_text) + strlen(text_number_2) + 1);
        memcpy(msg_for_admin, "4,", strlen("4,"));
        memcpy(msg_for_admin + strlen("4,"), game_id_text, strlen(game_id_text));
        memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text), ";", strlen(";"));
        memcpy(msg_for_admin + strlen("4,") + strlen(game_id_text) + strlen(";"), text_number_2, strlen(text_number_2)+1);
        /**oddeslání zprávy adminovi**/
        write(games[game_id].array_p[0].socket, msg_for_admin, sizeof(msg_for_admin));
        free(msg_for_admin);
    }
}

void id_request(int sock, char* player_name){
    int i;
    int rec = 0;
    for(i = 1; i < NUMBER_OF_PLAYERS; i++){
        if(players[i].id != -1) {
            /** pokud je hrac ve hre **/
            if(strcmp(players[i].name, player_name) == 0 && players[i].game_ended == 0){
                reconnect(sock, i);
                rec = 1;
                break;
            }
        }
    }

    /** novy hrac **/
    if (rec == 0){
        for(i = 1; i < NUMBER_OF_PLAYERS; i++){
            if(players[i].id == -1){
                players[i].id = i;
                players[i].socket = sock;
                players[i].points = 0;
                memcpy(players[i].name, player_name, strlen(player_name));
                break;
            }
        }

        char text_number[8];
        sprintf(text_number, "%d", i);

        char *msg = malloc(strlen("1,") + strlen(text_number) + 1);
        memcpy(msg, "1,", strlen("1,"));
        memcpy(msg + strlen("1,"), text_number, strlen(text_number)+1);
        write(sock, msg, sizeof(msg));
        free(msg);
    }
}

/**
 * @brief Vycisti mistnost a pripravi ji pro dalsi kola
 * @param room id mistnosti
*/
void clean_room(int room_id){
    int i;
    for(i = 0; i < 3; i++){
        games[room_id].availability[i] = 0; 
    }
    games[room_id].number_players = 0;
    games[room_id].round = 0;
    games[room_id].state = -1;
    games[room_id].id = -1;
}

/**
 * @brief funkce pro reconnect do hry
 * @param sock socket hrace
 * @param player_id id hráče
*/
void reconnect(int sock, int player_id){
    int g, i, game_id;
    game_id = -1;

    for (g = 0; g < NUMBER_OF_ROOMS;g++){
        for(i = 0; i < NUMBER_PLAYERS_IN_ONE_GAME; i++){
            if(games[g].array_p[i].id == player_id){
                game_id = g;
                break;
            }
        }   
    }

    if (game_id == -1){
        send_error("Nepovedlo se zpětné připojení.",sock);
        return;
    }
    
    if(games[game_id].state == STATE_IN_LOBBY){
        char text_number[8];
        sprintf(text_number, "%d", games[game_id].number_players);

        char *msg = malloc(strlen("2,") + strlen(text_number) + 1);
        memcpy(msg, "2,", strlen("2,"));
        memcpy(msg + strlen("2,"), text_number, strlen(text_number)+1);


        write(players[player_id].socket, msg, strlen(msg));
        free(msg);
    }
    else if (games[game_id].state == STATE_IN_GAME){
        question q = questions[games[game_id].q_ids[games[game_id].round]];

        char *msg = malloc(strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-")+ strlen(q.ans2) + strlen("-") + strlen(q.ans3) + strlen("-")+ strlen(q.ans4) + 1);
        memcpy(msg, "5,", strlen("5,"));
        memcpy(msg + strlen("5,"), q.question, strlen(q.question)); 
        memcpy(msg + strlen("5,") + strlen(q.question), ";", strlen(";")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";"), q.ans1, strlen(q.ans1)); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1), "-", strlen("-"));
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-"), q.ans2, strlen(q.ans2)); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2), "-", strlen("-")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-"), q.ans3, strlen(q.ans3)); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3), "-", strlen("-")); 
        memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3)+ strlen("-"), q.ans4, strlen(q.ans4) + 1); 


        write(players[player_id].socket, msg, strlen(msg));
        free(msg);
    }
    else{
        send_error("Něco se nepovedlo.", socket);
    }
}

/**
 * @brief odeslani chybove hlasky klientovy
 * @param msg zprava chyby
 * @param sock na jaky socket se hlaska odesle
*/
void send_error(char* msg, int sock){
    char* m = malloc(strlen("-1,") + strlen(msg) + 1);
    memcpy(m, "-1,", strlen("-1,"));
    memcpy(m + strlen("-1,"), msg, strlen(msg) + 1);
    write(sock, m, strlen(m));
    free(m);
}

/**
 * @brief funkce pro tvorbu dalšího kola
 * @param room id místnosti
*/
void next_round(int room_id){
    games[room_id].round += 1;
    if (games[room_id].round == NUMBER_OF_QUESTIONS){
        send_endgame_results(room_id);
        return;
    }

    question q = questions[games[room_id].q_ids[games[room_id].round]];
    send_question_to_all(q, room_id);
}

/**
 * @brief Vsem ve hre odesle otazku
 * 
 * @param q otazka
 * @param room_id id mistnosti, do ktere se odesila
 */
void send_question_to_all(question q, int room_id){
    char *msg = malloc(strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-")+ strlen(q.ans2) + strlen("-") + strlen(q.ans3) + strlen("-")+ strlen(q.ans4) + 1);
    memcpy(msg, "5,", strlen("5,"));
    memcpy(msg + strlen("5,"), q.question, strlen(q.question)); 
    memcpy(msg + strlen("5,") + strlen(q.question), ";", strlen(";")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";"), q.ans1, strlen(q.ans1)); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1), "-", strlen("-"));
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-"), q.ans2, strlen(q.ans2));
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2), "-", strlen("-")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-"), q.ans3, strlen(q.ans3)); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3), "-", strlen("-")); 
    memcpy(msg + strlen("5,") + strlen(q.question) + strlen(";") + strlen(q.ans1) + strlen("-") + strlen(q.ans2) + strlen("-") + strlen(q.ans3)+ strlen("-"), q.ans4, strlen(q.ans4) + 1); 


    int p;
    for (p = 0; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
        if (games[room_id].availability[p] == 1){
            write(games[room_id].array_p[p].socket, msg, strlen(msg));
        }       
    }
    free(msg);
}

/**
 * @brief odesle vsem v mistnosti informaci jestli vyhral ci ne
 * @param room id mistnosti
*/
void send_endgame_results(int room){
    int max_points, i, id_max_points;
    max_points = 0;
    id_max_points = 0;
    for(i = 0; i < NUMBER_PLAYERS_IN_ONE_GAME; i++){
        if(games[room].availability[i] != 0 && max_points < players[games[room].array_p[i].id].points){
            max_points = players[games[room].array_p[i].id].points;
            id_max_points = i;
        }
    }

    int p;
    for (p = 0; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
        if (games[room].availability[p] != 0){
            if(p != id_max_points){
                char *msg = malloc(strlen("11,") + strlen("Prohrál jsi") + 1);
                memcpy(msg, "11,", strlen("11,"));
                memcpy(msg + strlen("11,"), "Prohrál jsi", strlen("Prohrál jsi"));
                write(games[room].array_p[p].socket, msg, strlen(msg));
                free(msg);
                games[room].array_p[p].game_ended = 1;
            }
            else{
                char *msg = malloc(strlen("11,") + strlen("Vyhrál jsi") + 1);
                memcpy(msg, "11,", strlen("11,"));
                memcpy(msg + strlen("11,"), "Vyhrál jsi", strlen("Vyhrál jsi"));
                write(games[room].array_p[id_max_points].socket, msg, strlen(msg));
                free(msg);
                games[room].array_p[id_max_points].game_ended = 1;
            }
        } 
        
    }

    for (p = 0; p < NUMBER_PLAYERS_IN_ONE_GAME; p++){
        if (games[room].availability[p] == 1){
            games[room].array_p[p].points = 0;
        }       
    }

    clean_room(room);
}


/**
 * @brief Funkce je spustena ve vlastnim vlakne a obstarava jednoho klienta, zpracovava prijmute zpravy
 * 
 * @param socket_desc socket uzivatele
 * @return void* 
 */
void *connection_handler(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[CLIENT_MSG_SIZE];
 
    /**
     * @brief Nekonecna smycka na prijimani zprav
     */
    while( (read_size = recv(sock , client_message , CLIENT_MSG_SIZE , 0)) > 0 )
    {
        client_message[read_size] = '\0';
        int i = 0;
        char *p = strtok(client_message, ",");
        char *params[3];

        /** rozdeleni zpravy podle ,**/
        while (p != NULL)
        {
            params[i++] = p;
            p = strtok(NULL, ",");
        }
        printf("parametr 1: %s, parametr 2: %s, parametr 3: %s\n", params[0], params[1], params[2]);

        if (strcmp(params[1], CREATE_NEW_ROOM) == 0){
            create_room(sock, params[0]);
        }
        else if (strcmp(params[1], START_GAME) == 0){
            start_game(params[0]);
        }
        else if (strcmp(params[1], CONNECT_TO_GAME) == 0){
            connect_to_game(sock, params[2], params[0]);

        } 
        else if (strcmp(params[1], SEND_QUIZ_ANSWER) == 0){
            quiz_answer(params[0], params[2], sock);
        }
        else if (strcmp(params[1], LEAVE_GAME) == 0){
            leave_game(params[0]);
        } 
        else if (strcmp(params[1], REQ_ID) == 0){
            id_request(sock, params[2]);
        }
      
	    memset(client_message, 0, CLIENT_MSG_SIZE);
    }
     
    if(read_size == 0)
    {

        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
         
    return 0;
} 

/**
 * @brief Vstupni bod programu
 * 
 * @param argc pocet argumentu
 * @param argv pole argumentu
 * @return int navratova hodnota programu
 */
int main(int argc , char *argv[])
{
    int port = 10000;
    if (argc < 2){
        printf("Port byl nastaven na vychozi hodnotu 10000.\n");
    }
    else if (argc == 2){
        int param_port = atoi(argv[1]);
        if (param_port >= 1 && param_port <= 65335){
            port = param_port;
            printf("Port byl nastaven na %d\n", port);
        }
        else{
            printf("Nesprávný rozsah portu, povoly rozsah 1-65335.\n");
            exit(0);
        }
        
    }
    else{
        printf("Chyba. Nesprávný počet parametrů.\n");
        exit(0);
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    players = calloc(NUMBER_OF_PLAYERS,sizeof(player));
    games = calloc(NUMBER_OF_ROOMS, sizeof(game));
    questions = calloc(NUMBER_OF_QUESTIONS, sizeof(question));

    questions[0].question = "Kolik je 1+1?";
    questions[0].ans1 = "0";
    questions[0].ans2 = "2";
    questions[0].ans3 = "11";
    questions[0].ans4 = "Nelze";
    questions[0].correct = 2;

    questions[1].question = "Létají sloni?";
    questions[1].ans1 = "Ano";
    questions[1].ans2 = "Ne";
    questions[1].ans3 = "Jen malý sloni";
    questions[1].ans4 = "Jen ti co mají křídla";
    questions[1].correct = 4;


    questions[2].question = "Kolik mld lidí je na planetě?";
    questions[2].ans1 = "4";
    questions[2].ans2 = "42";
    questions[2].ans3 = "128";
    questions[2].ans4 = "8";
    questions[2].correct = 4;

    /**load_questions("otazky.txt",questions);**/
    init_players(players);

    /**příprava otázek pro všechny hry**/
    int h, i;
    for(i = 0; i < NUMBER_OF_ROOMS; i++){
        int questions[NUMBER_OF_QUESTIONS];
        int q_len = 0;
        int duplicate = 0;

        int j;
        while(q_len < NUMBER_OF_QUESTIONS){
            duplicate = 0;
            int rnd = rand() % NUMBER_OF_QUESTIONS;

            for (j = 0; j < q_len; j++){
                if (questions[j] == rnd){
                    duplicate = 1;
                }
            }

            if (duplicate == 0){
                questions[q_len] = rnd;
                q_len++;
            }
        }

        games[i].id = -1;
        games[i].round = 0;
        memcpy(games[i].q_ids, questions , sizeof(games[i].q_ids));
        for (h = 0;h < 3; h++){
            games[i].availability[h] = 0;
        }
    }

    
    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;
     
    /**vytvoreni socketu**/
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket\n");
    }
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        printf("setsocketopt(SO_REUSEADDR) failed");
        
    printf("Socket created\n");
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
     
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("bind failed. Error\n");
        return 1;
    }
    printf("bind done\n");

    listen(socket_desc , 3);

    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	
    /**přijmutí klienta**/
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        printf("Connection accepted\n");
        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            printf("could not create thread\n");
        }
        printf("Handler assigned\n");

       
    }
     
    if (client_sock < 0)
    {
        printf("accept failed\n");
        return 1;
    }
     
    free(players);
    free(games);
    free(questions);
    return 0;
}
