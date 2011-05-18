# common lib functions

function get_pid_of() {
        local TEST_PID=`ps -eo pid,args|grep $1|grep -v grep|cut -f1 -d' '`
        echo $TEST_PID
}
