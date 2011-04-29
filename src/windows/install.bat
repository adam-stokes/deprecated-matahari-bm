rem Install Matahari services
echo

rem Get the install directory 
set target=%0
set target=%target:\install.bat=%

rem Now remove quotes so we can add them as necessary
set target=%target:"=%

cd "%target%"

rem Now install the agents as services and start them

set agents=hostd networkd serviced
for %%A in (%agents%) do sc delete mh_%%A
for %%A in (%agents%) do del mh_%%A.exe
for %%A in (%agents%) do copy srvany.exe mh_%%A.exe
for %%A in (%agents%) do mh_%%A.exe install "%target%\matahari-qmf-%%A.exe" "%target%"
for %%A in (%agents%) do sc start mh_%%A

rem Remove the old name for the network agent
sc delete mh_netd
