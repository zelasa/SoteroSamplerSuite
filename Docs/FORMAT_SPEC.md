# Especificação Técnica do Formato: Sotero Poly Samples Archive

Este documento serve como a especificação formal para o registro e implementação do tipo de arquivo proprietário da **SoteropolySamples**.

## 1. Identificação de Amostra Técnica
- **Nome do Formato**: Sotero Poly Samples Archive
- **Extensões Sugeridas**: `.spsa`, `.sotero`
- **MIME Type**: `application/vnd.soteropolysamples.archive`
- **Assinatura de Arquivo (Magic Number)**: `SOTEROPY` (hex: `53 4F 54 45 52 4F 50 59`)
- **Versão Atual**: 1.0.0

## 2. Visão Geral
O formato é um container binário projetado para o empacotamento seguro de bibliotecas de áudio multinível (velocidades), incluindo metadados estruturados e recursos visuais. Sua arquitetura prioriza a compatibilidade cross-platform (Windows, macOS, Linux) e o acesso aleatório rápido (Instant seek) sem a necessidade de descompactação total em memória.

## 3. Arquitetura de Layout (Binário)

O arquivo é composto por três segmentos principais obrigatoriamente ordenados em **Little-Endian**:

### 3.1. Cabeçalho (Header) - Fixo
| Offset | Tamanho | Tipo | Descrição |
| :--- | :--- | :--- | :--- |
| 0 | 8 bytes | char[8] | Magic Number: `SOTEROPY` |
| 8 | 4 bytes | uint32 | Versão do Formato (atualmente 1) |
| 12 | 4 bytes | uint32 | Comprimento do Manifesto XML (N bytes) |
| 16 | 4 bytes | uint32 | flags de Criptografia (0=Nenhum, 1=DNA-Encryption) |

### 3.2. Manifesto (Metadata XML)
Localizado imediatamente após o cabeçalho.
- **Formato**: XML (UTF-8).
- **Estrutura de Amostras (One-Shots)**:
    - **Layers de Microfonação**: Suporte nativo a `Layer 1` e `Layer 2`.
    - **Mapeamento MIDI**: Visualização vertical (V0 a V127).
    - **Parâmetros por Região**: 
        - `sampleStart`, `sampleEnd`, `fadeIn`, `fadeOut` (int64).
        - `volume`, `tune`, `fineTune` (knobs dedicados).
        - `adsr` (A, D, S, R numéricos).
- **Curva de Velocity**:
    - `velocityCurveType`, `velocityFilterSens`.
- **Estrutura de Loops (MIDI clips)**:
    - **Abas**: A, B, C (12 slots cromáticos cada).
    - **Sync**: `dawSync` (ON/OFF), `bpmValue`.
- **Metadados da Livraria (Informações)**:
    - `nome`, `data`, `autor`, `instrumento`, `informacoes` (texto livre).
    - `artworkPath` (Suporte a JPG/PNG/TIFF até 25MB).
- **Flags de Exportação (To Player)**:
    - Switches individuais para: `Layer 1`, `Layer 2`, `Velocity Curve`, `Compressor`, `Reverb`, `Equalizer`, `Punch`, `Loops (A/B/C)`.

### 3.3. Bloco de Dados Binários (Payload)
Localizado após o Manifesto XML.
- **Recursos Visuais**: Imagens JPG/PNG/WebP para capas de livrarias.
- **Recursos de Áudio**: Amostras de áudio concatenadas.
- **Alinhamento**: Todos os sub-blocos dentro do payload devem estar alinhados em intervalos de **8 bytes** para otimização em arquiteturas ARM (Apple Silicon).

## 4. Conformidade Cross-Platform
- **Byte Order**: O arquivo deve ser lido e escrito estritamente em **Little-Endian**.
- **Fixed Width**: Implementações devem usar tipos `uint32_t` e `uint64_t` para garantir paridade entre sistemas de 32 e 64 bits.

## 5. Segurança e Propriedade
O acesso ao conteúdo bruto de áudio é protegido pela arquitetura de "Closed Engine" do SamplerPlayer. O DNA-Encryption (Fase 3) obfuscara o Bloco de Dados Binários, impedindo a extração de WAVs por ferramentas de extração genéricas ou visualizadores hexadecimais comuns.

---
© 2026 SoteropolySamples. Todos os direitos reservados.
