# Sotero Suite - AI Agent Instructions (AGENTS.md)

Este documento contém diretrizes obrigatórias para qualquer agente de IA trabalhando neste projeto. Estas regras garantem a integridade da marca e a estabilidade técnica da **SoteropolySamples**.

## 1. Diretrizes de IA e Desenvolvimento
As regras de versionamento (SemVer), arquitetura de projeto e padrões de código estão centralizadas no [Sotero Developer Guide](Docs/DEVELOPER_GUIDE.md). Agentes devem seguir essas diretrizes rigorosamente.

## 2. Padrões Adicionais
Consulte o guia técnico para naming, gerenciamento de memória e branding.

## 4. Fluxo de Trabalho do Agente

- Sempre valide o `CMakeLists.txt`- Always "Batch" changes in logical commits.
- Nunca apague seções do `Roadmap.md` sem autorização explícita do usuário.
- Ao encontrar um bug, priorize o conserto (Patch) antes de prosseguir com novas funcionalidades (Minor).

## 5. Protocolo de Commit e Versionamento de Código

Sempre que uma tarefa significativa (Checklist ID do `task.md`) for concluída:
1. **Mensagem de Commit**: Deve seguir o formato: `[PROJETO] Descrição curta (vX.Y.Z)`.
   - Ex: `[SamplerPlayer] Adicionado monitor de velocity (v0.2.1)`
2. **Push**: Sempre realizar o push para o repositório remoto após a conclusão de uma sessão de trabalho.

## 6. Protocolo de Backup Local (Offline)

A cada grande Milestone (mudança de Versão Minor ou Major), ou quando solicitado pelo usuário:
1. **Formato**: Arquivo ZIP.
2. **Nomenclatura**: `SoteroSuite_vX.Y.Z_YYYY-MM-DD.zip`.
3. **Conteúdo**: Apenas os arquivos de código/assets necessários (ignorar pastas de build como `out/`, `_juce/`, `.vs/`).
4. **Local**: Deve ser salvo na pasta externa `d:/Luis/OneDrive/Juce/Projetos/Backups/SoteroSuite/`.
5. **Periodicidade**: A cada Milestone ou fim de sessão produtiva importante.
