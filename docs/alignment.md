# Alinhamento SisTer-URT no Ecossistema SisTer

Documento de enquadramento constitutivo, funcional e operacional do **SisTer-URT**, alinhado ao **Documento Fundacional do SisTer (v0.2.0)**, ao repositório central **SisTer** e ao harness operacional **Sister-Infra**.

---

## 1. Enquadramento Ontológico e Papel Funcional

Conforme o Capítulo 5 do Documento Fundacional:
> *"SisTer é um paradigma para a constituição de sistemas inteligentes pela composição de sistemas autônomos, transparentes e reflexivos, capazes de compartilhar capacidades e fazer emergir inteligência das relações que estabelecem, sem abdicar de identidade, estado, história e responsabilidade próprios."*

### 1.1. Papel Funcional de Domínio (`Domain Role`)
O **SisTer-URT** exerce **Papel de Domínio** dentro do ecossistema:
* **Domínio de Autoridade**: Unidades de Referência Tecnológica (URTs) em Sistemas Silvipastoris (SP), Agrossilvipastoris (ASP) e Agroflorestais (SAF) no âmbito da parceria interinstitucional **CRSul / CNPF / EMATER-RS / EPAGRI / EMATER-PR**.
* **Estado Autoritativo Local**: Mantém a verdade e a integridade sobre o cadastro das URTs (Camada A: Essencial, Camada B: Caracterização Técnica, Camada C: Evidências e Documentos) com persistência atômica local em `AuthoritativeStorage`.
* **História Append-Only**: Preserva a linhagem e os recibos de transição (`TransitoryReceipt`) de forma append-only, sem alegações prematuras de imutabilidade criptográfica.
* **Autonomia e Desacoplamento**: Opera localmente sem dependência ontológica de um banco centralizado ou orquestrador controlador (Princípios C1 e C2 da Constituição SisTer).

---

## 2. Resposta à Fronteira Mínima do Participante (Seção 7.3 do Documento Fundacional)

A camada `sister_urt_participant` materializa a fronteira semântica de forma **independente de transporte**:

| Operação Semântica | Resposta do SisTer-URT | Interface C++ / Endpoint Técnico |
| :--- | :--- | :--- |
| `who_are_you()` | Identidade `sister_urt`, versão `0.1.0`, papel funcional `domain`, modelo de história `append_only_transition_history` | `UrtParticipant::who_are_you()` / `GET /_sister/identity` |
| `what_can_you_do()` | Catálogo de capabilities (`urt.cadastro.read`, `urt.cadastro.create`, `urt.validation.transition`, `urt.metrics.evaluate`, `urt.dataset.export`) | `UrtParticipant::what_can_you_do()` / `GET /_sister/capabilities` |
| `what_do_you_own()` | Domínio exclusivo do cadastro, caracterização e validação de URTs silvipastoris | `UrtParticipant::what_do_you_own()` / `GET /_sister/manifest` |
| `how_to_talk_to_you()` | Contratos `sister.subsystem/1.0.0`, `sister.participant/1.0.0`, bindings `http` e `in_process_cpp` | `UrtParticipant::how_to_talk_to_you()` / `GET /_sister/manifest` |
| `what_is_your_state()` | Prontidão observada de storage local, integridade do repositório, digest do manifesto e governança | `UrtParticipant::what_is_your_state()` / `GET /_sister/ready` |
| `what_happened(id)` | Trilha de auditoria append-only com `TransitoryReceipt` (`predecessor_state_id` $\rightarrow$ `successor_state_id`, autoridade e motivo) | `UrtParticipant::what_happened()` / `GET /api/urts/{id}` |
| `what_are_your_limits()` | Não impõe modelos climáticos globais (delega ao Atmos) nem centraliza avaliação metodológica (delega ao SisTer-Praxis) | `UrtParticipant::what_are_your_limits()` |
| `what_roles_can_you_play()` | Papéis contextuais potenciais: `["domain"]` | `UrtParticipant::what_roles_can_you_play()` |

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
│  - Camada Participant semântica e adapter HTTP              │
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
* **Contrato de Subconjunto**: `sister.subsystem/1.0.0` (Binding HTTP de compatibilidade)
