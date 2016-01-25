'open Rclm.5km.ctl' 
  'open R-15-35.ctl'
  'open R-15-34.ctl'
  'open R-15-33.ctl'
  'open R-14-33.ctl'
  'open R-14-32.ctl'
  'open R-13-34.ctl'
  'open R-13-31.ctl'
  'open blocks1km.ctl'

*swnet clev0 = '50 100 150 200 250 300 350 400 450 500 550 600 650'
clev0 = '-20 0 20 40 60 80 100 120 140 160 180 200' 
*var0 = 'Swnet' 
var0 = 'Qh' 
*'set mpdset hires'
'set grads off'
'set lon -100.795 -71.995' 
'set lat 30 45'
'set dfile 1' 
'set gxout contour'
'set clab off'
'set clevs 'clev0 
'd 'var0 
'set gxout shaded'
'set dfile 2'
'set clevs 'clev0 
'd 'var0 
'set dfile 3'
'set clevs 'clev0 
'd 'var0 
'set dfile 4'
'set clevs 'clev0 
'd 'var0 
'set dfile 5'
'set clevs 'clev0 
'd 'var0 
'set dfile 6'
'set clevs 'clev0 
'd 'var0 
'set dfile 7'
'set clevs 'clev0 
'd 'var0 
'set dfile 8'
'set clevs 'clev0 
'd 'var0 
'set dfile 9'
'set ccolor 10'
'set cthick 4'
'set gxout grid'
'd var' 
'draw title 'var0 ' CLM/GEOS/1KM 2001061121'
'cbar'

