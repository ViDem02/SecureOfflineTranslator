# Secure Offline Translator
## Software Requirements Specification (SRS)

**Versione:** 0.1 — Draft  
**Data:** 7 novembre 2026  
**Stato:** Requisiti preliminari, da validare  
**Piattaforma iniziale:** macOS — Apple Silicon M3  
**Classificazione del documento:** Non riservato

---

## 1. Introduzione

### 1.1 Scopo

Il progetto **Secure Offline Translator (SOT)** ha l'obiettivo di sviluppare un sistema di traduzione automatica basato su intelligenza artificiale, capace di elaborare documenti tecnici e industriali senza trasmettere dati a infrastrutture esterne.

Il sistema deve garantire il controllo locale dei dati, limitare le superfici di attacco, consentire la verifica del traffico di rete e offrire un'interfaccia grafica intuitiva.

### 1.2 Obiettivi principali

1. Tradurre testi e documenti utilizzando modelli AI eseguiti localmente.
2. Funzionare senza connessione Internet durante l'elaborazione dei dati.
3. Impedire la trasmissione non autorizzata di informazioni riservate.
4. Consentire il monitoraggio e la verifica delle comunicazioni di rete.
5. Sfruttare l'accelerazione hardware Apple Silicon.
6. Ridurre le dipendenze da software proprietario e servizi cloud.
7. Offrire un sistema riproducibile e verificabile dal punto di vista della sicurezza.

### 1.3 Ambito

Il sistema è destinato alla traduzione di documenti tecnici, specifiche, procedure, comunicazioni professionali e altre informazioni potenzialmente soggette a obblighi di riservatezza.

**Vincolo fondamentale:** l'impiego effettivo su dati industriali classificati o protetti da NDA è subordinato all'autorizzazione dell'organizzazione proprietaria delle informazioni. L'implementazione dei requisiti non costituisce, da sola, certificazione di conformità.

---

## 2. Architettura di riferimento

### 2.1 Componenti

| Componente | Tecnologia candidata | Funzione |
|---|---|---|
| Host | macOS, Apple Silicon M3 | Ambiente di esecuzione |
| Container runtime | Docker Desktop | Gestione dei servizi |
| AI inference | Docker Model Runner | Inferenza locale con accelerazione Metal |
| Modello AI | TranslateGemma 12B | Traduzione automatica |
| Frontend | Open WebUI | Interfaccia grafica |
| Network monitoring | LuLu + Netiquette | Osservazione e controllo del traffico |
| Network isolation | Firewall host e controlli di rete | Limitazione delle comunicazioni |
| Storage | Volumi locali protetti | Gestione dei dati persistenti |

Le tecnologie indicate costituiscono una configurazione iniziale candidata. La compatibilità effettiva, la configurazione delle interfacce e la sicurezza devono essere validate prima del rilascio.

### 2.2 Architettura logica

```text
┌──────────────────────────────────────┐
│              macOS Host              │
│                                      │
│  ┌────────────────────────────────┐  │
│  │      Browser locale           │  │
│  │      localhost:3000           │  │
│  └───────────────┬────────────────┘  │
│                  │                   │
│  ┌───────────────▼────────────────┐  │
│  │       Docker Desktop           │  │
│  │  ┌──────────────────────────┐  │  │
│  │  │       Open WebUI         │  │  │
│  │  └─────────────┬────────────┘  │  │
│  └────────────────┼───────────────┘  │
│                   │ API locale       │
│  ┌────────────────▼───────────────┐  │
│  │       Docker Model Runner      │  │
│  │       TranslateGemma           │  │
│  │       GPU / Metal              │  │
│  └────────────────────────────────┘  │
│                                      │
│  Firewall + Network Monitoring       │
└──────────────────┬───────────────────┘
                   X
                INTERNET
             (non autorizzato)
```

Le comunicazioni tra interfaccia e modello devono rimanere locali. Lo schema rappresenta l'architettura desiderata, non una prova di isolamento.

---

## 3. Requisiti funzionali

Priorità:

- **MUST:** obbligatorio per il rilascio.
- **SHOULD:** importante, ma non bloccante per il prototipo.
- **COULD:** funzionalità futura.

