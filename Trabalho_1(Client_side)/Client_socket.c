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
#include <sys/time.h>

#include "Client_socket.h"

int txt = 0;
int jpg = 0;
int mp4 = 0;

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

int cria_raw_socket(const char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
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
uint8_t calcula_crc8(const uint8_t *dados, int tamanho)
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

int verifica_crc8(const uint8_t *dados, int tamanho, uint8_t crc_recebido)
{
    uint8_t crc_calc = calcula_crc8(dados, tamanho);
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
    msg->tamanho = 0b10000;
    msg->sequencia = sequencia;
    msg->dados = "aaaaaaaaaaaaaaaa";

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

    msg->CRC = calcula_crc8(dados, 3);

    free(dados);
    return msg;

}

// Montar protocolo msg para ser enviado
uint8_t* monta_protocolo(Mensagem* msg){
    uint8_t* protocolo;
    if(!(protocolo = malloc(3 + msg->tamanho + 1))){
        perror("ERROR: protocolo = malloc\n");
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
    if(!(send(socket, buffer, sizeof(buffer),0))){
        perror("ERROR: send\n");
        return;
    }

    printf("msg enviado");
    free(msg);
    free(buffer);
}

Mensagem *desmontar_msg(char* buffer){
    if(buffer[0] != MARCA_INICIO){
        perror("ERROR: (desmontar_msg) Marca_inicio diferente\n");
        return NULL;
    }

    uint8_t tam = buffer[1];
    uint8_t crc_recv = buffer[4 + tam];

    if(!(verifica_crc8((uint8_t*)&buffer[4], tam, crc_recv))){
        perror("ERROR: CRC8 diferente\n");
        return NULL;
    }

    // se não haver erro, atribuir os seus valores na struct
    Mensagem *msg;
    if(!(msg = malloc(sizeof(Mensagem)))){
        perror("ERROR: malloc msg\n");
        return NULL;
    }
    
    msg->m_inicio = buffer[0];
    msg->tamanho = buffer[1];
    msg->sequencia = buffer[2];
    msg->tipo = buffer[3];
    msg->CRC = buffer[crc_recv];

    if(msg->tipo != FIM_TRANSMISSAO){
        if(!(msg->dados = malloc(tam))){
            perror("ERROR: malloc dados\n");
            free(msg);
            return NULL;
        }
        memcpy(msg->dados, &buffer[4], tam);
        msg->dados[tam] = '\0';
    }
    else{
        msg->dados = NULL;
    }
        

    printf("TESTE dados:\n");
    printf("%s\n",msg->dados);

    return msg;
}

// Recebe mensagem
int Receber_d_servidor(int socket, char game_map[MAP_SIZE * MAP_SIZE]){
    Mensagem *msg;
    int n;
    char *buffer = malloc(36);
    if((n = recv(socket, buffer, 36,0)) <= 0)
        return -1;  // erro timeout ou recv

    // desmonta a msg e atribui na estrutura
    while((msg = desmontar_msg(buffer)) == NULL || n <= 0){
        // erro na desmontagem da msg
        Enviar_p_servidor(socket, NACK, 0);   
        n = recv(socket, buffer, strlen(buffer),0);
    }

    int seq, slide = 1; // controle de sequencia e da janela

    // caso receber exceto ack e nack
    switch (msg->tipo){
        case ACK:
            break;
//-------------------------------------------------------------------------------------
        case NACK:
            return 0;
//-------------------------------------------------------------------------------------
        case VISUALIZACAO: //visualização;
            break;
//-------------------------------------------------------------------------------------
        case DADOS: // atualização do mapa
            seq = 0;
        
            do{
                if(msg->sequencia == seq && msg->tipo == DADOS){
                    for(int i = 0; i < msg->tamanho; i++){
                        game_map[i + seq * 32] = msg->dados[i];
                    }

                    if(slide == 4){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}
                }
                else{
                    // recebeu seq errada
                    Enviar_p_servidor(socket, NACK, seq);   
                    slide = 0;
                }
                
                DATA:
                n = recv(socket, buffer, strlen(buffer),0);  // espera o próximo pacote

                while((msg = desmontar_msg(buffer)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    n = recv(socket, buffer, sizeof(buffer),0);
                    slide = 0;  // reseta a janela
                }
            }while(msg->tipo != FIM_TRANSMISSAO);
            break;
//-------------------------------------------------------------------------------------
        case TXT: //txt
            FILE *arquivo;

            if(txt == 0){   arquivo = fopen("TEXTO_1.txt", "wb");}
            else{   arquivo = fopen("TEXTO_2.txt", "wb");}

            txt++;

            if(!arquivo){
                perror("ERROR: erro ao abrir o arquivo txt\n");
                return 0;
            }

            seq = 0;
            while(1){
                if(msg->tipo == FIM_TRANSMISSAO){
                    // muda para receber dados
                    seq = 0;
                    fclose(arquivo);
                    if(txt == 0){   system("less TEXTO_2.jpg &");}
                    else{   system("less TEXTO_1.jpg &");}
                    goto DATA;
                }

                if(msg->sequencia == seq){
                    fwrite(msg->dados, 1, msg->tamanho, arquivo);

                    if(slide == 4){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}
                }
                else{
                    // recebeu seq errada
                    Enviar_p_servidor(socket, NACK, seq);  
                    slide = 0; 
                }

                n = recv(socket, buffer, sizeof(buffer),0);  // espera o próximo pacote
             
                while((msg = desmontar_msg(buffer)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    n = recv(socket, buffer, sizeof(buffer),0);
                    slide = 0;
                }
            }
//-------------------------------------------------------------------------------------
        case JPG: //jpg
            FILE *imagem;

            if(jpg == 0){   imagem = fopen("IMAGEM_1.jpg", "wb");}
            else{   imagem = fopen("IMAGEM_2.jpg", "wb");}

            jpg++;

            if(!imagem){
                perror("ERROR: erro ao abrir o imagem jpg\n");
                return 0;
            }

            seq = 0;
            while(1){
                if(msg->tipo == FIM_TRANSMISSAO){
                    // muda para receber dados
                    seq = 0;
                    fclose(imagem);
                    if(jpg == 0){   system("feh IMAGEM_2.jpg &");}
                    else{   system("feh IMAGEM_1.jpg &");}
                    goto DATA;
                }

                if(msg->sequencia == seq){
                    fwrite(msg->dados, 1, msg->tamanho, imagem);

                    if(slide == 4){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}
                }
                else{
                    // recebeu seq errada
                    Enviar_p_servidor(socket, NACK, seq);   
                    slide = 0;
                }

                n = recv(socket, buffer, sizeof(buffer),0);  // espera o próximo pacote
             
                while((msg = desmontar_msg(buffer)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    n = recv(socket, buffer, sizeof(buffer),0);
                    slide = 0;
                }
            }
//-------------------------------------------------------------------------------------
        case MP4: //mp4
            FILE *video;

            if(mp4 == 0){   video = fopen("VIDEO_1.jpg", "wb");}
            else{   video = fopen("VIDEO_2.jpg", "wb");}

            mp4++;

            if(!video){
                perror("ERROR: erro ao abrir o video mp4\n");
                return 0;
            }

            seq = 0;
            while(1){
                if(msg->tipo == FIM_TRANSMISSAO){
                    // muda para receber dados
                    seq = 0;
                    fclose(video);
                    if(mp4 == 0){   system("mpv VIDEO_2.jpg &");}
                    else{   system("mpv VIDEO_1.jpg &");}
                    goto DATA;
                }

                if(msg->sequencia == seq){
                    fwrite(msg->dados, 1, msg->tamanho, video);

                    if(slide == 4){
                        Enviar_p_servidor(socket, ACK, seq);
                        slide = 0;
                    }
                    else{   slide++;}

                    if(seq == 63){  seq = 0;}   // reinicia a sequência
                    else{   seq++;}
                }
                else{
                    // recebeu seq errada
                    Enviar_p_servidor(socket, NACK, seq);  
                    slide = 0; 
                }

                n = recv(socket, buffer, sizeof(buffer),0);  // espera o próximo pacote
             
                while((msg = desmontar_msg(buffer)) == NULL || n <= 0){
                    // erro na desmontagem da msg
                    Enviar_p_servidor(socket, NACK, seq);   
                    n = recv(socket, buffer, sizeof(buffer),0);
                    slide = 0;
                }
            }            
            break;
//-------------------------------------------------------------------------------------
        case ERROS: //erros
            return 0;
//-------------------------------------------------------------------------------------
        case FIM_TRANSMISSAO: //fim_transmissao
            return 2;
//-------------------------------------------------------------------------------------
        default: //outros
            perror("ERROR: tipo invalido\n");
    }
    free(buffer);
    free(msg->dados);
    free(msg);
    return 1;
}