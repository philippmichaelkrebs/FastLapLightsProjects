# FastLapLights Projects

Dieses Repository enthält eine Sammlung von Projekten, die auf dem **FastLapLights-Gadget** basieren.
Das System erweitert eine digitale Carrera-Bahn um zusätzliche Anzeigen, Streckeneffekte und Telemetriedaten.

Die Projekte in diesem Repository nutzen die vom FastLapLights-Controller bereitgestellten Renndaten und machen daraus sichtbare Informationen auf Displays, Signallichtern oder anderen Streckenanzeigen.

Das Ziel ist eine modulare Erweiterungsplattform für Bastler, die ihrer Bahn etwas mehr Rennatmosphäre, Datenanalyse oder schlicht technische Spielereien hinzufügen möchten.

Alle Module können unabhängig voneinander betrieben werden oder miteinander kombiniert werden.

---

# Projektübersicht

## Speed Display & Optional Blitzer

Dieses Projekt befasst sich mit der Messung der Geschwindigkeit der Fahrzeuge und zeigt sie auf einer LED-Matrix an.

Neben der reinen Anzeige kann optional ein Blitzer aufgestellt werden.
Wer eine bestimmte Geschwindigkeit überschreitet – oder unterschreitet – wird entsprechend "geblitzdingst".

Angezeigt werden unter anderem:
* aktuelle Geschwindigkeit
* Topspeed
* Durchschnittsgeschwindigkeit
* bis zu sechs Fahrer gleichzeitig

Die Konfiguration erfolgt über einen Drehgeber direkt am Microcontroller des Displays.

Weitere Details findest du im entsprechenden Projektordner.

---

## Scoring Pylon

Der **Scoring Pylon** ist eine Anzeige für Renninformationen ähnlich der bekannten Türme aus dem Motorsport bzw. dem Position Tower von Carrera.

Er stellt beispielsweise dar:
* Fahrerpositionen
* Rennstatus
* Tankstände
* Streckentemperatur
* ggf. weitere Live-Daten des Rennens je nach Konfiguration

Der Pylon kann als zentrale Anzeige neben der Strecke oder im Fahrerbereich installiert werden und eignet sich besonders für Rennen mit mehreren Fahrern.

---

## Digitale Flaggen

Die digitalen Flaggen sind eine Streckenanzeige für Rennsignale.

Je nach Rennstatus können verschiedene Flaggen dargestellt werden:
* Grünphase
* Gelbphase / Safety Car
* Chaos (Doppel Gelb für die meisten von euch)
* Frühstart
* Mit Erweiterung
  * Rennende / Karierte Flagge
  * Weiße Flagge (Tank ist leer)
  * Nasse oder rutschige Strecke

---

# FastLapLights Dateninterface

Der **FastLapLights-Controller** stellt über eineUART-Schnittstelle mit 2 Hz eine Reihe von Renndaten bereit, die verwendet werden können.

Zu den verfügbaren Informationen gehören unter anderem:

Rennstatus
* Startphase
* Chaos
* Grünphase
* Safetycar / Gelbphase
* Frühstart
* Rennen offen

Renninformationen
* gefahrene Runden
* Anzahl der roten Startlichter
* Fahrer-ID bei Frühstart

Fahrerdaten
* Fahrer im Rennen
* aktuelle Position
* Tankstatus

Die Daten können frei ausgewertet und für eigene Anzeigen oder Funktionen genutzt werden.

---

# Projektstruktur

Die einzelnen Module befinden sich jeweils in eigenen Unterordnern.

Typischerweise enthalten die Projekte folgende Struktur:

```
project-name/
 ├─ src/          Firmware
 ├─ documents/    Schaltpläne und technische Unterlagen
 ├─ pictures/     Fotos des Aufbaus
 ├─ stl/          3D-Druckdateien
 └─ README.md     Projektdokumentation
```

---

# Hardware
Die jeweils benötigten Komponenten sind in den einzelnen Projektordnern dokumentiert.

---

# Ziel des Projekts

Es soll eine Sammlung von Ideen für den FastLapLights-Controller sein.
Die Projekte sind bewusst offen gehalten und können beliebig erweitert oder angepasst werden.

# Am Ende gilt
Eine Slotcar-Bahn ist nie wirklich fertig – sie bekommt nur regelmäßig neue Features.