| ID | Requisito | Priorità |
|---|---|---|
| FR-001 | Tradurre testi dall'italiano all'inglese e viceversa | MUST |
| FR-002 | Tradurre testi dal francese all'inglese e viceversa | MUST |
| FR-003 | Tradurre tra italiano e francese | MUST |
| FR-004 | Consentire l'inserimento manuale del testo | MUST |
| FR-005 | Fornire un'interfaccia grafica accessibile da browser locale | MUST |
| FR-006 | Consentire la selezione della lingua di origine e destinazione | MUST |
| FR-007 | Preservare numeri, unità di misura, simboli e identificatori tecnici | MUST |
| FR-008 | Supportare la traduzione di file TXT e Markdown | MUST |
| FR-009 | Supportare documenti DOCX | SHOULD |
| FR-010 | Supportare PDF con testo selezionabile | SHOULD |
| FR-011 | Supportare un glossario di termini tecnici personalizzato | SHOULD |
| FR-012 | Consentire l'esportazione manuale del testo tradotto | MUST |
| FR-013 | Supportare elaborazione di più documenti | COULD |
| FR-014 | Supportare PDF scansionati tramite OCR locale | COULD |
| FR-015 | Consentire il confronto affiancato tra originale e traduzione | SHOULD |

### 3.1 Accuratezza delle traduzioni

Il sistema deve essere progettato per tradurre documentazione tecnica senza alterare intenzionalmente informazioni o introdurre contenuti non presenti nel testo originale.

Devono essere preservati, salvo trasformazioni esplicitamente richieste, valori numerici, tolleranze, formule, codici identificativi, riferimenti normativi e denominazioni tecniche.

Non è richiesta una traduzione certificata. Le traduzioni destinate a processi ingegneristici critici devono essere sottoposte a revisione umana.

---

## 4. Requisiti di sicurezza

### 4.1 Isolamento dalla rete

| ID | Requisito | Priorità |
|---|---|---|
| SEC-001 | L'inferenza deve funzionare senza Internet | MUST |
| SEC-002 | Nessun testo o documento deve essere trasmesso a servizi esterni durante la traduzione | MUST |
| SEC-003 | I servizi applicativi devono essere accessibili esclusivamente attraverso interfacce locali autorizzate | MUST |
| SEC-004 | Devono essere bloccate le connessioni in uscita non necessarie | MUST |
| SEC-005 | L'interfaccia grafica non deve essere accessibile dalla LAN per impostazione predefinita | MUST |
| SEC-006 | Devono essere disabilitate funzionalità cloud, ricerca web e integrazioni remote | MUST |
| SEC-007 | Il sistema deve poter essere utilizzato con la connessione Internet fisicamente disattivata | MUST |
| SEC-008 | Deve essere possibile verificare il traffico in uscita dall'ambiente applicativo | MUST |
| SEC-009 | La perdita dei controlli di isolamento deve impedire l'elaborazione di dati riservati | MUST |

### 4.2 Monitoraggio

Il sistema deve consentire l'osservazione delle connessioni in uscita tramite strumenti come LuLu/Netiquette e controlli complementari.

Il monitoraggio deve permettere di identificare, dove tecnicamente possibile:

- Processo che genera la connessione.
- Indirizzo IP e porta di destinazione.
- Protocollo utilizzato.
- Stato della connessione.
- Eventuali tentativi di comunicazione non autorizzati.

**Limitazione:** il monitoraggio del processo Docker su macOS può aggregare il traffico di diversi container. Non è sufficiente, da solo, per attribuire ogni connessione a un container o dimostrare l'assenza di comunicazioni esterne.

### 4.3 Protezione dei dati

| ID | Requisito | Priorità |
|---|---|---|
| SEC-010 | Nessun documento deve essere inviato a servizi di telemetria o analytics | MUST |
| SEC-011 | La cronologia delle traduzioni deve essere disabilitabile | MUST |
| SEC-012 | Gli input riservati non devono comparire nei log applicativi ordinari | MUST |
| SEC-013 | I file temporanei devono essere eliminati al termine dell'elaborazione | MUST |
| SEC-014 | Il sistema deve utilizzare lo storage cifrato approvato dall'organizzazione | MUST |
| SEC-015 | Backup e sincronizzazioni cloud non autorizzati devono essere esclusi | MUST |
| SEC-016 | Deve esistere una procedura per eliminare dati persistenti, cronologia e cache applicative | MUST |
| SEC-017 | L'accesso all'interfaccia deve essere protetto contro accessi locali non autorizzati | MUST |

La cancellazione dei dati temporanei non deve essere presentata come garanzia di eliminazione forense. Occorre considerare anche swap, cache, backup e snapshot.

