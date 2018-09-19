import os, sys, time, lcd, smbus, random

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

def findFiles(dir):
    return list(map(lambda x: dir + x, os.listdir(dir)))

if __name__ == '__main__':
    driveJoystick = None
    lastDriveJoystickMovement = 0
    domeJoystick = None
    lastDomeJoystickMovement = 0
    lastJoystickInit = time.time()
    lcdScreen = lcd.LCD()
    lcdScreen.init()
    lastDomeDirection = 0
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

    def updateDomeSpeed(value):
        global lastDomeUpdate
        global lastDomeDirection

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
                i2cbus.write_byte(8, 1)
                i2cbus.write_byte(8, lastDomeDirection)
                i2cbus.write_byte(8, int(speed))
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
                        if event.axis == 0:
                            updateDomeSpeed(event.value)
                    if event.joy == 1:
                        lastDomeJoystickMovement = currentTime

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

            if driveJoystick and (time.time() - lastDomeUpdate) > 0.5:
                updateDomeSpeed(driveJoystick.get_axis(0))


    except KeyboardInterrupt:
        pass

    finally:
        lcdScreen.clear()
        lcdScreen.display("Program discontinued")