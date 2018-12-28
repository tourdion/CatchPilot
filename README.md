# CatchPilot
Monitors a trap and sends SMS-Messages for Status/Catch with Arduino

In the German state of North Rhine Westphalia, trap hunting is only allowed with live capture. The trap must be checked twice a day, and if caught, the captured animal must be released or killed if it is predatory.
The project presented here meets these requirements by sending status messages at adjustable times or a catch message via SMS.

Date: 22.01.2018

Feel free to send me a message at catchpilot.de@gmail.com if you have suggestions for improvement, especially for memory management.

The Project is under the GNU General Public License v3.0

Have fun and Weidmannsheil!
        
In dem deutschen Bundesland Nordrhein Westfalen ist die Fallenjagd nur mit Lebendfang erlaubt. Die Falle muß zweimal am Tag kontrolliert werden und bei einem Fang muß das gefangene Tier befreit oder aber getötet werden, wenn es sich um Raubwild handelt.
Das hier vorgestellte Projekt erfüllt diese Anforderungen, indem es zu einstellbaren Zeiten eine Statusmeldung oder aber eine Fangmeldung per SMS verschickt.        
        
Version Beta 0.3    Stand 28.12.2018

Änderungen:
- Umstellen auf Boolean-Variablen
- Reduzierung der globalen String-Variablen, um Abstürze zu vermeiden.
- Anpassen der getStatusGSM()-Subroutine. Dadurch konnte die globale Nachricht-Variable stark reduziert werden. Getestet nur mit einer ALDI-Talk Karte, für andere Netze müssen ggf. die strcpy und strncat-Werte angepasst werden.
- Einsatz der AVR-Watchdog-Funktion.

Geplant:
- Stromverbrauch reduzieren. Der aktuell verwendete Bleigel-Akku hält nur ca. 2-3 Tage.
