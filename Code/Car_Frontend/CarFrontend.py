import keyboard
import asyncio
from BtClient import *
from WifiSocket import *
from CvVideoScreen import *
import threading


class CarFrontend:
    def __init__(self, tcp_ip: str = None, tcp_port: int = None, car_uuid: str = None):
        self.command = {"right": "SET MOTOR_R", "left": "SET MOTOR_L", "photo_1": "GET PHOTO_1",
                        "photo_2": "GET PHOTO_2"}
        #Wifi
        self.tcp_port = tcp_port
        self.tcp_ip = tcp_ip
        #Bluetooth
        self.car_uuid = car_uuid
        self.status = {"right": 0, "left": 0, "end": False, "photo_1": False, "photo_2": False}
        self.status_last = self.status.copy()
        self.wifi_n_bt = True
        # threading.Thread(target=display, args=("./video.jpg", ), daemon=True)

    # ------------------------------Interface stuff------------------------------

    def update_interface(self):
        self.keyboard_control()

    async def execute(self):
        # print("Executing")
        # print(self.status["right"], self.status_last["right"])
        if self.status != self.status_last:
            if self.wifi_n_bt:

                if self.status["end"] is True:
                    self.wifi_socket.disconnect()
                    return True

                self.wifi_socket.send(self.command["left"] + " " + str(self.status["left"]) + "\n")
                self.wifi_socket.send(self.command["right"] + " " + str(self.status["right"]) + "\n")

                if self.status["photo_1"]:
                    print("send photo command!\n")
                    self.wifi_socket.send(self.command["photo_1"] + "\n")
                    self.wifi_socket.daemon_image_receive()

                if self.status["photo_2"]:
                    self.wifi_socket.send(self.command["photo_2"] + "\n")
                    self.wifi_socket.daemon_image_receive()
            else:
                if self.status["end"] is True:
                    await self.bt_client.disconnect()
                    return True

                await self.bt_client.send_com(self.command["left"] + " " + str(self.status["left"]) + "\n")
                await self.bt_client.send_com(self.command["right"] + " " + str(self.status["right"]) + "\n")

                if self.status["photo_1"]:
                    print("send photo command!\n")
                    await self.bt_client.send_com(self.command["photo_1"] + "\n")
                elif self.status["photo_2"]:
                    print("send photo command!\n")
                    await self.bt_client.send_com(self.command["photo_2"] + "\n")

        self.status_last = self.status.copy()
        return False

    def keyboard_control(self):
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
            self.status["left"] = 100  # turn on the spot
            self.status["right"] = -100
        elif keyboard.is_pressed('left'):
            self.status["left"] = -100
            self.status["right"] = 100
        elif keyboard.is_pressed('space'):
            self.status["photo_1"] = True
        elif keyboard.is_pressed('esc'):
            self.status["end"] = True

    # -----------------------------main------------------------------
    async def main_wifi(self):
        self.wifi_n_bt = True
        self.wifi_socket = WifiSocket(self.tcp_ip, self.tcp_port)
        self.wifi_socket.init()
        await self.main()

    async def main_bluetooth(self):
        self.wifi_n_bt = False
        self.bt_client = BtClient(self.car_uuid)
        await self.bt_client.init()
        await self.main()

    async def main(self):
        while True:
            # print("Loop")
            self.update_interface()
            end = await self.execute()
            time.sleep(0.001)
            if end:
                break


def test():
    while True:
        print(keyboard.read_key())


def main():
    # test()

    car = CarFrontend(tcp_ip="192.168.1.240", tcp_port=80, car_uuid="2A2352C9-2D9F-D46E-7D24-0E5B47901F49")
    # car.wifi_connect()
    asyncio.run(car.main_bluetooth())


# car.main_wifi()


if __name__ == "__main__":
    main()
    # test()
