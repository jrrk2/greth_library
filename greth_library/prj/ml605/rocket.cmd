setMode -acecf
addCollection -name "rocket"
addDesign -version 0 -name "rocket"
addDeviceChain -index 0
setCurrentDesign -version 0
setCurrentDeviceChain -index 0
addDevice -p 1 -file "rocket_soc.bit"
generate -active rocket
quit