### 4.4 Supply chain e aggiornamenti

**SEC-018 — MUST:** utilizzare soltanto immagini Docker e modelli provenienti da fonti approvate.

**SEC-019 — MUST:** registrare versioni e hash degli artefatti installati.

**SEC-020 — MUST:** impedire download automatici di modelli, plugin o aggiornamenti durante l'elaborazione riservata.

**SEC-021 — MUST:** controllare le vulnerabilità note delle dipendenze prima del rilascio.

**SEC-022 — MUST:** verificare le licenze dei componenti e l'autorizzazione all'utilizzo professionale.

**SEC-023 — SHOULD:** prevedere una procedura per il trasferimento controllato e la verifica degli artefatti da un ambiente di preparazione a uno isolato.

### 4.5 Sicurezza dell'applicazione

I documenti devono essere trattati come contenuti non attendibili. Eventuali istruzioni presenti nei testi da tradurre non devono poter modificare il comportamento dell'applicazione, invocare strumenti, accedere a file estranei o attivare comunicazioni di rete.

**SEC-024 — MUST:** il modello non deve avere accesso ad azioni esterne, strumenti di esecuzione, browsing o file non esplicitamente destinati alla traduzione.

**SEC-025 — MUST:** gli upload devono essere validati per tipo, dimensione e struttura, usando parser locali aggiornati.

**SEC-026 — MUST:** devono essere impiegati privilegi minimi per processi, container, volumi e interfacce API.

---

## 5. Requisiti non funzionali

### 5.1 Compatibilità

| ID | Requisito | Priorità |
|---|---|---|
| NFR-001 | Supportare Apple Silicon M3 | MUST |
| NFR-002 | Sfruttare l'accelerazione Metal per l'inferenza | MUST |
| NFR-003 | Utilizzare Docker per i componenti compatibili | MUST |
| NFR-004 | Non richiedere l'installazione di un servizio di inferenza cloud | MUST |
| NFR-005 | Consentire il funzionamento indipendente da account cloud durante l'esecuzione | MUST |
| NFR-006 | Consentire una futura distribuzione su Linux con GPU NVIDIA | SHOULD |

### 5.2 Prestazioni

**NFR-007 — MUST:** l'inferenza deve utilizzare un modello capace di funzionare nelle risorse effettivamente disponibili sull'host.

**NFR-008 — SHOULD:** la prima configurazione deve utilizzare TranslateGemma 12B in una variante compatibile e adeguatamente quantizzata, previa verifica della memoria disponibile.

**NFR-009 — SHOULD:** durante l'inferenza, l'interfaccia deve mostrare lo stato dell'elaborazione e consentire l'interruzione della richiesta.

**NFR-010 — MUST:** il sistema non deve diventare inutilizzabile quando il modello supera le risorse disponibili; in tale situazione deve interrompere l'operazione in modo controllato.

I valori obiettivo di latenza e throughput saranno definiti dopo un benchmark sul dispositivo di riferimento.

### 5.3 Manutenibilità e riproducibilità

**NFR-011 — MUST:** la configurazione deve essere documentata e riproducibile.

**NFR-012 — MUST:** i componenti Docker devono essere definiti tramite Docker Compose, dove applicabile.

**NFR-013 — MUST:** le configurazioni sensibili devono essere separate dal codice applicativo.

**NFR-014 — SHOULD:** prevedere uno script di verifica dello stato del sistema.

**NFR-015 — SHOULD:** permettere il rollback a una configurazione precedentemente approvata.

---

## 6. Requisiti dell'interfaccia grafica

L'interfaccia deve fornire un'esperienza di traduzione semplice e priva di funzionalità remote non necessarie.

### 6.1 Funzioni principali

- Area di inserimento del testo originale.
- Selezione lingua sorgente e destinazione.
- Pulsante per avviare la traduzione.
- Visualizzazione del testo tradotto.
- Copia del risultato negli appunti.
- Importazione ed esportazione locale dei file supportati.
- Indicazione dello stato del modello.
- Messaggi di errore comprensibili.
- Nessuna condivisione automatica delle traduzioni.

### 6.2 Stato di sicurezza

**UI-001 — SHOULD:** mostrare se il modello locale è disponibile.

**UI-002 — SHOULD:** mostrare lo stato dei controlli di isolamento tramite verifiche tecniche effettive, e non semplicemente tramite la presenza di un'impostazione.

