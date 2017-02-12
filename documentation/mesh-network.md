<meta charset="utf-8" />
<link rel="stylesheet" href="style.css">
<script>document.isPrototype = true</script>

<heading>MESH Networking</heading>
<h1 id="toc-header">Inhaltsverzeichnis</h1>
<ol id="toc"></ol>

# Netztopologie
Jeder Knoten im Netz besitzt mindestens einen Port, über den Daten gesendet
und empfangen werden können. Jeder Port wird mit entweder keinem oder aber
exakt einem weiteren Port verbunden.

Um nun ein Netzwerk zu bilden, werden einzelne Netzwerkknoten über die Ports
verbunden. Hierbei ist wichtig, dass das Netz eine Baumstruktur bildet und
keine Schleifen erzeugt.

## Nachrichtenversand
Um nun eine Nachricht von einem Knoten zu einem beliebigen anderen Knoten zu
versenden, schickt der sendende Knoten die Nachricht auf allen seinen Ports
ins Netzwerk. Jeder Knoten, der nun eine Nachricht empfängt, prüft, ob er 
diese verarbeiten oder weiterreichen (propagieren) soll.

Wenn die Nachricht propagiert werden soll, schickt der Knoten die Nachricht auf
all seinen Ports (ausgenommen dem Port, auf dem die Nachricht empfangen wurde),
weiter. Hierdurch wird sichergestellt, dass das gesamte Netz erreichbar ist.

## Knoten-Konfigurationen
Knoten können eine beliebige Anzahl an Ports besitzen, benötigen aber mindestens
einen Port. Die Anzahl der Ports bestimmt nun die Klassifizierung eines Knotens
in eine der folgenden Kategorien.

### Endknoten
Ein Endknoten besitzt nur einen einzigen Port und erzeugt hiermit das Ende eines
Netzwerkastes.

![End Node](img/endnode.png)

### Serieller Knoten
Ein serieller Knoten besitzt zwei Ports und kann in Reihe mit weiteren Knoten
geschaltet werden.

![Serial Node](img/serialnode.png)

### Hub-Knoten
Ein Knoten mit mehr als zwei Ports wird als Hub-Knoten bezeichnet. Diese Knoten
bilden aufwändigere Teile eines Netzwerkes und können als Verzweigungen benutzt
werden.

![Hub Node](img/hubnode.png)

### Switch-Knoten
Ein Switch-Knoten ist ein *intelligenter* Hub-Knoten, welcher alle mit ihm
verbundenen Addressen kennt und somit die Weitergabe von Nachrichten effizienter
gestalten kann. Zudem erlauben Switch-Knoten Ringe in einem Netzwerk, da sie
eine Nachricht max. in eine Richtung propagieren.

# Knoteninformationen

## Register
Jeder Knoten besitzt ein Set an Registern, über das das Verhalten des Knotens
gesteuert werden kann. Ein Knoten kann maximal 256 Register besitzen, wobei
alle Register mit der Register-Nummer kleiner 128 8-Bit-Register sind und alle
Register oberhalb 16 Bit breit.

| Register |  Größe |
|----------|--------|
|   `0x00` |  8 Bit |
|   `0x7F` |  8 Bit |
|   `0x80` | 16 Bit |
|   `0xFF` | 16 Bit |

Die Bedeutung der einzelnen Register werden zum Teil über die Knotenklasse
festgelegt. Alle anderen Register stehen dem Gerät zur freien Verfügung.

Register können schreibgeschützt sein. Wenn auf ein schreibgeschütztes Register
geschrieben wird, soll der vorherige Wert des Registers erhalten bleiben und
eine Fehlermeldung versandt werden.

## Knotenklasse
Die Knotenklasse bestimmt die grundlegende Funktionalität eines Knotens und
schreibt zudem noch einige, vordefinierte Verhaltensmuster vor.

| Knotenklasse | Name         | Beschreibung                                   |
|--------------|--------------|------------------------------------------------|
|       `0x00` | Network Node | Netzwerkknoten, dient der Infrastruktur.       |
|       `0x10` | Sensor       | Passiver Knoten, stellt Messwerte bereit.      |
|       `0x20` | Actor        | Aktiver Knoten, schaltet.                      |
|       `0x30` | Control Unit | Regelnder Knoten, steuert Knotenverhalten.     |
|       `0x40` | Memory       | Speichernder Knoten, dient zur Archivierung.   |

### Network Node

