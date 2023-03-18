import cv2
import threading


def display(path):
    while True:
        image = cv2.imread(path)

        if image is not None:
            window_name = "video_window"
            cv2.imshow(window_name, image)

        key = cv2.waitKey(1)
        if key == 27:  # exit on ESC
            cv2.destroyAllWindows()
            break

def close_window():
    cv2.destroyAllWindows()

if __name__ == "__main__":
    x = threading.Thread(target=display, args=("./video.jpg", ), daemon=True)
    x.run()
