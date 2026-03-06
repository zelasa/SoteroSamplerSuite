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

## 5. Camada de Segurança e Anti-Pirataria (Sotero Shield)

Esta camada garante que o conteúdo seja acessível apenas por usuários legítimos através do SamplerPlayer.

### 5.1. Mecanismos de Proteção
- **DNA Encryption**: Criptografia binária dos assets (`.spsa`) usando uma chave única que combina o ID do usuário e o Hardware ID (HWID) da máquina.
- **Validação Online (Call-Home)**: O Player exige login (Plataforma Sotero) para validar a sessão do usuário.
- **Licenciamento por Serial**: Cada biblioteca adquirida gera um Serial/Key vinculado à conta do usuário, validado no primeiro carregamento.
- **Stream de Leitura Blindado**: O motor de áudio lê os samples diretamente da memória criptografada, sem extrair arquivos temporários no disco (evitando interceptação).

### 5.2. Fluxo de Ativação do Usuário
1. **Login**: Usuário insere credenciais no Player.
2. **Serial**: Player solicita a Chave da Biblioteca (comprada ou via assinatura).
3. **Binding**: O sistema vincula a licença ao HWID do computador do usuário.
4. **Desbloqueio**: O arquivo `.spsa` passa a ser descriptografado em tempo real pelo motor de áudio.

---

## 6. Evolução das Fases com Foco em Segurança

### Fase 3: Engine Foundation (Atual)
- [ ] **Encrypted Reader**: Implementar a classe `SoteroEncryptedStream` para leitura segura de blocos binários.
- [ ] **Format Security**: Ativar os `encryptionFlags` no cabeçalho do arquivo .spsa.

### Fase Q&A 1: Validação Técnica
- [ ] **Pentest Básico**: Tentar extrair áudio do arquivo .spsa sem a chave de descriptografia.

### Fase 5: Evolução do "SamplerPlayer"
- [ ] **Módulo de Autenticação**: Interface de Login/Senha integrada ao Player.
- [ ] **Gerenciador de Licenças**: Sistema de input de Serial e validação via API.
- [ ] **Hardware Locking**: Algoritmo de geração de HWID para travar o uso na máquina autorizada.

---
*Atualizado em 2026-03-05*
