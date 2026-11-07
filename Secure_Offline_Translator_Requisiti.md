# Secure Offline Translator

**Documento dei requisiti software (SRS)**  
**Versione:** 1.1 — bozza di progetto  
**Data:** 7 novembre 2026  
**Stato:** proposta per implementazione e verifica  
**Piattaforma iniziale:** macOS su Apple Silicon (MacBook Air M3)

---

## 1. Scopo e contesto

### 1.1 Obiettivo

Sviluppare un'applicazione desktop per la **traduzione di testi riservati interamente in locale**, con interfaccia grafica Qt, motore d'inferenza `llama.cpp` e controlli tecnici verificabili che impediscano al processo applicativo di stabilire connessioni di rete.

L'applicazione dovrà essere semplice da installare e utilizzare, minimizzando dipendenze, servizi, processi ausiliari e funzioni non necessarie.

### 1.2 Motivazione

Il traduttore può essere destinato a contenuti soggetti a riservatezza professionale o segreto industriale. L'assenza di trasmissione intenzionale dei dati è necessaria, ma **non sufficiente**: occorrono restrizioni imposte da macOS, controlli sulla catena di fornitura e prove ripetibili.

L'uso di dati aziendali resta subordinato all'**autorizzazione del titolare dei dati** e alle sue politiche di sicurezza. Questo software non è, da solo, una certificazione di conformità né una garanzia assoluta contro l'esfiltrazione.

### 1.3 Perimetro

- **Incluso:** applicazione autonoma `SecureTranslator.app`, GUI Qt Widgets, traduzione testuale, modello GGUF locale, inferenza C/C++ con Metal, sandbox macOS, verifiche di sicurezza, gestione esplicita dei file, test.
- **Escluso:** Ollama, Qwen, agenti con ricerca web, Python, Docker, server HTTP, login, account, telemetria, aggiornamenti o download automatici, accesso cloud.
- **Release iniziale (MVP):** testo copiato/incollato e file `.txt` UTF-8. Supporto a PDF, DOCX, OCR e conservazione dell'impaginazione **non incluso** nell'MVP.

## 2. Principi architetturali

1. **Offline by design:** nessuna funzionalità richiede Internet in fase di utilizzo.
2. **Fail closed:** se i controlli obbligatori di isolamento non sono soddisfatti, l'app non apre documenti riservati e non avvia l'inferenza.
3. **Least privilege:** ogni risorsa — rete, file, processi e servizi di sistema — è accessibile soltanto quando strettamente necessaria.
4. **Single-model:** un solo modello approvato e verificato per sessione, senza cambio automatico.
5. **No content retention:** nessuna memorizzazione automatica di testo originale, traduzione o prompt.
6. **Defense in depth:** sandbox macOS come controllo primario; LuLu facoltativo come controllo indipendente aggiuntivo.
7. **Auditability:** versione applicativa, identità del modello e configurazione di sicurezza devono essere verificabili senza registrare il contenuto tradotto.

## 3. Utenti e casi d'uso

**Utente principale:** persona che deve tradurre brevi testi tecnici e documenti testuali potenzialmente riservati su un Mac personale autorizzato o su un computer aziendale.

**Casi d'uso prioritari**

- **UC-01 — Traduzione interattiva:** incollare testo, scegliere lingua di origine e destinazione, tradurre e copiare il risultato.
- **UC-02 — Traduzione di file:** selezionare un file `.txt`, generare una traduzione e scegliere esplicitamente dove salvarla.
- **UC-03 — Verifica ambiente:** consultare identificativo del modello, hash, stato di sandbox e stato dell'accelerazione Metal.
- **UC-04 — Errore sicuro:** in caso di modello non valido, permesso mancante o sandbox non applicata, impedire la traduzione e mostrare un errore senza divulgare il contenuto.
- **UC-05 — Monitoraggio avanzamento:** durante la traduzione mostrare una barra di avanzamento e uno stato leggibile, senza bloccare l’interfaccia.

## 4. Requisiti funzionali

Priorità: **M** = obbligatorio per MVP, **S** = desiderabile per versione successiva, **C** = opzionale.

