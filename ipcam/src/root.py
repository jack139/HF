#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# yum install smartmontools
#

from os import system
import os
import locale
import curses
import time

def get_param(prompt_string):
	try:
		screen.clear()
		screen.border(0)
		screen.addstr(2, 2, prompt_string)
		screen.refresh()
		input = screen.getstr(4, 2, 60)
	except KeyboardInterrupt:
		input = ''
	return input

def execute_cmd(cmd_string, secs=0.1):
	system("clear")
	if len(cmd_string)>0:
		for cmd0 in cmd_string:
			system(cmd0)
	print ""
	time.sleep(secs)
	raw_input("[ Press ENTER back to menu ... ]")

if __name__ == "__main__":
	
	x = 0

	while x != ord('7'):
		try:		
			screen = curses.initscr()
	
			screen.clear()
			screen.border(0)
			screen.addstr(2, 2, "Please choose a command [1-6]:")
			screen.addstr(4, 4, "1 - Show service status")
			screen.addstr(5, 4, "2 - Show disk space")
			screen.addstr(6, 4, "3 - Show S.M.A.R.T. health infomation")
			screen.addstr(7, 4, "4 - Restart service")
			screen.addstr(8, 4, "5 - Reboot host")
			screen.addstr(9, 4, "6 - Open a shell")
			screen.addstr(10, 4, "7 - Exit")
			screen.addstr(12, 3, " ")
			screen.refresh()
	
			x = screen.getch()
	
			if x == ord('1'):
				curses.endwin()
				execute_cmd(["ps -f -C uwsgi -C rec -C streaming -C nginx -C python -C sync_kam"])
			if x == ord('2'):
				curses.endwin()
				execute_cmd(["df -h"])
			if x == ord('3'):
				curses.endwin()
				execute_cmd([
					"smartctl -H /dev/sda|grep -v Copyright|grep -v linux", 
					"smartctl -H /dev/sdb|grep -v Copyright|grep -v linux",
					"smartctl -H /dev/sdc|grep -v Copyright|grep -v linux", 
					"smartctl -H /dev/sdd|grep -v Copyright|grep -v linux",
					"smartctl -H /dev/sde|grep -v Copyright|grep -v linux", 
					"smartctl -H /dev/sdf|grep -v Copyright|grep -v linux"])
			if x == ord('4'):
				re = get_param("Are sure to restart all service? [yes/No]")
				if re=='yes':
					curses.endwin()
					execute_cmd(["/root/my_server restart"], 2)
			if x == ord('5'):
				re = get_param("Are sure to reboot the host? [yes/No]")
				if re=='yes':
					re = get_param("Do you know this action will stop all service for several minutes? [I do/...]")
					if re=='I do':
						curses.endwin()
						execute_cmd(["reboot"])
			if x == ord('6'):
				re = get_param("Do you know how to use shell? [yes/No]")
				if re=='yes':
					re = get_param("Do you know you have the ROOT privilege to DESTROY ANYTHING? [I DO/...]")
					if re=='I DO':
						curses.endwin()
						execute_cmd(["bash"])
		except KeyboardInterrupt:
			None

	curses.endwin()

