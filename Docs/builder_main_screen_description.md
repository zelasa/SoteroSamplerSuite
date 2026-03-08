# Descrição Técnica da Main Screen - SoteroBuilder (Modo DEV)

Este documento descreve detalhadamente os elementos visuais e as funcionalidades presentes no mockup da interface principal do SoteroBuilder (Desenvolvedor).

## 1. Cabeçalho e Navegação Global
*   **Selector de Modo (Canto Superior Direito)**: Botões tipo aba para alternar entre `DEVELOPER` (Foco atual) e `USER PLAYER`.
*   **Controles de Projeto (Canto Superior Direito)**: Grid de botões com as ações `SAVE`, `LOAD`, `NEW`, `CLOSE`.
*   **Título Central**: "SOTEROPOLYSAMPLES - DEV" em tipografia elegante e moderna.

---

## 2. Painéis de Edição de Sample (S/E/Fades)
Localizados no canto superior esquerdo para controle granular de cada região/sample.
*   **Controles Numéricos**: Knobs de precisão para `Volume`, `Tune` e `Finetune`.
*   **Visualizador de Waveform Dual (LAYER 1 e LAYER 2)**:
    *   Fundo Azul (Layer 1) e Fundo Vermelho (Layer 2).
    *   **Handles Interativos**: Triângulos brancos nos cantos para ajuste visual de `Fade In` e `Fade Out`. Linhas verticais brancas para `Start Point` e `End Point`.
    *   **Indicadores Numéricos**: Exibição em tempo real dos valores de START e END na base do visor.
    *   **Botões de Play e Navegação**: Botão de Play (vermelho) e setas de navegação lateral para mudar a região selecionada.

---

## 3. Escultura e DSP (Filtro, ADSR, Velocity)
Centralizados na parte superior para moldar o comportamento dinâmico.
*   **Painel de Filtro**:
    *   Knobs para `Cutoff`, `Resonance` e `Vel/Filter Sens`.
    *   Toggle Switch para `Velocity to Filter`.
    *   Seletores para aplicar o filtro por `Tecla` ou por `Oitava`.
*   **Amp Envelope (ADSR) - Gráfico Amarelo**:
    *   Visualizador gráfico interativo com nós arrastáveis.
    *   Exibição dos tempos de Attack (A), Decay (D), Sustain (S)% e Release (R) logo abaixo do gráfico.
*   **Velocity Curve Edit**:
    *   Gráfico de curva de resposta (estilo Keyscape).
    *   Puxador (Handle) central para ajustar a curvatura (Linear vs Não-Linear).
    *   Menu dropdown para `Curve Type` e cadeado de trava (Lock).

---

## 4. Área de Mapeamento (Piano Rolls Dual Layer)
O cupa o lado esquerdo inferior da tela.
*   **Layer 1 (Mic 1) e Layer 2 (Mic 2)**: 
    *   Duas grids independentes com teclados individuais na base.
    *   Botão `TO PLAYER (ON/OFF)` individual para cada camada no topo.
    *   Indicação visual de mapping de samples em azul (Layer 1) e vermelho (Layer 2).

---

## 5. Painéis de Processamento e Avançados
Localizados no centro inferior.
*   **Recursos Exportados para o Player**:
    *   Lista de efeitos com toggles ON/OFF independentes: `Compressor`, `Reverb`, `Equalizer`, `Punch`.
*   **Choke Groups (Matriz)**:
    *   Interface para assinalar teclas a grupos de cancelamento.
*   **Módulo MIDI Loops (Abas A, B, C)**:
    *   Grid de 12 slots cromáticos por aba.
    *   Botões de tempo: `Half`, `Double`.
    *   Controles de Sincronia: Input de BPM e toggle `DAW SYNC`.

---

## 6. Informações da Livraria e Monitoração (Canto Inferior Direito)
*   **Metadados**:
    *   Painel para `Drag Foto da Livraria` (Suporte JPG/PNG/TIFF).
    *   Campos de texto: NOME, DATA, AUTOR, INSTRUMENTO, INFORMAÇÕES.
*   **Medidores MASTER (Vu Meters)**:
    *   Dois medidores verticais LED (Layer 1 e Layer 2) com indicação de CLIP no topo.