| ID | Priorità | Requisito | Criterio di accettazione sintetico |
|---|:---:|---|---|
| FR-01 | M | GUI desktop nativa con **Qt 6 Widgets**. | Avvio come `.app` senza browser o server locale. |
| FR-02 | M | Due aree testuali distinte: originale e traduzione. | Il risultato non sovrascrive l'originale. |
| FR-03 | M | Selettori per lingua di origine e destinazione. | Le lingue disponibili dipendono dal modello approvato; l'utente vede la selezione effettiva. |
| FR-04 | M | Comando **Traduci**. | Il risultato appare nell'area di output; errori gestiti. |
| FR-05 | M | Pulsante **Annulla** per interrompere la generazione. | L'app resta reattiva e interrompe la generazione appena tecnicamente possibile. |
| FR-06 | M | Elaborazione su thread di lavoro, distinta dal thread GUI. | La finestra rimane utilizzabile durante l'inferenza. |
| FR-07 | M | Copia negli appunti **solo su azione esplicita**. | Nessuna copia automatica nel clipboard. |
| FR-08 | M | Apertura di file `.txt` UTF-8 tramite dialogo di selezione. | L'accesso è limitato al file scelto dall'utente. |
| FR-09 | M | Salvataggio della traduzione tramite dialogo dedicato. | Nessun salvataggio non richiesto e nessuna sovrascrittura senza conferma. |
| FR-10 | M | Visualizzazione di nome, percorso logico, hash SHA-256 e versione del modello. | Identità del modello disponibile prima della traduzione. |
| FR-11 | M | Messaggi di errore leggibili senza includere il testo riservato nei log. | I test non rilevano contenuti nei log applicativi. |
| FR-12 | S | Traduzione di grandi testi mediante segmentazione controllata. | Ordine preservato; limiti di contesto gestiti; avvisi su possibili incoerenze. |
| FR-13 | S | Modalità CLI opzionale che usa lo stesso motore e le stesse restrizioni. | Nessun servizio o processo aggiuntivo richiesto. |
| FR-14 | C | Glossario locale tecnico definito dall'utente. | Funziona offline e non è condiviso con altri processi. |
| FR-15 | M | **Barra di avanzamento Qt (`QProgressBar`)** durante la traduzione, con indicazione dello stato e possibilità di annullamento. | Compare all'avvio, si aggiorna senza bloccare la GUI, scompare o si conclude correttamente dopo completamento, errore o annullamento. |

### 4.1 Avanzamento della traduzione

- La finestra mostra una **barra di avanzamento (`QProgressBar`)** durante ogni traduzione, insieme a una descrizione dello stato: `Caricamento modello`, `Preparazione input`, `Traduzione`, `Completata`, `Annullata` o `Errore`.
- **Testo breve / singola generazione:** se il motore non fornisce una stima attendibile della quantità totale di token in output, la barra deve essere **indeterminata** (modalità busy). È vietato mostrare una percentuale fittizia basata soltanto sul tempo trascorso o sui token generati rispetto a un limite massimo teorico.
- **File o testo segmentato (quando FR-12 sarà implementato):** la barra può essere **determinata (0–100%)** in base a segmenti completati / segmenti totali, indicando esplicitamente che si tratta dell'avanzamento *per segmenti*, non di una previsione esatta del tempo rimanente. Il segmento in elaborazione deve essere indicato come tale.
- Eventualmente mostrare **token generati**, velocità di generazione e tempo trascorso come informazioni accessorie; non presentarli come percentuale di completamento.
- Aggiornare `QProgressBar` e l'etichetta di stato **solo nel thread GUI**, tramite segnali e slot dal worker; limitare la frequenza degli aggiornamenti per evitare overhead e rallentamenti.
- Il pulsante `Annulla` deve rimanere accessibile durante l'elaborazione. In caso di annullamento o errore, fermare la barra, riportare uno stato appropriato e non far apparire una traduzione parziale come completata.
- Quando l'elaborazione termina correttamente, visualizzare temporaneamente `100%` solo se esiste una misura determinata, oppure lo stato `Completata` senza inventare una percentuale.

### 4.2 Comportamento linguistico

- L'applicazione formula un'istruzione deterministica di traduzione, privilegiando **fedeltà semantica, terminologia tecnica, unità, numeri, simboli e struttura**.
- Deve minimizzare aggiunte, omissioni e parafrasi non richieste.
- I risultati dei modelli generativi **non devono essere presentati come garantiti**: il sistema avvisa che una revisione umana è necessaria prima di utilizzare traduzioni in documenti contrattuali, tecnici o di sicurezza critica.
- La selezione del modello verrà confermata solo dopo test su un campione rappresentativo; **nessun GGUF specifico è approvato in questa versione del documento**.

## 5. Requisiti di sicurezza

### 5.1 Modello di minaccia

