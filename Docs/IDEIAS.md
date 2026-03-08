# SoteropolySamples - Ideias e Requisitos Originais

Este documento armazena a transcrição fiel dos requisitos e ideias numeradas fornecidas pelo usuário para o desenvolvimento da suite.

## Repositório de Ideias

### SEÇÃO A: Estrutura e Workflow do Desenvolvedor
1. O software Desenvolvedor terá o sistema de drag and drop de samples, que já está funcionando muito bem e será mantido.
2. O sistema será estruturado para trabalhar oitava por oitava, em vez de quatro oitavas juntas como no Player.
3. Essa abordagem permite criar bibliotecas individuais para cada oitava, facilitando a organização e o gerenciamento dos samples.
4. Cada oitava funcionará como uma biblioteca separada, permitindo montar instrumentos de forma modular.
5. Dentro dessas bibliotecas individuais, existirão elementos e configurações que precisam estar presentes no software Desenvolvedor.
6. Tudo o que existir no Desenvolvedor será posteriormente compilado.
7. Após a compilação, algumas dessas funções ou elementos poderão ou não ser incluídos no Player, dependendo da necessidade e da função de cada recurso.

### SEÇÃO B: Recursos Tecnológicos e Processamento (DSP)
1. O software Desenvolvedor vai incluir um filtro como o que eu mostrei no sampler do Logic Pro.
2. Esse filtro deve ter a opção de ser adaptado ou não para responder ao velocity.
3. Teremos curvas de velocity, como eu mostrei no Keyscape da Spectrasonics.
4. As curvas de velocity precisam ter controles para:
    1. Serem lineares ou não lineares.
    2. Não atingir certos valores de velocity ou atingir, conforme configuração.
    3. Ajustes finos de resposta de velocity, como eu mostrei para você.
5. Teremos Envelope ADSR de amplitude, com controles de:
    1. Attack (Ataque)
    2. Decay (Decaimento)
    3. Sustain (Sustentação)
    4. Release (Liberação).
6. Teremos Compressor no Desenvolvedor para eu poder ouvir e ajustar sem precisar compilar.
    1. Também deve existir um controle para definir se o compressor vai ou não para o Player (se entra na biblioteca compilada).
7. Teremos Reverb no Desenvolvedor para ajuste auditivo antes da compilação.
    1. Controle para definir se vai ou não para o Player.
8. Teremos Punch no Desenvolvedor para ajuste auditivo antes da compilação.
    1. Controle para definir se vai ou não para o Player.
9. Teremos EQ no Desenvolvedor para ajuste auditivo antes da compilação.
    1. Controle para definir se vai ou não para o Player.
10. Cada um desses recursos precisa ter um controle para decidir se ele vai ou não para o Player, ou seja, se será incluído na biblioteca quando ela for compilada.
11. Teremos uma aba de Biblioteca para:
    12. Inserir foto da biblioteca.
    13. Adicionar informações em texto sobre a biblioteca.
14. Teremos o sistema de edição do sample com:
    15. Fade in
    16. Fade out
    17. Start point
    18. End point
19. Esses ajustes de fade, start e end devem funcionar:
    20. Individualmente em cada região.
21. Também por tecla, permitindo que cada nota tenha seus próprios ajustes.

### SEÇÃO C: Hierarquia de Controles e Regras de Compilação
1. O Desenvolvedor trabalha oitava por oitava, e a maioria dos controles deve afetar a oitava inteira (ou seja, tudo dentro daquela biblioteca de oitava).
2. Controles que serão por tecla (por nota), dentro da oitava:
    1. Start point e End point do sample.
    2. Fade in e Fade out do sample.
    3. Tune por tecla.
    4. Fine Tune por tecla.
    5. Filtro por tecla (como eu mostrei no sampler do Logic Pro), se for possível implementar.
3. Envelope de amplitude ADSR:
    1. A ideia inicial era deixar por oitava para evitar excesso de controles.
    2. Porém, se der para implementar, é melhor ter ADSR também por tecla, porque pode ser bem útil no ajuste fino.
