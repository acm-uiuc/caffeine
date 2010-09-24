#!/usr/bin/python
# New Caffeine Interface

import serial, threading, MySQLdb, time, sys
from PyQt4 import QtGui,QtSvg

actually_do_serial_stuff = True
actually_do_database_stuff = True

class ClearTimeout(threading.Thread):
	def __init__(self, parent, timeout):
		threading.Thread.__init__(self)
		self.parent = parent
		self.timeout = timeout
	def run(self):
		time.sleep(self.timeout)
		self.parent.gui.disp("Welcome to Caffeine!<br />Please scan your card.")

class SerialHandler(threading.Thread):
	def __init__(self, parent):
		threading.Thread.__init__(self)
		self.parent = parent
		self.is_running = True
	def run(self):
		while self.is_running:
			# get incoming data
			# time.sleep(1)
			incoming = ""
			while self.parent.se.inWaiting() > 0:
				incoming += self.parent.se.read(1)
			# print incoming
			if len(incoming) > 0:
				print "[debug] Incoming data:", incoming
				# Process incoming
				if incoming[0] == 'D':
					try:
						buttonId = int(incoming[1:])
					except:
						print "[debug] Button down: This isn't a number..."
						continue
					print "[debug] Button down:", str(buttonId)
				elif incoming[0] == 'U':
					try:
						buttonId = int(incoming[1:])
					except:
						print "[debug] Button up: This isn't a number..."
						continue
					print "[debug] Button up:", str(buttonId)
				elif incoming[0] == 'C':
					cardData = ""
					try:
						cardData = incoming[1:]
						print "[debug] Card data:", cardData
						if cardData[0] != ';':
							print "[debug] Card is invalid (no ;)"
							self.cardError("Could not read card.<br />Try again.")
							continue
						if cardData[17] != '=':
							print "[debug] Card is invalid (17 != '=')"
							self.cardError("Could not read card.<br />Try again.")
							continue
						print "[debug] Card seems valid:", cardData[1:17]
						cardData = cardData[5:14]
					except:
						print "[debug] Bombed somewhere reading card id."
						self.cardError("Could not read card.<br />Try again.")
						continue
					print "[debug] UIN is:", cardData
					self.parent.db_integrate.query("SELECT * FROM `users` WHERE uin='%s'" % cardData)
					user_result = self.parent.db_integrate.store_result()
					user = user_result.fetch_row(how=1)
					if len(user) > 0:
						user = user[0]
					else:
						self.cardError("Card was read, but I don't know who you are.<br />Try again.")
						continue;
					self.parent.db_integrate.query("SELECT * FROM `vending` WHERE uid=%s" % user['uid'])
					vending_result = self.parent.db_integrate.store_result()
					vending = vending_result.fetch_row(how=1)
					if len(vending) > 0:
						vending = vending[0]
					else:
						self.cardError("I know who you are, but the vending databases doesn't.<br />Sorry.")
						continue
					print "You have $%.2f." % vending['balance']
					self.user = user
					self.parent.gui.disp("<b>%s %s</b><br /><b>Balance: </b>$%.2f" % (user['first_name'],user['last_name'],vending['balance']))
					# Inform the interface.
				elif incoming[0] == 'E':
					print "[debug] Card read failure (try again)"
					self.parent.gui.dispError("Could not read card.<br />Try again.")
	def cardError(self, message):
		self.dispError(message)
		self.parent.user = None
class CaffeineTool:
	def __init__(self):
		global actually_do_serial_stuff, actually_do_database_stuff
		self.serialHander = None
		self.db = None
		self.se = None
		self.gui = None
		self.user = None
		if actually_do_database_stuff:
			self.db_integrate = MySQLdb.connect("db1.acm.uiuc.edu","soda","m568EXUFS")
			self.db_integrate.select_db("acm_integrate")
			self.db_soda = MySQLdb.connect("db1.acm.uiuc.edu", "soda", "m568EXUFS")
			self.db_soda.select_db("soda")
		if actually_do_serial_stuff:
			self.se = serial.Serial('/dev/ttyUSB0',115200) # hello thar
			self.serialHandler = SerialHandler(self)
			self.serialHandler.start()
	def vend(self, tray):
		print "[debug] Sending: V" + str(tray)
		if actually_do_serial_stuff:
			self.se.write("V" + str(tray))
	def setString(self, tray, string):
		print "[debug] Sending: S" + str(tray) + string
		if actually_do_serial_stuff:
			self.se.write("S" + str(tray) + string)
	def setCharacter(self, tray, x, y, c):
		print "[debug] Sending: C" + str(tray) + str(x) + str(y) + c
		if actually_do_serial_stuff:
			self.se.write("C" + str(tray) + str(x) + str(y) + c)
	def setColor(self, tray, c, v):
		print "[debug] Sending: B" + str(tray) + c + str(v)
		if actually_do_serial_stuff:
			self.se.write("B" + str(tray) + c + str(v))
	def reset(self):
		print "[debug] Sending reset."
		if actually_do_serial_stuff:
			self.se.write("\xa0")
print "[debug] Welcome to Caffeine."
Caffeine = CaffeineTool()

class CaffeineWindow():
	def __init__(self, caffeine_instance):
		self.caffeine = caffeine_instance
		self.app = QtGui.QApplication(sys.argv)
		self.main_window = QtGui.QMainWindow()
		self.main_window.resize(640, 480)
		self.main_window.setWindowTitle('Caffeine')
		self.status = QtGui.QLabel("<center>Welcome to Caffeine!</center>")
		self.main_window.setCentralWidget(self.status)
		self.main_window.show()
		print "[debug] GUI is running."
	def dispError(self, error):
		self.status.setText("<center><span style='font-size: 24px; color: #FF0000;'>%s</span></center>" % error)
		ClearTimeout(self.caffeine, 3).start()
	def disp(self, message):
		self.status.setText("<center><span style='font-size: 24px;'>%s</span></center>" % message)
	def drawStuff(self):
		pass

print "[debug] Starting GUI..."
Caffeine.gui = CaffeineWindow(Caffeine)
Caffeine.gui.app.exec_()
print "[debug] GUI has exited, killing serial..."
Caffeine.serialHandler.is_running = False
Caffeine.serialHandler.join()
print "[debug] Caffeine is terminating."
