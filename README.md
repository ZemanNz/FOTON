# FOTON – Uživatelský manuál & Průvodce spuštěním

Tento dokument slouží jako kompletní návod k obsluze, checklist před startem a popis chování robota **FOTON**. Pro spolehlivý chod a bezpečnost hardwaru dodržujte následující pokyny.

---

## 📋 Checklist před každou jízdou

Před každým spuštěním robota na dráze je nutné provést následující kroky, abyste předešli selhání odometrie nebo poškození hardwaru.

### 1. 🔋 Baterie
* **Požadavek:** Robot musí mít plně nabitou baterii.
* **Detekce napětí:** Program monitoruje napětí baterie (pracovní rozsah 7.0 V až 8.8 V). Pokud napětí klesne pod **7.0 V (7000 mV)**, systém vypíše varování do konzole: `Je třeba nabít baterii!`. 
* **Doporučení:** Před ostrou jízdou vždy změřte napětí nebo zkontrolujte výpis při startu.

### 2. 🔌 Zapojení barevných a gyroskopických senzorů
* **Kontrola kabelů:** Zkontrolujte fyzické připojení I2C kabelů k barevným senzorům **S1 (Velké klepeto)** a **S2 (Malé klepeto)**.
* **Gyroskop:** Ověřte zapojení UART kabelů na pinech RX (16) a TX (17) pro komunikaci s gyroskopem na externím ESP32.
* **Častý problém:** Na RBCX desce může občas selhat inicializace barevného senzoru. Ujistěte se, že jsou kabely pevně usazeny a nedochází k přerušení spojení otřesy.

### 3. ⚓ Závaží
* **Fyzická kontrola:** Ujistěte se, že přídavné závaží na robotu pevně drží na svém místě a nemá vůli.
* **Vliv:** Uvolněné závaží během prudkého zrychlení nebo otáčení změní těžiště robota, což fatálně ovlivní přesnost gyroskopu a odometrie.

### 4. 🔴 Infračervené (IR) senzory
* **Funkčnost:** Zkontrolujte, zda IR senzory na obou ramenech správně reagují na přítomnost předmětu přímo před nimi.
* **Využití v programu:** Senzory se používají k přesné detekci kostky a spuštění úchopové sekvence (`DriveToBricksWithSensors`). Pokud nereagují, robot se může o kostky zaseknout nebo je minout.

### 5. 🧼 Očista kol
* **Nutnost:** Před **každou** jízdou důkladně očistěte gumové běhouny kol od prachu a mastnoty (např. pomocí izolační pásky nebo isopropylalkoholu).
* **Důvod:** Sebemenší prach způsobí prokluz kol při akceleraci, což znehodnotí výpočty ujeté vzdálenosti a natočení.

> [!WARNING]
> ### ⚠️ Pozor na USB připojení!
> Při zapojeném USB kabelu do RBCX desky hrozí **zlomení konektoru nebo kabelu**, jakmile se rameno robota začne pohybovat směrem dozadu nebo nahoru. 
> * Před spuštěním jízdy **vždy odpojte USB kabel**, nebo se ujistěte, že je veden tak, aby do něj rameno nenarazilo!

---

## 🎮 Ovládání robota (Význam tlačítek RBCX)

Po zapnutí robota proběhne inicializace a na sériovém portu se zobrazí nabídka. Robot čeká na stisk jednoho z tlačítek na RBCX desce:

| Tlačítko | Akce | Popis |
| :--- | :--- | :--- |
| **ON** | **Konzervativní jízda** | Spustí hlavní program v opatrnějším režimu s nižším zrychlením a rychlostí (`konzervativni_jizda()`). |
| **UP** | **Agresivní jízda** | Spustí rychlý soutěžní program s vyšším zrychlením a rychlostními profily (`agresivni_jizda()`). |
| **RIGHT**| **Zkouška klepet** | Spustí test serv: zavře malé i velké klepeto, počká 1 sekundu, pak obě otevře a ukončí test. |
| **LEFT** | **Čtení barev** | Spustí nepřetržitý výpis hodnot z barevných senzorů S1 a S2 (naměřené R, G, B, Clear a detekovaná barva). Pro návrat do menu je nutný reset robota. |
| **OFF**  | **Otočení o 90°** | Otočí robota na místě o 90 stupňů doleva (vhodné pro rychlou kalibraci nebo test směrování). |
| **DOWN** | **Výpis logů / STOP** | **V menu:** Vypíše uložené logy do konzole.<br>**Během jízdy:** Funguje jako **EMERGENCY STOP** (viz níže). |

---

## 📝 Logování a Nouzové zastavení (Emergency Stop)

### Jak funguje výpis logů
V programu je implementována funkce `logMsg(const char* format, ...)`. 
1. **Odeslání na Serial:** Každá zpráva je okamžitě odeslána na sériovou linku (pokud je připojen počítač).
2. **Uložení do bufferu:** Zpráva se zároveň ukládá do interní paměťové proměnné `logBuffer`.
3. **Výpis bez PC:** Pokud robot jezdí odpojený na baterii, ukládá si historii celé jízdy. Po dojezdu stačí připojit USB, stisknout tlačítko **DOWN** a robot zpětně vypíše celou historii jízdy z `logBuffer` na obrazovku.
4. **Mazání:** `logBuffer` se automaticky vymaže při startu nové jízdy (stiskem ON nebo UP).

### Emergency Stop (Nouzové zastavení)
Během jakéhokoliv pohybu nebo prodlevy (funkce `delay` je přesměrována na `safeDelay`) program neustále kontroluje stisk tlačítka **DOWN**.

Pokud je tlačítko **DOWN** stisknuto:
1. Okamžitě se zastaví všechny motory (`M1` až `M4` na rychlost 0).
2. Do sériové linky se okamžitě vypíše celý obsah `logBuffer` (kompletní historie událostí před zastavením).
3. Robot přejde do bezpečné nekonečné smyčky a zabrání jakémukoli dalšímu pohybu.
4. Opětovným stisknutím tlačítka **DOWN** v tomto stavu můžete logy vypsat znovu. Pro nový start je nutné robota restartovat tlačítkem RST na desce.