4. Curvas de velocity (como eu mostrei no Keyscape da Spectrasonics) ficam como controle de resposta dentro da oitava, com opção de curvas lineares e não lineares e limites de atuação, conforme os controles que eu mostrei.
5. Recursos de efeito e processamento no Desenvolvedor, com objetivo de eu ajustar ouvindo sem precisar compilar:
    1. Compressor
    2. Reverb
    3. Punch
    4. EQ
6. Cada recurso acima deve ter um controle de “vai para o Player” (inclui na biblioteca compilada) ou “não vai para o Player”.
7. Regra importante de compilação que eu quero:
    1. Na hora de compilar, não vai envelope para o Player.
    2. Na hora de compilar, não vai filtro para o Player.
    3. Ou seja, envelope e filtro existem para desenvolvimento e ajuste, mas não entram no Player.
8. Também preciso de uma aba de Biblioteca para:
    1. Colocar foto da biblioteca.
    2. Inserir informações em texto (descrição e metadados).

### SEÇÃO D: Modo de Loops MIDI e Sincronização
1. No Desenvolvedor, ao trocar o modo de One Shot para Loops, o sistema passa a trabalhar com 3 abas, cada uma contendo 12 loops:
    1. A (12 loops)
    2. B (12 loops)
    3. C (12 loops)
2. Ao selecionar a aba A, B ou C, o teclado deve indicar visualmente qual aba está ativa:
    1. Pode ser mudança de cor ou a mesma cor ficando mais escura conforme a aba.
    2. Precisa ser uma solução discreta, para não virar um teclado “colorido demais”.
    3. Alternativa: deixar as outras abas em estilo ghost/fantasma e a aba ativa bem destacada, para ficar claro onde está clicando.
3. Interface dos loops:
    1. Teremos 12 quadradinhos (slots) correspondendo às 12 teclas na ordem cromática, por exemplo: Dó, Dó sustenido, Ré, Ré sustenido… até fechar as 12.
    2. Cada quadradinho representa uma tecla específica do teclado.
4. Fluxo de alimentação dos loops:
    1. Eu vou drag and drop para cada quadradinho MIDIs já cortados.
    2. Esses MIDIs podem ser de 1 compasso, 2 compassos, 4 compassos, ou livre, desde que o loop esteja cortado certinho para repetir corretamente.
    3. O sistema só precisa garantir que o loop esteja fechado corretamente (ponto de corte consistente).
5. Sincronização e andamento:
    1. Um botão de Sync, com duas possibilidades:
        1. Sync automático (por exemplo, baseado no “padrão” do projeto).
        2. Andamento escolhido pelo usuário.
    2. Ter um controle de variação de tempo com 3 estados:
        1. Half (metade do tempo)
        2. Normal (no tempo)
        3. Double (dobro do tempo)
    3. As nomenclaturas finais desses botões podem ser definidas depois, mas a função é essa.
6. Decisão do que vai para o Player na compilação:
    1. Eu escolho quantos loops entram no Player na hora de compilar.
    2. Posso compilar com:
        1. 12 loops (só uma aba)
        2. 24 loops (duas abas)
        3. 36 loops (três abas)
        4. Ou menos do que 12, por exemplo, compilar só 4 loops, se eu quiser.
    3. Ou seja, o Desenvolvedor comporta o máximo, e eu recorto o que vai para o Player no momento da compilação.
7. Disparo dos loops:
    1. Os loops serão MIDI.
    2. Esses MIDIs serão disparados pela mesma tecla que dispara os One Shots, só que no modo Loops.
8. Execução contínua e edição enquanto toca:
    1. Enquanto um MIDI loop estiver tocando, eu quero poder:
        1. Deixar o loop tocando.
        2. Voltar para outras abas, por exemplo a aba de compressor, e ir mexendo com o MIDI tocando.
    2. Idealmente, isso deve funcionar tanto no Desenvolvedor quanto no Player.
