import cv2
import numpy as np
import serial


# Serial port configuration
port = 'COM4'  # Adjust this to your serial port
baud_rate = 115200
ser = serial.Serial(port, baud_rate, timeout=1)


# Function to send data over serial
def SendData(data):
    data = f"{data}\n"  # Ensure data ends with a newline
    ser.write(data.encode('utf-8'))


cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'))
cap.set(3,960) # Image height
cap.set(4,640) # Image width



# Load image
ret, image = cap.read()
hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)

# Define HSV ranges
lower_red1 = np.array([0, 120, 70])
upper_red1 = np.array([10, 255, 255])
lower_red2 = np.array([170, 120, 70])
upper_red2 = np.array([180, 255, 255])

lower_blue = np.array([90, 50, 50])
upper_blue = np.array([140, 255, 255])

# Create masks
mask_red1 = cv2.inRange(hsv, lower_red1, upper_red1)
mask_red2 = cv2.inRange(hsv, lower_red2, upper_red2)
mask_red = cv2.bitwise_or(mask_red1, mask_red2)
mask_blue = cv2.inRange(hsv, lower_blue, upper_blue)

# Lists to store centers
red_centers = []
blue_centers = []

# Find red object contours
contours_red, _ = cv2.findContours(mask_red, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
for cnt in contours_red:
    area = cv2.contourArea(cnt)
    if area > 500:
        x, y, w, h = cv2.boundingRect(cnt)
        cx = x + w // 2
        cy = y + h // 2
        red_centers.append((cx, cy))
        cv2.rectangle(image, (x, y), (x + w, y + h), (0, 0, 255), 2)
        cv2.circle(image, (cx, cy), 5, (0, 0, 255), -1)
        cv2.putText(image, "Red", (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

# Find blue object contours
contours_blue, _ = cv2.findContours(mask_blue, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
for cnt in contours_blue:
    area = cv2.contourArea(cnt)
    if area > 500:
        x, y, w, h = cv2.boundingRect(cnt)
        cx = x + w // 2
        cy = y + h // 2
        blue_centers.append((cx, cy))
        cv2.rectangle(image, (x, y), (x + w, y + h), (255, 0, 0), 2)
        cv2.circle(image, (cx, cy), 5, (255, 0, 0), -1)
        cv2.putText(image, "Blue", (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 0), 2)

# Find closest red and blue object to image center
img_center = (image.shape[1] // 2, image.shape[0] // 2)

def closest(center_list):
    if not center_list:
        return None
    return min(center_list, key=lambda c: (c[0] - img_center[0])**2 + (c[1] - img_center[1])**2)

def closest_to_bottom(center_list):
    if not center_list:
        return None
    # Find the center with the largest y value (closest to bottom)
    return max(center_list, key=lambda c: c[1])

closest_red = closest_to_bottom(red_centers)
closest_blue = closest_to_bottom(blue_centers)

# Send only the closest red and blue object coordinates
if closest_red:
    SendData(closest_red)
if closest_blue:
    SendData(closest_blue)

# Display image
cv2.imshow("Detected Objects with Centers", image)
cv2.waitKey(0)
cv2.destroyAllWindows()

# Print centers
print("Closest Red Object:", closest_red)
print("Closest Blue Object:", closest_blue)

