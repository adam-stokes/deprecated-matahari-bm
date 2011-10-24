rem UnInstall Matahari services
echo

rem Get the install directory 
set target=%0
set target=%target:\uninstall.bat=%

rem Now remove quotes so we can add them as necessary
set target=%target:"=%

cd "%target%"

rem Now uninstall the agents

set agents=hostd networkd serviced sysconfigd
for %%A in (%agents%) do (
  sc stop mh_%%A
  sc delete mh_%%A
  taskkill /IM matahari-qmf-%%A.exe /f
  del mh_%%A.exe
)
