import sensor
import image
import time
import struct
from machine import UART

uart = UART(1, 921600)

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=2000)

def compute_stats(img):
    # Convert to LAB (OpenMV does not have HSV, but LAB L = brightness)
    # and use RGB for green detection
    hist = img.get_histogram()
    avg_brightness = hist.get_statistics().l_mean()  # L is from 0–100
    brightness_percent = avg_brightness

    green_pixels = 0

    # Define thresholds for "green" RGB
    for y in range(0, img.height(), 5):  # Sample every 5 pixels for speed
        for x in range(0, img.width(), 5):
            r, g, b = img.get_pixel(x, y)
            if g > r and g > b and g > 50:
                green_pixels += 1

    green_percent = (green_pixels / ((img.width()//5)*(img.height()//5))) * 100
    return brightness_percent, green_percent

while True:
    if uart.any():
        cmd = uart.read()
        if b"__TAKE_SNAPSHOT__" in cmd:
            img = sensor.snapshot()
            brightness, green_percent = compute_stats(img)
            jpeg = img.compress(quality=60)
            size = jpeg.size()

            uart.write(size.to_bytes(4, 'big'))
            uart.write(jpeg)
            uart.write(bytearray(struct.pack('ff', brightness, green_percent)))

            print("Sent image. Brightness: {:.2f}% | Green: {:.2f}%".format(brightness, green_percent))
