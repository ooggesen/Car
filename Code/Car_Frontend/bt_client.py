from PIL import Image
import io
import bleak
import asyncio
import time

class bt_client:
    def __init__(self) -> None:
        self.car_uuid="FE201AB6-FAC6-1203-F10C-395C478DDC0E"
        self.com_uuid = "38e7bf15-7170-4bc3-935a-d5cb99622342"
        self.photo_uuid = "38e7bf16-7170-4bc3-935a-d5cb99622342"

    def photo_callback(self, sender:int, data:bytearray):
        print("Received photo information")
        print(sender) #not yet tested
        print(data)
        self.photo_buffer.extend(data)
        if self.photo_buffer[-1] == 0xd9 and self.photo_buffer[-2] == 0xff:
            pil_image = Image.open(io.BytesIO(self.photo_buffer))
            pil_image.save("{}.jpeg".format(self.image_count), "JPEG")
            self.image_count+=1
            print("Received photo!")
            self.photo_buffer = bytearray()
            

    async def init(self):
        self.wifi_n_bt = False
        self.photo_buffer = bytearray()
        
        await self.discover()
        await self.connect()
        self.print_info()
        await self.car_client.start_notify("38e7bf16-7170-4bc3-935a-d5cb99622342", self.photo_callback)
    
    async def connect(self):
        print()
        for i in range(0,5) : 
            try:
                print(f"Connecting to Bluetooth client, UUID: {self.car_uuid}")
                self.car_client = bleak.BleakClient(self.car_uuid)
                await self.car_client.connect()
                return
            except Exception as e :
                print(e)
                time.sleep(1)
        raise OSError("Could not connect to device")

    def print_info(self):
        print()
        print("Services: ")
        for service in self.car_client.services:
            print(service)
            for char in service.characteristics:
                print(char)
        print()
    
    async def disconnect(self):
        await self.car_client.disconnect()
    
    async def discover(self):
        print("scanning for 5 seconds, please wait...")

        devices = await bleak.BleakScanner.discover(return_adv=True)

        for d, a in devices.values():
            print()
            print(d)
            print("-" * len(str(d)))
            print(a)

    async def send_com(self, command):
        await self.car_client.write_gatt_char(self.com_uuid, command.encode("ascii"))#TODO

    async def receive_com(self):
        message_b = await self.car_client.read_gatt_char(self.com_uuid)
        message = message_b.decode("ascii")
        return message

    async def image_receive(self):
        time.sleep(0.1)
        length_b = await self.car_client.read_gatt_char(self.photo_uuid) #TODO
        length = int.from_bytes(length_b, byteorder='little', signed=False)
        print("Size of image in bytes: ", length)