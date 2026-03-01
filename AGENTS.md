# Sotero Suite - AI Agent Instructions (AGENTS.md)

Este documento contém diretrizes obrigatórias para qualquer agente de IA trabalhando neste projeto. Estas regras garantem a integridade da marca e a estabilidade técnica da **SoteropolySamples**.

## 1. Estratégia de Versionamento (SemVer x.y.z)

Seguimos estritamente o **Semantic Versioning 2.0.0**.

- **X (MAJOR)**: Mudanças incompatíveis (Breaking Changes).
    - Ex: Alteração no motor de criptografia .SSA (livrarias antigas param de funcionar).
    - Ex: Reformulação total da UI/UX que mude o fluxo de trabalho.
- **Y (MINOR)**: Novas funcionalidades retrocompatíveis.
    - Ex: Adição de um novo efeito (Delay), suporte a novos tipos de samples.
    - Ex: Adição de uma nova aba de visualização.
- **Z (PATCH)**: Correções de bugs e otimizações internas.
    - Ex: Correção de um crash, otimização de CPU no motor de áudio.

### Regras de Ouro de Versão:
1. O **SoteroBuilder** e o **SamplerPlayer** devem manter paridade de versão MAJOR para garantir compatibilidade do formato .SSA.
2. Cada release deve ser documentada no `Docs/Changelog.md` (a ser criado).

## 2. Arquitetura do Projeto (Monorepo)

O projeto é organizado em uma única suite para evitar dessincronia de código.

- `/Common`: Contém lógica de criptografia, parse de .SSA e modelos de dados compartilhados.
- `/SamplerPlayer`: Código específico do Plugin/Standalone de áudio.
- `/SoteroBuilder`: Código específico da ferramenta de criação.

## 3. Padrões de Código (JUCE/C++)

1. **Naming**: Usar `camelCase` para métodos e variáveis, `PascalCase` para Classes.
2. **Propriedade**: Sempre usar `std::unique_ptr` para gerenciamento de vida de componentes JUCE.
3. **UI**: Todas as dimensões devem ser pensadas para telas High-DPI (Escalabilidade).
4. **Branding**: SoteropolySamples deve ser tratada como uma marca premium. UI nunca deve parecer amadora.

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
