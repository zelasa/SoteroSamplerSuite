# Sotero Suite - Project Plan

Este documento consolida a visão, o roadmap e as fases de desenvolvimento da suite SoteropolySamples.

## 1. Visão do Produto: "Closed Player"
O SamplerPlayer não será um sampler aberto para qualquer WAV. Ele será um reprodutor otimizado que carrega apenas livrarias curadas e editadas internamente, garantindo a qualidade final do som (Corte de samples, volumes e mixagem prontos).

## 2. Estrutura da Suite (Monorepo)
Para garantir que o **Player** e o **Builder** falem a mesma língua (formato .SPSA), utilizamos uma estrutura de projeto unificada:

- **Common/**: Código compartilhado (Lógica do formato .SPSA, Estruturas de Áudio).
- **SamplerPlayer/**: O motor de reprodução unificado (Plugin VST3/AU/Standalone).
- **SoteroBuilder/**: A ferramenta de criação de livrarias (App Standalone).
- **Assets/**: Logos, fontes e recursos visuais comuns.

## 3. Estratégia de Desenvolvimento: "Foundation First"

O desenvolvimento é focado em construir engines robustas que sustentam as funcionalidades modulares ("Lego").

### Fase 3: Engines de Base e Metadados (Atual)
*Foco: Expandir o motor de áudio e o formato .SPSA para suportar a nova hierarquia.*
- [ ] **Engine de Samples**: Implementação de leitura de offsets (Start/End) e envelopes de Fade não-destrutivos.
- [ ] **Engine de Loops**: Motor de execução de MIDI Clips sincronizado com o host.
- [ ] **Sistema de Camadas**: Suporte a Dual Mic (A/B) com processamento independente.
- [ ] **Novo Formato**: Migração para o manifesto XML expandido (Loops, Mics, Metadados).

### Fase Q&A 1: Validação Técnica
- [ ] **Testes de Estresse**: Carregamento de livrarias com 36 loops e Dual Layer.
- [ ] **Verificação de Performance**: Impacto de CPU/I/O no novo motor de áudio.
- [ ] **Integridade de Arquivo**: Testes de corrupção e retrocompatibilidade do formato .SPSA.

### Fase 4: O Novo "Sotero Builder/Developer"
*Foco: A ferramenta de design profissional oitava por oitava.*
- [ ] **Workflow por Oitava**: Interface focada na modularidade e montagem cromática.
- [ ] **Escultura Sonora**: Implementação de ADSR e Filtros (apenas para o Developer).
- [ ] **Módulo de Compilação**: Sistema de "Toggles" para decidir o que entra na livraria final.
- [ ] **Monitoração Profissional**: Compressor, Reverb, EQ e Punch integrados para ajuste auditivo.

### Fase Q&A 2: UX e Workflow
- [ ] **Beta Test Interno**: Teste de montagem de instrumento completo por um produtor.
- [ ] **Validação de Estética**: Refinamento visual dos knobs e teclados (Visual Feedback).
- [ ] **Bug Hunting**: Correção de falhas no sistema de Drag & Drop e Compilação.

### Fase 5: Evolução do "SamplerPlayer"
*Foco: Transformar o player em uma central de bibliotecas premium.*
- [ ] **Painel de Controle**: Volume/Pan por oitava, Tone e Fine Tune por nota.
- [ ] **Player de Loops**: Interface de disparo polifônico vs cancelamento global e MIDI Drag-to-DAW.
- [ ] **Visualizador de Metadados**: Pop-up informativo sobre a história e criador da livraria.
- [ ] **DNA Encryption**: Proteção final dos assets consolidados.

## 4. Diferencial Funcional: Developer vs Player

| Recurso | Developer (Design) | Player (Performance) |
| :--- | :--- | :--- |
| **ADSR / Filtro** | Completo (Design) | N/A (Striped na compilação) |
| **Fades / Tuning** | Ajustável por região | Leitura de Metadados |
| **Efeitos (Comp/EQ/etc)** | Ajuste auditivo total | Apenas se ativado no Developer |
| **Loops** | Importação e Sync | Disparo e Drag-to-DAW |
| **Microfones** | Mixagem de Camadas | Som consolidado/Mixado |

---
*Atualizado em 2026-03-05*
