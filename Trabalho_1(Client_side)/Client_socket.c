#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <sys/time.h>

#include "Client_socket.h"
#include <stdarg.h>

static FILE *log_file = NULL;

void init_log(const char *filename) {
    if (log_file) fclose(log_file);
    log_file = fopen(filename, "w");
}

int debug_mode = 0;

void log_print(const char *format, ...) {
    va_list args;
    
    if (debug_mode) {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    if (log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

int txt = 0;
int jpg = 0;
int mp4 = 0;
int boo_img = 0;
int seq = 0, slide = 0;

// timeout
void configurar_timeout(int soquete, int timeoutMillis){
    struct timeval timeout = {
        .tv_sec = timeoutMillis / 1000,
        .tv_usec = (timeoutMillis % 1000) * 1000
    };

    // setar para recv
    setsockopt(
        soquete, SOL_SOCKET, SO_RCVTIMEO,
        (char*) &timeout, sizeof(timeout)
    );
}

int cria_raw_socket(const char* nome_interface_rede, int use_sock_raw) {
    // Cria o socket
    int tipo_socket = use_sock_raw ? SOCK_RAW : SOCK_DGRAM;
    int soquete = socket(AF_PACKET, tipo_socket, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

// Calcula CRC
uint8_t calcula_crc8(const char *dados, int tamanho)
{
    uint8_t crc = 0x00;
    for (int i = 0; i < tamanho; i++)
    {
        crc ^= dados[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

int verifica_crc8(const char *dados, int tamanho, uint8_t crc_recebido)
{
    uint8_t crc_calc = calcula_crc8(dados, tamanho);
    if (crc_calc != crc_recebido) {
        printf("crc_calculado = %d, crc_recebido = %d", crc_calc, crc_recebido);
    }
    return crc_calc == crc_recebido;
}

// Montar a msg para enviar
Mensagem* cria_msg(uint8_t tipo, uint8_t sequencia){
    Mensagem* msg;
    if(!(msg = malloc(sizeof(Mensagem)))){
        perror("ERROR: malloc msg\n");
        return NULL;
    }

    msg->m_inicio = MARCA_INICIO;
    msg->tamanho = 0;
    msg->sequencia = sequencia;
    msg->dados = NULL;

    switch (tipo){
        case 0:
            msg->tipo = ACK;
            break;
        case 1:
            msg->tipo = NACK;
            break;
        case 3:
            msg->tipo = INICIALIZACAO;
            break;
        case 10:
            msg->tipo = DIREITA;
            break;
        case 11:
            msg->tipo = ESQUERDA;
            break;
        case 12:
            msg->tipo = CIMA;
            break;
        case 13:
            msg->tipo = BAIXO;
            break;
        case 15:
            msg->tipo = ERROS;
            break;
        case 16:
            msg->tipo = FIM_TRANSMISSAO;
            break;
        default:
            perror("ERRO: Case inapropriado\n");
    }

    uint8_t* dados;
    if(!(dados = malloc(3 + msg->tamanho))){
        perror("ERROR: dados = malloc\n");
        return NULL;
    }

    dados[0] = msg->tamanho;
    dados[1] = msg->sequencia;
    dados[2] = msg->tipo;
    memcpy(&dados[3], msg->dados, msg->tamanho);

    msg->CRC = calcula_crc8(msg->dados, msg->tamanho);

    free(dados);
    return msg;

}

// Montar protocolo msg para ser enviado
uint8_t* monta_protocolo(Mensagem* msg){
    uint8_t* protocolo;
    // Pela regra de negócio, todo pacote na rede terá tamanho fixo de 35 bytes (31 de área de payload)
    if(!(protocolo = calloc(35, sizeof(uint8_t)))){
        perror("ERROR: protocolo = calloc\n");
        return NULL;
    }

    protocolo[0] = msg->m_inicio;
    protocolo[1] = (msg->tamanho << 3);
    protocolo[1] |= (msg->sequencia >> 3);
    protocolo[2] = (msg->sequencia & 0b111) << 5;
    protocolo[2] |= msg->tipo;
    memcpy(&protocolo[3], msg->dados, msg->tamanho);
    protocolo[3 + msg->tamanho] = msg->CRC;

    return protocolo;   
}

// Envia mensagem
void Enviar_p_servidor(int socket, uint8_t tipo, uint8_t sequencia){
     log_print("envio de mensagem, sequencia: %d, tipo: %d\n", sequencia, tipo);
    if (tipo == NACK) {
         log_print("envio de nack\n");
    }

    if (tipo == ACK) {
         log_print("envio de ack\n");
    }
    Mensagem* msg;
    if(!(msg = cria_msg(tipo, sequencia))){
        perror("ERRO: cria_msg\n");
        return;
    }

    uint8_t* buffer;     
    if(!(buffer = monta_protocolo(msg))){
        perror("ERROR: buffer\n");
        return;
    }
    if((send(socket, buffer, 35,0) <= 0)){
        perror("ERROR: send\n");
        return;
    }

    log_print("msg \n");
    fflush(stdout);
    free(msg);
    free(buffer);
}

Mensagem *desmontar_msg(char* buffer){
    if(buffer[0] != MARCA_INICIO){
        fprintf(stderr, "ERROR: (desmontar_msg) Marca_inicio diferente\n");
        return NULL;
    }

    uint8_t tam = (((unsigned char)buffer[1]) >> 3) & 0x1F;
    uint8_t crc_recv = (unsigned char)buffer[3 + tam];

    if(!(verifica_crc8((char *)&buffer[3], tam, crc_recv))){
        uint8_t crc_calc = calcula_crc8((char *)&buffer[3], tam);
        int seq_lida = ((((unsigned char)buffer[1]) & 0b111) << 3) | (((unsigned char)buffer[2]) >> 5);
        fprintf(stderr, "ERROR: CRC8 diferente (Calc: %02x, Recv: %02x, Tam: %d, Seq lida: %d)\n", crc_calc, crc_recv, tam, seq_lida);
        return NULL;
    }

    // se não haver erro, atribuir os seus valores na struct
    Mensagem *msg;
    if(!(msg = malloc(sizeof(Mensagem)))){
        perror("ERROR: malloc msg");
        return NULL;
    }

    msg->m_inicio = (unsigned char)buffer[0];
    msg->tamanho = (((unsigned char)buffer[1]) >> 3) & 0x1F;
    msg->sequencia = ((((unsigned char)buffer[1]) & 0b111) << 3) |
                        (((unsigned char)buffer[2]) >> 5);


                    
    msg->tipo = buffer[2] & 0b11111;

    if(msg->tipo != FIM_TRANSMISSAO){
        if(!(msg->dados = malloc(tam))){
            perror("ERROR: malloc dados\n");
            free(msg);
            return NULL;
        }
        memcpy(msg->dados, &buffer[3], msg->tamanho);
    }
    else{
        msg->dados = NULL;
    }
        

    
    log_print("TESTE dados seq: %d:\n", msg->sequencia);
    if (msg->tamanho > 0 && msg->dados != NULL) {
        for (int i = 0; i < msg->tamanho; i++) {
            log_print("%c", msg->dados[i]);
        }
        log_print("\n");
    } else {
        log_print("(sem dados)\n");
    }
    if (msg == NULL) {
        log_print("Mensagem nula");
    }
    
    return msg;
}

int Receber_d_servidor(int socket, char game_map[MAP_SIZE * MAP_SIZE]){
    char *buffer = malloc(35);
    Mensagem *msg = NULL;
    if (!buffer) { Enviar_p_servidor(socket, ERROS, 0); exit(1); }
    char *pacote = malloc(35);
    if (!pacote) { Enviar_p_servidor(socket, ERROS, 0); exit(1); }
    struct sockaddr_ll sll;
    socklen_t sll_len = sizeof(sll);
    log_print("Sequencia esperada %d\n", seq);
    int n;
WAIT_FOR_MSG:
    while (1) {
        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
        
        if (n < 0) {
            free(buffer);
            free(pacote);
            return 0; // Timeout: sinaliza para a lógica superior retransmitir
        }

        if (n < 35) continue;
        if (sll.sll_pkttype == PACKET_OUTGOING) continue; // Ignora pacotes enviados por si mesmo
        if (pacote[0] == MARCA_INICIO) {
            int tipo_pacote = pacote[2] & 0b11111;
            // Se for um pacote que o próprio cliente envia (eco da rede), descartamos imediatamente
            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
            fflush(stdout);
            usleep(10000);
                    continue; 
            }
            log_print("tipo =%d\n", tipo_pacote);
            msg = desmontar_msg(pacote);
            if (msg != NULL) {
                 log_print("Sucesso, mensagem recebida!\n");
                break;
            } else {
                Enviar_p_servidor(socket, NACK, seq);
            }
        }
    }
    slide++;
    int vis_len = 0;
    uint8_t temp_vis_buffer[2000];
    // caso receber exceto ack e nack
    log_print("Sequencia esperada %d\n", seq);
    switch (msg->tipo){
        case ACK:
            return 1;
//-------------------------------------------------------------------------------------
        case NACK:
            return 0;
//-------------------------------------------------------------------------------------
        case DADOS  : //visualização;
            break;
//-------------------------------------------------------------------------------------
        case VISUALIZACAO: // atualização do mapa     
             log_print("visualização recebida\n");   
            do{
                 log_print("Sequencia esperada:  %d\n", seq);
                if(msg->sequencia == seq){
                    for(int i = 0; i < msg->tamanho; i++){
                        if (vis_len < 2000) {
                            temp_vis_buffer[vis_len++] = msg->dados[i];
                        }
                    }

                    if(slide == 4 || msg->tipo == FIM_TRANSMISSAO){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){
                          seq = 0;
                    } else{  
                         seq++;
                    }

                    if (msg->tipo == FIM_TRANSMISSAO) {
                        if (vis_len >= 4) {
                            int minX = temp_vis_buffer[0];
                            int maxX = temp_vis_buffer[1];
                            int minY = temp_vis_buffer[2];
                            int maxY = temp_vis_buffer[3];
                            memset(game_map, '~', MAP_SIZE * MAP_SIZE);
                            int p = 4;
                            for (int i = minX; i <= maxX; i++) {
                                for (int j = minY; j <= maxY; j++) {
                                    if (p < vis_len) {
                                        game_map[i * MAP_SIZE + j] = temp_vis_buffer[p];
                                    }
                                    p++;
                                }
                            }
                        }
                        break;
                    }
                }
                else{
                    // recebeu seq errada
                    int diff = (msg->sequencia - seq + 64) % 64;
                    if (diff < 32) {
                        log_print("aqui (pacote futuro %d, enviando NACK %d)\n", msg->sequencia, seq);
                        Enviar_p_servidor(socket, NACK, seq);   
                        slide = 0;
                    } else {
                        log_print("ignorado pacote antigo %d (esperava %d)\n", msg->sequencia, seq);
                    }
                }
                
                DATA:
                while (1) {
                    n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                    if (n <= 0) break;
                    if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                    if (n >= 35 && pacote[0] == MARCA_INICIO) {
                        int tipo_pacote = pacote[2] & 0b11111;
                        if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                            tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                            fflush(stdout);
                            usleep(10000);
                            continue; 
                        }
                    }
                    break;
                }

                while((msg = desmontar_msg(pacote)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                     log_print("ali\n");
                    Enviar_p_servidor(socket, NACK, seq);   
                    while (1) {
                        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                        if (n <= 0) break;
                        if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                        if (n >= 35 && pacote[0] == MARCA_INICIO) {
                            int tipo_pacote = pacote[2] & 0b11111;
                            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                                fflush(stdout);
                                usleep(10000);
                                continue; 
                            }
                        }
                        break;
                    }
                    slide = 0;  // reseta a janela
                }

                // caso receber NACK
                if(msg->tipo == NACK){
                    if(slide == 0){
                        if(seq == 0){
                            Enviar_p_servidor(socket, ACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, ACK, seq-1);
                        }
                        goto DATA;
                    }
                    else{
                         log_print("talvez aqui\n");
                        if(seq == 0){
                            Enviar_p_servidor(socket, NACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, NACK, seq-1);
                        }
                        goto DATA;
                    }
                }
                else if(msg->tipo == GAME_CLEAR){   
                    Enviar_p_servidor(socket, ACK, msg->sequencia);
                    goto GC;
                }
                else if(msg->tipo == GAME_OVER){    
                    Enviar_p_servidor(socket, ACK, msg->sequencia);
                    goto GO;
                }
            } while(1);
            log_print("fim vizualização\n");
            break;
//-------------------------------------------------------------------------------------
        case TXT: //txt
            FILE *arquivo;

            if(txt == 0){   arquivo = fopen("TEXTO_1.txt", "wb");}
            else{   arquivo = fopen("TEXTO_2.txt", "wb");}

            txt = !txt;

            if(!arquivo){
                perror("ERROR: erro ao abrir o arquivo texto\n");
                Enviar_p_servidor(socket, ERROS, 0);
                exit(1);
            }

            do{
                 log_print("Sequencia esperada (TXT): %d\n", seq);
                if(msg->sequencia == seq){
                    if(msg->tamanho > 0) fwrite(msg->dados, 1, msg->tamanho, arquivo);

                    if(slide == 4 || msg->tipo == FIM_TRANSMISSAO){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}

                    if (msg->tipo == FIM_TRANSMISSAO) {
                        break;
                    }
                }
                else{
                    // recebeu seq errada
                    int diff = (msg->sequencia - seq + 64) % 64;
                    if (diff < 32) {
                        log_print("aqui (pacote futuro %d, enviando NACK %d)\n", msg->sequencia, seq);
                        Enviar_p_servidor(socket, NACK, seq);   
                        slide = 0;
                    } else {
                        log_print("ignorado pacote antigo %d (esperava %d)\n", msg->sequencia, seq);
                    }
                }

                TEXT:
                while (1) {
                    n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                    if (n <= 0) break;
                    if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                    if (n >= 35 && pacote[0] == MARCA_INICIO) {
                        int tipo_pacote = pacote[2] & 0b11111;
                        if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                            tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                            fflush(stdout);
                            usleep(10000);
                            continue; 
                        }
                    }
                    break;
                }
             
                while((msg = desmontar_msg(pacote)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    while (1) {
                        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                        if (n <= 0) break;
                        if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                        if (n >= 35 && pacote[0] == MARCA_INICIO) {
                            int tipo_pacote = pacote[2] & 0b11111;
                            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                                fflush(stdout);
                                usleep(10000);
                                continue; 
                            }
                        }
                        break;
                    }
                    slide = 0;
                }

                // caso receber NACK
                if(msg->tipo == NACK){
                    if(slide == 0){
                        if(seq == 0){
                            Enviar_p_servidor(socket, ACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, ACK, seq-1);
                        }
                        goto TEXT;
                    }
                    else{
                        if(seq == 0){
                            Enviar_p_servidor(socket, NACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, NACK, seq-1);
                        }
                        goto TEXT;
                    }
                }
            }while(1);
            // muda para receber dados
            fclose(arquivo);
            t1++;
            if(txt == 0){   system("cat TEXTO_2.txt"); remove("TEXTO_2.txt");}
            else{   system("cat TEXTO_1.txt"); remove("TEXTO_1.txt");}
            if(msg && msg->dados) { free(msg->dados); msg->dados = NULL; }
            if(msg) { free(msg); msg = NULL; }
            goto WAIT_FOR_MSG;            
//-------------------------------------------------------------------------------------
        case JPG: //jpg
            FILE *imagem;

            if(jpg == 0){   imagem = fopen("IMAGEM_1.jpg", "wb");}
            else{   imagem = fopen("IMAGEM_2.jpg", "wb");}

            jpg = !jpg;

            if(!imagem){
                perror("ERROR: erro ao abrir o arquivo imagem\n");
                Enviar_p_servidor(socket, ERROS, 0);
                exit(1);
            }

            do{
                 log_print("Sequencia esperada (JPG): %d\n", seq);
                if(msg->sequencia == seq){
                    if(msg->tamanho > 0) fwrite(msg->dados, 1, msg->tamanho, imagem);

                    if(slide == 4 || msg->tipo == FIM_TRANSMISSAO){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}

                    if (msg->tipo == FIM_TRANSMISSAO) {
                        break;
                    }
                }
                else{
                    // recebeu seq errada
                    int diff = (msg->sequencia - seq + 64) % 64;
                    if (diff < 32) {
                        log_print("aqui (pacote futuro %d, enviando NACK %d)\n", msg->sequencia, seq);
                        Enviar_p_servidor(socket, NACK, seq);   
                        slide = 0;
                    } else {
                        log_print("ignorado pacote antigo %d (esperava %d)\n", msg->sequencia, seq);
                    }
                }

                IMAGE:
                while (1) {
                    n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                    if (n <= 0) break;
                    if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                    if (n >= 35 && pacote[0] == MARCA_INICIO) {
                        int tipo_pacote = pacote[2] & 0b11111;
                        if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                            tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                            fflush(stdout);
                            usleep(10000);
                            continue; 
                        }
                    }
                    break;
                }
             
                while((msg = desmontar_msg(pacote)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    while (1) {
                        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                        if (n <= 0) break;
                        if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                        if (n >= 35 && pacote[0] == MARCA_INICIO) {
                            int tipo_pacote = pacote[2] & 0b11111;
                            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                                fflush(stdout);
                                usleep(10000);
                                continue; 
                            }
                        }
                        break;
                    }
                    slide = 0;
                }

                // caso receber NACK
                if(msg->tipo == NACK){
                    if(slide == 0){
                        if(seq == 0){
                            Enviar_p_servidor(socket, ACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, ACK, seq-1);
                        }
                        goto IMAGE;
                    }
                    else{
                        if(seq == 0){
                            Enviar_p_servidor(socket, NACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, NACK, seq-1);
                        }
                        goto IMAGE;
                    }
                }
            }while(1);
            // muda para receber dados
            fclose(imagem);
            t2++;
            if(jpg == 0){   system("feh IMAGEM_2.jpg > /dev/null 2>&1"); remove("IMAGEM_2.jpg");}
            else{   system("feh IMAGEM_1.jpg > /dev/null 2>&1"); remove("IMAGEM_1.jpg");}
            if(msg && msg->dados) { free(msg->dados); msg->dados = NULL; }
            if(msg) { free(msg); msg = NULL; }
            goto WAIT_FOR_MSG;
//-------------------------------------------------------------------------------------
        case MP4: //mp4
            FILE *video;

            if(mp4 == 0){   video = fopen("VIDEO_1.mp4", "wb");}
            else{   video = fopen("VIDEO_2.mp4", "wb");}

            mp4 = !mp4;

            if(!video){
                perror("ERROR: erro ao abrir o video\n");
                Enviar_p_servidor(socket, ERROS, 0);
                exit(1);
            }

            do{
                 log_print("Sequencia esperada (MP4): %d\n", seq);
                if(msg->sequencia == seq){
                    if(msg->tamanho > 0) fwrite(msg->dados, 1, msg->tamanho, video);

                    if(slide == 4 || msg->tipo == FIM_TRANSMISSAO){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}

                    if (msg->tipo == FIM_TRANSMISSAO) {
                        break;
                    }
                }
                else{
                    // recebeu seq errada
                    int diff = (msg->sequencia - seq + 64) % 64;
                    if (diff < 32) {
                        log_print("aqui (pacote futuro %d, enviando NACK %d)\n", msg->sequencia, seq);
                        Enviar_p_servidor(socket, NACK, seq);   
                        slide = 0;
                    } else {
                        log_print("ignorado pacote antigo %d (esperava %d)\n", msg->sequencia, seq);
                    }
                }

                VIDEO:
                while (1) {
                    n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                    if (n <= 0) break;
                    if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                    if (n >= 35 && pacote[0] == MARCA_INICIO) {
                        int tipo_pacote = pacote[2] & 0b11111;
                        if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                            tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                            fflush(stdout);
                            usleep(10000);
                            continue; 
                        }
                    }
                    break;
                }
             
                while((msg = desmontar_msg(pacote)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    while (1) {
                        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                        if (n <= 0) break;
                        if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                        if (n >= 35 && pacote[0] == MARCA_INICIO) {
                            int tipo_pacote = pacote[2] & 0b11111;
                            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                                fflush(stdout);
                                usleep(10000);
                                continue; 
                            }
                        }
                        break;
                    }
                    slide = 0;
                }

                // caso receber NACK
                if(msg->tipo == NACK){
                    if(slide == 0){
                        if(seq == 0){
                            Enviar_p_servidor(socket, ACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, ACK, seq-1);
                        }
                        goto VIDEO;
                    }
                    else{
                        if(seq == 0){
                            Enviar_p_servidor(socket, NACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, NACK, seq-1);
                        }
                        goto VIDEO;
                    }
                }
            }while(1);       
            // muda para receber dados
            fclose(video);
            t3++;
            if(mp4 == 0){   system("mpv VIDEO_2.mp4 > /dev/null 2>&1"); remove("VIDEO_2.mp4");}
            else{   system("mpv VIDEO_1.mp4 > /dev/null 2>&1"); remove("VIDEO_1.mp4");}
            if(msg && msg->dados) { free(msg->dados); msg->dados = NULL; }
            if(msg) { free(msg); msg = NULL; }
            goto WAIT_FOR_MSG;
//-------------------------------------------------------------------------------------
        case FANTASMA: //BOO.png
            pacman_life--;
            FILE *fantasma_file;

            if(boo_img == 0){   fantasma_file = fopen("BOO_1.png", "wb");}
            else{   fantasma_file = fopen("BOO_2.png", "wb");}

            boo_img = !boo_img;

            if(!fantasma_file){
                perror("ERROR: erro ao abrir o boo_img\n");
                Enviar_p_servidor(socket, ERROS, 0);
                exit(1);
            }

            do{
                 log_print("Sequencia esperada (FANTASMA): %d\n", seq);
                if(msg->sequencia == seq){
                    if(msg->tamanho > 0) fwrite(msg->dados, 1, msg->tamanho, fantasma_file);

                    if(slide == 4 || msg->tipo == FIM_TRANSMISSAO){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}

                    if (msg->tipo == FIM_TRANSMISSAO) {
                        break;
                    }
                }
                else{
                    // recebeu seq errada
                    int diff = (msg->sequencia - seq + 64) % 64;
                    if (diff < 32) {
                        log_print("aqui (pacote futuro %d, enviando NACK %d)\n", msg->sequencia, seq);
                        Enviar_p_servidor(socket, NACK, seq);   
                        slide = 0;
                    } else {
                        log_print("ignorado pacote antigo %d (esperava %d)\n", msg->sequencia, seq);
                    }
                }

                FANT:
                while (1) {
                    n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                    if (n <= 0) break;
                    if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                    if (n >= 35 && pacote[0] == MARCA_INICIO) {
                        int tipo_pacote = pacote[2] & 0b11111;
                        if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                            tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                            fflush(stdout);
                            usleep(10000);
                            continue; 
                        }
                    }
                    break;
                }
             
                while((msg = desmontar_msg(pacote)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    while (1) {
                        n = recvfrom(socket, pacote, 35, 0, (struct sockaddr *)&sll, &sll_len);
                        if (n <= 0) break;
                        if (sll.sll_pkttype == PACKET_OUTGOING) continue;
                        if (n >= 35 && pacote[0] == MARCA_INICIO) {
                            int tipo_pacote = pacote[2] & 0b11111;
                            if ( tipo_pacote == CIMA || tipo_pacote == 3 || tipo_pacote == BAIXO || 
                                tipo_pacote == ESQUERDA || tipo_pacote == DIREITA || tipo_pacote == 0 || tipo_pacote == 1) {
                                fflush(stdout);
                                usleep(10000);
                                continue; 
                            }
                        }
                        break;
                    }
                    slide = 0;
                }

                // caso receber NACK
                if(msg->tipo == NACK){
                    if(slide == 0){
                        if(seq == 0){
                            Enviar_p_servidor(socket, ACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, ACK, seq-1);
                        }
                        goto FANT;
                    }
                    else{
                        if(seq == 0){
                            Enviar_p_servidor(socket, NACK, 63);
                        }
                        else{
                            Enviar_p_servidor(socket, NACK, seq-1);
                        }
                        goto FANT;
                    }
                }
            }while(1);
            // muda para receber dados
            fclose(fantasma_file);
            if(boo_img == 0){   
                system("feh BOO_2.png > /dev/null 2>&1");
                remove("BOO_2.png");
            }
            else{   
                system("feh BOO_1.png > /dev/null 2>&1");
                remove("BOO_1.png");
            }
            if(msg && msg->dados) { free(msg->dados); msg->dados = NULL; }
            if(msg) { free(msg); msg = NULL; }
            goto WAIT_FOR_MSG;
//-------------------------------------------------------------------------------------
        case GAME_CLEAR:
            GC:
            Enviar_p_servidor(socket, ACK, msg->sequencia);
            printf("\n============ JOGO CONCLUIDO =====\n");
            return 9;
//-------------------------------------------------------------------------------------
        case GAME_OVER:
            GO:
            Enviar_p_servidor(socket, ACK, msg->sequencia);
            printf("\n====== GAME_OVER ======\n");
            return 14;
//-------------------------------------------------------------------------------------
        case ERROS: //erros
            printf("Erro recebido do servidor. Encerrando jogo.\n");
            exit(1);
            return 0;
//-------------------------------------------------------------------------------------
        case FIM_TRANSMISSAO: //fim_transmissao
            return 2;
//-------------------------------------------------------------------------------------
        default: //outros
            perror("ERROR: tipo invalido\n");
    }
    free(buffer);
    free(pacote);
    if(msg && msg->dados) free(msg->dados);
    if(msg) free(msg);
    return 1;
}