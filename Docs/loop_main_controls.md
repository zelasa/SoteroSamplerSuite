# Descrição Técnica: Loop Main Controls (Popup)

Este documento detalha o painel flutuante de controle de loops MIDI presente no SamplerPlayer.

## 1. Cabeçalho: DAW Sync & Groups
*   **Título do Módulo**: "DAW SYNC & GROUPS".
*   **Display de Tempo**: Exibe o andamento atual em BPM (ex: `TEMPO: 128 BPM`).
*   **DAW Sync**: Botão com ícone de cadeado para travar a sincronia com o host (DAW).

---

## 2. Seleção de Grupos (Abas)
*   **Abas A, B e C**: Três botões superiores (`GROUP A`, `GROUP B`, `GROUP C`) para alternar entre os conjuntos de 12 loops disponíveis em cada oitava/biblioteca.

---

## 3. Matriz de Loops (12 Slots)
Uma grid central de 4 colunas por 3 linhas representando os 12 slots cromáticos da oitava selecionada.

*   **Tipos de Slot**:
    *   **Slot Vazio**: Exibe um ícone de "+" para importação.
    *   **Slot com Loop**: Exibe uma miniatura da forma de onda (waveform) do áudio que o MIDI dispara.
    *   **Slot com Exportação**: Ícone de arquivo `.wav` com o texto "**Drag to DAW**".
*   **Funcionalidade de Arraste**: Permite que o usuário clique no ícone do arquivo e arraste o conteúdo (MIDI ou Áudio renderizado) diretamente para a trilha da DAW.

---

## 4. Rodapé: Controles de Variação de Tempo
Três botões grandes na base do painel para alterar a velocidade de reprodução dos loops em tempo real:

*   **HALF (x0.5)**: Reproduz o loop na metade da velocidade original.
*   **ORIGINAL (x1.0)**: Reproduz na velocidade padrão/sincronizada.
*   **DOUBLE (x2.0)**: Reproduz no dobro da velocidade original.

---

## 5. Integração com o Player
*   Este painel é uma janela "pop-over" que aparece quando o modo `LOOPS` está ativo em um canal.
*   As cores dos elementos (bordas, glows) acompanham a cor temática do canal selecionado (ex: Azul para o Canal 1).
*   A sincronia de tempo afeta apenas o motor de disparo MIDI, sem alterar o timbre (preservando o pitch) através de algoritmos de time-stretching se necessário, ou apenas re-triggering MIDI.
