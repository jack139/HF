#!/bin/bash

LOG_PATH="/usr/local/nginx/logs/"
LOG_PATH_B="/usr/local/nginx/logs/backup/"
TMP_PATH="/usr/local/nginx/html/static/tmp/"

LOG_FILES=`ls $LOG_PATH*.log`

TO_DATE=`date +%Y%m%d`

rm -f $LOG_PATH_B/*

for file in $LOG_FILES
do
	cp $file $LOG_PATH_B`basename $file`.$TO_DATE
	cat /dev/null > $file
done

rm -f $TMP_PATH/download_*
