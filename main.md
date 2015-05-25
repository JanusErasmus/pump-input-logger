# Pump Input logger #

Logs and displays the time a water pump was on. The pump will only be switched on when in a valid pump time frame and if the water level input is high.

## Operation ##
The unit will monitor a input from a tank level sensor and switch a relay according to defined time variables.
The unit monitors the sensor input and pump frames every 30s.
There are three time variables that has to be setup:

### PUMP TIME FRAME ###
This is the time frame in which the unit may switch the relay ON.
  * A frame is setup by changing its START and END hours.
  * The START hour should be less than the END hour.
  * The END hour should be more than the START hour.
  * When the level sensor input is high and the current time is within the frame, the unit will switch the relay ON.
  * If at any moment the level input goes low, the unit will switch the relay OFF.
  * If the level sensor input us high and the current time is NOT within the frame, the unit will keep the relay OFF. Lets name this state as the DISABLED state of the relay

### UP TIME ###
This is the time the relay may be kept on continuously.
  * The UP time is set in Minutes.
  * When the level sensor is high, and the current time is within the time frame, the relay will be switched ON. As soon as the UP time has passed the relay will be switched off. Lets name this state as the RESTING state of the relay.

### REST TIME ###
This is the time the relay will rest after it has been switched off due to the UP time expiring.
  * The REST time is set in Minutes.
  * When the REST time has passed the relay will be switched back ON again.

## Standby Screen ##
When there has not been an input to the unit, either from buttons or sensor, the Standby Screen will be shown. The level sensor input and current relay sate is shown.
### Tank Full ###
When the Tank is full the level sensor input should be low. The relay will be switched off. The Standby screen will look like this:

http://wiki.pump-input-logger.googlecode.com/git/standby_full.JPG

### Tank Low, within pump frame ###
When the Tank is lower than full, lets name it low, and the current time is within the pump frame. The Standby screen will look like this:

http://wiki.pump-input-logger.googlecode.com/git/standby_on.JPG

### Tank Low, pump Resting ###
When the Tank is low and the pump has been running longer than the UP time. The Standby screen will look like this:

http://wiki.pump-input-logger.googlecode.com/git/standby_resting.JPG

### Tank Low, pump Disabled ###
When the Tank is low and the current time is NOT within the pump frame. The Standby screen will look like this:

http://wiki.pump-input-logger.googlecode.com/git/standby_disabled.JPG

## LCD Menu ##
The variables for the pump and current time can be configured through the Menu of the unit.
  * The upper button is ENTER.
  * The middle button toggles up and down.
  * The lower button is CANCEL.

Pressing Enter from the Standby menu opens the Main Menu. The menus are then navigated with the buttons.
Here is a drawing for the different menus available:

http://wiki.pump-input-logger.googlecode.com/git/lcdMenu.JPG