**Beni protetti:** testo originale, traduzione, file selezionati, prompt e contesto, metadati dei documenti, configurazione del modello.

**Avversari e guasti considerati:** bug nell'app, librerie vulnerabili, modello o file GGUF alterati, configurazioni permissive, connessioni accidentali, tentativi di avviare strumenti esterni, salvataggi automatici non intenzionali.

**Fuori dalla garanzia di isolamento dell'app:** malware già presente con privilegi superiori, kernel compromesso, amministratore ostile, cattura schermo, clipboard letto da altre applicazioni, backup esterni, sincronizzazioni del sistema operativo, swap o copie in memoria. Questi aspetti richiedono controlli del dispositivo e organizzativi.

### 5.2 Requisiti verificabili

| ID | Priorità | Requisito di sicurezza | Verifica richiesta |
|---|:---:|---|---|
| SEC-01 | M | **App Sandbox attiva** per la build destinata a dati riservati. | Verifica firma ed entitlements del bundle distribuito, non soltanto del sorgente. |
| SEC-02 | M | **Nessun entitlement di rete client o server** (`com.apple.security.network.client`, `.server`). | Ispezione entitlements e prove negative su TCP/UDP, verso IP esterni e loopback. |
| SEC-03 | M | Nessuna chiamata intenzionale a API di rete nel prodotto; non usare Qt Network/WebEngine. | Revisione dipendenze, simboli, configurazione e test dinamici. |
| SEC-04 | M | Nessun server HTTP o API in ascolto. | Verifica socket/listener mentre l'app è in esecuzione. |
| SEC-05 | M | Nessun download di modelli, tokenizer o aggiornamenti durante l'esecuzione. | Prove con rete assente, monitoraggio firewall, analisi del codice. |
| SEC-06 | M | Il modello deve essere presente localmente e appartenere a una **allowlist di hash SHA-256 approvati**. | Modello alterato di un byte → caricamento rifiutato. |
| SEC-07 | M | Vietare l'avvio intenzionale di programmi esterni: niente `QProcess`, `system`, `popen` o analoghi. | Revisione sorgenti; test contro process spawn dove supportato. |
| SEC-08 | M | **Separare il requisito di non-spawn dall'effettiva garanzia OS.** Non assumere che App Sandbox neghi automaticamente tutti i `fork/exec`. | Verificare una policy OS aggiuntiva praticabile, oppure documentare esplicitamente il limite e applicare altre mitigazioni prima dell'uso con segreti industriali. |
| SEC-09 | M | Accesso ai file soltanto dove consentito dalla sandbox e previa selezione dell'utente. | Tentativo di leggere un documento non selezionato → negato. |
| SEC-10 | M | Nessuna telemetria, analytics, crash reporting remoto o verifica aggiornamenti. | Ispezione dipendenze e traffico durante test prolungati. |
| SEC-11 | M | Non registrare prompt, testo, traduzioni o percorsi sensibili nei log ordinari. | Ricerca di stringhe sentinella nei log e negli artefatti applicativi. |
| SEC-12 | M | Errore in configurazione di sicurezza → **nessuna elaborazione**. | Build senza entitlements richiesti → avvio della modalità riservata rifiutato. |
| SEC-13 | S | Regola LuLu permanente **deny outbound** sull'app come difesa indipendente. | Test di connessione negata e osservabilità in LuLu. |
| SEC-14 | S | Firma del codice, verifica d'integrità del bundle, catena di build riproducibile per quanto possibile. | Artifacts e dipendenze tracciati, firma verificata. |
| SEC-15 | S | Valutazione di vulnerabilità delle dipendenze e audit delle licenze prima di ogni rilascio. | Checklist di release completata. |

**Nota tecnica essenziale:** non includere gli entitlement di rete in un'app con App Sandbox limita l'apertura di connessioni da parte dell'app sandboxed, ma non equivale alla prova che *tutti* i possibili canali di esfiltrazione tramite servizi esterni siano impossibili. I controlli IPC e i confini di fiducia devono essere analizzati separatamente.

### 5.3 Strategia sandbox

**Soluzione prevista per la release:** bundle macOS firmato con **App Sandbox** e entitlement minimali. Consentire l'accesso a file selezionati dall'utente con il meccanismo di sistema appropriato. Valutare e testare l'effettiva compatibilità di Qt, Metal e backend llama.cpp in questa configurazione.