**UI-003 — MUST:** non dichiarare il sistema "sicuro" o "isolato" in assenza di controlli verificati.

**UI-004 — SHOULD:** offrire una funzione di cancellazione esplicita della sessione corrente.

---

## 7. Modello delle minacce

| Minaccia | Impatto | Contromisure principali |
|---|---|---|
| Esfiltrazione via Internet | Critico | Isolamento di rete, firewall, test di blocco |
| Accesso dalla LAN | Alto | Bind locale, restrizioni di rete |
| Telemetria non autorizzata | Alto | Disabilitazione funzionalità, controllo del traffico |
| Compromissione di dipendenze | Critico | Versioni fissate, verifica artefatti, analisi vulnerabilità |
| Accesso a file riservati sul disco | Alto | Cifratura, permessi, isolamento e autorizzazione |
| Cronologia e file temporanei | Alto | Minimizzazione e gestione della conservazione |
| Prompt injection nei documenti | Medio/Alto | Nessun tool esterno, input trattato come dato |
| Traduzione tecnicamente errata | Alto | Glossari, test terminologici e revisione umana |
| Compromissione dell'host | Critico | Controlli endpoint aziendali e isolamento |

### 7.1 Limiti del modello di sicurezza

Il progetto non può garantire la protezione contro un host già compromesso, un amministratore malevolo o una vulnerabilità sconosciuta nel sistema operativo.

Il sistema non è considerato *air-gapped* se possiede ancora interfacce di rete attive che possono raggiungere reti esterne. Un firewall software rappresenta un controllo di isolamento, non un equivalente automatico della separazione fisica.

---

## 8. Verifica e validazione

### 8.1 Security acceptance tests

| Test | Procedura | Risultato atteso |
|---|---|---|
| TEST-001 | Scollegare Internet e tradurre un testo | Traduzione completata |
| TEST-002 | Tentare una connessione HTTPS esterna dal container | Connessione bloccata |
| TEST-003 | Verificare con strumenti indipendenti il traffico di rete | Nessuna trasmissione non autorizzata |
| TEST-004 | Tentare l'accesso all'interfaccia da un secondo dispositivo LAN | Accesso negato |
| TEST-005 | Riavviare il sistema senza Internet | Sistema ancora utilizzabile |
| TEST-006 | Esaminare log e cronologie dopo una traduzione | Nessuna registrazione non autorizzata del contenuto |
| TEST-007 | Verificare processi host, Model Runner e Docker | Interfacce e connessioni conformi alla configurazione approvata |
| TEST-008 | Disattivare un controllo di sicurezza essenziale | Elaborazione riservata impedita |
| TEST-009 | Sottoporre documenti contenenti istruzioni malevole | Nessuna azione esterna o accesso non autorizzato |

I test di rete devono essere eseguiti anche dopo riavvio, aggiornamento o modifica della configurazione. Una singola prova negativa non dimostra l'assenza di tutte le possibili vie di esfiltrazione.

### 8.2 Translation acceptance tests

Preparare un insieme di documenti tecnici di prova, privi di informazioni riservate reali.

Il set deve contenere:

- Terminologia meccanica e industriale.
- Specifiche di produzione.
- Valori numerici e tolleranze dimensionali.
- Sigle, identificatori e riferimenti.
- Tabelle e documenti strutturati.
- Testi in italiano, francese e inglese.

La qualità deve essere valutata tramite confronto con traduzioni di riferimento e revisione umana. I valori soglia saranno definiti prima del collaudo.

### 8.3 Evidenze di verifica

Per ogni test critico devono essere conservati risultato, versione del software, configurazione, data, metodo di prova e responsabile della validazione.

Le evidenze non devono includere contenuti riservati non necessari.

---

## 9. Requisiti operativi

### 9.1 Preparazione iniziale

È consentita una connessione Internet durante una fase separata e controllata per:

1. Scaricare gli installer.
2. Scaricare le immagini Docker.
3. Scaricare i modelli AI.
4. Verificare integrità, provenienza e licenze.
5. Installare gli aggiornamenti di sicurezza approvati.

Terminata la preparazione, l'esecuzione deve essere indipendente da servizi Internet.

### 9.2 Modalità operativa

Prima dell'introduzione di documenti riservati devono essere verificate:

- Conformità del dispositivo alle policy aziendali.
- Assenza di comunicazioni esterne non autorizzate.
- Disponibilità del modello locale.
- Corretta configurazione delle interfacce.
- Protezione dello storage e dei dati temporanei.
- Esito positivo dei test di sicurezza previsti.

