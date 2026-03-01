# SamplerPlayer Roadmap

Este documento define a visão do SamplerPlayer como uma plataforma exclusiva e fechada para livrarias da SoteropolySamples.

## Visão do Produto: "Closed Player"
O SamplerPlayer não será um sampler aberto para qualquer WAV. Ele será um reprodutor otimizado que carrega apenas livrarias curadas e editadas internamente, garantindo a qualidade final do som (Corte de samples, volumes e mixagem prontos).

## Próximas Implementações (Curto Prazo)

### 1. Novo Motor de Velocities (5 Níveis)
- [ ] Implementar suporte interno para 5 camadas de velocity por timbre.
- [ ] Transição dinâmica e invisível para o usuário.

### 2. Compressor de Precisão (VCA Moderno)
- [ ] Threshold: -60 dB a 0 dB.
- [ ] Ratios: 2:1, 3:1, 4:1, 6:1 e INF (Limiter).
- [ ] Attack: 5, 10, 15, 25, 35 ms.
- [ ] Release: 20, 40, 60, 90, 120 ms.

### 3. Reverb Algorítmico Customizado
- [ ] Tipos: Room, Hall, Plate e Stadium.
- [ ] Controles: On/Off, Size (Tamanho da sala), Pre-delay e Wet/Dry (Mix).

### 4. Mecanismo de Choke Groups (Cancelamento)
- [ ] Implementar lógica de "Exclusive Groups" (Ex: Tecla X corta Tecla Y).
- [ ] Configuração embutida nos metadados da biblioteca.

## Ferramentas de Desenvolvedor (Internal Tooling) - App Independente
- [ ] **SoteroBuilder / SSA-Export**: Desenvolver um software **standalone e separado do player** para criação de livrarias.
    - [ ] Interface focada em importação de WAVs e mapeamento de camadas.
    - [ ] Conversão e empacotamento em um único arquivo de **Soundbank (.SSA)**.
    - [ ] Empacotamento de todos os samples em um único arquivo binário.
    - [ ] Compressão interna (ex: FLAC/Ogg) para reduzir tamanho sem perder qualidade.
    - [ ] Criptografia de DNA para impedir abertura por players externos.
    - [ ] Mapeamento de 12 teclas (Dó a Si) com suporte às 5 camadas de velocity.

## Estratégia de Desenvolvimento: Monorepo
Para garantir que o **Player** e o **Builder** falem a mesma língua (formato .SSA), utilizaremos uma estrutura de projeto unificada:

- **SoteroSamplerSuite/** (Raiz)
    - **Common/**: Código compartilhado (Lógica do formato .SSA, Criptografia, Estruturas de Áudio).
    - **SamplerPlayer/**: O motor de reprodução (Plugin VST3/AU/Standalone).
    - **SoteroBuilder/**: A ferramenta de criação de livrarias (App Standalone).
    - **Assets/**: Logos, fontes e recursos visuais comuns.

## Ideias de Médio/Longo Prazo
- [ ] **Interface de Performance**: Foco total em 12 pads/teclas visuais com medidores de pico (dB) e indicadores de clip.
- [ ] **Multi-saída**: Roteamento individual para DAW.
- [ ] **Host de Plugins**: Carregamento de VSTs externos (conforme `Future Ideas`).

## Estratégia de Versionamento (SemVer)
Adotaremos o padrão **x.y.z** para controle de qualidade:
- **Major (x)**: Mudanças estruturais ou quebras de compatibilidade (ex: Novo formato .SSA).
- **Minor (y)**: Novas funcionalidades (ex: Adição de Reverb ou novos filtros).
- **Patch (z)**: Correções de bugs e melhorias de performance.

---
*Nota: Este Roadmap é um documento vivo e deve ser atualizado conforme novas necessidades surgirem.*
