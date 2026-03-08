# Checkpoint de Retomada: SoteroBuilder Layout Refactor

Este documento serve como guia para a próxima sessão de desenvolvimento, garantindo que o progresso da Fase 6 não seja perdido.

## Status Atual (Concluído hoje)
- [x] **Modularização da UI**: `MainComponent` agora usa structs separadas (`HeaderPanel`, `WaveformEditor`, `SculptingPanel`, `MappingPanel`, `MetadataPanel`, `AdvancedPanel`).
- [x] **Layout Resized**: Toda a lógica de posicionamento foi reescrita no `MainComponent::resized()` principal.
- [x] **Controles de Projeto**: Botões `SAVE`, `LOAD` e `NEW` no cabeçalho estão funcionais e conectados ao `SoteroArchive`.
- [x] **Mapeamento Dual Layer**: Piano Rolls e teclados para Layer 1 (Ciano) e Layer 2 (Vermelho) estão visíveis e funcionam simultaneamente.
- [x] **Seletor de Oitavas**: Centralizado no `MappingPanel` e atualizando o range midi corretamente para ambas as camadas.

## Ponto de Retomada (Próximos Passos)
1. **Sistema de Seleção**: Implementar a lógica para clicar em uma região do grid e carregar seus parâmetros nos `SculptingTools` (ADSR/Filtro).
2. **Visualizador de Waveform**: Implementar o componente real no `WaveformEditor` (atualmente é um placeholder).
3. **Advanced Processing**: Começar a implementação do painel de FX e Loops MIDI.
4. **Fase 7**: Preparar a exportação definitiva do formato `.SPSA` com todos os novos metadados.

## Arquivos Críticos
- `MainComponent.h/cpp`: Estrutura principal da interface.
- `SoteroFormat.h`: Definição das estruturas de dados (`KeyMapping`, `LibraryMetadata`).
- `SoteroArchive.h`: Lógica de salvamento e carregamento.

## Observações Técnicas
- **Lints**: Existem avisos de undeclared identifiers para `juce::` em alguns headers de Common devido à forma como o VS limpa o cache, mas eles resolvem na compilação.
- **D Drive**: Todo o código e documentação estão no disco `D:\Luis\OneDrive\Juce\Projetos\SoteroSamplerSuite\`.

---
*Checkpoint criado em 07/03/2026.*
