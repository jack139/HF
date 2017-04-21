#!/bin/bash

LOG_PATH="/usr/local/nginx/logs"

HOSTS=" \
192.168.100.5:554
192.168.100.6:554
192.168.100.7:554
192.168.100.8:554
192.168.100.9:554
192.168.100.10:554
192.168.100.11:554
192.168.100.12:554
192.168.100.13:554
192.168.100.14:554
192.168.100.15:554
192.168.100.16:554
192.168.100.17:554
192.168.100.18:554
192.168.100.19:554
192.168.100.20:554
192.168.100.21:554
192.168.100.22:554
192.168.100.23:554
192.168.100.24:554
192.168.100.25:554
192.168.100.26:554
192.168.100.27:554
192.168.100.28:554
192.168.100.29:554
192.168.100.30:554
192.168.100.31:554
192.168.100.32:554
192.168.100.33:554
192.168.100.34:554
192.168.100.35:554
192.168.100.36:554
192.168.100.37:554
192.168.100.38:554
192.168.100.39:554
192.168.100.250:80
192.168.100.250:21
192.168.100.250:2500
192.168.100.250:2501
"

TO_DATE=`date "+%Y-%m-%d %H:%M:%S"`

echo ${TO_DATE}

#echo $TO_DATE > ${LOG_PATH}/check_port

cat /dev/null > ${LOG_PATH}/check_port

for host in $HOSTS
do
	nc -z -n -w 2 -v `echo ${host//:/ }` 2>> ${LOG_PATH}/check_port
done

grep "MAX" ${LOG_PATH}/rec_*.log >> ${LOG_PATH}/check_port
grep "Max" ${LOG_PATH}/rec_*.log >> ${LOG_PATH}/check_port
grep "max" ${LOG_PATH}/rec_*.log >> ${LOG_PATH}/check_port
#grep "Fail" ${LOG_PATH}/*.log >> ${LOG_PATH}/check_port
#grep "fail" ${LOG_PATH}/*.log >> ${LOG_PATH}/check_port
grep "Error" ${LOG_PATH}/*.log >> ${LOG_PATH}/check_port
grep "error" ${LOG_PATH}/*.log >> ${LOG_PATH}/check_port
python /root/HF/test_snap.py check >> ${LOG_PATH}/check_port

if [ -s ${LOG_PATH}/check_port ]; then
	echo "send mail"
	du -hs /var/data >> ${LOG_PATH}/check_port
	df -h >> ${LOG_PATH}/check_port
	cat ${LOG_PATH}/check_port | mailx -s "HF check" 2953116@qq.com
else
	echo "not send mail"
fi
