# Dados do SisTer-URT

- `pilot_urts.json`: fixture piloto **não autoritativa**, preservada apenas para demonstração/testes e carregada somente quando um seed é solicitado explicitamente.
- `fixtures/pilot_authoritative_store.snapshot.json`: snapshot preservado do antigo store pré-populado com os 16 registros piloto. Não é carregado como estado operacional.
- `authoritative_store.json`: estado local de runtime quando usado em desenvolvimento; não é versionado e nasce ausente/vazio.

Uma instalação normal do SisTer-URT nunca deve promover automaticamente dados piloto a registros autoritativos.
