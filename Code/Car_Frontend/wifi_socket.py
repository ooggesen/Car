import socket
import time
import keyboard
import threading
from PIL import Image
import io

class wifi_socket:
    def __init__(self, tcp_ip, tcp_port) -> None:
        self.tcp_port = tcp_port 
        self.tcp_ip = tcp_ip 
        self.image_count = 0
        self.photo_busy = False

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
    
    def is_photo_busy(self):
        return self.photo_busy

    def init(self):
        assert(self.tcp_ip is not None)
        assert(self.tcp_port is not None)
        self.wifi_n_bt = True

        try:
            print("Address: ", self.car_socket.AF_INET)
        except AttributeError:
            self.connect()
            print("Address: ", self.car_socket.AF_INET)

    def connect(self) :
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

    def disconnect(self) :
        self.car_socket.close()

    def send(self, string):
        #print(string)
        self.car_socket.send(string.encode('ascii'))

    def daemon_image_receive(self):
        if ( self.photo_busy ==  False):
            x = threading.Thread(target=self.image_receive, args= (), daemon=True)
            x.start()

    def image_receive(self) :
        #For recption of photos only
        try:
            self.photo_busy = 1
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
            # self.disconnect()
            # raise OSError("Transmission failure!")
        finally:
            self.photo_busy = 0