Ein Netzwerk-Knoten ist ein Infrastruktur-Knoten wie beispielsweise ein Hub oder
ein Switch. Diese Knoten dienen nicht der Steuerung des Netzes sondern sorgen
für dessen Funktionalität und Stabilität.

### Sensor
Ein Sensor ist ein passiver Netzwerkteilnehmer, welcher Messwerte erfasst und 
dieses bereitstellt. Die Messwerte sind einheitenbehaftet und mit einem
Schwellwert versehen. Dieser Schwellwert löst eine Notify-Benachrichtigung aus,
wenn der Messwert mehr als die angegebene Schwelle vom letzten Messwert
abweicht.

Jeder Sensor bietet mindestens zwei Register an:

| Register | Read-Only | Beschreibung                                          |
|----------|-----------|-------------------------------------------------------|
|   `0x00` | Ja        | Einheit des Messwertes                                |
|   `0x80` | Nein      | Aktuellester Messwert in der angegebenen Einheit.     |
|   `0x81` | Nein      | Schwellwert für Notify-Nachrichten                    |

Wenn ein Sensor mehr als einen Messwert anbietet, wird dieser in weiteren,
geräteabhängigen Registern angegeben und es wird ein Treiber benötigt.

### Actor
Ein Aktor ist ein aktiver Knoten, welcher eine steuernde Funktion hat. Hierbei
kann es sich beispielsweise um ein Relais, eine Heizungssteuerung oder auch eine
Lampe handeln.

Jeder Aktor bietet mindestens vier Register an:

| Register | Read-Only | Beschreibung                                          |
|----------|-----------|-------------------------------------------------------|
|   `0x00` | Ja        | Einheit des Steuerwerts                               |
|   `0x80` | Nein      | Steuerwert                                            |
|   `0x81` | Ja        | Minimum des Steuerwerts                               |
|   `0x82` | Ja        | Maximum des Steuerwerts                               |

Hierbei wird der Steuerwert zwischen einschließlich das Minimum und
einschließlich das Maximum begrenzt.

Für Aktoren, welche nur eine binäre Operation (An/Aus) erlauben, sind das
Minimum `0,00` und das Maximum `1,00`, wobei das System aktiv geschaltet ist,
wenn der Steuerwert `≥0,50` ist.

### Control Unit
Eine Steuereinheit hat den Zweck, Teile des Netzwerkes zu steuern und zu
regeln. Zudem kann sie eine Interaktionsmöglichkeit mit dem Benutzer zur
Verfügung stellen.

Eine *Control Unit* bietet mindestens ein Register an:

| Register | Read-Only | Beschreibung                                          |
|----------|-----------|-------------------------------------------------------|
|   `0x00` | Ja        | Aktuelle Zustands-Nummer der Control Unit             |

Die *Zustands-Nummer* gibt an, in welchem Zustand sich die Control Unit aktuell
befindet. Ein Zustand ist eine Sammlung an Regeln, nach welchen die Control Unit
arbeitet. Hiermit können versch. Tageszustände oder auch Netzmodi implementiert
werden.

Um die Zustands-Nummer einer Control Unit zu ändern, muss eine Notify-Nachricht
verschickt werden, welche ein *Control Signal* verschickt.

Es gibt vier grobe Klassifizierungen von Control Units, welche einen bestimmte
Gruppe an Aufgaben erfüllen.

#### Central Control Unit
Eine *CCU* ist eine interaktionslose Regeleinheit, welche
an einer zentralen Stelle im Netz sitzt. Die Einheit übernimmt komplexe
Aufgaben und prüft Zusammenhänge.

#### Human Control Unit
Eine *HCU* ist eine Control Unit, welche die Steuerung des Netzwerks durch
einen Menschen ermöglicht. Hierbei besitzt sie eine Eingabemöglichkeit sowie
ein Display.

#### Trigger
Ein *Trigger* ist eine sehr einfache Control Unit, welche nur eine
Eingabemöglichkeit bereitstellt, die ein oder mehrere Schaltbefehle auslöst.
Hiermit verschmelzen die Funktionen einer Control Unit mit der eines
Sensors.

*Trigger* können beispielsweise zur Aktivierung aller Lichter in einem System
verwendet werden.

#### Display
Ein *Display* ist eine sehr einfache Control Unit, welche zur Darstellung von
Messwerten dient. Hierbei reagiert das *Display* selbst auf Notify-Nachrichten
und aktualisiert die vom Display überwachten Messwerte.

