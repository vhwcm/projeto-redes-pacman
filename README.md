# Pacman "No Escuro" Remoto (Cliente-Servidor)

Este projeto consiste na implementação de um jogo de Pacman remoto jogado "no escuro", utilizando uma arquitetura cliente-servidor. A comunicação é realizada diretamente na **Camada de Enlace (Layer 2)** através de **Raw Sockets**, utilizando um protocolo próprio e um controle de fluxo baseado em **Janela Deslizante** (Sliding Window).

---
## 👥 Autores
* **Viktor Hugo** — GRR: 20245275
* **Maurício Takechi Hirata** — GRR: 20211771
---

<img width="1462" height="927" alt="Captura de tela de 2026-06-28 11-43-13" src="https://github.com/user-attachments/assets/240efc84-0642-4a9c-bf3a-783e920518db" />


## 🎮 O Jogo e Seu Funcionamento

O jogo consiste em uma versão adaptada do clássico Pacman, executada de forma distribuída.

### Conceito e Dinâmica
1. **Pacman "No Escuro":** No início, o Pacman possui uma visão extremamente limitada do labirinto, enxergando apenas as casas adjacentes (raio de visualização = 1).
2. **Expansão da Visão:** A cada 5 movimentos válidos realizados pelo jogador, a visão do Pacman se expande em 1 unidade de raio.
3. **Labirinto:** O tabuleiro possui dimensões de 40x40.
   - É carregado na memória do servidor a partir de um arquivo CSV delimitado por `;`.
   - Se nenhum arquivo for fornecido, o servidor carrega um labirinto padrão com paredes desenhando a sigla **"UFPR"**, distribuindo aleatoriamente o Pacman, os fantasmas e as pastilhas.
   - Caracteres de representação:
     - `P`: Pacman
     - `X`: Parede
     - `0`: Espaço vazio
     - `R`, `B`, `G`, `Y`: Fantasmas (Vermelho, Azul, Verde, Amarelo)
     - `1` a `6`: Pastilhas douradas

### Funcionamento em Rodadas
O jogo opera de forma síncrona em turnos (rodadas):
1. O **Cliente** envia a direção desejada para o Pacman (Cima, Baixo, Esquerda ou Direita).
2. O **Servidor** recebe a jogada, valida e calcula a nova posição do Pacman.
3. O **Servidor** calcula a movimentação de cada um dos fantasmas de acordo com suas regras específicas.
4. O **Servidor** gera a nova área visível (matriz de visualização) com base no raio atual do Pacman e a transmite para o cliente.
5. Se o Pacman coletar uma pastilha ou colidir com um fantasma, arquivos especiais de prêmio ou de colisão são transmitidos pelo servidor.

### Comportamento dos Fantasmas
Cada fantasma possui uma inteligência de movimentação distinta:
* 🔴 **Vermelho (R):** Segue a regra da mão esquerda.
* 🔵 **Azul (B):** Segue a regra da mão direita.
* 🟢 **Verde (G):** Alterna entre virar à direita e à esquerda a cada decisão / anda em espiral no sentido horário.
* 🟡 **Amarelo (Y):** Movimentação puramente aleatória.

### Condição de Vitória e Derrota
* **Vitória:** O Pacman deve coletar **6 pastilhas douradas**. Cada pastilha corresponde a um prêmio enviado pelo servidor (2 arquivos `.txt`, 2 imagens `.jpg` e 2 vídeos `.mp4`).
* **Derrota (Game Over):** O Pacman inicia com **5 vidas** (mantidas no servidor). Se colidir com um fantasma, perde uma vida e um arquivo de "encontro com fantasma" (imagem) é transmitido ao cliente. O jogo acaba se as vidas chegarem a 0.

---

## 🔗 Camada de Enlace e Janela Deslizante

Toda a comunicação é baseada no modelo de **Raw Sockets** (`AF_PACKET`, `SOCK_RAW`), permitindo o envio e recebimento de quadros brutos diretamente pela interface de rede física ou virtual. Com isso, contornamos as pilhas UDP/TCP/IP tradicionais da camada de transporte e de rede.

### Janela Deslizante (Sliding Window)
Para a transmissão eficiente de arquivos (pastilhas e encontros com fantasmas), que podem variar de centenas de bytes a alguns megabytes, o projeto implementa o protocolo de **Janela Deslizante com tamanho de janela igual a 5** ($N = 5$).
* **Timeout:** Definido em **200ms**. Caso o transmissor não receba a confirmação (ACK) de um quadro dentro desse período, ocorre o timeout e a janela é retransmitida.
* **Controle de Erro:** O receptor valida cada quadro individualmente via **CRC-8** e número de sequência. Se um pacote intermediário for perdido ou corrompido, o receptor descarta os pacotes fora de ordem e pode responder com **NAK**, forçando o transmissor a recuar a janela e retransmitir a partir do pacote perdido.

---

## 📨 Estrutura do Protocolo de Comunicação

Cada quadro transmitido na rede possui um **tamanho fixo de 35 bytes**. A carga útil (payload) de dados pode ter até 31 bytes, e o espaço não utilizado é preenchido com bytes nulos (`0x00`).

### Formato do Quadro

