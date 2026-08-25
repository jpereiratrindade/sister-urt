# SisTer-URT (Cadastro e Caracterização de URTs)

**Sistema observacional, georreferenciado e governado para Unidades de Referência Tecnológica (Sistemas Silvipastoris e arranjos associados)** — Parceria interinstitucional **CRSul / CNPF / EMATER-RS / EPAGRI**.

Construído em **C++23** com **interface web integrada** sobre os princípios epistêmicos do ecossistema **SisTer-Praxis**.

---

## 1. Estratégia em Três Camadas

Conforme a proposta interinstitucional v0.1:
1. **Camada A — Registro Essencial**: Identificação única, localização georreferenciada (coordenadas), município/UF, instituição de referência, técnico responsável e propriedade.
2. **Camada B — Caracterização Técnica**: Tipo de sistema (`SP` - Silvipastoril, `ASP` - Agrossilvipastoril, `SAF` - Agroflorestal), área (ha), data de implantação, espécies arbóreas, pastagem forrageira, componente animal, arranjo espacial e síntese de manejo.
3. **Camada C — Evidências e Histórico**: Fontes documentais, fotos, relatórios técnicos, datas de atualização e proveniência epistêmica.

---

## 2. Governança Epistêmica (Praxis)

* **Estados de Validação**: `preliminar` $\rightarrow$ `requer_revisao` $\rightarrow$ `validado`.
* **Imutabilidade e Linhagem**: Transições de estado geram recibos (`TransitoryReceipt`) imutáveis, associando `predecessor_state_id`, `successor_state_id`, autoridade e justificativa técnica, sem sobrescrever o passado.
* **Indicadores em Tempo Real**: Cálculo contínuo de % com coordenadas válidas, % com responsável técnico, % com Camada B completa e distribuição interinstitucional.

---

## 3. Compilação e Execução

### Requisitos
* Compilador C++23 (GCC 13+ ou Clang 17+)
* CMake 3.25+

### Compilação
```bash
./scripts/build.sh
```

### Executar Testes
```bash
./scripts/test.sh
```

### Executar CLI de Inspeção
```bash
./build/sister-urt-cli list
./build/sister-urt-cli show urt-rs-001
./build/sister-urt-cli metrics
```

### Executar Servidor Web / API REST
```bash
./scripts/run.sh 8094
# Acesse no navegador: http://127.0.0.1:8094/
```

---

## 4. Endpoints REST da API

* `GET /` — Interface Web interativa (SPA com mapa Leaflet e fichas por camadas).
* `GET /api/health` — Status de integridade e contagem da base.
* `GET /api/urts` — Listagem com filtros por instituição, município, tipo de sistema, situação e status.
* `GET /api/urts/{id}` — Ficha completa por Camadas A, B, C e trilha de transições.
* `POST /api/urts` — Inclusão governada de novo cadastro de URT.
* `POST /api/urts/{id}/validate` — Transição epistêmica com registro de recibo e autoridade.
* `GET /api/metrics` — Indicadores globais de cobertura e completude.
* `GET /api/export` — Exportação da base completa em JSON.

---

## 5. Licença

SPDX-License-Identifier: GPL-3.0-or-later
