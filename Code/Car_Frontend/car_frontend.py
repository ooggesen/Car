import socket
import time
import sys
import keyboard
import threading
from PIL import Image
import io

class Receive_Thread(threading.Thread):
   def __init__(self, socket, buffer_size):
      threading.Thread.__init__(self)
      self.buffer_size = buffer_size
      self.socker = socket
   def run(self):
      print ("Starting " + self.name)
      print_time(self.name, self.counter, 5)
      print ("Exiting " + self.name)

class car_control:
	def __init__(self, tcp_ip, tcp_port):
		self.command = {"right" : "SET MOTOR_R", "left" : "SET MOTOR_L", "photo_1" : "GET PHOTO_1", "photo_2" : "GET PHOTO_2"}
		self.tcp_port = tcp_port 
		self.tcp_ip = tcp_ip 
		self.status = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}
		self.status_last = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}

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
		#print("Executing")
		end_temp = self.status["end"]
		#print(self.status["right"], self.status_last["right"])
		if self.status != self.status_last:
			self.wifi_send(self.command["left"] + " " + str(self.status["left"]) + "\n")
			self.wifi_send(self.command["right"] + " " + str(self.status["right"]) + "\n")

			if self.status["photo_1"] is True :
				print("send photo command!\n")
				self.wifi_send(self.command["photo_1"] + "\n")
				self.wifi_receive()
				print("Received photo!")
				self.status["photo_1"] = False
			if self.status["photo_2"] is True :
				self.wifi_send(self.command["photo_2"] + "\n")
				self.wifi_receive()
				self.status["photo_2"] = False

			if self.status["end"] is True:
				 self.wifi_disconnect()
				 end = False

		self.status_last = self.status.copy()
		return end_temp

	def keyboard_control(self) : 
		self.status["left"] = 0
		self.status["right"] = 0
		self.status["photo_1"] = False
		self.status["photo_2"] = False
		self.status["end"] = False

		if keyboard.is_pressed('up') and keyboard.is_pressed('right'):
			self.status["left"] = 100
			self.status["right"] = 50
		elif keyboard.is_pressed('up') and keyboard.is_pressed('left'):
			self.status["left"] = 100
			self.status["right"] = 50
		elif keyboard.is_pressed('up'):
			self.status["left"] = 100
			self.status["right"] = 100
		elif keyboard.is_pressed('down') and keyboard.is_pressed('right'):
			self.status["left"] = -100
			self.status["right"] = -50
		elif keyboard.is_pressed('down') and keyboard.is_pressed('left'):
			self.status["left"] = -100
			self.status["right"] = -50
		elif keyboard.is_pressed('down'):
			self.status["left"] = -100
			self.status["right"] = -100
		elif keyboard.is_pressed('right'):
			self.status["left"] = 100 #turn on the spot
			self.status["right"] = -100
		elif keyboard.is_pressed('left'):
			self.status["left"] = -100
			self.status["right"] = 100
		elif keyboard.is_pressed('space'):
			self.status["photo_1"] = True
		elif keyboard.is_pressed('esc'):
			self.status["end"] = True

# ------------------------------Wifi stuff------------------------------

	def wifi_connect(self) :
		for i in range(0,5) : 
			try:
				s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				print("Trying to connect to {0}, port {1}, {2} try".format(self.get_tcp_ip(), self.get_tcp_port(), i+1))
				s.settimeout(10)
				s.connect((self.get_tcp_ip(), self.get_tcp_port()))
				print("Successfully connected!")
				self.car_socket = s
				return True
			except OSError :
				print("Connection failed")
				time.sleep(1)
		raise OSError("Could not connect to device")

	def wifi_disconnect(self) :
		self.car_socket.close()

	def wifi_send(self, string):
		#print(string)
		self.car_socket.send(string.encode('ascii'))

	def wifi_receive(self) :
		#For recption of photos only
		#TODO not yet tested
		try:
			length = self.car_socket.recv(32)#, socket.MSG_WAITALL)
			#print(length.hex())
			length = int.from_bytes(length, byteorder='little', signed=False)
			print("Size of image: ", length)

			self.car_socket.settimeout(None)
			buffer = self.car_socket.recv(length, socket.MSG_WAITALL)
			self.car_socket.settimeout(10)
			#print(buffer.hex())
			pil_image = Image.open(io.BytesIO(buffer))#, formats="JPEG")
			pil_image.save("Test.jpeg", "JPEG")
			return buffer
		except:
			self.wifi_disconnect()
			raise OSError("Transmission failure!")

# -----------------------------main------------------------------
	def main(self) :
		assert(self.tcp_ip is not None)
		assert(self.tcp_port is not None)

		try:
			print("Address: ", self.car_socket.AF_INET)
		except AttributeError:
			self.wifi_connect()

		while(True) : 
			#print("Loop")
			self.update_interface()	
			end = self.execute()
			time.sleep(0.001)
			if end == True :
				break 
		



def test():
	while True:
		print(keyboard.read_key())	
	

def main() :
	#test()

	car = car_control(tcp_ip = "192.168.1.240", tcp_port = 80)
	#car.wifi_connect()
	car.main()
	

if __name__ == "__main__" :
	main()
	#test()
