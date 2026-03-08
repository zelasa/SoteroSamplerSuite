# Tarefas de Desenvolvimento - Sotero Suite

## Planejamento e Documentação
- [x] Criar `Docs/IDEIAS.md` com os requisitos originais
- [x] Refatorar `Docs/PROJECT_PLAN.md` (Plano Mestre consolidado)
- [x] Criar `implementation_plan.md` para a Fase 6 (Interface Avançada)

## Fase 6: Arquitetura de Layout do SoteroBuilder
- [x] Implementar Seletor de Oitavas (C1-C5) focado em 12 notas
- [x] Criar layout Dual Layer (Mic 1 & Mic 2) simultâneo
- [x] Reestruturação Total do Layout (Modularização)
    - [x] Implementar Painel Superior (Waveform Editor Placeholder + Sculpting Tools)
    - [x] Implementar Painel Inferior (Mapping + Processing + Metadata)
    - [x] Adicionar Controles de Projeto (SAVE/LOAD/NEW/CLOSE)
- [ ] Implementar sistema global de Seleção de Nota/Região

## Correções e Melhorias Concluídas
- [x] Corrigir crash (jassert) ao tocar nota (falta de vozes no sintetizador)
- [x] Corrigir crash (jassert) ao carregar livraria (duplicate format registration)
- [x] Corrigir bug de troca de abas (visibilidade persistente)
- [x] Corrigir erros de compilação (Builder e Player sync)
- [x] Implementar tela Library (Metadata & Artwork) no SamplerPlayer
    - [x] Estética "Neon-Pulse" com brilho (glow) e tipografia ciano/amarelo

## Próximos Passos
- [ ] **Módulo de Edição de Sample (Waveform)**
    - [ ] Implementar visualizador de onda (Waveform Component)
    - [ ] Adicionar handles visuais para Start, End, Fade In e Fade Out
- [ ] **Módulo de Escultura (ADSR & Filtro)**
    - [ ] Criar componente gráfico interativo para ADSR (Amarelo)
    - [ ] Implementar controles de Filtro (Logic style) com resposta a velocity
- [ ] **Mapeamento Dual Layer**
    - [ ] Implementar Piano Rolls separados para Layer 1 e Layer 2

## Fase 7: Compilação e Integração
- [ ] Lógica de compilação condicional .SPSA
- [ ] Atualização do formato para suportar novos metadados
