# smc_fan_util

A small utility to control your MacBook's fans.  
(currently only applies to 15-inch MacBooks from 2016 to 2019 without the Vega graphics card. Support for other models is not guaranteed. You can edit the fan curve in the code yourself to make it work on other Macs.)

- Auto-fan-stop.
- Custom or SMC-defined fan curve.
- Manually control fan speeds.

You can use this tool with [my-hammerspoon-config](https://github.com/charlie0129/my-hammerspoon-config). A nice little GUI and a icon will be placed on your menu bar (see [here](https://github.com/charlie0129/my-hammerspoon-config/blob/master/README.md)).  

### Why I created this utility:
As we all know, the recent 15-inch MacBook Pros do not have a auto-fan-stop feature. That is to say, when the computer is idle, the fans automatically stops, like the fans on the 13-inch MacBook Pros do. Due to the lack of auto-fan-stop feature, my 15-inch MacBook Pro is always taking in dust and debris even if the computer is idle, which results in the heatsink being blocked. If I just stop the fans completely(this feature is provided) and I forgot to turn it back on, the computer will overheat. I need the fans to stop if everything is under control and the computer is idle. And if any monitored component tmperature reaches a certain threshold, I need the fans to turn back on. So, a automation tool is needed and I created this tools.  
This tool is made only because I needed auto-fan-stop and manual fan speed control features on my 2018 15-inch MacBook Pro. Therefore, support for other MacBook models is limited.  
Since I use this tool every day, I will continue to update it if I encounter bugs or I need more features.  

---
Personally, I use the `--SMC-enhanced` option the most. So, this option is probably the most maintained option.  

In-detail description:  

```
DESCRIPTION:
    This utility enables you to control your Mac's fans manually.
    Note: This utility only applies to Intel Macs with 2 fans.
    !!!You should execute this utility with root privileges!!!
SYNOPSIS:
    smc_fan_util [-a] | [-A] | [--SMC-enhanced] | [-d] | [-h] | [-i]
               [-m speed_percentage] | [-m speed_left speed_right]
OPTIONS:
    -a: Auto mode (controlled by SMC).
    -A: Auto mode (controlled by this program).
    --SMC-enhanced: Auto mode (enabled fan-auto-stop feature while using native smc fan curves, recommended).
    -d: turn off fans completely.
        Note: Be sure to monitor temperatures to avoid overheat!!!
    -h: display this message.
    -i: show fan information.
    -m <percentage>: set fan speeds to a specific percentage manually.
    -m <speed_left> <speed_right>: set fan speeds to specific speeds manually.
        Note: If you set fan speeds by RPM, it ignores Apple's limits.
              You can "overclock" or "underclock" your fans,
              but ridiculous values may damage you machine!!!
EXAMPLES:
    smc_fan_util -m 50        // set both fans to 50 percent
    smc_fan_util -m 1080 1000 // left: 1080rpm; right: 1000rpm
    smc_fan_util -a           // set fans to auto mode (SMC)
```

---
*THE SOFTWARE IS PROVIDED **"AS IS"**, WITHOUT WARRANTY OF ANY KIND. AND IS BY NO MEANS COMPLETE.*  

---