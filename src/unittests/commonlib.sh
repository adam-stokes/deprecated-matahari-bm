# common lib vars
TEST_DEFAULT_AGENTS=( matahari-qmf-hostd \
                     matahari-qmf-networkd \
                     matahari-qmf-serviced \
                     matahari-qmf-configd  )
TEST_DEFAULT_SERVICE_NAMES=( matahari-network \
                            matahari-host \
                            matahari-service \
                            matahari-config )
# common lib functions

function get_pid_of() {
        local TEST_PID=`ps -eo pid,args|grep $1|grep -v grep|cut -f1 -d' '`
        echo $TEST_PID
}

function is_established() {
    local PID=$1
    local PPORT=$2
    local OUT=`lsof -p $PID |grep -q $PPORT.*ESTABLISHED`
    echo $OUT
}
