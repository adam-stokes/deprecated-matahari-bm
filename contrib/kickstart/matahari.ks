# Include base kickstart file of your choosing

# %include kickstarts/example.com.ks

%packages
matahari
matahari-agent-lib
matahari-service
matahari-broker
matahari-host
matahari-net
matahari-lib
-matahari-devel
%end
