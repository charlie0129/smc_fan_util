# smc_fan_util

A small utility to control your MacBook's fans with a built-in fan curve optimized for smooth fan speed shifts.

```
DESCRIPTION:
    This utility enables you to control your Mac's fans manually.
    Note: This utility only applies to Intel Macs with 2 fans.
    !!!You should execute this utility with root privileges!!!
SYNOPSIS:
    sfc_manual [-a] | [-A] | [-d] | [-h] | [-i] |
               [-m speed_percentage] | [-m speed_left speed_right]
OPTIONS:
    -a: set fans to auto mode (controlled by SMC).
    -A: set fans to auto mode (controlled by this program).
    -d: turn off fans completely.
        Note: This could easily cause your machine to overheat!!!
    -h: display this message.
    -i: show fan information.
    -m <percentage>: set fan speeds to a specific percentage manually. (only input integers)
    -m <speed_left> <speed_right>: set fan speeds to specific speeds manually. (only input integers)
        Note: If you set fan speeds by RPM, it ignores Apple's limits.
              You can "overclock" or "underclock" your fans,
              but ridiculous values may damage you machine!!!
EXAMPLES:
    sfc_manual -m 50        // set both fans to 50 percent
    sfc_manual -m 1080 1000 // left: 1080rpm; right: 1000rpm
    sfc_manual -a           // set fans to auto mode (SMC)
```
