#!/usr/bin/python
# New Caffeine Interface

import serial, threading, MySQLdb, time, sys
from PyQt4 import QtGui,QtSvg,QtCore
import subprocess, signal, datetime, os

sys.stdout = open("debug.out", 'a')
#sys.stderr = open("debug.err", 'a')

signal.signal(signal.SIGINT, signal.SIG_DFL)

os.putenv("DISPLAY",":0")

def debug(output):
	print >>sys.stdout, output
	sys.stdout.flush()
def error(output):
	print >>sys.stderr, output
	sys.stderr.flush()

debug("** Log Beginning %s **" % str(datetime.datetime.now()))
error("** Log Beginning %s **" % str(datetime.datetime.now()))

# System states
class State():
	Initializing = -1
	Waiting = 0
	Authenticated = 1
	Confirm = 2
	Vended = 3
	Blocking = 4

# User handling
class User():
	def __init__(self, db_user, vending):
		self.id = db_user['uid']
		self.uin = db_user['uin']
		self.name = db_user['first_name'] + ' ' + db_user['last_name']
		self.balance = vending['balance']
		# ... other vending information

# Serial device wrapper
class SerialDevice():
	def __init__(self):
		while True:
			try:
				self.real = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)
				debug("[debug] Attached to ttyUSB0")
				self.use_real = True
				return
			except:
				try:
					self.real = serial.Serial('/dev/ttyUSB1', 115200, timeout=0.1)
					debug("[debug] Attached to ttyUSB1")
					self.use_real = True
					return
				except:
					error("No serial devices available right now, waiting a few seconds.")
					time.sleep(2)
	def read(self):
		# read
		if self.use_real:
			incoming = self.real.read(255)
			return incoming
		else:
			incoming = raw_input(":")
			return incoming
	def write(self, data):
		# write
		if self.use_real:
			while True:
				try:
					self.real.write(data)
					return True
				except:
					print >>sys.stderr, "[error] Failed on write, trying to restart."
					self = SerialDevice()
		else:
			debug("[simulator] %s" % data)

# Structure for tray content, combines tray entry and sodas
class TrayContent():
	def __init__(self, tray, soda):
		self.name = soda['name']
		self.quantity = tray['qty']
		self.price = tray['price']
		self.soda_id = tray['sid']
		self.calories = soda['calories']
		self.caffeine = soda['caffeine']
		self.dispensed = soda['dispensed']

# ClearTimeout(caeffineInstance, timeout).start() ...
# Clear the screen and reset the state.
class ClearTimeout(threading.Thread):
	def __init__(self, parent, timeout):
		threading.Thread.__init__(self)
		self.parent = parent
		self.timeout = timeout
	def run(self):
		self.parent.state = State.Blocking
		time.sleep(self.timeout)
		self.parent.state = State.Waiting
		self.parent.setupTrays()
		fortune = subprocess.Popen(["/usr/games/fortune"], stdout=subprocess.PIPE).communicate()[0].replace("\n", "<br />")
		self.parent.gui.disp("Welcome to Caffeine!<br />Please scan your card.<br /><span style='font-size: 14px;'><i>%s</i></span>" % fortune)

