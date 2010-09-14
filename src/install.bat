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
mh_broker.exe install "%target%\qpidd.exe --auth 0 --log-to-file broker.log" "%target%"

copy rhsrvany.exe mh_host.exe
mh_host.exe install "%target%\matahari-host.exe" "%target%"
