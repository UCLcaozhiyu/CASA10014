# CE Final Work

## Background
In the lab, tools left on the table are often borrowed or moved while I’m away for soldering or 3D printing. This results in frequent searches across the lab to locate my tools. To address this issue, I designed a tool that acts as an assistant to monitor my equipment and notify me when items are moved or returned.

## Aim
This project aims to create a portable lighting control system, designed in the shape of a gas lamp, using a photoresistor and an ultrasonic distance sensor. 

### Features:
- **Distance-based lighting**: The ultrasonic sensor measures the distance between the lamp and nearby objects.  
  - Within 20 cm: LEDs light up red.  
  - Beyond 20 cm: The color transitions from red to blue as the distance increases.  
- **Idle behavior**: When no object is detected, the LEDs progressively turn off at a rate of one LED per second. Upon detecting an object, all LEDs light up again.  
- **Photoresistor as a switch**: The photoresistor mimics the behavior of an oil lamp. Covering it turns off the LEDs, and uncovering it lights up the lamp.  

## Circuit and Software Design
This project builds upon hardware and software design concepts from the UCL CE coursework. All related code is stored in the `CE` folder. Below is a breakdown of the files and their purposes:

### Code Versions:
- **`sketch_cefinal3`**: Final version with full functionality.  
- **`sketch_cefinal1-1.2`**: Debugging the ultrasonic distance sensor, photoresistor, and MQTT publishing.  
- **`sketch_cefinal2-2.3`**: Testing the ring buffer and threshold filtering to remove noise.  
- **`sketch_ultrasonic_ranging`**: Ultrasonic ranging functionality.  
- **`sketch_contalled`**: MQTT connection and publishing.  
- **`sketch-firstcalss`**: Photoresistor input mapping and calibration.

### Circuit Design:
The circuit follows concepts from the UCL CASA0016 coursework [(1)] and includes:
- Ultrasonic distance measurements using the formula [(2)]:
 ![circuits](circuits.png)
  ```cpp
  long duration = pulseIn(ECHO_PIN, HIGH);
  float distance = (duration * 0.034) / 2.0;