# Serial handling thread
class SerialHandler(threading.Thread):
	def __init__(self, parent):
		threading.Thread.__init__(self)
		self.parent = parent
		self.is_running = True
	def run(self):
		while self.is_running:
			# get incoming data
			try:
				t_before = datetime.datetime.now()
				incoming = self.parent.se.read()
				t_after = datetime.datetime.now()
				if (t_after - t_before).microseconds < 1000:
					error("ERROR: I think we lost serial!")
					sys.exit(2)
					self.parent.se = SerialDevice()
			except SystemExit:
				error("EXCEPTION: SYSTEMEXIT RAISED!")
				break
			except:
				error("Failed to read from serial (exception raised). Restarting it.")
				self.parent.se = SerialDevice()
				continue
			# print incoming
			if len(incoming) > 0:
				error("[debug] Incoming data: %s" % incoming)
				debug("[debug] Incoming data: %s" % incoming)
				# Process incoming
				if incoming[0] == 'D':
					try:
						buttonId = int(incoming[1:])
					except:
						debug("[debug] Button down: This isn't a number...")
						continue
					debug("[debug] Button down: %s" % str(buttonId))
					if buttonId == 0:
						self.parent.cancelTransaction()
					self.parent.buttonPress(buttonId)
				elif incoming[0] == 'U':
					try:
						buttonId = int(incoming[1:])
					except:
						debug("[debug] Button up: This isn't a number...")
						continue
					debug( "[debug] Button up: %s" % str(buttonId))
				elif incoming[0] == 'C':
					if self.parent.state is not State.Waiting:
						debug("[debug] Ignoring card read is not waiting for one.")
						continue
					cardData = ""
					try:
						cardData = incoming[1:]
						debug( "[debug] Card data: %s" % cardData)
						if cardData[0] != ';':
							debug( "[debug] Card is invalid (no ;)" )
							self.cardError("Could not read card.<br />Try again.")
							continue
						if cardData[17] != '=':
							debug( "[debug] Card is invalid (17 != '=')" )
							self.cardError("Could not read card.<br />Try again.")
							continue
						debug("[debug] Card seems valid: %s" % cardData[1:17])
						cardData = cardData[5:14]
					except:
						debug("[debug] Bombed somewhere reading card id.")
						self.cardError("Could not read card.<br />Try again.")
						continue
					debug( "[debug] UIN is: %s" % cardData)
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
					debug("[debug] User has $%.2f." % vending['balance'])
					self.parent.user = User(user, vending)
					self.parent.gui.disp("<b>%s %s</b><br /><b>Balance: </b>$%.2f" % (user['first_name'],user['last_name'],vending['balance']))
					self.parent.state = State.Authenticated
					# Inform the interface.
				elif incoming[0] == 'E':
					debug( "[debug] Card read failure (try again)")
					self.parent.gui.dispError("Could not read card.<br />Try again.")
		error("Serial Handler loop has exited, this should not happen, KILL EVERYTHING!")
		self.parent.gui.app.quit()
	def cardError(self, message):
		self.parent.gui.dispError(message)
		self.parent.user = None

