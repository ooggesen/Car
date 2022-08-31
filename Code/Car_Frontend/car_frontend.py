import socket
import time
import sys
import keyboard


class car_control:

	def __init__(self, tcp_ip, tcp_port):
		self.command = {"right" : "SET MOTOR_R", "left" : "SET MOTOR_L", "photo_f" : "GET PHOTO_F"}
		self.tcp_port = tcp_port 
		self.tcp_ip = tcp_ip 
		self.status = {"right" : 0, "left" : 0, "end" : False}

	def get_tcp_ip(self):
		return self.tcp_ip

	def set_tcp_ip(self, tcp_ip):
		self.tcp_ip = tcp_ip 

	def set_tcp_port(self, tcp_port):
		self.tcp_port = tcp_port 

	def get_tcp_port(self):
		return self.tcp_port

	def get_status(self):
		return self.status

# ------------------------------Interface stuff------------------------------

	def update_interface(self) :
		self.keyboard_control()

	def execute(self) :
		self.wifi_send(self.command["left"] + " " + str(self.status["left"]) + "\n")
		self.wifi_send(self.command["right"] + " " + str(self.status["right"]) + "\n")
		if self.status["end"] is True:
			 self.car_socket.close()
		return self.status["end"]

	def keyboard_control(self) : 	
		if keyboard.is_pressed('up'):
			self.status["left"] = 100
			self.status["right"] = 100
		elif keyboard.is_pressed('down'):
			self.status["left"] = -100
			self.status["right"] = -100
		elif keyboard.is_pressed('right'):
			self.status["left"] = 100 #turn on the spot
			self.status["right"] = -100
		elif keyboard.is_pressed('left'):
			self.status["left"] = -100
			self.status["right"] = 100
		elif keyboard.is_pressed('esc'):
			self.status["end"] = True
		else : 
			self.status["left"] = 0
			self.status["right"] = 0

# ------------------------------Wifi stuff------------------------------

	def wifi_connect(self) :
		for i in range(0,10) : 
			try:
				s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				print("Trying to connect to {0}, port {1}, tries {2}".format(self.get_tcp_ip(), self.get_tcp_port(), i))
				s.connect((self.get_tcp_ip(), self.get_tcp_port()))
				print("Successfully connected!")
				self.car_socket = s
				return True
			except OSError :
				print("Connection failed")
				time.sleep(3)
		return False

	def wifi_send(self, string):
		#print(string)
		self.car_socket.send(string.encode('ascii'))

# -----------------------------main------------------------------

def test():
	while True:
		print(keyboard.read_key())	
	

def main() :
	#test()

	car = car_control(tcp_ip = "192.168.1.240", tcp_port = 80)
	car.wifi_connect()

	while(True) : 
		car.update_interface()	
		end = car.execute()
		time.sleep(0.1)
		if end == True :
			break 
	

if __name__ == "__main__" :
	main()
