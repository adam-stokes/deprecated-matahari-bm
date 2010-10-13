# Install Matahari services
echo

rem Get the install directory 
set target=%0
set target=%target:\install.bat=%

rem Now remove quotes so we can add them as necessary
set target=%target:"=%

cd "%target%"

rem Change the arguments to specify an alternate config file
copy rhsrvany.exe mh_broker.exe
mh_broker.exe install "%target%\qpidd.exe --config matahari-broker.conf" "%target%"

copy rhsrvany.exe mh_host.exe
mh_host.exe install "%target%\matahari-hostd.exe" "%target%"

copy rhsrvany.exe mh_net.exe
mh_net.exe install "%target%\matahari-netd.exe" "%target%"