# Interface object
class CaffeineTool:
	def __init__(self):
		self.serialHander = None
		self.db = None
		self.gui = None
		self.user = None
		self.state = State.Initializing
		self.button = -1
		debug("[debug] Connecting to databases.")
		self.db_integrate = MySQLdb.connect("db1.acm.uiuc.edu","soda","m568EXUFS")
		self.db_integrate.select_db("acm_integrate")
		self.db_soda = MySQLdb.connect("db1.acm.uiuc.edu", "soda", "m568EXUFS")
		self.db_soda.select_db("soda")
		self.db_integrate.query("SET TRANSACTION ISOLATION LEVEL READ COMMITTED")
		self.db_soda.query("SET TRANSACTION ISOLATION LEVEL READ COMMITTED")
		debug( "[debug] Starting serial handling.")
		self.se = SerialDevice()
		self.serialHandler = SerialHandler(self)
		self.serialHandler.start()
		debug( "[debug] Ready to go.")
		self.state = State.Waiting
		self.tray_contents = None
	def buttonPress(self, button):
		debug( "[debug] Button %d was pressed." % button)
		if self.state == State.Blocking:
			debug( "[debug] System is blocking to clear, ignoring.")
		if self.state == State.Waiting:
			debug( "[debug] Ignorning (not ready for a button press)")
		elif self.state == State.Authenticated:
			debug( "[debug] Users wants to vend from tray %d." % button)
			self.button = button
			self.db_soda.query("SELECT * FROM `trays` WHERE tid=%s" % button)
			tray_result = self.db_soda.store_result()
			try:
				tray = tray_result.fetch_row(how=1)[0]
			except:
				self.state = State.Authenticated
				self.gui.disp("... What?")
				return
			self.db_soda.query("SELECT * FROM `sodas` WHERE sid=%d" % int(tray['sid']))
			soda_result = self.db_soda.store_result()
			soda = soda_result.fetch_row(how=1)[0]
			self.trayContents = TrayContent(tray,soda)
			#print soda
			debug( self.trayContents.quantity)
			if (self.trayContents.quantity < 1):
				self.gui.disp("This slot is empty.<br />It used to be %s.<br />Select another item." % self.trayContents.name)
				self.state = State.Authenticated
			elif self.user.balance < self.trayContents.price:
				self.gui.disp("You have selected slot %d.<br />This slot contains %s.<br />You can not afford this item." % (button, self.trayContents.name))
			else:
				self.state = State.Confirm
				self.gui.disp("You have selected slot %d.<br />This slot contains %s.<br />Press button %d again to vend." % (button, self.trayContents.name, button))
		elif self.state == State.Confirm:
			if self.button == button:
				debug( "[debug] User has confirmed, vend tray %d." % button)
				self.gui.disp("Vending %s.<br />Thank you for using Caffeine.<br />Please open your can slowly!" % self.trayContents.name)
				self.state = State.Vended
				self.charge(self.trayContents.price)
				self.vend(button)
				ClearTimeout(self,5).start()
			else:
				debug( "[debug] User has changed request to tray %d." % button)
				self.state = State.Authenticated
				self.buttonPress(button)
		elif self.state == State.Vended:
			self.state = State.Authenticated
			self.buttonPress(button)
	def charge(self, amount):
		debug( "[debug] Charging user %s $%.2f ($%.2f -> $%.2f)" % (self.user.name, amount, self.user.balance, self.user.balance - amount))
		# First add the transaction to vending_transactions
		self.db_integrate.query("INSERT INTO `vending_transactions` VALUES (NULL, NULL, %d, %.2f, -1)" % (self.user.id, amount))
		debug( "INSERT INTO `vending_transactions` VALUES (NULL, NULL, %d, %.2f, -1)" % (self.user.id, amount))
		self.db_integrate.query("UPDATE `vending` SET `balance`=%.2f WHERE `uid`=%d" % (self.user.balance - amount, self.user.id))
		self.db_integrate.commit()
	def vend(self, tray):
		debug( "[debug] Sending: V" + str(tray))
		# Update tray amounts.
		self.db_soda.query("SELECT * FROM `trays` WHERE `tid`=%d" % tray)
		dbtray = self.db_soda.store_result().fetch_row(how=1)[0]
		self.db_soda.query("UPDATE `trays` SET `qty`=%d WHERE `tid`=%d" % (dbtray['qty'] - 1, tray))
		self.db_soda.commit()
		self.setupTrays()
		# Vend the item
		self.se.write("V" + str(tray))
	def setString(self, tray, string):
		debug( "[debug] Sending: S" + str(tray) + string)
		self.se.write("S" + str(tray) + string)
	def setCharacter(self, tray, x, y, c):
		debug( "[debug] Sending: C" + str(tray) + str(x) + str(y) + c)
		self.se.write("C" + str(tray) + str(x) + str(y) + c)
	def setColor(self, tray, c, v):
		debug( "[debug] Sending: B" + str(tray) + c + str(v))
		self.se.write("B" + str(tray) + c + str(v))
	def reset(self):
		debug( "[debug] Sending reset.")
		self.se.write("\xa0")
	def cancelTransaction(self):
		self.user = None
		if not self.state == State.Waiting:
			self.state = State.Waiting
			self.gui.disp("Transaction cancelled, you have been signed out.<br />Thank you for using Caffeine.")
			ClearTimeout(self, 2).start()
	def setupTrays(self):
		# In the future, this will set LCD text
		self.trays = []
		self.db_soda.query("SELECT * FROM `trays`")
		trayh = self.db_soda.store_result()
		for i in xrange(9):
			tray = trayh.fetch_row(how=1)[0]
			self.db_soda.query("SELECT * FROM `sodas` WHERE sid=%d" % int(tray['sid']))
			soda_result = self.db_soda.store_result()
			soda = soda_result.fetch_row(how=1)[0]
			self.trays.append(TrayContent(tray,soda))
			self.gui.setTrayLabel(i, "Tray %d<br />%s<br />Price: $%.2f<br />Calories: %d<br />Caffeine: %d<br />Quantity: %d" % (i, self.trays[i].name, self.trays[i].price, self.trays[i].calories, self.trays[i].caffeine, self.trays[i].quantity), self.trays[i].quantity)