### Memory
Ein *Memory* Node ist eine Speicherzelle des Netzwerkes. Hier können Daten
für verschiedene Zwecke persistent abgespeichert werden.

#### Storage
Ein *Storage* ist ein Speicherknoten, welcher als RAM gesehen werden kann.
Diese Knotenklasse kann zum persistenten Datenaustausch zwischen einzelnen
Control Units verwendet werden, um beispielsweise die Parametriesierung des
Netzes zentral zu verwalten.

Die Daten in einem *Storage*-Knoten werden in den Registern gespeichert. Hierbei
werden die 8- und 16-Bit-Register je nach Speichergröße von der kleinsten zur 
größtmöglichsten Registernummer bereitgestellt. Die Register sind hierbei alle
frei les- und schreibbar.

Eine beispielhafte Implementierung eines *Storage*-Knotens mit 256 Byte Speicher
könnte die Register `0x00`-`0x7F` sowie `0x80`-`0xBF` bereitstellen.

#### Recorder
Ein *Recorder* ist ein Knoten, welcher Messwerte chronologisch abspeichert.
Hierbei kann die Speicherung durch verschiedene Auslöser aktiviert werden.

Ein Beispiel wäre, dass ein *Recorder* auf Notify-Nachrichten reagiert und alle
signifikaten Änderungen der Datenbasis sichert.

Eine andere Möglichkeit wäre, dass der *Recorder* in periodischen Abständen
einzelne Sensoren abfragt und deren Messwerte abspeichert.

# Paketformat

| Offset | Länge | Inhalt     | Beschreibung                                   |
|--------|-------|------------|------------------------------------------------|
|      0 |     8 | `0x77`     | Magic Byte                                     |
|      1 |     3 | Major      | Version Major                                  |
|        |     3 | Minor      | Version Minor                                  |
|        |     1 | Flags      | Command Specific                               |
|        |     1 | Response   | Gib an, ob die Nachricht eine Antwort ist.     |
|      2 |     8 | Receiver   | Zieladdresse                                   |
|      3 |     8 | Sender     | Quelladdresse                                  |
|      4 |     8 | Command    | Paket-Befehl                                   |
|      5 |     8 | Length     | Länge der Nutzdaten in Byte                    |
|      6 |   n·8 | Data       | Nutzdaten der Länge `Length`                   |
|    6+n |     8 | Checksum   | Σ(Dataᵢ) mod 256                             |

## Response-Bit
Falls dieses Bit gesetzt ist, ist die Nachricht 

## Prüfsumme
Die Prüfsumme enthält einen vorzeichenlosen Wert, welcher die Summe aller
Bytes aus dem Datenblock enthält. Bei einem leeren Datenblock muss die
Prüfsumme `0` sein.

## Addressen
Das System verwendet ein 8-Bit-Addressformat mit zwei reservierten Addressen:

| Addresse | Benutzung |
|----------|-------------------------------------------------------------------|
|   `0x00` | Anycast-Addresse                                                  |
|   `0x01` | Erste Knotenaddresse                                              |
|        … |                                                                   |
|   `0xFE` | Letzte Knotenaddresse                                             |
|   `0xFF` | Broadcast-Addresse                                                |

### Anycast-Addresse
Ein Paket, welches an die Anycast-Addresse gesandt wird, soll vom ersten
empfangenden Knoten verarbeitet werden. Das Paket wird in keiner Weise
propagiert.

### Broadcast-Addresse
Ein Paket, welches an die Broadcast-Addresse gesandt wird, soll von jedem
Knoten verarbeitet werden. Dazu muss es von jedem Knoten weiter propagiert
werden, zudem aber auch verarbeitet.

### Knotenaddresse
Ein Knoten soll ein Paket, welches an seine Addresse gerichtet ist, verarbeiten.
Ein Paket, welches nicht an die Addresse des Knotens gerichtet ist, muss
propagiert werden.

# Befehle

| Befehls-ID | Beschreibung                                                    |
|------------|-----------------------------------------------------------------|
|     `0x00` | Blank / Ungültig                                                |
|     `0x01` | Discover                                                        |
|     `0x02` | Query                                                           |
|     `0x03` | Notify                                                          |
|     `0x04` | Error                                                           |
|     `0x05` | Write                                                           |
|     `0x06` | Read                                                            |
|          … |                                                                 |
|    ≥`0x10` | Knotenabhängiger Befehl                                         |

