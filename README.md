#phosphor - buttons

Phosphor-buttons has a collection of IO event handler interfaces
for physical inputs which are part of OCP front panel.

It defines an individual dbus interface object for each physical
button/switch inputs such as power button, reset button etc.
Each of this button interfaces monitors it's associated io for event changes and calls
the respective event handlers.

## Gpio defs config
    In order to monitor a button/input interface the
respective gpio config details should be mentioned in the
gpio defs json file - "/etc/default/obmc/gpio/gpio_defs.json"

 1. The button interface type name.
 2. An array consists of single or multiple
    gpio configs associated with the specific button interface.
 3. The name of the gpio line must be included
 4. The edge (rising or falling) must be specified. For instance,
    if a button is LOW when asserted, then edge would be falling.

## example gpio def Json config

{
    "gpio_definitions": [
        {
            "name": "POWER_BUTTON",
            "gpio_name": "PWR_BTN_L-I",
            "direction": "falling"
        },
        {
            "name": "RESET_BUTTON",
            "gpio_name": "RST_BTN_L-I",
            "direction": "falling"
        },
        {
            "name": "ID_BTN",
            "gpio_name": "ID_BTN_N-I",
            "direction": "falling"
        },
        {
            "name" : "HOST_SELECTOR",
            "gpio_config" : [
            {
                "gpio_name": "AA4",
                "name": "one",
                "direction": "rising"
            },
            {
                "gpio_name": "AA5",
                "name": "two",
                "direction": "falling"
            },
            {
                "gpio_name": "AA6",
                "name": "three",
                "direction": "rising"
            },
            {
                "gpio_name": "AA7",
                "name": "four",
                "direction": "falling"
            }
            ]
        },
}
