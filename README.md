# SisTer-URT (Cadastro e Caracterização de URTs)

**Sistema de domínio autônomo, observacional e governado para Unidades de Referência Tecnológica (Sistemas Silvipastoris e arranjos associados)** — Parceria interinstitucional **CPPSul / CNPF / EMATER-RS / EPAGRI / EMATER-PR**.

Construído em **C++23** com **fronteira semântica de participante SisTer**, persistência atômica local e **interface web integrada**.

---

## 1. Status de Implementação e Garantias Demonstradas

| Dimensão / Propriedade | Status | Evidência Técnica |
| :--- | :--- | :--- |
| **Autonomia de Existência (C1)** | `IMPLEMENTADO` | Opera e persiste localmente sem dependência de centro global ou banco compartilhado. |
| **Autoridade Local de Estado (C2)** | `IMPLEMENTADO` | Mantém autoridade primária sobre as Camadas A, B e C e sua integridade referencial. |
| **Persistência Autoritativa Local** | `IMPLEMENTADO` | `AuthoritativeStorage` com escrita atômica (`write tmp + rename`) e recuperação pós-restart (`persistence_test`). |
| **História Append-Only (C4)** | `IMPLEMENTADO` | Transições geram `TransitoryReceipt`; atualizações normais não sobrescrevem nem apagam recibos anteriores. |
| **Fail-Closed de Autoridade (C5)** | `IMPLEMENTADO` | Transições de validação sem `autoridade` ou `motivo` são estritamente rejeitadas no Core e no HTTP Adapter. |
| **Máquina de Estados Formal** | `IMPLEMENTADO` | Matriz de transições explícita em `StateMachine` (`state_machine_test`). |
| **Fronteira Semântica do Participante** | `IMPLEMENTADO` | Camada `sister_urt_participant` transport-neutral respondendo `who_are_you()`, `what_can_you_do()`, `what_do_you_own()`, etc. |
| **Readiness Observada** | `IMPLEMENTADO` | `what_is_your_state()` avalia storage, repositório, governança e integridade do manifesto canônico. |
| **Manifesto Canônico e Digest** | `IMPLEMENTADO` | Fonte única em `contracts/system_manifest.json` com cálculo de digest SHA-256 (`participant_test`). |
| **Capabilities Semânticas** | `IMPLEMENTADO` | Contratos semânticos versionados em `contracts/capabilities/`. |
| **Relação com SisTer-Praxis** | `PLANEJADO / DRAFT` | Draft formal da relação em `contracts/relations/relation_urt_praxis.draft.json` (sem acoplamento de código). |
| **Imutabilidade Criptográfica** | `NÃO DEMONSTRADO` | O sistema demonstra **história append-only estrutural** e não alega imutabilidade criptográfica prematura. |

---

## 2. Estratégia de Domínio em Três Camadas

Conforme a proposta interinstitucional v0.1:
1. **Camada A — Registro Essencial**: Identificação única (`codigo_urt`), denominação local, localização georreferenciada (`latitude`/`longitude`), município/UF, instituição de referência, técnico responsável e dados da propriedade.
2. **Camada B — Caracterização Técnica**: Tipo de sistema (`SP` - Silvipastoril, `ASP` - Agrossilvipastoril, `SAF` - Agroflorestal), área da URT (ha), data de implantação, situação atual (`Ativa`, `Inativa`, `Descaracterizada`), espécies arbóreas, pastagem forrageira, componente animal, arranjo espacial e síntese do manejo atual.
3. **Camada C — Evidências e Documentação**: Fontes de informação, registros documentais, datas de atualização e notas observacionais.

---

## 3. Arquitetura de Software em C++23

O projeto é modularizado em bibliotecas estáticas e executáveis independentes:

```text
┌─────────────────────────────────────────────────────────────┐
│                    sister-urt-http (Daemon)                 │
│                 GET /, /api/* e /_sister/*                  │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                  sister_urt_participant                     │
│    Fronteira Semântica Transport-Neutral (Seção 7.3)        │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                  sister_urt_repository                      │
│     Repositório Thread-Safe com AuthoritativeStorage        │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                    sister_urt_core                          │
│   Types, Invariants, StateMachine, Governance & Storage     │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Compilação e Testes

### Requisitos
* Compilador C++23 (GCC 13+ ou Clang 17+)
* CMake 3.25+

### Compilar
```bash
./scripts/build.sh
```

### Executar Testes Unitários e de Conformidade (6/6)
```bash
./scripts/test.sh
```
Suíte de testes executada:
1. `sister_urt_domain_test` — Invariantes de coordenadas e completude técnica.
2. `sister_urt_governance_test` — Transições governadas e geração de recibos.
3. `sister_urt_state_machine_test` — Matriz de estados e fail-closed de autoridade/motivo (`URT-STATE-01`, `URT-AUTH-01`, `URT-AUTH-02`).
4. `sister_urt_persistence_test` — Persistência atômica, reinício e preservação de história append-only (`URT-PERSIST-01`, `URT-RESTART-01`, `URT-HIST-01`).
5. `sister_urt_participant_test` — Fronteira semântica, digest do manifesto canônico, readiness observada e capabilities (`URT-PARTICIPANT-01`, `URT-MANIFEST-01`, `URT-READY-01`, `URT-CAPABILITY-01`).
6. `sister_urt_http_test` — Endpoints REST, rotas `/_sister/*` e validação fail-closed no transporte.

---

## 5. Executar o Servidor Web e API REST

```bash
./scripts/run.sh 8094
```
Acesse no navegador:
**[http://127.0.0.1:8094/](http://127.0.0.1:8094/)**

### Endpoints Técnicos de Participação (`/_sister/*`)
* `GET /_sister/manifest` — Manifesto canônico do participante
* `GET /_sister/health` — Vitalidade do processo
* `GET /_sister/ready` — Prontidão observada de armazenamento e governança
* `GET /_sister/capabilities` — Catálogo semântico de capabilities
* `GET /_sister/identity` — Resposta à fronteira `who_are_you()`

---

## 6. Licença

SPDX-License-Identifier: GPL-3.0-or-later