| Campo | Tamanho (Bits) | Descrição |
| :--- | :---: | :--- |
| **Marcador de Início** | 8 | Sempre `01111110` (`0x7E`) |
| **Tamanho** | 5 | Número de bytes válidos no campo de dados (0 a 31) |
| **Sequência** | 6 | Número de sequência do quadro (0 a 63) |
| **Tipo** | 5 | Código identificador da mensagem (0 a 16) |
| **Dados** | 248 (31 bytes) | Payload útil da mensagem |
| **CRC-8** | 8 | Código de redundância cíclica calculado sobre o campo de Dados |

### Divisão de Bits dos Bytes de Cabeçalho (Header)
O cabeçalho ocupa exatamente os 3 primeiros bytes do quadro:
* **Byte 0:** `0x7E` (Marcador de início)
* **Byte 1:** `[Tamanho (5 bits) | Sequência MSB (3 bits)]`
* **Byte 2:** `[Sequência LSB (3 bits) | Tipo (5 bits)]`

### Tabela de Tipos de Mensagens

| Código (Dec) | Tipo | Descrição |
| :---: | :--- | :--- |
| **0** | `ACK` | Confirmação de recebimento bem-sucedido |
| **1** | `NACK` | Sinalização de erro de recepção / pacote perdido |
| **2** | `VISUALIZACAO` | Matriz de visualização parcial do labirinto |
| **3** | `INICIALIZACAO` | Mensagem de início de conexão / partida |
| **4** | `DADOS` | Bloco de dados genérico |
| **5** | `TXT` | Dados pertencentes a arquivo de texto |
| **6** | `JPG` | Dados pertencentes a arquivo de imagem |
| **7** | `MP4` | Dados pertencentes a arquivo de vídeo |
| **8** | `FANTASMA` | Arquivo do encontro com o fantasma |
| **9** | `FIM_JOGO_VITORIA` | Sinalização de fim de jogo com vitória do Pacman |
| **10** | `DIREITA` | Comando de movimentação para a direita |
| **11** | `ESQUERDA` | Comando de movimentação para a esquerda |
| **12** | `CIMA` | Comando de movimentação para cima |
| **13** | `BAIXO` | Comando de movimentação para baixo |
| **14** | `GAME_OVER` | Sinalização de fim de jogo com derrota |
| **15** | `ERROS` | Sinalização de erro geral no protocolo |
| **16** | `FIM_TRANSMISSAO` | Indica que a transmissão do arquivo ou fluxo atual foi concluída |

---

## 🚀 Como Executar o Jogo

Como o projeto utiliza **Raw Sockets**, os executáveis precisam de privilégios de superusuário (`root` / `sudo`) para interagir diretamente com as interfaces de rede.

### 1. Compilação
No diretório raiz do projeto, compile o cliente e o servidor executando:
```bash
make
```
Para limpar os arquivos intermediários e objetos:
```bash
make clean
```

---

### 2. Inicialização Local (Usando Redes Virtuais - `veth`)
Se você estiver testando o jogo na mesma máquina, pode criar um par de interfaces de rede virtuais interconectadas (um cabo de rede virtual).

1. **Criar as interfaces virtuais:**
   Execute o script fornecido no repositório:
   ```bash
   sudo ./setup_rede_veth.sh
   ```
   *Isso criará as interfaces `veth-srv` e `veth-cli` e as colocará em estado ativo (`UP`).*

2. **Iniciar o Servidor:**
   Em um terminal, execute o servidor especificando a interface dele:
   ```bash
   sudo ./servidor/server veth-srv
   ```

3. **Iniciar o Cliente:**
   Em outro terminal, execute o cliente especificando a interface dele:
   ```bash
   sudo ./Trabalho_1\(Client_side\)/Pacman_game veth-cli
   ```

---

### 3. Inicialização Real (Usando Cabo Ethernet Físico)
Para jogar de fato em dois computadores separados conectados diretamente por um cabo de rede:

1. **Conexão Física:**
   Conecte um cabo Ethernet (cat5e/cat6) diretamente entre as portas de rede dos dois computadores.

2. **Identificar as Interfaces de Rede:**
   Em cada máquina, descubra o nome da interface de rede física que está conectada ao cabo. Execute:
   ```bash
   ip link
   ```
   *(Identifique nomes como `eth0`, `enp3s0`, `enp0s31f6`, etc.)*

3. **Ativar as Interfaces:**
   Garanta que a interface está ativa em ambas as máquinas:
   ```bash
   sudo ip link set dev <nome_da_interface> up
   ```
   > [!NOTE]
   > **Não é necessário configurar endereço IP** (como `192.168.1.X`) nas interfaces de rede. O protocolo funciona puramente na camada de enlace (L2), enviando quadros brutos para o canal.

4. **Executar o Servidor (Máquina A):**
   ```bash
   sudo ./servidor/server <interface_da_maquina_A>
   ```

5. **Executar o Cliente (Máquina B):**
   ```bash
   sudo ./Trabalho_1\(Client_side\)/Pacman_game <interface_da_maquina_B>
   ```

---

## 🛠️ Implementação de Timeouts de Socket (Referência Técnica)
Para evitar bloqueios indefinidos no recebimento de mensagens devido a perdas de pacotes físicos, foi configurada a opção `SO_RCVTIMEO` no socket:

```c
struct timeval tv;
tv.tv_sec = 0;
tv.tv_usec = 200000; // 200ms
setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
```

Se a chamada `recv` ou `recvfrom` estourar o tempo limite de 200ms sem receber dados válidos, ela retornará `-1` (com `errno` definido como `EAGAIN` ou `EWOULDBLOCK`), permitindo que a lógica de retransmissão de janela do transmissor entre em ação.
