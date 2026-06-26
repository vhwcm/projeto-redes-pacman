\author{
Viktor Hugo -- GRR: 20245275\\
Mauricio Takechi Hirata -- GRR: 20211771
}

# projeto-redes-pacman

Dúvidas a retirar com o parceiro:

- Pacman vai ter 5 vidas. isso tem que estar salvo no servidor

Adicionado protocolo, recebimento e envio de mensagens

a cada 4 mensagens envia um ak

- crc em cima dos dados apenas; da para saber o tamanho dos dados apartir do tamanho

- mensagem de Ak POR:
TEMPO: 200ms
TAMANHO JANELA: 5;


Temos que ter sequencialização no envio de mensagens ou seja, há de haver uma variavel para guardar a sequencialização do canal de comunicação.
-> é importante que o sistema seja resiliente a falhas (cair conexão, pacote perdido, naks, etc..); 

Regras:

Trabalho PacMan
• Implementar um jogo de Pacman “no escuro” remoto, na modalidade
cliente-servidor
• Em duplas
• Usar o Cliente em um computador e o servidor em outro
o Conectar os dois computadores através de um cabo de rede
diretamente
• Valor 40,0
o Bônus não faz a nota do trabalho ultrapassar 40 pontos na média
• Trabalho deve ser apresentado pelos dois membros da equipe
o Entregar um relatório impresso no dia da apresentação com uma
página com as escolhas que a equipe teve que fazer ao longo do
desenvolvimento
• Entrega via UFPR Virtual
o Código fonte e arquivo executável em um arquivo .tgz – nome do
arquivo deve ser o GRR da dupla
o Relatório em pdf
• Respeitar o protocolo de comunicação definido em sala
• Detalhes implementação:
o Timeout é obrigatório
o Controle de Fluxo é Para-e-espera
o Implementação da transmissão dos arquivos das pastilhas com
janela deslizante de tamanho 5 gera um bônus de 10%.
• Obrigatório C ou C++
• Comunicação por RAWSocket
o RAWSocket só pode ser executado como root nas máquinas
Jogo:
• Labirinto 40x40 – carregado na memória como uma matriz de 40x40
o O labirinto deve ser lido de um arquivo no início do servidor
o Arquivo do labirinto é um arquivo csv com os itens separados por ;
o Caso o usuário não forneça o labirinto, o programa deve carregar o
labirinto padrão com as paredes com o escrito UFPR e sortear
aleatoriamente a posição do PacMan, fantasmas, pastilhas.
o Representações do arquivo do labirinto:
§ P – PacMan
§ X – Parede
§ 0 – Espaços vazios
§ R – fantasma vermelho
§ B – fantasma azul
§ G – fantasma verde
§ Y – fantasma amarelo
§ 1 – pastilha dourada arquivo texto (1.txt)
§ 2 – pastilha dourada arquivo texto (2.txt)
§ 3 – pastilha dourada arquivo jpg (3.jpg)
§ 4 – pastilha dourada arquivo jpg (4.jpg)
§ 5 – pastilha dourada arquivo mp4 (5.mp4)
§ 6 – pastilha dourada arquivo mp4 (6.mp4)
• Funciona em rodadas
• A cada rodada o jogador faz um movimento
• A cada rodada o servidor calcula um movimento para cada fantasma
• PacMan deve pegar 6 pastilhas douradas para terminar a fase
• Cada pastilha dourada corresponde a um arquivo mostrando o prêmio: 2
arquivos são texto (.txt), 2 são imagens (.jpg) e 2 são vídeos (.mp4)
• Caso o PacMan encontre um fantasma, deve enviar um arquivo mostrando
o encontro – arquivo pode ser definido pela dupla
• A cada 5 movimentos a visualização do PacMan é expandida de 1 no raio
o No início o PacMan só enxerga uma casa para cada lado ao seu
redor
• Movimentos dos fantasmas:
o Vermelho – regra da mão esquerda
o Azul – regra da mão direita
o Verde – alterna direita e esquerda a cada decisão que deve tomar
o Amarelo – aleatório



tipos de mensagem:
0 - ACK
1 - NACK
2 - visualização
3 - inicialização
4 -> Dados
5 -> txt
6 -> jpg
7 -> mp4
8 -> fantasma
9  - Fim de jogo (sucesso)
10 - direita
11 - esquerda
12 - cima
13 - baixo
14 - game over
15 - Erros
16 - Fim de Transmissão