- **Non** usare `sandbox-exec` deprecato come unico meccanismo di protezione della versione destinata a documenti industriali.
- `sandbox-exec` può essere usato soltanto in prototipi o test locali, con chiara indicazione dei limiti.
- Le restrizioni su creazione processi e comunicazioni IPC richiedono un piano tecnico separato e verifica empirica; non sono considerate automaticamente soddisfatte dal solo entitlement App Sandbox.
- Le prove dinamiche non devono richiedere privilegi elevati all'applicazione né disabilitare temporaneamente la protezione.

### 5.4 Sicurezza dei dati in memoria e su disco

- Non creare file temporanei contenenti documenti o traduzioni, salvo necessità esplicita e autorizzata.
- Non eseguire autosalvataggio, indicizzazione, sincronizzazione o cronologia.
- Evitare `qDebug()` e messaggi di errore che includano testo riservato.
- Cancellare i buffer applicativi quando non più necessari **best effort**, senza promettere cancellazione certa dalla RAM, dalla memoria GPU o dallo swap.
- Riconoscere che il **clipboard** è condiviso con altre applicazioni: mostrarne il rischio all'utente e usarlo solo su richiesta.
- I file esportati restano sotto la responsabilità dell'utente e delle policy del dispositivo (es. cartelle sincronizzate/backup).

## 6. Architettura tecnica

### 6.1 Stack

| Livello | Scelta |
|---|---|
| Linguaggio | C++20 |
| Interfaccia | Qt 6 Widgets (senza Qt WebEngine) |
| Build | CMake |
| Motore di inferenza | `llama.cpp` integrato come libreria (`libllama`/GGML) |
| Accelerazione | Metal su Apple Silicon |
| Modello | GGUF locale, versione e hash fissati |
| Piattaforma | macOS ARM64; primo target MacBook Air M3 |
| Packaging | Bundle `SecureTranslator.app`, firmato e sandboxed |
| Firewall aggiuntivo | LuLu (opzionale, non dipendenza runtime) |

### 6.2 Componenti logici

```text
SecureTranslator.app
└── Processo applicativo (C++20)
    ├── MainWindow / Qt Widgets           [thread GUI]
    ├── TranslationController              [coordinamento]
    ├── TranslationWorker                  [thread di inferenza]
    │   └── llama.cpp / GGML / Metal       [modello GGUF locale]
    ├── ModelVerifier                      [hash, formato, allowlist]
    ├── FileAccess                          [open/save espliciti]
    └── SecurityStatus                      [verifiche e diagnostica]

Controlli esterni:
  macOS App Sandbox + code signing
  LuLu [eventuale difesa aggiuntiva]
```

**Aspettativa di processo:** l'app non avvia intenzionalmente subprocessi; Qt e il backend possono generare thread. Servizi macOS già in esecuzione possono interagire con l'app, quindi «un processo» descrive l'architettura applicativa, **non** l'intero insieme di processi del sistema operativo coinvolti.

### 6.3 Threading

Il lavoro d'inferenza avviene in un worker spostato in `QThread`, con comunicazione tramite segnali/slot Qt e gestione ordinata della cancellazione. Oggetti GUI sono modificati esclusivamente dal thread principale. Il modello non deve essere usato contemporaneamente da due richieste in assenza di sincronizzazione provata.

### 6.4 Gestione del modello

1. Il download del modello avviene **fuori dall'applicazione**, in una fase amministrativa separata.
2. Il file GGUF è ispezionato, la licenza verificata, l'origine documentata e l'hash approvato.
3. Il file è installato in posizione accessibile alla sandbox, con permessi appropriati.
4. L'app calcola/verifica SHA-256 **prima** di caricare il modello.
5. L'app blocca modelli sconosciuti e non tenta fallback automatici verso altri modelli o servizi.

## 7. Requisiti non funzionali

| ID | Priorità | Requisito | Misura / decisione |
|---|:---:|---|---|
| NFR-01 | M | Utilizzabilità senza Internet. | Test completo con Wi-Fi e rete disattivati. |
| NFR-02 | M | Reattività della GUI. | Nessuna inferenza bloccante nel thread GUI. |
| NFR-03 | M | Compatibilità Apple Silicon / Metal. | Build e test reali sul MacBook Air M3. |
| NFR-04 | M | Gestione memoria prevedibile. | Dimensione contesto e modello configurati secondo RAM disponibile; errori non fatali. |
| NFR-05 | M | Riproducibilità. | Versioni Qt, llama.cpp, compiler e modello fissate per release. |
| NFR-06 | M | Portabilità del codice. | CMake e core separati dalla GUI; macOS è l'unico target garantito iniziale. |
| NFR-07 | S | Tempi di risposta e accuratezza misurati. | Benchmark su testi campione dopo scelta del modello; nessuna soglia numerica inventata ora. |
| NFR-08 | S | Accessibilità minima. | Scorciatoie tastiera, focus visibile, supporto copia/incolla e dimensione testo. |
| NFR-09 | M | Indicatore di avanzamento reattivo e comprensibile. | Stato aggiornato tramite segnali Qt, senza rallentamenti evidenti della GUI; percentuali mostrate solo se giustificate. |

