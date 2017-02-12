<meta charset="utf-8" />
<link rel="stylesheet" href="style.css">

# MESH Networking

## Anforderungen
- ☑ Netzwerk an Geräten
	- ☑ Keine Loops vorgesehen
	- ☑ 256 Teilnehmer sind ausreichend
- ☑ Übertragung von Daten
	- ☑ Übertragung von Sender zu Empfänger
	- ☑ Sender muss Empfänger bekannt sein (Antwortmöglichkeit)
	- ☑ Checksumme/Erkennung von Übertragungsfehlern
	- ☑ Kleine Paketgrößen (≤256 Byte)
- Geschwindigkeit unwichtig
- Knotentypen
	- ☑ Sensor
		- ☐ Helligkeit
		- ☐ Temperatur
		- ☐ Luftdruck
		- ☐ Luftfeuchte
		- ☐ Zeit / Uhr / Timer
		- ☐ Schalter
	- ☑ Aktor
		- ☐ Schalter/Relais
		- ☐ Warnsignal
	- Mixed (Sensor + Aktor) [Könnte man auch über die Verwendung von doppelter Addressierung lösen]
	- ☑ Memory
		- ☑ Storage  (kann ein oder mehrere Blöcke speichern)
		- ☑ Recorder (reagiert auf Notify und speichert die Daten eines Sensors)
	- ☑ Control Unit (Erlaubt Steurung und Dateneinsicht)
		- ☑ Trigger (Lösen ein bestimmtes Event aus, "minimale Control Unit")
		- ☑ Display (Zeigen einen Messwert an, "minimale Controlunit")
- ☑ Nachrichtenformat
	- ☑ Kopf (Magic Number + Protokollversion + Befehlsflags)
	- ☑ Empfänger-Addresse
	- ☑ Sender-Addresse
	- ☑ Befehl (Systembefehl oder spezieller Knotenbefehl)
	- ☑ Länge Datensatz
	- ☑ Daten, wenn Länge > 0
	- ☑ Checksumme der Daten
- Globale Nachrichtentypen
	- ☑ Query    (Gerät bereit, Gerätetyp o.ä.) 
	- Notify   (Sensor ändert Wert)
	- Error    (Ein Fehler trat auf)
	- ☑ ReadReq  (Fordert Messdaten von einem Knoten an)
	- ☑ ReadRsp  (Überträgt Messdaten als Antwort auf ein `ReadReq`)
	- ☑ Write    (Schreibt Steuer- oder Konfigurationsdaten in einen Knoten, abhängig von Knotenimplementierung)
	- ☑ Discover (Finde Geräte)
		- ☑ Ping über Discover+Unicast
		- ☑ Netconnect über Discover+Anycast
- ☑ Nachrichten Ziele
	- ☑ Konkreter Knoten (Unicast)
	- ☑ Alle Knoten (Broadcast=MAX(Addresse))
	- ☑ Ein Knoten  (Anycast=MIN(Addresse))
- ☑ Knotenkonfiguration (Netport = Anschluss ans Netz)
	- ☑ End Node    (Knoten besitzt nur einen Netport)
	- ☑ Serial Node (Knoten besitzt zwei Netports und reicht Nachrichten durch)
	- ☑ Hub Node    (Knoten mit vielen Netports, leitet Nachrichten von einem Netport zu allen anderen weiter)
	- ☑ Switch Node (Wie Hub, kennt aber die Geräte an den Ports)
