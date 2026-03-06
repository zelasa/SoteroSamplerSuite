# Sotero Suite - Developer Guide

Este guia contém as diretrizes técnicas, padrões de código e fluxos de trabalho para desenvolvedores e agentes de IA trabalhando na suite.

## 1. Padrões de Código (JUCE/C++)
1. **Naming**: Usar `camelCase` para métodos e variáveis, `PascalCase` para Classes.
2. **Propriedade**: Sempre usar `std::unique_ptr` para gerenciamento de vida de componentes JUCE.
3. **UI**: Todas as dimensões devem ser pensadas para telas High-DPI (Escalabilidade).
4. **Branding**: SoteropolySamples deve ser tratada como uma marca premium. UI nunca deve parecer amadora.

## 2. Estratégia de Versionamento (SemVer 2.0.0)
Seguimos estritamente o Semantic Versioning (x.y.z):
- **MAJOR (x)**: Mudanças incompatíveis (Breaking Changes).
- **MINOR (y)**: Novas funcionalidades retrocompatíveis.
- **PATCH (z)**: Correções de bugs e otimizações internas.

*Regra de Ouro*: O **SoteroBuilder** e o **SamplerPlayer** devem manter paridade de versão MAJOR para garantir compatibilidade do formato .SSA/.SPSA.

## 3. Workflow de Git e GitHub

### Configuração Inicial
```powershell
git config --global user.name "Seu Nome"
git config --global user.email "seu-email@exemplo.com"
```

### Protocolo de Commits
Toda tarefa significativa deve ser commitada com o formato: `[PROJETO] Descrição curta (vX.Y.Z)`.
Ex: `[SamplerPlayer] Adicionado motor de ADSR (v0.3.0)`

## 4. Protocolo de Backup Local (Offline)
A cada Milestone importante:
1. **Destino**: `d:/Luis/OneDrive/Juce/Projetos/Backups/SoteroSuite/`
2. **Nome**: `SoteroSuite_vX.Y.Z_YYYY-MM-DD.zip`
3. **Comando**:
   `Compress-Archive -Path SoteroSamplerSuite/* -DestinationPath d:/Luis/OneDrive/Juce/Projetos/Backups/SoteroSuite/SoteroSuite_vX.Y.Z_YYYY.zip`

---
*Nota: Consulte o AGENTS.md para diretrizes específicas de IA.*