9. Drag do MIDI para a sessão:
    1. O MIDI que está no loop deve poder ser arrastado para a sessão/DAW, como o Kontakt fazia.
    2. Ou seja, além de tocar internamente, o loop pode virar um “clip” exportável via drag and drop.

### SEÇÃO E: Correções e Definições de Escopo (Luís)
1. **Contexto**: Estas definições aplicam-se exclusivamente ao software **Desenvolvedor**.
2. **Volume**: O ajuste de Volume é **por região** (cada sample arrastado), não apenas por tecla.
3. **Filtro**: O filtro é **por tecla** (ajuste por nota afeta o comportamento daquela tecla específica). O cutoff pode ou não responder ao velocity dentro deste escopo.
4. **Fine Tune**: O Fine Tune é **por região** (acompanha o sample específico na região carregada).
5. **Envelope ADSR**: O ADSR deve ser **por região**, permitindo ajustar o comportamento individual de cada sample/região.
6. **Edição de Sample (S/E/Fades)**: 
    - Start point e End point são **por região**.
    - Fade in e Fade out são **por região**.
7. **Resumo de Hierarquia**:
    - **Por Região**: Volume, Fine Tune, ADSR, Start/End, Fade in/out.
    - **Por Tecla**: Filtro (com cutoff opcionalmente vinculado ao velocity).

### SEÇÃO F: Sincronia de Processamento entre One Shot e Loop
1. **Herança de Caminho**: Como os loops são disparados pelas mesmas teclas do modo One Shot, o loop **herda o mesmo “caminho de áudio”** e o mesmo processamento do One Shot.
2. **Implicação Prática**: Tudo o que for configurado para One Shot (ex: EQ, Compressor) afetará o Loop automaticamente.
3. **Escopo da Aba Loop**:
    - Função limitada a disparar o MIDI do loop.
    - Não cria um processamento de áudio separado.
4. **Regra de Comportamento**: O ajuste feito no Player para One Shot é global para o sistema de voz daquela tecla, valendo tanto para o Desenvolvedor quanto para o Player.

### SEÇÃO G: Fluxo Completo (Do Desenvolvedor ao Player)

#### 1. Montagem inicial no Desenvolvedor (Ex: Repique)
- Drag and drop de samples para articulações (Centro, Canto, Aro, Lata) em 12 teclas cromáticas.
- Criação de um instrumento rico e expressivo em uma única oitava.

#### 2. Ajustes Finos (Por Região)
- Correção de `Tune` e `Fine Tune`.
- Definição de `Start/End Points` e `Fade In/Out`.
- Escultura do `Envelope ADSR` para cada sample.

#### 3. Metadados e Estética
- Inclusão de foto da livraria.
- Preenchimento de Nome, Data, Autor e Descrição técnica/artística.

#### 4. Gerenciamento de Loops
- Alimentação de até 36 loops MIDI (divididos em Abas A, B e C) no Desenvolvedor.

#### 5. Configuração da Compilação
- Decisão sobre quais módulos de efeito (`Comp`, `Rev`, `Punch`, `EQ`) e abas de `Loops` serão incluídos no `.spsa` final.

#### 6. Dinâmica e Performance
- Ajuste das `Curvas de Velocity` e ativação do `Filtro` com resposta à força da nota.

#### 7. Sistema de Duas Camadas (Dual Layer / Mic)
- Estrutura espelhada para segundo microfone (ex: Close vs Over).
- Processamento independente para cada camada (Tune, Env, Filter, etc), funcionando como um mixer de 2 canais.

#### 8. Teste e Compilação
- Auditoria auditiva completa antes de gerar o arquivo `.spsa`.

#### 9. Experiência no Player
- Carregamento instantâneo com módulos ativos conforme configurado no Builder.
- Controle direto pelo usuário sobre: Tone, Tune, Pan, Volume e Efeitos (afetando o instrumento globalmente ou por oitava).
- Loops MIDI disparando os sons processados (herança total de efeito).
- Sincronização Flexível: Sync Automático (DAW) ou Manual (Manual BPM).
- Visualização detalhada da biblioteca (texto e foto ampliada) via clique com botão direito.
