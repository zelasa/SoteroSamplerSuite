# Guia de Configuração Git e GitHub (Sotero Suite)

Para que possamos realizar commits e pushes corretamente, precisamos seguir estes passos de configuração na sua máquina.

## 1. Instalação e Identificação
Se você ainda não tem o Git instalado, baixe em [git-scm.com](https://git-scm.com/). No terminal (PowerShell), execute os comandos abaixo substituindo pelos seus dados:

```powershell
git config --global user.name "Seu Nome"
git config --global user.email "seu-email@exemplo.com"
```

## 2. Inicialização do Repositório Local
Abra o terminal na pasta raiz do projeto (`d:/Luis/OneDrive/Juce/Projetos/SoteroSamplerSuite`) e execute:

```powershell
git init
git add .
git commit -m "[Suite] Initial Commit (v0.1.0)"
```

## 3. Autenticação no GitHub (Importante)
Para que eu tenha permissão de enviar o código, é necessário configurar um **Personal Access Token (PAT)** ou usar o **GitHub CLI**.

### Via GitHub CLI (Recomendado)
1. Instale o [GitHub CLI](https://cli.github.com/).
2. No terminal, digite `gh auth login` e siga as instruções para autorizar o seu navegador.

### Via Token (Manual)
1. Vá em [GitHub Settings > Developer Settings > Personal Access Tokens (Classic)](https://github.com/settings/tokens).
2. Gere um novo token com as permissões de `repo`.
3. Copie o token.

## 4. Conectando ao Repositório Remoto
Crie um repositório vazio no GitHub chamado `SoteroSamplerSuite` e vincule-o:

```powershell
git remote add origin https://github.com/SEU_USUARIO/SoteroSamplerSuite.git
git branch -M main
# Se usar Token: git remote set-url origin https://SEU_TOKEN@github.com/SEU_USUARIO/SoteroSamplerSuite.git
git push -u origin main
```

## 5. Backups Locais (Zips)
Para criar o backup zipado (conforme definido no AGENTS.md):
- **Destino**: `d:/Luis/OneDrive/Juce/Projetos/Backups/SoteroSuite/`
- **Nome**: `SoteroSuite_vX.Y.Z_YYYY-MM-DD.zip`
- No PowerShell, execute na pasta `Projetos/`:
  `Compress-Archive -Path SoteroSamplerSuite/* -DestinationPath Backups/SoteroSuite/SoteroSuite_v0.1.0_2026-03-01.zip`
