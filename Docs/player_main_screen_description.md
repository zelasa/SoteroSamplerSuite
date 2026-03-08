# Descrição Técnica da Main Screen - SamplerPlayer (Visão do Usuário)

Este documento descreve a interface do SamplerPlayer baseada no mockup "NEON-PULSE", focada na experiência do usuário final.

## 1. Conceito Visual e Estrutura Global
*   **Nome do Plugin**: "NEON-PULSE" (estética Dark com cores vibrantes em Neon).
*   **Seleção de Presets (Top)**: Menu central para carregar presets (ex: `CYBER_DYSTOPIA_01`).
*   **Controles Globais (Top Right)**: Botão de configurações (engrenagem) e notificações (sino).
*   **Teclado (Bottom)**: Teclado de piano completo para disparo de notas e visualização de atividade MIDI.
*   **Layout de Canais**: 4 faixas verticais independentes, cada uma com uma cor temática (Azul, Verde, Amarelo, Magenta).

---

## 2. Componentes de cada Canal (Vertical)
Cada um dos 4 canais possui os seguintes controles e módulos:

*   **Seleção de Modo**: Toggles superiores para alternar entre `ONE-SHOT` e `LOOPS`.
*   **Visualizador de Sample/Slot**:
    *   Exibe o nome do arquivo carregado (ex: `PULSE_OSC_01`).
    *   Botão "+" para carregar manualmente (ou via drag-and-drop no Builder).
*   **Controles Finos (Knobs)**:
    *   `PAN` (Panoramização).
    *   `TUNE` (Afinação em semitons).
    *   `FINETUNE` (Afinação fina em cents).
*   **Visualizador de Envelope**: Gráfico simplificado mostrando a curva de amplitude aplicada.
*   **Fader de Volume (Esquerda)**: Slider vertical fino para controle de ganho do canal.
*   **Seção de FX (Bottom)**:
    *   Toggle Switch para `REVERB`.
    *   Seletor de Módulo Ativo: Botões tipo aba para `[COMP]`, `[FX]`, `[EQ]`, `[PUNCH]`.
    *   **Área de Parâmetros Variáveis**: O conteúdo desta área muda conforme o módulo selecionado.

---

## 3. Detalhamento dos Módulos de Efeito (Exemplos no Mockup)

### Módulo Azul [COMP] (Compressor)
*   **Visualização de GR (Gain Reduction)**: Medidor vertical.
*   **Knobs Principais**: `THRESHOLD`, `RATIO`.
*   **Knobs Secundários**: `ATTACK`, `RELEASE`, `MAKEUP`.

### Módulo Verde [REVERB / FX]
*   **Room Type**: Opções selecionáveis `[Small]`, `[Medium]`, `[Large]`.
*   **Controles de Espaço**: `SIZE`, `PRE-DELAY`, `WET/DRY`, `ROOM CHARACTER`.

### Módulo Amarelo [EQ] (Equalizador)
*   **Filtros Independentes**: `HPF FREQ` e `LPF FREQ`.
*   **Gráfico de Curva de EQ**: Visualizador central da resposta de frequência.
*   **Controles de Banda**: `LOW FREQ`, `LOW GAIN`, `MID FREQ`, `MID GAIN`, `MID Q`, `HIGH FREQ`, `HIGH GAIN`, `HIGH Q`.
*   > [!WARNING]
    > **Observação de Design**: No mockup, este módulo (Amarelo) aparece com controles "sobressaltados" (flutuando fora da faixa), o que foi identificado como **não desejável**. A versão final deve manter todos os controles dentro do limite vertical do canal.

### Módulo Magenta [PUNCH]
*   **Dynamic Shaping**: `ATTACK PUNCH`, `SUSTAIN`, `PUNCH DRIVE`, `PEAK CLIPPER`.

---

## 4. Notas Importantes sobre Implementação
1.  **Variabilidade de Controles**: Os parâmetros dentro de cada módulo (`EQ`, `Comp`, etc.) podem variar dependendo da biblioteca carregada ou de refinamentos futuros, mantendo a estrutura modular.
2.  **Consistência de Cor**: A cor do canal deve permear todos os seus controles (Glow, bordas, fontes) para facilitar a identificação rápida pelo usuário.
3.  **Encadeamento**: O áudio processado segue a ordem de One-Shot/Loop -> Filtro (se ativo) -> Módulos de FX.