## Blank / Ungültig
Dieser Befehl besitzt keine Nutzdaten und führt nichts aus. Er sollte nicht
verwendet werden, da es kein gültiger Befehl ist.

## Discover
Discover ist der Befehl, um die Netztopologie zu erfassen. Wenn ein Knoten
einen Discover-Befehl verarbeitet, antwortet er dem Sender einfach mit einer
Discover-Nachricht, welche das `Response`-Bit gesetzt hat.

## Query
Mit dem Query-Befehl kann man einzelne Funktionen eines Knotens abfragen,
wie beispielsweise die Herkunft und den Typ des Gerätes.

Bei einer Query-Anfrage bestehen die Daten aus einem einzigen Byte, welches
die ID der angefragten Option spezifiziert. Die entsprechende Antwort enthält
dann im ersten Byte die ID der angefragten Information und darauffolgenden die
Information selbst.

|  ID    | Name          | Beschreibung                                        |
|--------|---------------|-----------------------------------------------------|
| `0x01` | Device Info   | Enthält grundlegende Informationen über das Gerät.  |
| `0x02` | Device Name   | Der Name des Geräts, kodiert in UTF-8.              |

### Device Info
Device Info enthält die grundlegenden Informationen über das Gerät, welche im
Daten-Block der Nachricht seriell angeordnet sind.

| Info         | Datentyp | Beschreibung                                       |
|--------------|----------|----------------------------------------------------|
| Device Class | u8       | Geräte-Klasse                                      |
| Vendor-ID    | u16      | ID des Herstellers                                 |
| Device-ID    | u16      | Herstellerabhängige Geräteidentifikationnummer     |
| Serial No.   | u8[6]    | 6 Byte große Seriennummer                          |
| HW Revision  | u8       | Hardware-Revisionsnummer                           |
| SW Revision  | u8       | Software-Revisionsnummer                           |

### Device Name
Der Name des Gerätes in UTF-8-Kodierung. Das letzte Byte des Textes ist ein
0-Byte.

## Notify
Die Notify-Nachricht stellt eine Benachrichtung über ein Event dar. Events
werden in der Regel von passiven Geräten wie Sensoren bereitgestellt, können
aber auch von einem überwachenden oder aktiven Gerät versandt werden.

Die Nachricht einer Notify-Meldung enthält im ersten Byte die Klassifizierung
der Benachrichtigung, in den nachfolgenden Bytes stehen die Nutzdaten.

| Klasse | Name            | Beschreibung                                      |
|--------|-----------------|---------------------------------------------------|
| `0x00` | Blank Signal    | Eine inhaltslose Benachrichtigung.                |
| `0x01` | Reading Changed | Ein Messwert hat sich geändert.                   |
| `0x02` | Control Signal  | Ein Steuersignal.                                 |

### Blank Signal
Das *Blank Signal* ist ein inhaltsloses Notify-Event, welches von einem Gerät
versandt wird, um eine inhaltslose Benachrichtigung zu machen (periodisches
Aufwecken von Geräten o.ä.)

### Reading Changed
Das *Reading Changed*-Signal wird übertragen, wenn sich der Messwert eines
Sensors um das angegebene Delta ändert.

Hierbei wird der folgende Nachrichteninhalt übertragen:

| Offset | Typ | Beschreibung                                                  |
|--------|-----|---------------------------------------------------------------|
| `0x00` | u8  | Header der Notify-Nachricht (`0x01`)                          |
| `0x01` | u8  | Einheit des übertragenen Messwerts                            |
| `0x02` | f16 | Der neue Messwert                                             |
| `0x03` | f16 | Der vorherige Messwert                                        |

### Control Signal
Ein *Control Signal* wird übertragen, um dem Netzwerk eine globale
Zustandsänderung der Steuerung mitzuteilen. Diese Nachricht enthält eine frei
vom Benutzer festlegbare Zustandsnummer, welche den Control Units ein neuen
Steuerzustand übermittelt.

Der Nachrichteninhalt sieht wiefolgt aus:

| Offset | Typ | Beschreibung                                                  |
|--------|-----|---------------------------------------------------------------|
| `0x00` | u8  | Header der Notify-Nachricht (`0x02`)                          |
| `0x01` | u8  | Zustands-Nummer                                               |

## Error

<todo>
	Fehlerpackete definieren
</todo>

