# Guia de Referência DSP: Filosofia ASPiK no Ecossistema JUCE

Este documento serve como uma base de conhecimento para importar e traduzir conceitos brilhantes de Processamento Digital de Sinais (DSP) da literatura de Will Pirkle (criador do ASPiK/RackAFX) para o desenvolvimento prático dos nossos plugins usando o framework JUCE.

## 1. A Filosofia ASPiK (Fundamentos do Will Pirkle)
A abordagem do ASPiK para DSP de áudio é fortemente centrada na matemática pura voltada para processamento **Sample-a-Sample** (amostra por amostra).

*   **Separação Estrita:** DSP puro não conhece interface gráfica (GUI). Os objetos DSP (filtros, osciladores, ADSRs) recebem parâmetros brutos (ex: cutoff em Hz, attack em ms) e os convertem em coeficientes matemáticos.
*   **A "Função Mágica":** O núcleo de um objeto DSP no ASPiK geralmente gira em torno de uma função `processAudioSample(double xn)`, que recebe um único valor de amplitude (amostra de entrada) e devolve a amostra processada (`yn`).
*   **Atualização de Estado:** Quando a taxa de amostragem (`sampleRate`) muda na DAW, todos os coeficientes matemáticos do DSP devem ser recalculados (geralmente através de um método `reset()` ou `update()`).

## 2. O Casamento: ASPiK (Matemática) + JUCE (Engenharia)
O JUCE é otimizado para lidar com **Blocos de Áudio** (`AudioBuffer`) por questões de performance (SIMD, vetorização). Para trazer a clareza didática do ASPiK para o JUCE, fazemos uma ponte arquitetural:

No `renderNextBlock` do JUCE (ou no método `process` do `juce::dsp`), processamos o bloco quebrando-o em amostras individuais, iterando sobre o buffer, e aplicando a lógica de DSP estilo ASPiK amostra por amostra.

### Comparação Prática

**Modelo ASPiK (Teórico/Portátil):**
```cpp
// Processamento de LFO ou Filtro
double yn = myFilter.processAudioSample(xn);
```

**Modelo JUCE Híbrido (Onde embutimos o ASPiK):**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float xn = channelData[i]; // Amostra de entrada
            
            // ---> AQUI ENTRA A MATEMÁTICA DO ASPIK <---
            float yn = myCustomPirkleFilter.processAudioSample(xn); 
            
            channelData[i] = yn; // Amostra de saída
        }
    }
}
```

*Nota: O módulo `juce::dsp` moderno faz algo parecido, mas usando `ProcessContextReplacing` para facilitar a cadeia.*

## 3. Padrões de DSP Úteis (Dicionário ASPiK -> JUCE)

Ao estudar os livros de Will Pirkle (Ex: *Designing Software Synthesizer Plug-Ins in C++*), você pode transpor os módulos para o JUCE. Abaixo está o mapeamento dos conceitos:

### A. Osciladores (Wavetables vs VA)
*   **ASPiK:** Usa `Oscillator` base e deriva `WavetableOscillator` ou `VAOscillator` (Virtual Analog usando *Blep* para evitar aliasing). O loop do oscilador mantém um `m_dPhase` interno.
*   **No JUCE:** Nós traduzimos isso usando o leitor de Sampler/Wavetable manual e avançando o índice de leitura. A variável `currentSamplePosition` no nosso `SoteroSamplerVoice` atua exatamente como o acumulador de fase.

### B. Filtros (Biquads e SVF)
O coração da síntese subtrativa é o filtro.
*   **Filtros Biquad:** No ASPiK usamos a topologia Direct Form 1 ou 2. (Equação da diferença).
*   **Integração no JUCE:**
    O JUCE já possui as matemáticas do Pirkle implementadas via:
    `juce::dsp::IIR::Filter` (Para Biquads Padrões) e `juce::dsp::StateVariableTPTFilter` (Topology Preserving Transform). Optamos pelo SVF (TPT) do JUCE no Sotero porque eles respondem melhor à modulação rápida (LFO no cutoff) sem explodir o áudio (estabilidade).

### C. Envelopes (ADSR)
*   **ASPiK/Pirkle:** O Envelope Generator (EG) roda uma máquina de estados (`attack`, `decay`, `sustain`, `release`, `idle`, `shutdown`). O estado `shutdown` no ASPiK é útil em *choke groups* para matar a voz em 10ms rapidamente.
*   **No JUCE:** Usamos o `juce::ADSR`. Ele cobre todos esses estados. O mapeamento mental principal é lembrar que ao final do Release no JUCE, precisaremos avisar a GUI/Engine (via `clearCurrentNote()`) que a fase `idle` chegou.

### D. Efeitos baseados em Atraso (Delay/Chorus/Flanger)
*   **ASPiK:** Faz uso intenso de `CircularBuffer` (Buffer Circular / Ring Buffer) usando índices dinâmicos para ler amostras passadas com interpolação linear ou fracionária.
*   **No JUCE:** O JUCE não fornece um "Delay" algorítmico pronto em `juce::dsp`, mas fornece componentes de memória. Para criar um Delay com cara de ASPiK, criamos um `juce::AudioBuffer` auxiliar, e avançamos um ponteiro cíclico para escrever e ler do passado.

## 4. Regras de Ouro de DSP a Lembrar no Sotero

1.  **Sem alocação no fio da navalha:** Nunca use `new`, `std::vector::push_back` ou crie variáveis pesadas dentro do bloco `renderNextBlock` ou funções `processAudioSample()`.
2.  **O Tempo é Rei (Sample Rate):** Se o limite de Nyquist mudar (ex: usuário muda a interface de áudio de 44.1kHz para 96kHz), **TODOS** os tempos (Attack ms, Delay Time, Frequências de filtro) devem ter seus coeficientes re-processados utilizando o novo `sampleRate`.
3.  **Smoothers (Parâmetros Amaciados):** No mundo DSP, pular abruptamente o volume de 0.0 para 1.0 ou o Cutoff do filtro gera um "clique" mecânico no áudio. Usamos `juce::SmoothedValue` (ou o conceito de *One-Pole Filter* do Pirkle) para suavizar a transição dos knobs da UI.

---
*Este documento será atualizado conforme implementamos algoritmos complexos no Sotero Engine (como time-stretching, LFOs, ring-modulation, etc).*