### 7.1 Metriche di qualità della traduzione

La scelta del modello dovrà considerare: fedeltà, terminologia tecnica IT↔FR e EN↔FR/IT, preservazione di numeri/unità/codici, gestione di acronimi, omissioni o allucinazioni, velocità e memoria impiegata. Le coppie di lingue effettivamente supportate saranno dichiarate **dopo** benchmark; nessuna prestazione viene garantita prima della valutazione.

## 8. Interfaccia utente prevista

**Finestra principale**

- Barra superiore: nome applicazione, modello attivo e informazioni sull'isolamento (**stato fondato su verifiche, non un semplice indicatore decorativo**).
- Lingue di origine e destinazione (selettori).
- Riquadro del testo sorgente e riquadro della traduzione.
- Pulsanti `Apri .txt`, `Traduci`, `Annulla`, `Copia`, `Salva .txt`.
- **Barra di avanzamento `QProgressBar`** sotto i campi di testo, etichetta di stato, indicazione opzionale del segmento corrente e pulsante `Annulla` sempre raggiungibile durante la traduzione.
- Messaggi di errore non sensibili.
- Pannello `Informazioni e sicurezza`: versione, hash modello, firma app, policy attesa e risultati dei controlli.

**Principio UX:** non mostrare la dicitura «rete verificata bloccata» se l'applicazione non dispone di un'evidenza valida per quella specifica build/esecuzione. Distinguere fra `Configurazione prevista`, `Controllo superato`, `Controllo non eseguito`, `Errore`.

## 9. Piano di verifica e criteri di accettazione

### 9.1 Test funzionali

- **T-F01:** testo breve IT→FR produce una traduzione e non blocca la finestra.
- **T-F02:** cambia la lingua e verifica l'aggiornamento del prompt di traduzione.
- **T-F03:** annulla una generazione lunga senza crash né risposta parziale spacciata per completa.
- **T-F04:** apre `.txt` e salva la traduzione solo dopo scelta esplicita della destinazione.
- **T-F05:** modello assente, GGUF corrotto e SHA-256 non autorizzato sono rifiutati con messaggi sicuri.
- **T-F06 — Progress bar:** all’avvio di una traduzione la barra compare e la GUI rimane reattiva; con output di lunghezza ignota è indeterminata, senza percentuali arbitrarie.
- **T-F07 — Segmentazione:** quando disponibile, l’avanzamento per segmenti cresce monotonamente e raggiunge il 100% solo dopo il completamento di tutti i segmenti.
- **T-F08 — Interruzione:** annullamento ed errore fermano immediatamente l’indicatore; lo stato non indica mai falsamente `Completata`.

### 9.2 Test di sicurezza obbligatori

- **T-S01 — Bundle:** verificare firma, App Sandbox ed entitlement effettivi del `.app` rilasciato; assicurarsi che non siano presenti permessi `network.client` o `network.server`.
- **T-S02 — Connessioni:** eseguire prove negative da codice applicativo di test verso indirizzi TCP/UDP esterni e verso `127.0.0.1`/`::1`, con monitoraggio indipendente; distinguere un blocco di permesso da un errore DNS/timeout.
- **T-S03 — Listener:** osservare che l'app non apre server locali.
- **T-S04 — Avvio processi:** provare esplicitamente `fork/exec` nel medesimo contesto di esecuzione e documentare risultato; **se il vincolo OS non è imposto, SEC-08 non è superato**.
- **T-S05 — File:** tentare la lettura di un file non selezionato e non concesso dalla sandbox; accesso negato.
- **T-S06 — Dati nei log:** inserire una stringa sentinella nell'input, tradurre e cercarla in log, file temporanei e artefatti creati dall'app.
- **T-S07 — Operazioni offline:** eseguire la suite con rete fisicamente disconnessa; nessun download o servizio esterno richiesto.
- **T-S08 — Integrità:** alterare un byte del modello; blocco immediato prima dell'inferenza.
- **T-S09 — Compromissione controllata:** verificare che errori di inizializzazione sandbox e permessi mancanti non permettano un fallback a modalità non isolata.
- **T-S10 — LuLu (se utilizzato):** verificare una regola persistente sull'eseguibile firmato e tentativi di connessione osservabili senza aprire la rete.

