import socket
import time
import keyboard
import threading
from PIL import Image
import io
import bleak
import asyncio

class car_control:
	def __init__(self, tcp_ip, tcp_port, uuid):
		self.command = {"right" : "SET MOTOR_R", "left" : "SET MOTOR_L", "photo_1" : "GET PHOTO_1", "photo_2" : "GET PHOTO_2"}
		self.tcp_port = tcp_port 
		self.tcp_ip = tcp_ip 
		self.status = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}
		self.status_last = {"right" : 0, "left" : 0, "end" : False, "photo_1" : False, "photo_2" : False}
		self.image_count = 0
		self.uuid = uuid
		self.wifi_n_bt = True
		self.com_uuid = "38e7bf15-7170-4bc3-935a-d5cb99622342"
		self.photo_uuid = "38e7bf16-7170-4bc3-935a-d5cb99622342"

		#states: 0: idle, 1:take photo state
		self.photo_state = 0


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

	async def execute(self) :
		#print("Executing")
		#print(self.status["right"], self.status_last["right"])
		if self.status != self.status_last:
			if self.wifi_n_bt == True:
				if self.status["end"] is True:
					self.wifi_disconnect()
					return True

				self.wifi_send(self.command["left"] + " " + str(self.status["left"]) + "\n")
				self.wifi_send(self.command["right"] + " " + str(self.status["right"]) + "\n")

				if self.photo_state == 0:
					if self.status["photo_1"] is True:
						print("send photo command!\n")
						self.wifi_send(self.command["photo_1"] + "\n")
						self.daemon_image_receive()

					if self.status["photo_2"] is True:
						self.wifi_send(self.command["photo_2"] + "\n")
						self.daemon_image_receive()
			else:
				if self.status["end"] is True:#not yet tested
					await self.bt_disconnect()
					return True

				self.bt_send_com(self.command["left"] + " " + str(self.status["left"]) + "\n")
				self.wifi_send_com(self.command["right"] + " " + str(self.status["right"]) + "\n")

				if self.photo_state == 0:
					await self.car_client.stop_notify(self.photo_uuid)
					if self.status["photo_1"] is True:
						print("send photo command!\n")
						await self.bt_send_com(self.command["photo_1"] + "\n")
						await self.bt_image_receive()

				if self.status["photo_2"] is True:
						print("send photo command!\n")
						await self.bt_send_com(self.command["photo_2"] + "\n")
						await self.bt_image_receive()

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

# ------------------------------Wifi stuff------------------------------
	def wifi_init(self):
		assert(self.tcp_ip is not None)
		assert(self.tcp_port is not None)
		self.wifi_n_bt = True

		try:
			print("Address: ", self.car_socket.AF_INET)
		except AttributeError:
			self.wifi_connect()
			print("Address: ", self.car_socket.AF_INET)

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
			except Exception as e :
				print(e)
				time.sleep(1)
		raise OSError("Could not connect to device")

	def wifi_disconnect(self) :
		self.car_socket.close()

	def wifi_send(self, string):
		#print(string)
		self.car_socket.send(string.encode('ascii'))

	def daemon_image_receive(self):
		x = threading.Thread(target=self.wifi_image_receive, args= (), daemon=True)
		x.start()

	def wifi_image_receive(self) :
		#For recption of photos only
		try:
			self.photo_state = 1
			start = time.time()
			length = self.car_socket.recv(32)
			length = int.from_bytes(length, byteorder='little', signed=False)
			print("Size of image in bytes: ", length)

			buffer = bytearray()
			while True:
				buffer.extend(self.car_socket.recv(4096))
				if buffer[-1] == 0xd9 and buffer[-2] == 0xff:
					break

			pil_image = Image.open(io.BytesIO(buffer))
			pil_image.save("{}.jpeg".format(self.image_count), "JPEG")
			self.image_count+=1
			print("Received photo!")
			print("In {} s".format(time.time()-start))
		except socket.timeout:
			print("Error in reception!")
			print(buffer.hex())
			# self.wifi_disconnect()
			# raise OSError("Transmission failure!")
		finally:
			self.photo_state = 0
# -----------------------------bluetooth stuff ----------------------------
	def photo_callback(self, sender, data):
		print(sender) #not yet tested
		self.photo_buffer.extend(data)
		if self.photo_buffer[-1] == 0xd9 and self.photo_buffer[-2] == 0xff:
			pil_image = Image.open(io.BytesIO(self.photo_buffer))
			pil_image.save("{}.jpeg".format(self.image_count), "JPEG")
			self.image_count+=1
			print("Received photo!")
			self.photo_buffer = bytearray()
			

	async def bluetooth_init(self):
		assert(self.uuid is not None)#not yet tested
		self.wifi_n_bt = False
		self.photo_buffer = bytearray()
		
		try:
			await self.bt_discover()
			self.bt_print_info()
		except:
			await self.bt_connect()
			self.bt_print_info()
	
	async def bt_connect(self):
		for i in range(0,5) : 
			try:
				print(f"Connecting to Bluetooth client, UUID: {self.uuid}")
				self.car_client = bleak.BleakClient(self.uuid)
				await self.car_client.connect()
				return
			except Exception as e :
				print(e)
				time.sleep(1)
		raise OSError("Could not connect to device")

	def bt_print_info(self):
		print("Services: ")
		for service in self.car_client.services:
			print(service)
			for char in service.characteristics:
				print(char)
	
	async def bt_disconnect(self):
		await self.car_client.disconnect()
	
	async def bt_discover(self):
		print("scanning for 5 seconds, please wait...")

		devices = await bleak.BleakScanner.discover(return_adv=True)

		for d, a in devices.values():
			print()
			print(d)
			print("-" * len(str(d)))
			print(a)

	async def bt_send_com(self, command):
		await self.car_client.write_gatt_char(self.com_uuid, command.encode("ascii"))#TODO

	async def bt_receive_com(self):
		message_b = await self.car_client.read_gatt_char(self.com_uuid)
		message = message_b.decode("ascii")
		return message

	async def bt_image_receive(self):
		length_b = await self.car_client.read_gatt_char(self.photo_uuid) #TODO
		length = int.from_bytes(length_b, byteorder='little', signed=False)
		print("Size of image in bytes: ", length)

		await self.car_client.start_notify(self.photo_uuid, self.callback())



# -----------------------------main------------------------------
	def main_wifi(self) :
		self.wifi_init()
		self.main()
 
	
	async def main_bluetooth(self):
		await self.bluetooth_init()
		self.main()

	def main(self):
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

	car = car_control(tcp_ip = "192.168.1.240", tcp_port = 80, uuid="FE201AB6-FAC6-1203-F10C-395C478DDC0E")
	# car.wifi_connect()
	asyncio.run(car.main_bluetooth())
	# car.main_wifi()
	

if __name__ == "__main__" :
	main()
	#test()