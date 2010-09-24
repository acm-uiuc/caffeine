#!/usr/bin/python
# New Caffeine Interface

import serial, threading, MySQLdb, time, sys
from PyQt4 import QtGui,QtSvg,QtCore


# System states
class State():
	Initializing = -1
	Waiting = 0
	Authenticated = 1
	Confirm = 2
	Vended = 3

class User():
	def __init__(self, db_user, vending):
		self.id = db_user['uid']
		self.uin = db_user['uin']
		self.name = db_user['first_name'] + ' ' + db_user['last_name']
		self.balance = vending['balance']
		# ... other vending information

class SerialDevice():
	def __init__(self):
		try:
			self.real = serial.Serial('/dev/ttyUSB0', 115200)
			self.use_real = True
		except:
			print "[debug] Serial failure. Enabling simulation."
			self.use_real = False
	def read(self):
		# read
		if self.use_real:
			incoming = ""
			while self.real.inWaiting() > 0:
				incoming += self.real.read(1)
			return incoming
		else:
			incoming = raw_input(":")
			return incoming
	def write(self, data):
		# write
		if self.use_real:
			self.real.write(data)
		else:
			print "[simulator]", data


# ClearTimeout(caeffineInstance, timeout).start() ...
# Clear the screen and reset the state.
class ClearTimeout(threading.Thread):
	def __init__(self, parent, timeout):
		threading.Thread.__init__(self)
		self.parent = parent
		self.timeout = timeout
	def run(self):
		time.sleep(self.timeout)
		self.parent.state = State.Waiting
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
			incoming = self.parent.se.read()
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
					self.parent.buttonPress(buttonId)
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
					self.parent.user = User(user, vending)
					self.parent.gui.disp("<b>%s %s</b><br /><b>Balance: </b>$%.2f" % (user['first_name'],user['last_name'],vending['balance']))
					# Inform the interface.
				elif incoming[0] == 'E':
					print "[debug] Card read failure (try again)"
					self.parent.gui.dispError("Could not read card.<br />Try again.")
	def cardError(self, message):
		self.parent.gui.dispError(message)
		self.parent.user = None
class CaffeineTool:
	def __init__(self):
		self.serialHander = None
		self.db = None
		self.se = SerialDevice()
		self.gui = None
		self.user = None
		self.state = State.Initializing
		self.button = -1
		self.db_integrate = MySQLdb.connect("db1.acm.uiuc.edu","soda","m568EXUFS")
		self.db_integrate.select_db("acm_integrate")
		self.db_soda = MySQLdb.connect("db1.acm.uiuc.edu", "soda", "m568EXUFS")
		self.db_soda.select_db("soda")
		self.serialHandler = SerialHandler(self)
		self.serialHandler.start()
		self.state = State.Waiting
	def buttonPress(self, button):
		print "[debug] Button %d was pressed." % button
		if self.state == State.Waiting:
			print "[debug] Ignorning (not ready for a button press)"
		elif self.state == State.Authenticated:
			print "[debug] Users wants to vend from tray %d." % button
			self.gui.disp("Press button again to vend $TRAY_CONTENTS")
			self.state = State.Confirm
			self.button = button
		elif self.state == State.Confirm:
			if self.button == button:
				print "[debug] User has confirmed, vend tray %d." % button
				self.gui.disp("Vending $TRAY_CONTENTS...")
				self.state = Self.Vended
				self.vend(button)
			else:
				print "[debug] User has changed request to tray %d." % button
				self.gui.disp("Press button again to vend $TRAY_CONTENTS")
				self.state = State.Confirm
				self.button = button
	def vend(self, tray):
		print "[debug] Sending: V" + str(tray)
		self.se.write("V" + str(tray))
	def setString(self, tray, string):
		print "[debug] Sending: S" + str(tray) + string
		self.se.write("S" + str(tray) + string)
	def setCharacter(self, tray, x, y, c):
		print "[debug] Sending: C" + str(tray) + str(x) + str(y) + c
		self.se.write("C" + str(tray) + str(x) + str(y) + c)
	def setColor(self, tray, c, v):
		print "[debug] Sending: B" + str(tray) + c + str(v)
		self.se.write("B" + str(tray) + c + str(v))
	def reset(self):
		print "[debug] Sending reset."
		self.se.write("\xa0")
print "[debug] Welcome to Caffeine."
Caffeine = CaffeineTool()

class CaffeineWindow():
	def __init__(self, caffeine_instance):
		self.caffeine = caffeine_instance
		self.app = QtGui.QApplication(sys.argv)
		self.main_window = QtGui.QMainWindow()
		self.main_window.resize(1280,1024)
		self.main_window.setWindowTitle('Caffeine')
		self.status = QtGui.QLabel("<center>Initializing...</center>")
		self.closeButton = QtGui.QPushButton("*DEBUG* Exit GUI *DEBUG*")
		self.app.connect(self.closeButton, QtCore.SIGNAL('clicked()'), self.closeButton_pressed)
		self.vbox = QtGui.QVBoxLayout()
		self.vbox.addWidget(self.status)
		self.vbox.addWidget(self.closeButton)
		self.cwidget = QtGui.QWidget()
		self.cwidget.setLayout(self.vbox)
		self.main_window.setCentralWidget(self.cwidget)
		self.main_window.show()
		ClearTimeout(self.caffeine, 1).start()
		print "[debug] GUI is running."
	def closeButton_pressed(self):
		print "[debug] Exiting!"
		self.app.exit()
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