## Write
Schreibt einen Wert in ein Geräte-Register. Jeder Schreibbefehl besitzt eine
Datengröße von 3 Byte, wobei das erste Byte die Registernummer angibt.
Das zweite und dritte Byte enthalten nun den Integer, welcher in das Register
geschrieben werden soll. Bei 8-Bit-Registern wird nur das zweite Byte beachtet,
der Wert des dritten wird ignoriert.

## Read
Liest einen Wert aus einem Geräte-Register. Bei einer Anfrage wird in den
Daten nur die Nummer des Registers übertragen, welches gelesen werden soll.

Auf eine Lese-Anfrage antwortet das angefragte Gerät nun mit einer Antwort,
die das selbe Format nutzt wie ein `Write`-Befehl.

# Einheiten
Das System bietet eine Menge an vordefinierten Einheiten, welche für Messwerte
verwendet werden sollen. Diese Einheitenwerte werden in einem Festkommaformat
gespeichert, welches zwei dezimale Nachkommastellen bietet (Faktor 100).

Hiermit erreicht man einen Wertebereich von `0,00` bis `655,35`.

| ID     | Einheit | Anmerkungen                                               |
|--------|---------|-----------------------------------------------------------|
| `0x00` | Custom  | Diese Einheit benötigt einen Treiber, um ausgewertet werden zu können. |
| `0x01` |       1 | Ein einheitenloser Zählwert                               |
| `0x02` |     1 % | Ein Anteil in Prozent.                                    |
| `0x03` |    1 Hz | Eine Frequenz in Hertz.                                   |
| `0x04` |     1 m | Eine Distanz in Metern.                                   |
| `0x05` |    60 s | Ein Zeitwert in Minuten.                                  |
| `0x06` |     1 K | Eine Temperatur in Kelvin (da nur vorzeichenlose Werte)   |
| `0x07` |     1 W | Eine Leistung in Watt.                                    |
| `0x08` |     1 V | Eine Spannung in Volt.                                    |
| `0x09` |     1 A | Ein Strom in Ampere                                       |
| `0x0A` |     1 Ω | Ein Widerstand in Ohm.                                    |
| `0x0B` |     1 C | Eine Ladung in Coloumb.                                   |
| `0x0C` |    1 µF | Eine Kapazität in Mikrofarad.                             |
| `0x0D` |    1 Pa | Ein Druck in Pascal.                                      |
| `0x0E` |     1 N | Eine Kraft in Newton.                                     |
| `0x0F` |    1 kg | Ein Gewicht in Kilogramm.                                 |
| `0x10` |     1 ° | Ein Winkel in Grad.                                       |
| `0x11` |    1 lm | Lichtstrom in Lumen.                                      |
| `0x12` |    1 cd | Lichtstärke in Candela.                                   |
| `0x13` |  bitfld | Der Wert ist ein Bit-Feld mit 16 einzelnen Bits.          

# Verwendete Datentypen

Dieses Dokument verwendet einige Datentypen, welche hier spezifiziert werden:

| Typ | Größe (Bits) | Beschreibung                                            |
|-----|--------------|---------------------------------------------------------|
|  u8 | 8            | Ein vorzeichenloser 8-Bit-Integer.                      |
| u16 | 16           | Ein vorzeichenloser 16-Bit-Integer.                     |
| u32 | 32           | Ein vorzeichenloser 32-Bit-Integer.                     |
|  i8 | 8            | Ein vorzeichenbehafteter, im Zweierkomplement gespeicherter 8-Bit-Integer. |
| i16 | 16           | Ein vorzeichenbehafteter, im Zweierkomplement gespeicherter 16-Bit-Integer. |
| i32 | 32           | Ein vorzeichenbehafteter, im Zweierkomplement gespeicherter 32-Bit-Integer. |
| f16 | 16           | Ein 16-Bit-Festkommawert, welcher durch einen vorzeichenlosesn 16-Bit-Integer dargestellt wird. Die Anzahl an Nachkommastellen beträgt 2 Dezimalstellen. Um den Wert des Typen zu erhalten, wird der Integer mit `0,01` multipliziert. Um den Integer zu erzeugen, wird der Festkommawert mit `100,0` multipliziert und alle nachfolgenden Stellen abgeschnitten. |






<script type="text/javascript" src="/home/felix/artifacts/mdext/toc.js"></script>
<script type="text/javascript" src="/home/felix/artifacts/mdext/todo.js"></script>
