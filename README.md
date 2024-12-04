# PomodoroTimer

Pomodoro Timer für den Atmega 2560 mit LCD Keypad Shield HW-555

Das LCD Display mit den Keypad draufstecken

Den Buzzer auf Pin50 (Plus Pol) und den andern Pin auf GND stecken.

Firmware draufspielen

Start Timer mit Up bzw. Down auswählen und mit Select starten.

Standard Runde 25min

Standard Pause 5min

Standard Piepzeit 5 Sec

Standard Longpause 15min

Mit Left und Right werden die Zeiten im Menü in +/- 1 Schritten geändert

Mit Up und Down werden die Zeiten im Menü in +/- 5 Schritten geändert

Wenn die Zeit runterläuft, dann wird 1/4 angezeigt.

wieviele Runden bis zur ersten 15min Pause noch da sind.

Nach 15 Sekunden ohne Tastendruck startet der Timer

Mit der Taste Right wird eine Phase übersprungen

Mit der Taste Left wird die Phase abgebrochen

Alle Daten werden nach dem Verlassen des Menüpunkts im Eeeprom gespeichert und nach einem Neustart automatisch geladen.

Wenn keine Daten hinterlegt sind, dann werden die Standardwerte(siehe oben) geladen.
