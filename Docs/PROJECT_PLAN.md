# Sotero Suite - Master Project Plan

Este documento é o guia mestre para o desenvolvimento da suite **SoteropolySamples**, consolidando a visão do produto, requisitos técnicos, arquitetura de software e roadmap de implementação.

---

## 1. Visão do Produto: O Conceito "Closed Player"
O **SoteropolySamples** não é um sampler genérico. É um ecossistema fechado composto por:
*   **SoteroBuilder (Developer)**: Uma ferramenta profissional de escultura sonora e montagem de livrarias.
*   **SamplerPlayer (User)**: Um reprodutor otimizado para livrarias curadas, garantindo que o usuário final receba o timbre exatamente como o desenvolvedor o moldou.

---

## 2. Arquitetura do Projeto (Monorepo)
O projeto é organizado em uma estrutura unificada para garantir consistência entre as ferramentas:
*   **Common/**: Lógica compartilhada, definições do formato `.spsa`, estruturas de DSP e modelos de dados.
*   **SoteroBuilder/**: Aplicativo standalone para criação de bibliotecas (Foco em oitava-por-oitava).
*   **SamplerPlayer/**: Plugin VST3/AU/Standalone (Interface "Neon-Pulse" de 4 canais).
*   **Docs/**: Documentação técnica, requisitos (`IDEIAS.md`) e guias de design.

---

## 3. SoteroBuilder: A Ferramenta do Desenvolvedor

### 3.1. Workflow Modular (Oitava por Oitava)
Diferente do Player (que exibe várias oitavas), o Builder foca no detalhamento de **uma oitava de cada vez**.
*   Cada oitava funciona como uma biblioteca modular independente.
*   Sistema de **Drag and Drop** de samples para mapeamento cromático instantâneo.

### 3.2. Sistema de Duas Camadas (Dual Layer / Mic)
Suporte nativo para dois grupos de timbres simultâneos (ex: *Close Mic* e *Over Mic*).
*   Estrutura de montagem espelhada para o segundo microfone.
*   Processamento independente (Mixer de 2 canais) para cada camada.

### 3.3. Edição e Escultura Sonora
*   **Waveform Editor**: Visualização dual (Azul/Vermelho) com handles interativos para `Start`, `End`, `Fade In` e `Fade Out`.
*   **Amp Envelope (ADSR)**: Gráfico interativo amarelo com visualização de tempos de ataque, decaimento, sustentação e release.
*   **Velocity Curve**: Editor de curva XY (estilo Keyscape) para respostas lineares e não-lineares.
*   **Filtro Profissional**: Filtro estilo Logic Pro com controle de `Cutoff`, `Resonance` e resposta ao velocity.

### 3.4. Hierarquia de Ajustes (Região vs Tecla)
*   **Por Região (Sample Individual)**: Volume, Fine Tune, ADSR, Start/End, Fades.
*   **Por Tecla (Nota)**: Filtro (Cutoff/Velocity), Tune.

---

## 4. SamplerPlayer: A Experiência do Usuário

### 4.1. Interface "Neon-Pulse"
*   Layout de **4 canais verticais** independentes, diferenciados por cores Neon (Azul, Verde, Amarelo, Magenta).
*   Cada canal possui fader de volume, pan, tune e seletor de modo `One-Shot/Loops`.

### 4.2. Módulos de Efeito Flexíveis
Cada canal pode alternar entre módulos de processamento:
*   **[COMP]**: Compressor com medidor de Gain Reduction.
*   **[REVERB/FX]**: Algoritmos de espaço com seleção de Room Type.
*   **[EQ]**: Equalizador paramétrico com visualizador de curva.
*   **[PUNCH]**: Módulo de shaping dinâmico e clipper.

### 4.3. Visualização de Metadados
Ao clicar com o botão direito na imagem da livraria, o usuário acessa uma ficha completa com:
*   Foto ampliada e nome do instrumento.
*   Data de criação, Autor e descrição detalhada.

---

## 5. O Sistema de Loops MIDI

### 5.1. Arquitetura de Disparo
Os loops são arquivos MIDI que utilizam o **mesmo caminho de áudio** e processamento das vozes One-Shot.
*   Qualquer processamento (EQ, Comp, ADSR) aplicado ao instrumento afeta o loop automaticamente.

### 5.2. Gerenciamento de 36 Slots
*   Três abas (`GROUP A`, `GROUP B`, `GROUP C`), cada uma com 12 slots cromáticos.
*   O teclado indica visualmente a aba ativa (estilo ghost/destaque).

### 5.3. Sincronia e Exportação
*   Sincronia com a DAW (Host Sync) ou ajuste manual de BPM.
*   Comutadores de velocidade: `Half (0.5x)`, `Original (1.0x)`, `Double (2.0x)`.
*   **Drag to DAW**: Capacidade de arrastar o loop diretamente para a sessão da DAW.

---

## 6. Lógica de Compilação e Exportação (.SPSA)

O Desenvolvedor possui o máximo de recursos para ajuste, mas nem todos vão para o Player:
*   **Sempre Excluídos**: Filtros e ADSR sumiram na compilação do Player (o som é "congelado" conforme o ajuste do Builder).
*   **Opcionais (Toggles)**: Efeitos (`Comp`, `Rev`, `Punch`, `EQ`) e Abas de Loops podem ser incluídos ou não via botões "To Player".

---

## 7. Segurança e Proteção (Sotero Shield)
*   **DNA Encryption**: Criptografia binária dos assets vinculada ao Hardware ID (HWID).
*   **Licenciamento**: Sistema de serial e ativação online (Call-Home).
*   **Blindagem**: Leitura direta da memória criptografada sem arquivos temporários em disco.

---

## 8. Roadmap de Desenvolvimento (Status de Implementação)

### Fase 1: Fundação Mono-repo
- [x] Estrutura de pastas unificada (Common, Builder, Player).
- [x] Lógica de Synth básica no SamplerPlayer.

### Fase 2: Builder Inicial (MVP)
- [x] Sistema de Drag and Drop de samples funcional.
- [x] Mapeamento básico de teclas e persistência XML.

### Fase 3: Segurança (Sotero Shield Foundation)
- [x] Geração de Machine ID (DNA) para travar uso na máquina.
- [x] Ofuscamento de metadados binários em `SoteroSecurity.h`.

### Fase 4: Engine de Áudio Avançada (Core)
- [x] Suporte a **offsets não-destrutivos** (Start, End, Fade In, Fade Out).
- [x] Motor de DSP com **ADSR** e **Filtros** (LPF, HPF, BPF) em `SoteroSamplerVoice`.
- [x] Sistema de **Fine Tune** e **Volume** por região.
- [x] Sistema de **Choke Groups** (Grupos de cancelamento).

### Fase 5: Formato .SPSA Expandido
- [x] Estrutura binária de cabeçalho com detecção de versão.
- [x] Manifesto XML suportando **Layers**, **Efeitos** e **MIDI Loops**.

### Fase 6: Nova Interface do SoteroBuilder (PRÓXIMO PASSO)
- [ ] Implementação do layout modular (Developer / Player View).
- [ ] Criação dos visores de onda e controles ADSR/Filtro gráficos interativos.
- [ ] Sistema de Dual Layer (Mic 1/2) e monitoração Master (Vus).

### Fase 7: Integração MIDI Loop e DAW Drag
- [/] Fundação técnica em `SoteroLoopEngine.h`.
- [ ] Implementação das abas A/B/C e motor de sincronia (BPM/Host).
- [ ] Lógica de drag-and-drop de arquivos MIDI para fora do plugin.

### Fase 8: Polimento e Proteção Final
- [ ] Finalização da Interface "Neon-Pulse" no Player.
- [ ] Integração de Login/Serial (Sotero Shield completo).
- [ ] Testes de estresse e performance final.

---
*Documento revisado e atualizado em 07/03/2026 - Auditoria de Código vs Requisitos Concluída.*

