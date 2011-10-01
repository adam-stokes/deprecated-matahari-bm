Here is how I am running these:
===========================================
nosetests -v --nologcapture                     #  run all tests from the command line
nosetests -v --nologcapture --with-xunit        #  generate xml result file for hudson/jenkins
nosetests -v --nologcapture test_host_api       #  run individual suite

**********************************
NOTE: requires python-nose > v1.0
**********************************