Servidor:
• Cria / Carrega o mapa inicial
• Conhece todo o tabuleiro
• Conecta o Cliente
• Envia a visualização inicial para o cliente
• Espera receber movimentações
• Controla os fantasmas.
o Verde – Anda em espiral sentido horário
o Azul – Anda em espiral sentido anti-horário
o Vermelho – Regra da mão esquerda
o Amarelo – Regra da mão direita
• A cada rodada:
o deve receber o movimento do PacMan
o calcular a nova posição do PacMan
o calcular os movimentos dos fantasmas
o gerar a nova visualização do PacMan
o enviar a nova visualização para o cliente – pode ser mais de uma
mensagem
o se achou pastilha – envia o arquivo correspondente para o cliente
• Comunicação
o Recebe a mensagem com a movimentação do PacMan
o Calcula o novo mapa
o Gera a nova visualização do PacMan
o Cria a mensagem e envia para o cliente com a visualização atual
o Caso o PacMan tenha encontrado uma pastilha ou um fantasma
deve enviar o arquivo correspondente.
o Mensagens do servidor para o cliente tem tamanho variável
§ Se for a nova visualização, ela aumenta a cada 5 jogadas
§ Se for os arquivos, eles podem ter tamanhos desde algumas
centenas de bytes até alguns megabytes
• Log mas mensagens recebidas e enviadas em uma janela separada.

Estrutura da Mensagem
Campo	Tamanho
Marcador de Inicio	8 bits
Tamanho	5 bits
Sequencia	6 bits
Tipo	5 bits
Dados	n bytes
CRC	8 bits
Detalhamento

    Marcador Inicio → 01111110
    Tamanho → dados
    Sequencia 
    CRC → 8 bits


# Como implementar os timeouts:

Implementando Timeouts

Um timeout, em sua definição mais genérica, é um evento que ocorre após um determinado tempo. Se especifica um determinado tempo, o timeout interval, e depois desse tempo, alguma coisa acontece. No contexto de redes isso é muito relevante, pois existe a chance de enviarmos algo e não obtermos uma confirmação, porque o outro lado não recebeu a nossa mensagem ou porque o outro lado não conseguiu mandar uma mensagem de resposta para nós. O razoável a se fazer é enviar a nossa mensagem novamente caso nenhuma mensagem seja recebida. No caso geral, é provado matematicamente que é impossível ter certeza que o outro lado recebeu a nossa mensagem, esse é o problema dos dois generais, mas não nos custa ao menos tentar.
Sockets

Os sockets podem ter timeout nos seus métodos de send e recv, como visto no artigo sobre raw sockets. Porém, isso não é suficiente para reenviarmos a mensagem só quando um determinado tempo passar, porque os raw sockets recebem todos os pacotes da placa de rede. Isso significa que se no meio tempo o seu computador decidir tentar configurar a Internet, o seu socket vai receber todas as mensagens dessa transação, mesmo não sendo as que você quer, e isso significa que o timeout dessas funções nunca funciona, pois ele sempre será reiniciado com essas outras mensagens da rede. A solução é além de usar o timeout no socket, é manter o seu próprio timeout. Isso pode ser feito simplesmente mantendo o seu próprio relógio.

// usando long long pra (tentar) sobreviver ao ano 2038
long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}
 
int protocolo_e_valido(char* buffer, int tamanho_buffer) {
    if (tamanho_buffer <= 0) { return 0; }
    // insira a sua validação de protocolo aqui
    return buffer[0] == 0x7f;
}
 
// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer) {
    long long comeco = timestamp();
    struct timeval timeout = { .tv_sec = timeoutMillis/1000, .tv_usec = (timeoutMilis%1000) * 1000 };
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (protocolo_e_valido(buffer, bytes_lidos)) { return bytes_lidos; }
    } while (timestamp() - comeco <= timeoutMillis);
    return -1;
}

Recuo Exponencial

Pode ser útil variar o tempo que se espera pela resposta de forma exponencial. Isso significa que na primeira retransmissão você espera um segundo para receber a mensagem, já na próxima espera dois, e na próxima quatro e assim por diante. Isso ajuda no caso por exemplo de um servidor ficar lento e não conseguir responder todas as mensagens que lhe foram enviadas. Assim, as mensagens vão enfileirando, e se elas chegarem num ritmo constante, o servidor nunca vai conseguir responder todas elas. Esse é o conceito do recuo exponencial, que é implementado em protocolos como o TCP mas se aplica a muito mais lugares, como por exemplo para evitar colisões na rede através da inserção de um componente probabilístico.
