# Alinhamento SisTer-URT no Ecossistema SisTer

Documento de enquadramento constitutivo, funcional e operacional do **SisTer-URT**, alinhado ao **Documento Fundacional do SisTer (v0.2.0)**, ao repositório central **SisTer** e ao harness operacional **Sister-Infra**.

---

## 1. Enquadramento Ontológico e Papel Funcional

Conforme o Capítulo 5 do Documento Fundacional:
> *"SisTer é um paradigma para a constituição de sistemas inteligentes pela composição de sistemas autônomos, transparentes e reflexivos, capazes de compartilhar capacidades e fazer emergir inteligência das relações que estabelecem, sem abdicar de identidade, estado, história e responsabilidade próprios."*

### 1.1. Papel Funcional de Domínio (`Domain Role`)
O **SisTer-URT** exerce **Papel de Domínio** dentro do ecossistema:
* **Domínio de Autoridade**: Unidades de Referência Tecnológica (URTs) em Sistemas Silvipastoris (SP), Agrossilvipastoris (ASP) e Agroflorestais (SAF) no âmbito da parceria interinstitucional **CRSul / CNPF / EMATER-RS / EPAGRI**.
* **Estado Autoritativo Local**: Mantém a verdade e a integridade sobre o cadastro das URTs (Camada A: Essencial, Camada B: Caracterização Técnica, Camada C: Evidências e Documentos).
* **Autonomia e Desacoplamento**: Opera localmente sem dependência ontológica de um banco centralizado ou orquestrador controlador (Princípios C1 e C2 da Constituição SisTer).

---

## 2. Resposta à Fronteira Mínima do Participante (Seção 7.3 do Documento Fundacional)

| Operação Semântica | Resposta do SisTer-URT | Endpoint Técnico |
| :--- | :--- | :--- |
| `who_are_you()` | Identidade `sister_urt`, versão `0.1.0`, papel funcional `domain` | `GET /_sister/identity` |
| `what_can_you_do()` | Catálogo de capabilities (`urt.cadastro.read`, `urt.cadastro.create`, `urt.validation.transition`, `urt.metrics.evaluate`, `urt.dataset.export`) | `GET /_sister/capabilities` |
| `what_do_you_own()` | Domínio exclusivo do cadastro, caracterização e validação de URTs silvipastoris | `GET /_sister/manifest` |
| `how_to_talk_to_you()` | Contrato `sister.subsystem/1.0.0`, REST JSON em `127.0.0.1:8094` | `GET /_sister/manifest` |
| `what_is_your_state()` | Prontidão de domínio, repositório e governança (`status: ready`) | `GET /_sister/ready`, `GET /_sister/health` |
| `what_happened()` | Trilha de auditoria imutável com `TransitoryReceipt` (`predecessor_id` $\rightarrow$ `successor_id`, autoridade e motivo) | `GET /api/urts/{id}` |
| `what_are_your_limits()` | Não impõe modelos climáticos globais (delega a Atmos) nem centraliza ontologia de avaliação (delega a Praxis) | Manifesto & Contratos |

---

## 3. Relações Inter-Sistemas Governadas

```text
┌─────────────────────────────────────────────────────────────┐
│                      SISTER-INFRA                           │
│  - Gateway HAProxy / TLS (Porta 8443)                       │
│  - Hostname virtual: urt-gateway.test                       │
│  - Roteamento para loopback interno (127.0.0.1:8094)        │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────▼──────────────────────────────┐
│                       SISTER-URT                            │
│                  (Papel de Domínio)                         │
│  - Autoridade de estado: Camadas A, B e C                   │
│  - Persistência e governança de transições locais           │
│  - Endpoints /_sister/* e interface web integrada           │
└──────────────┬──────────────────────────────▲───────────────┘
               │                              │
     evidências e contextos           avaliação governada
               │                              │
┌──────────────▼──────────────────────────────┴───────────────┐
│                      SISTER-PRAXIS                          │
│            (Papel Cognitivo-Metodológico)                   │
│  - Avaliação de conformidade e maturidade                   │
│  - Gramática epistêmica sem possuir a semântica silvipastoril│
└─────────────────────────────────────────────────────────────┘
```

---

## 4. Alinhamento Operacional no Sister-Infra

* **Porta Interna**: `127.0.0.1:8094`
* **Health Check**: `GET /_sister/health`
* **Host LAN/TLS**: `urt-gateway.test:8443`
* **Contrato de Subconjunto**: `sister.subsystem/1.0.0`
