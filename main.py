import numpy as np
import time
import matplotlib.pyplot as plt
import cv2


#rozmezi barev puku v hsv modrych 
low_b = np.array([180, 10, 10])
high_b = np.array([260, 100,100])


low_r = np.array([30,50,100])#deafult 160,50,100
high_r = np.array([190,255,255])



#najde modre puky, a vrati jejich souradnice
def FindBluePucks (image, low, high):
    #color masking
    pucks_cords=[]
    
    pozicepuku = []

    mask = cv2.inRange(img,low,high)
    contours, hierarchy = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE )
    if len(contours) != 0:
        for contour in contours:
            if cv2.contourArea(contour) > 10000:
                x,y,w,h = cv2.boundingRect(contour)
                p= x,y,w,h
                cv2.rectangle(img, (x,y),(x+w,y+h),(255,255,255),5)
                pozicepuku.append(p)

    i = 0
    for a in pozicepuku:
        x,y,w,h = pozicepuku[i]
        center_x = (x + w/2)
        center_y = (y + h/2)
        print(center_x, center_y)
        cords = center_x,center_y
        pucks_cords.append(cords)
        i = i + 1
    return pucks_cords


def FindRedPucks (image, low, high):
    #color masking
    pucks_cords=[]
    
    pozicepuku = []

    mask = cv2.inRange(img,low,high)
    contours, hierarchy = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE )
    if len(contours) != 0:
        for contour in contours:
            if cv2.contourArea(contour) > 10000:
                x,y,w,h = cv2.boundingRect(contour)
                p= x,y,w,h
                cv2.rectangle(img, (x,y),(x+w,y+h),(0,0,0),5)
                pozicepuku.append(p)

    i = 0
    for a in pozicepuku:
        x,y,w,h = pozicepuku[i]
        center_x = (x + w/2)
        center_y = (y + h/2)
        print(center_x, center_y)
        cords = center_x,center_y
        pucks_cords.append(cords)
        i = i + 1
    return pucks_cords


#camera setup
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'))
cap.set(3,960) # sirka px
cap.set(4,640) # vyska px


#udela fotku
ret, img = cap.read()
img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
FindBluePucks(img,low_b,high_b)
FindRedPucks(img,low_r,high_r)

#toto ukaze fotku
cv2.imshow('image', cv2.cvtColor(img, cv2.COLOR_HSV2BGR))
#odejdu b
cv2.waitKey(0) == ord('b')
cv2.destroyAllWindows()
