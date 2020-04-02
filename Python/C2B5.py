import os, sys, time, lcd, smbus, random
import RPi.GPIO as GPIO
from subprocess import call

os.environ["SDL_VIDEODRIVER"] = "dummy"
import pygame.transform

if 1:
    import pygame.display
    pygame.display.init()
    screen = pygame.display.set_mode((1,1))

pygame.init();
pygame.joystick.init()
pygame.mixer.init()

i2cbus = smbus.SMBus(1)

GPIO.setmode(GPIO.BCM)
GPIO.setup(26, GPIO.IN)

def findFiles(dir):
    return list(map(lambda x: dir + x, os.listdir(dir)))

def sendI2C(address, contents):
    i2cbus.write_byte(address, 6)
    time.sleep(0.0005)
    i2cbus.write_byte(address, 133)
    time.sleep(0.0005)
    i2cbus.write_byte(address, len(contents))
    time.sleep(0.0005)

    checksum = len(contents)
    for c in contents:
        checksum = checksum ^ c
        i2cbus.write_byte(address, c)
        time.sleep(0.0005)

    i2cbus.write_byte(address, checksum)
    time.sleep(0.0005)

if __name__ == '__main__':
    driveJoystick = None
    lastDriveJoystickMovement = 0
    lastDriveCommand = 0
    domeJoystick = None
    lastDomeJoystickMovement = 0
    lastJoystickInit = time.time()
    lcdScreen = lcd.LCD()
    lcdScreen.init()
    lastDomeDirection = 0
    lastDomeSpeed = 0
    lastDomeUpdate = 0

    genericAudioFiles = findFiles('/home/pi/audio/generic/')
    chatAudioFiles = findFiles('/home/pi/audio/chat/')
    happyAudioFiles = findFiles('/home/pi/audio/happy/')
    sadAudioFiles = findFiles('/home/pi/audio/sad/')
    whistleAudioFiles = findFiles('/home/pi/audio/whistle/')
    screamAudioFiles = findFiles('/home/pi/audio/scream/')

    domeAudioFiles = list(genericAudioFiles)
    domeAudioPending = []
    driveAudioFiles = list(genericAudioFiles)
    driveAudioPending = []

    def playOneAudio(pending, allFiles):
        if not pending:
            pending.extend(allFiles)
            random.shuffle(pending)

        audiofile = pending.pop()        
        pygame.mixer.music.load(audiofile)
        pygame.mixer.music.play()

    def updateMovementSpeed(x, y):
        x_valid = (x < 0.05) or (x > 0.05)
        y_valid = (y < 0.05) or (y > 0.05)

        if x_valid and y_valid:
            x = int(x * 127)
            y = int(y * -127)
        else:
            x = 0
            y = 0
        
        cs = 2 ^ x ^ y

        try:
            i2cbus.write_byte(9, 6)
            time.sleep(0.0005)
            i2cbus.write_byte(9, 133)
            time.sleep(0.0005)
            i2cbus.write_byte(9, 2)
            time.sleep(0.0005)
            i2cbus.write_byte(9, int(x))
            time.sleep(0.0005)
            i2cbus.write_byte(9, int(y))
            time.sleep(0.0005)
            i2cbus.write_byte(9, int(cs))
            time.sleep(0.0005)
        except:
            print("Drive I2C error")

    def updateDomeSpeed(value):
        global lastDomeUpdate
        global lastDomeDirection
        global lastDomeSpeed

        if ((time.time() - lastDomeUpdate) < 0.2) and (value == lastDomeSpeed):
            return

        lastDomeSpeed = value
        lastDomeUpdate = time.time()
        if driveJoystick:
            if value < 0.05 :
                speed = value * -100
                lastDomeDirection = 1
            if value > 0.05:
                speed = value * 100
                lastDomeDirection = 0
            if value == 0:
                speed = 0

            try:
                sendI2C(8, [ 1, lastDomeDirection, int(speed) ])
            except:
                print("Dome I2C error")

    try:
        lcdScreen.display("Main loop ready")
        while True:
            lcdScreen.update()

            if (not driveJoystick or not domeJoystick) and ((time.time() - lastJoystickInit) > 5):
                if driveJoystick:
                    driveJoystick.quit()
                if domeJoystick:
                    domeJoystick.quit()

                domeJoystick = None
                driveJoystick = None
                lastJoystickInit = time.time()
                print "Resetting joystick system"
                pygame.joystick.quit()
                pygame.joystick.init()

            if not driveJoystick:
                try:
                    driveJoystick = pygame.joystick.Joystick(0)
                    driveJoystick.init()
                    lastDriveJoystickMovement = time.time()
                    lcdScreen.display("Drive Joystick found")
                except pygame.error:
                    pass

            if not domeJoystick:
                try:
                    domeJoystick = pygame.joystick.Joystick(1)
                    domeJoystick.init()
                    lastDomeJoystickMovement = time.time()
                    lcdScreen.display("Dome Joystick found")
                except pygame.error:
                    pass

            events = pygame.event.get()
            for event in events:
                currentTime = time.time()

                if event.type == pygame.JOYAXISMOTION:
                    if event.joy == 0:
                        lastDriveJoystickMovement = currentTime
                        if event.axis == 0 or event.axis == 1:
                            lastDriveCommand = time.time()
                            updateMovementSpeed(driveJoystick.get_axis(0), driveJoystick.get_axis(1))
                    if event.joy == 1:
                        lastDomeJoystickMovement = currentTime
                        if event.axis == 0:
                            updateDomeSpeed(event.value)

                #if not event.type == pygame.JOYAXISMOTION:
                #    print event

                if event.type == pygame.JOYBUTTONDOWN:
                    pending = driveAudioPending
                    allFiles = driveAudioFiles

                    if event.joy == 1:
                        pending = domeAudioPending
                        allFiles = domeAudioFiles

                    if event.button == 4:
                        playOneAudio(pending, allFiles)

                    if event.button == 13:
                        del pending[:]
                        del allFiles[:]
                        allFiles.extend(chatAudioFiles)

                    if event.button == 14:
                        del pending[:]
                        del allFiles[:]
                        allFiles.extend(genericAudioFiles)

                    if event.button == 15:
                        del pending[:]
                        del allFiles[:]
                        allFiles.extend(sadAudioFiles)

                    if event.button == 16:
                        del pending[:]
                        del allFiles[:]
                        allFiles.extend(happyAudioFiles)

                    if event.button == 0:
                        playOneAudio([], whistleAudioFiles)

                    if event.button == 1:
                        playOneAudio([], screamAudioFiles)

            if domeJoystick and (time.time() - lastDomeUpdate) > 0.5:
                updateDomeSpeed(domeJoystick.get_axis(0))

            if driveJoystick and (time.time() - lastDriveCommand) > 1:
                lastDriveCommand = time.time()
                updateMovementSpeed(driveJoystick.get_axis(0), driveJoystick.get_axis(1))

            if not GPIO.input(26):
                print("Need to shutdown")
                lcdScreen.clear()
                lcdScreen.display("Shutdown")
                call("sudo shutdown --poweroff now", shell=True)
                exit()

    except KeyboardInterrupt:
        pass

    finally:
        GPIO.cleanup()
        lcdScreen.clear()
        lcdScreen.display("Program discontinued")