# FastLapLights – Speed Display & Optional Blitzer

Dieses Projekt erweitert das FastLapLights-Gadget um eine Geschwindigkeitsanzeige für deine Carrera-Bahn. Neben der reinen Anzeige kann das System optional als freundlicher Streckenblitzer agieren. Wer zu schnell oder zu langsam unterwegs ist, wird entsprechend verewigt.

Die Geschwindigkeitsanzeige erfolgt über eine HUB75 LED-Matrix. Optional kann zusätzlich ein Blitzer verbaut werden. Wer lieber ohne fotografische Beweisführung fährt, kann den Blitzer einfach weglassen :)

Auf dem Display werden in Echtzeit verschiedene Informationen dargestellt
* aktuelle Geschwindigkeit der Fahrzeuge
* Topspeed
* Durchschnittsgeschwindigkeit
* Darstellung für bis zu sechs Fahrer gleichzeitig

Die Konfiguration erfolgt über einen Drehgeber, über den sich verschiedene Parameter einstellen lassen, zum Beispiel:
* Auslösegeschwindigkeit des Blitzers
* ob der Blitzer bei zu hoher oder auch bei zu niedriger Geschwindigkeit auslösen soll
* Skalierungsfaktor für die Geschwindigkeitsanzeige (für unterschiedliche Streckenlayouts)

---

## Erweiterbarkeit

Der angezeigte Code ist bewusst offen gehalten und kann beliebig erweitert werden.
Der FastLapLights-Controller stellt über eine UART-Schnittstelle (2 Hz) eine Reihe weiterer Daten bereit, die sich problemlos in eigene Anzeigen oder Funktionen integrieren lassen.

Zu den verfügbaren Informationen gehören unter anderem:

Rennstatus

* Startphase
* Chaos
* Grünphase
* Safety-Car / Gelbphase
* Frühstart
* Rennen offen

Weitere Daten

* gefahrene Runden
* Fahrer-ID bei Frühstart
* Anzahl der roten Lichter der Startampel

Fahrerdaten

* Teilnahme am Rennen
* aktuelle Position
* Tankstatus

Wer also noch mehr Anzeigen, Statistiken oder Spielereien einbauen möchte, hat hier reichlich Material zum Experimentieren.

---

## Pausenmodus

Wenn zwei Minuten lang keine Aktivität auf der Strecke stattfindet (keine Überfahrt), schaltet das Display automatisch in den Pausenmodus.

Zunächst erscheint eine karierte Flagge, anschließend laufen verschiedene Idle-Animationen oder Anzeigen im Wechsel.
Eigene Inhalte können jederzeit ergänzt werden. Neue Bitmaps lassen sich einfach in der Datei `images.h` hinzufügen und anschließend im Code verwenden.

Der Pausenmodus ist ein guter Ort für alles, was während des Rennens zu viel Aufmerksamkeit bekommen würde - Logos, Streckeninfos, Temperatur oder andere kreative Ideen.

---

## Benötigte Hardware

Für dieses Projekt benötigst du den FastLapLights-Controller sowie folgende Komponenten

| Teil                     | Stück | Kommentar                  | Distributor |
| ------------------------ | ----- | -------------------------- | ----------- |
| HUB75 32x64px            | 1     | LED Matrix Waveshare       | Amazon      |
| ESP32S3 Adafruit Feather | 1     | Adafruit 5323              | Botland     |
| Drehgeber                | 1     | DFRobot EC11               | Botland     |
| LEDs Orange Turm         | 4     | EVL 333-2USOC/S4           | Reichelt    |
| 25 Ohm Widerstand        | 2     | Vorwiderstand LEDs Turm    | Reichelt    |
| 1000 uF 10V Cap          | 1     | RAD FR 1.000/16            | Reichelt    |
| Ledtreiber               | 1     | Adafruit 4886              | Botland     |
| LEDs Strecke             | 16    | 6603R1C-LHA-B              | Reichelt    |
| Widerstand 4.7 kOhm      | 3     | Pull Up I2C, ST Track Data | Reichelt    |
| Widerstand 1 kOhm        | 1     | ST Track Data              | Reichelt    |
| SFH 309 FA               | 4     | IR Transistor              | Reichelt    |
| Komperator               | 2     | TLC 393 DIP                | Reichelt    |
| Widerstand 1.1 kOhm      | 4     | IR Pull-Down               | Reichelt    |
| Widerstand 4.7 kOhm      | 4     | Komperator Output Pull-Up  | Reichelt    |
| Widerstand 10 kOhm       | 5     | Teiler 1 und Feedback      | Reichelt    |
| Widerstand 2.2 kOhm      | 1     | Teiler 2                   | Reichelt    |
| 3W LED Blitzer           | 2     | LB-P200W3C-H3              | Reichelt    |
| 1 Ohm Widerstand         | 1     | CAD MP915-1,00             | Reichelt    |
| 12000 uF Kondensator     | 1     | RAD LXZ 10/12K             | Reichelt    |
| Mosfet                   | 1     | IRLB 3813 IR               | Reichelt    |
| Netzteil                 | 1     | GLP GPV-20-5               | Reichelt    |

---

## Repository-Struktur

* Documents - Schaltpläne und technische Unterlagen
* pictures - Beispielbilder des Aufbaus
* stl - 3D-Druckdateien für Gehäuse und Halterungen
* misc - Aktuell Python-Skripte die dir dabei helfen Änderungen vorzunehmen
  * `bmp_to_arr.py`: Erstellt aus einem Bitmap ein uin16_t array
  * `color_calculator.py`: Berechnet aus hsv oder rgb Werten den zugehörigen Wert für das Display
  * `speed_calculator.py`: Berechnet aus einer Geschwindigkeit die Ticks zur Übermittlung an STM32

---

## Abschließende Worte

Wer seine Strecke um ein paar technische Spielereien erweitern möchten ist hier gut aufgehoben. Der Code darf gerne angepasst, erweitert oder zweckentfremdet werden.

Viel Spaß beim Nachbasteln und Experimentieren.
