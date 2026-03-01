# SamplerPlayer Roadmap

Este documento define a visão do SamplerPlayer como uma plataforma exclusiva e fechada para livrarias da SoteropolySamples.

## Visão do Produto: "Closed Player"
O SamplerPlayer não será um sampler aberto para qualquer WAV. Ele será um reprodutor otimizado que carrega apenas livrarias curadas e editadas internamente, garantindo a qualidade final do som (Corte de samples, volumes e mixagem prontos).

## Próximas Implementações (Curto Prazo)

### 5. Articulação Avançada (Envelope ADSR)
- [ ] Implementar controles globais de Attack, Decay, Sustain e Release.
- [ ] Moldagem dinâmica do som da livraria por nota.

### 6. Filtro Mestre Profissional
- [ ] Filtros Low-pass e High-pass com controles de Cutoff e Resonance.

### 7. Motor de Realismo (Round Robin)
- [ ] Implementar alternância inteligente de samples para a mesma nota/velocity.
- [ ] Evitar o efeito "metralhadora" em instrumentos percussivos.

### 8. Gestão de Livrarias (Library Browser)
- [ ] Barra lateral visual para navegação rápida entre arquivos `.spsa`.
- [ ] Carregamento visual com exibição da capa/artwork da biblioteca.

### 9. Modulação e Vida (LFO)
- [ ] LFO para modulação de Volume, Pitch (Vibrato) e Cutoff.

### 10. Feedback Visual e Performance
- [ ] Visualizador de Waveform em tempo real com indicador de playhead.
- [ ] Suporte a Keyswitches para troca de articulações em tempo real.

## Ferramentas de Desenvolvedor (Internal Tooling) - App Independente
- [x] **SoteroBuilder / Exportador**: Software **standalone** para criação de livrarias.
    - [x] Interface focada em importação de WAVs e mapeamento de camadas.
    - [x] Suporte a metadados em **XML** e inclusão de **Artes/Capa**.
    - [x] Conversão e empacotamento em um arquivo de **Soundbank (.spsa)**.
    - [x] Empacotamento de todos os samples em um único arquivo binário.
    - [ ] Compressão interna (ex: FLAC/Ogg) para reduzir tamanho.
    - [ ] Criptografia de DNA para impedir abertura por players externos.
    - [x] Registro oficial do tipo de arquivo (IANA/MIME) para conformidade internacional.
    - [x] Mapeamento de 12 teclas (Dó a Si) com suporte às 5 camadas de velocity.

## Estratégia de Desenvolvimento: Monorepo
Para garantir que o **Player** e o **Builder** falem a mesma língua (formato .SPSA), utilizamos uma estrutura de projeto unificada:

- **SoteroSamplerSuite/** (Raiz)
    - **Common/**: Código compartilhado (Lógica do formato .SPSA, Estruturas de Áudio).
    - **SamplerPlayer/**: O motor de reprodução unificado (Plugin VST3/AU/Standalone).
    - **SoteroBuilder/**: A ferramenta de criação de livrarias (App Standalone).
    - **Assets/**: Logos, fontes e recursos visuais comuns.

## Cronograma de Desenvolvimento
O desenvolvimento será realizado em fases focadas em UX e Profissionalismo.
- **Fase 3**: Redesenho do Player (Unified Library View) + ADSR e Filtro Mestre.
- **Fase 4**: Motor de Realismo (Round Robin) + Browser de Livrarias e Keyswitches.
- **Ver detalhes em**: [Sotero_Suite_Development_Phases.md](file:///d:/Luis/OneDrive/Juce/Projetos/SoteroSamplerSuite/Docs/Sotero_Suite_Development_Phases.md)

## Ideias de Longo Prazo
- [ ] **Multi-saída**: Roteamento individual para DAW.
- [ ] **Host de Plugins**: Carregamento de VSTs externos.

## Estratégia de Versionamento (SemVer)
Adotaremos o padrão **x.y.z** para controle de qualidade:
- **Major (x)**: Mudanças estruturais ou redesenho total (ex: Versão 1.0.0 com Player Unificado).
- **Minor (y)**: Novas funcionalidades (ex: Adição de Round Robin ou LFO).
- **Patch (z)**: Correções de bugs e melhorias de performance.

---
*Nota: Este Roadmap é um documento vivo e deve ser atualizado conforme novas necessidades surgirem.*