**Criterio di rilascio:** tutti i test `M` devono passare. La sola prova di assenza di traffico osservato non è sufficiente a dimostrare il blocco delle connessioni: occorrono policy e test negativi.

## 10. Fasi di realizzazione

| Fase | Obiettivo | Deliverable |
|---|---|---|
| P0 — Spike tecnico | Provare Qt Widgets + llama.cpp + Metal dentro il sandbox target. | Prototipo minimo e verbale di compatibilità. |
| P1 — MVP | GUI, inferenza, `.txt`, validazione modello, cancel, **barra di avanzamento**. | Applicazione funzionante offline con stato della traduzione visibile. |
| P2 — Hardening | Firma, entitlements minimali, restrizioni OS, test di rete/processi/file, audit log senza contenuto. | Build di sicurezza verificata. |
| P3 — Validazione | Benchmark linguistici, test memoria, regressione e documentazione. | Prima release candidata per uso autorizzato. |
| P4 — Evoluzione | Glossari, grandi documenti, formati strutturati se approvati. | Requisiti separati e nuova valutazione del rischio. |

## 11. Questioni aperte / decisioni da prendere

1. **Modello GGUF approvato:** famiglia, versione, licenza e accuratezza per le lingue desiderate.
2. **RAM del Mac M3:** scelta di quantizzazione, dimensione del contesto e massimo input ammissibile.
3. **Distribuzione:** uso personale vs aziendale; identità di firma e notarizzazione, se necessaria.
4. **Blocco process spawn:** meccanismo OS realmente applicabile in combinazione con App Sandbox, Qt e Metal; serve prova sperimentale prima di dichiarare il controllo soddisfatto.
5. **Percorso dei modelli:** risorsa nel bundle o posizione amministrata consentita dalla sandbox; eventuali aggiornamenti sempre offline e verificati.
6. **Ambito documenti:** l'MVP supporta solo testo e `.txt`; ogni altro formato richiede parser e analisi di rischio specifici.
7. **Requisiti aziendali:** gestione MDM, disco cifrato, backup, clipboard, crash dump, protezione dello schermo e autorizzazioni all'uso.

## 12. Definition of Done dell'MVP

L'MVP è completato soltanto se:

- [ ] `SecureTranslator.app` si avvia su macOS ARM64 con GUI Qt Widgets.
- [ ] Carica esclusivamente un modello GGUF locale presente nell'allowlist SHA-256.
- [ ] Esegue traduzione su Metal in un worker thread senza server HTTP.
- [ ] Supporta input testuale, `.txt`, annullamento, copia e salvataggio espliciti.
- [ ] Mostra una barra di avanzamento Qt reattiva, senza percentuali inventate, con stati corretti per completamento, errore e annullamento.
- [ ] Non effettua download, non integra telemetria e non conserva automaticamente contenuti.
- [ ] La build firmata è distribuita con App Sandbox ed entitlements minimi verificati.
- [ ] I test negativi di rete, file e integrità passano.
- [ ] Il requisito di divieto dei processi figli è **dimostrato** oppure esplicitamente **non soddisfatto** (e in tal caso non si dichiara il prodotto pronto per dati industriali riservati).
- [ ] Il documento di test riporta limiti residui e configurazione effettivamente validata.

## 13. Riferimenti tecnici

- Apple — [App Sandbox](https://developer.apple.com/documentation/security/app-sandbox)
- Apple — [Entitlement: connessioni in uscita](https://developer.apple.com/documentation/BundleResources/Entitlements/com.apple.security.network.client)
- Apple — [File selezionati dall'utente](https://developer.apple.com/documentation/BundleResources/Entitlements/com.apple.security.files.user-selected.read-write)
- Qt — [Qt for macOS: deployment](https://doc.qt.io/qt-6/macos-deployment.html)
- Qt — [QThread](https://doc.qt.io/qt-6/qthread.html)
- llama.cpp — [Repository ufficiale](https://github.com/ggml-org/llama.cpp)
- llama.cpp — [Build e Metal](https://github.com/ggml-org/llama.cpp/blob/master/docs/build.md)

---

*Documento di requisiti — non costituisce certificazione di sicurezza. Le garanzie effettive dipendono da implementazione, configurazione del sistema operativo, test e contesto operativo.*
