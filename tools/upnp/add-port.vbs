
'This program forwards router's port 12345 to local computer 192.168.1.3
'UPnP needs to be enabled on the router and localhost 
'For XP, seee http://support.microsoft.com/kb/941206

Set theNatter = CreateObject( "HNetCfg.NATUPnP")

Dim mappingPorts

Set mappingPorts = theNatter.StaticPortMappingCollection

for each mappingPort in mappingPorts

MsgBox("Wan IP: " & mappingPort.ExternalIPAddress) 

exit for

next

mappingPorts.Add 12345, "TCP", 12345, "192.168.1.3", TRUE, "my12345" 

MsgBox("Port 12345 mapped. Hit OK when you're done and to remove mappping") 

mappingPorts.Remove 12345, "TCP"

MsgBox("Port mapping removed. Good bye.") 