debug( "[debug] Welcome to Caffeine.")
Caffeine = CaffeineTool()

# GUI (not actually a window)
class CaffeineWindow():
	def __init__(self, caffeine_instance):
		self.caffeine = caffeine_instance
		self.caffeine.gui = self
		self.app = QtGui.QApplication(sys.argv)
		self.main_window = QtGui.QMainWindow()
		self.main_window.resize(1280,1024)
		self.main_window.setWindowTitle('Caffeine')
		self.status = QtGui.QLabel("<center>Initializing...</center>")
		self.cancel = QtGui.QPushButton("Cancel")
		self.header = QtGui.QLabel("<center><img src='caffeine.png' /></center>")
		self.header.setMaximumHeight(142)
		self.vbox = QtGui.QVBoxLayout()
		self.vbox.addWidget(self.header)
		self.vbox.addWidget(self.status)
		self.vbox.setSpacing(0)
		self.main_window.setContentsMargins(0,0,0,0)
		self.hbox = QtGui.QHBoxLayout()
		self.itemLabels = []
		for i in xrange(9):
			self.itemLabels.append(QtGui.QLabel("Tray %d" % i))
			self.hbox.addWidget(self.itemLabels[i])
		self.trayWidget = QtGui.QWidget()
		self.trayWidget.setLayout(self.hbox)
		self.trayWidget.setMaximumHeight(150)
		self.vbox.addWidget(self.trayWidget)
		# self.vbox.addWidget(self.cancel)
		self.main_window.connect(self.cancel, QtCore.SIGNAL("clicked()"), self.caffeine.cancelTransaction)
		self.cwidget = QtGui.QWidget()
		self.cwidget.setLayout(self.vbox)
		self.main_window.setCentralWidget(self.cwidget)
		self.main_window.show()
		self.caffeine.setupTrays()
		ClearTimeout(self.caffeine, 1).start()
		debug( "[debug] GUI is running.")
	def closeButton_pressed(self):
		debug( "[debug] Exiting!")
		self.main_window.hide()
		self.app.exit()
	def dispError(self, error):
		self.status.setText("<center><span style='font-size: 24px; color: #FF0000;'>%s</span></center>" % error)
		ClearTimeout(self.caffeine, 1).start()
	def disp(self, message):
		self.status.setText("<center><span style='font-size: 24px;'>%s</span></center>" % message)
	def drawStuff(self):
		pass
	def setTrayLabel(self, tray, label, quantity):
		if quantity == 0:
			self.itemLabels[tray].setText("<center><span style='font-size: 12px; font-weight: bold; color: #FF0000'>%s</span></center>" % label)
		else:
			self.itemLabels[tray].setText("<center><span style='font-size: 12px; font-weight: bold;'>%s</span></center>" % label)

debug( "[debug] Starting GUI...")
CaffeineWindow(Caffeine).app.exec_()
debug( "[debug] GUI has exited, killing serial...")
Caffeine.serialHandler.is_running = False
if (not Caffeine.se.use_real):
	debug( "[debug/simulator] Press enter to kill the simulator.")
Caffeine.serialHandler.join()
debug( "[debug] Caffeine is terminating.")
