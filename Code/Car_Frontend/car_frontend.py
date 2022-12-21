import time
import keyboard
from PIL import Image
import io
import bleak
import asyncio
from bt_client import *
from wifi_socket import *

"""
Need to test wifi functions at home.
Bt functions not completed yet
"""

class car_control:
	def __init__(self, tcp_ip, tcp_port):
		self.command = {"right" : "SET MOTOR_R", "left" : "SET MOTOR_L", "photo_1" : "GET PHOTO_1", "photo_2" : "GET PHOTO_2"}
		self.tcp_port = tcp_port 
		self.tcp_ip = tcp_ip 
		self.status = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}
		self.status_last = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}
		# self.image_count = 0
		self.wifi_n_bt = True

# ------------------------------Interface stuff------------------------------

	def update_interface(self) :
		self.keyboard_control()

	async def execute(self) :
		#print("Executing")
		#print(self.status["right"], self.status_last["right"])
		if self.status != self.status_last:
			if self.wifi_n_bt == True:

				if self.status["end"] is True:
					self.wifi_socket.disconnect()
					return True

				self.wifi_socket.send(self.command["left"] + " " + str(self.status["left"]) + "\n")
				self.wifi_socket.send(self.command["right"] + " " + str(self.status["right"]) + "\n")

				if self.status["photo_1"] is True:
					print("send photo command!\n")
					self.wifi_socket.send(self.command["photo_1"] + "\n")
					self.wifi_socket.daemon_image_receive()

				if self.status["photo_2"] is True:
					self.wifi_socket.send(self.command["photo_2"] + "\n")
					self.wifi_socket.daemon_image_receive()
			else:
				if self.status["end"] is True:#not yet tested
					await self.bt_client.disconnect()
					return True

				await self.bt_client.send_com(self.command["left"] + " " + str(self.status["left"]) + "\n")
				await self.bt_client.send_com(self.command["right"] + " " + str(self.status["right"]) + "\n")

				if self.status["photo_1"] is True:
					print("send photo command!\n")
					await self.bt_client.send_com(self.command["photo_1"] + "\n")
					await self.bt_client.image_receive()

				if self.status["photo_2"] is True:
					print("send photo command!\n")
					await self.bt_client.send_com(self.command["photo_2"] + "\n")
					await self.bt_client.image_receive()

		self.status_last = self.status.copy()
		return False

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


# -----------------------------main------------------------------
	async def main_wifi(self) :
		self.wifi_n_bt = True
		self.wifi_socket = wifi_socket(self.tcp_ip, self.tcp_port)
		self.wifi_socket.init()
		await self.main()
 
	
	async def main_bluetooth(self):
		self.wifi_n_bt = False
		self.bt_client = bt_client()
		await self.bt_client.init()
		await self.main()

	async def main(self):
		while(True) : 
			#print("Loop")
			self.update_interface()	
			end = await self.execute()
			time.sleep(0.001)
			if end == True :
				break

		
def test():
	while True:
		print(keyboard.read_key())	
	

def main() :
	#test()

	car = car_control(tcp_ip = "192.168.1.240", tcp_port = 80)
	# car.wifi_connect()
	asyncio.run(car.main_bluetooth())
	# car.main_wifi()
	

if __name__ == "__main__" :
	main()
	#test()