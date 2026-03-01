# SoteropolySamples: Cronograma de Desenvolvimento da Suite

Este documento detalha as fases de evolução do **SoteroBuilder** e sua integração com o **SamplerPlayer**.

## Fase 1: Fundação e Formato Proprietário (Concluída)
*Foco: Criar a linguagem comum entre os apps.*
- [x] Definição do formato binário (.spsa) na pasta `/Common`.
- [x] Implementação de manifestos em **XML** para metadados e artes de capa.
- [x] **SoteroBuilder**: Interface básica de 12 teclas e importação de WAVs.
- [x] **SamplerPlayer**: Motor básico de leitura do arquivo .spsa.

## Fase 2: Velocidades e Choke Groups (Concluída)
*Foco: Implementar a inteligência de performance.*
- [x] **SoteroBuilder**: Interface para mapear as 5 camadas de velocity por tecla.
- [x] Sistema de marcação de **Choke Groups**.
- [x] **SamplerPlayer**: Motor de 5 camadas e lógica de cancelamento de voz (Choke).

## Fase 3: Redesenho Unificado e Articulação (Em Planejamento)
*Foco: Consolidar o player e adicionar ferramentas de escultura sonora.*
- [ ] **SamplerPlayer**: Redesenho total para interface unificada de biblioteca (Library-Centric).
- [ ] **ADSR Envelope**: Controles globais de Attack, Decay, Sustain e Release.
- [ ] **Master Filter**: Filtros Low-pass e High-pass com Cutoff e Resonance.
- [ ] **Workflow**: Remoção do modelo de 3 tracks em favor de um motor cromático único.

## Fase 4: Realismo e Gestão de Livrarias (Futuro)
*Foco: Refinar o uso profissional e a navegação.*
- [ ] **Round Robin**: Alternância inteligente de samples para maior realismo.
- [ ] **Library Browser**: Barra lateral visual para carregar arquivos .spsa rapidamente.
- [ ] **Waveform Visualizer**: Display visual do sample sendo tocado com playhead.
- [ ] **Keyswitches**: Suporte a troca de articulações via notas MIDI de comando.
- [ ] **Segurança**: Criptografia DNA final para proteção de Soundbank.