### 9.3 Gestione degli incidenti

Deve essere prevista una procedura per interrompere immediatamente il servizio, preservare le evidenze tecniche necessarie, segnalare l'incidente all'organizzazione responsabile e impedire ulteriori elaborazioni fino a nuova autorizzazione.

---

## 10. Criteri di accettazione del progetto

Il primo rilascio potrà essere considerato tecnicamente accettabile quando:

1. TranslateGemma traduce correttamente, secondo i criteri di collaudo stabiliti, nelle tre lingue previste.
2. L'elaborazione è completamente locale.
3. L'accelerazione hardware M3 è operativa.
4. L'interfaccia grafica funziona senza Internet.
5. Non risultano porte esposte non autorizzate.
6. I test di isolamento della rete hanno esito positivo.
7. Nessun contenuto riservato viene conservato o trasmesso al di fuori delle modalità approvate.
8. Le dipendenze e le configurazioni sono documentate e verificabili.
9. Esiste una procedura di gestione e cancellazione dei dati.
10. L'organizzazione proprietaria dei documenti ha autorizzato l'ambiente e l'utilizzo previsto.

L'accettazione tecnica non equivale automaticamente a certificazione di sicurezza o conformità normativa.

---

## 11. Roadmap di implementazione

### Fase 1 — Proof of Concept

Obiettivo: dimostrare la fattibilità tecnica.

- Installazione Docker Model Runner.
- Caricamento TranslateGemma.
- Configurazione Open WebUI.
- Verifica accelerazione Metal.
- Esecuzione di traduzioni di esempio prive di dati riservati.

**Deliverable:** prototipo di traduzione locale funzionante.

### Fase 2 — Security hardening

Obiettivo: ridurre la superficie di attacco.

- Restrizione delle interfacce di rete.
- Disabilitazione delle funzioni cloud.
- Configurazione firewall.
- Monitoraggio con LuLu/Netiquette.
- Protezione dei volumi e gestione della cronologia.
- Verifica delle dipendenze.
- Test di isolamento.

**Deliverable:** prototipo con controlli di sicurezza documentati.

### Fase 3 — Document processing

Obiettivo: introdurre funzioni di gestione dei documenti.

- Importazione dei formati supportati.
- Esportazione locale.
- Gestione dei file temporanei.
- Glossario tecnico.
- Confronto tra originale e traduzione.

**Deliverable:** traduttore documentale utilizzabile.

### Fase 4 — Validation & release

Obiettivo: verificare qualità e sicurezza.

- Benchmark di traduzione.
- Collaudo dei requisiti.
- Revisione della configurazione.
- Preparazione delle procedure operative.
- Autorizzazione organizzativa prima dell'uso di dati riservati.

**Deliverable:** rilascio approvato per uno specifico ambiente e livello di classificazione dei dati.

---

## 12. Decisioni aperte

Prima del rilascio dovranno essere definite:

| ID | Decisione |
|---|---|
| DEC-001 | Quantità di memoria disponibile sul Mac M3 |
| DEC-002 | Versione e quantizzazione definitiva del modello |
| DEC-003 | Livello di isolamento richiesto: firewall software o air gap |
| DEC-004 | Utilizzo di un dispositivo personale o aziendale |
| DEC-005 | Classificazione massima dei dati trattabili |
| DEC-006 | Necessità di conservare cronologie e documenti |
| DEC-007 | Formati documentali obbligatori |
| DEC-008 | Politica per aggiornamenti e manutenzione offline |
| DEC-009 | Eventuali obblighi normativi, contrattuali e di audit |
| DEC-010 | Requisiti quantitativi di accuratezza e prestazioni |

---

## 13. Conclusione

Secure Offline Translator è concepito come un sistema di traduzione AI locale, verificabile e controllabile.

Il suo principio progettuale fondamentale è:

**I documenti vengono elaborati localmente e non devono mai essere trasmessi all'esterno dell'ambiente autorizzato.**

La sicurezza deve derivare da controlli tecnici verificabili, isolamento di rete, gestione controllata dei dati e procedure organizzative, non dalla sola dichiarazione che un'applicazione funzioni offline.

Il sistema dovrà essere considerato idoneo al trattamento di informazioni industriali riservate soltanto dopo il completamento della validazione tecnica e l'approvazione dell'organizzazione responsabile.