# GainSense: Exercise Repetition and TUT Counter

### Video: [[YouTube] GainSense: Exercise Repetition and TUT Counter](https://youtu.be/r0f6S_rDfo0)

### Lab Notebook: [0notebook.md](/notebook/0arhang2_notebook.md)

## Problem. 
**Gym trainers are expensive! Maximize your gains by tracking your reps and hitting your time-under-tension targets.** <br><br>
Most people struggle to maintain high-quality workouts, especially without a gym trainer. Trainers are expensive and most trainers only correct basic form, count reps, and ensure reps are done slowly to reach the desired time-under-tension (TUT) . This problem gets exacerbated when progressively overloading or when muscles are tired at the end of the workout. Counting reps and reaching the desired TUT are the main metrics most gym-goers understand and struggle to hit. 

## Solution. 
Our wristwatch-style device counts the number of reps the user performs and measures TUT. It contains a vibration motor which buzzes once desired TUT is reached, prompting the user to bring the arm down to complete the rep.

## Motivation. 
The modern fitness landscape is increasingly shifting toward personalized and self-guided workouts. However, one major challenge persists for users exercising without professional supervision: the inability to accurately track workout quality, particularly in terms of repetition count and time-under-tension (TUT). These metrics are critical for building strength and ensuring proper form but are often estimated imprecisely, especially in the absence of visual feedback or a trainer. While smartwatches and fitness trackers have made strides in heart-rate monitoring and activity tracking, most either offer limited rep counting functionality (~65% accuracy) or fail entirely to measure TUT. 

Our entire team loves going to the gym and regularly faces this struggle. We decided to use our Senior Design project to solve this very real problem we face on a daily basis.

## High-Level Requirements. 
1.	Adjustable TUT duration between 1 to 10 seconds in 1-second increments.
2.	Minimum 90% accuracy in exercise rep detection.
3.	Time-under-tension measurement error margin less than ±1 second per rep.


## Final Specs

### Exercises Supported to 98% accuracy as of May 2025

- Bicep Curls
- Hammer Curls
- Bench Press (flat, incline, decline)
- Shoulder Press
- Upright Row
- Shoulder Shrugs
- Rear Delt Fly
- Lateral and Front Raises
- Bent Over Rows
- …&c.

### Final Product

- **Block Diagram:** ![Final Block Diagram](/notebook/block-diagram-final.png)
- **PCB Code:** [pcbCode_final.ino](/pcbCode/pcbCode_final.ino)
- **Final Product:** ![Final Product](/notebook/final-product.jpg)


## The Team

1. [Prithvi Patel](https://www.linkedin.com/in/prithvi-patel-56b831229/)
2. [Arhan Goyal](https://www.linkedin.com/in/arhangoyal/)
3. [Vikrant Banerjee](https://www.linkedin.com/in/vikrant-banerjee/)
