import sys
from PyQt6.QtWidgets import QApplication, QWidget, QMainWindow
from PyQt6.QtGui import QImage
import threading


class PyQtVideo:
    def __init__(self, *args, **kwargs):
        pass

    class MainWindow(QMainWindow):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.setWindowTitle("Video stream")

    def video_window(self):
        self.app = QApplication(sys.argv)

        self.window = QMainWindow()
        self.window.show()

        self.app.exec()

    def print_photo(self, image: QImage):
        self.window.setCentralWidget(image)


if __name__ == "__main__":
    video_window = PyQtVideo()
    image = QImage("./077B65B4-9936-4C58-B1F856F562F98259_source.jpg")
    x = threading.Thread(target=video_window.video_window, args=(), daemon=True)
    x.start()

    y = threading.Thread(target=video_window.print_photo(image), daemon=True)
    y.start()
