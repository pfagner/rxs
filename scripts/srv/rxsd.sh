#!/bin/bash
PREFIX_ERRN="ERRN:"
PREFIX_INFO="INFO:"

SERVICE="rxsd"
if [ "$2" != "--encoder" ]; then
    OPERATION=$1 # start, stop, restart
    RUNNING_MODE=$2 # --mode=daemon
    THIS_ADDRESS=$3 # --addr_rxs=192.168.0.2
    THIS_PORT=$4 # --port_rxs=1302
    ALLOW_ADDRESS=$5 # --addr_allowed=192.168.0.0
    FILE_USERS=$6 # --file_users=/usr/local/etc/rxs/rxs_users
    PID=$7 # --pid_host=4444
else
    OPERATION=$1
    ENCODER=$2
    RUNNING_MODE=$3
    THIS_ADDRESS=$4
    THIS_PORT=$5
    ALLOW_ADDRESS=$6
    FILE_USERS=$7
    PID=$8
fi
if [ ! -f "/usr/sbin/${SERVICE}" ]; then
  echo "${PREFIX_ERRN} file doesn't exist"
  exit 1
fi
##########################################################################
# Running mode
##########################################################################
case "$OPERATION" in
    'start')
        echo "${PREFIX_INFO} starting ${SERVICE} server"
	if [ "$2" != "--encoder" ]; then
            /usr/sbin/"$SERVICE" "$RUNNING_MODE" "$THIS_ADDRESS" "$THIS_PORT" "$ALLOW_ADDRESS" "$FILE_USERS" "$PID"
            res=$?
	else
            /usr/sbin/"$SERVICE" "$ENCODER" "$RUNNING_MODE" "$THIS_ADDRESS" "$THIS_PORT" "$ALLOW_ADDRESS" "$FILE_USERS" "$PID"
            res=$?
	fi
    if [ $res -ne 0  ]; then
        echo "${PREFIX_ERRN} server ${SERVICE} has been failed"
        exit 1
    fi        
    ;;
    'stop')
        echo "${PREFIX_INFO} stopping ${SERVICE} server"
        killall $SERVICE
    ;;
    'restart')
        /etc/rc.d/init.d/$SERVICE.sh stop
        sleep 1
        /etc/rc.d/init.d/$SERVICE.sh start
      ;;
    *)
      echo "${PREFIX_ERRN} usage: $0 {start|stop|restart}"
      exit 1
esac

exit 0
