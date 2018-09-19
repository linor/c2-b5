import smbus, time

class LCD:
    I2C_ADDR  = 0x3F # I2C device address
    LCD_WIDTH = 20   # Maximum characters per line

    # Define some device constants
    LCD_CHR = 1 # Mode - Sending data
    LCD_CMD = 0 # Mode - Sending command

    LCD_LINE_1 = 0x80 # LCD RAM address for the 1st line
    LCD_LINE_2 = 0xC0 # LCD RAM address for the 2nd line
    LCD_LINE_3 = 0x94 # LCD RAM address for the 3rd line
    LCD_LINE_4 = 0xD4 # LCD RAM address for the 4th line

    LCD_LINES = [ LCD_LINE_1, LCD_LINE_2, LCD_LINE_3, LCD_LINE_4 ]

    LCD_BACKLIGHT  = 0x08  # On 0X08 / Off 0x00

    E_PULSE = 0.0005
    E_DELAY = 0.0005

    ENABLE = 0b00000100 # Enable bit

    BACKLIGHT_DURATION = 10

    def __init__(self):
        self.bus = smbus.SMBus(1) # Rev 2 Pi uses 1
        self.messages = [ "", "", "", "" ]

    def init(self):
        self.lcd_byte(0x33, self.LCD_CMD) # 110011 Initialise
        self.lcd_byte(0x32, self.LCD_CMD) # 110010 Initialise
        self.lcd_byte(0x06, self.LCD_CMD) # 000110 Cursor move direction
        self.lcd_byte(0x0C, self.LCD_CMD) # 001100 Display On,Cursor Off, Blink Off
        self.lcd_byte(0x28, self.LCD_CMD) # 101000 Data length, number of lines, font size
        self.lcd_byte(0x01, self.LCD_CMD) # 000001 Clear display

    def lcd_toggle_enable(self, bits):
        time.sleep(self.E_DELAY)
        self.bus.write_byte(self.I2C_ADDR, (bits | self.ENABLE))
        time.sleep(self.E_PULSE)
        self.bus.write_byte(self.I2C_ADDR,(bits & ~self.ENABLE))
        time.sleep(self.E_DELAY)

    def lcd_byte(self, bits, mode):
        bits_high = mode | (bits & 0xF0) | self.LCD_BACKLIGHT
        bits_low = mode | ((bits<<4) & 0xF0) | self.LCD_BACKLIGHT

        self.bus.write_byte(self.I2C_ADDR, bits_high)
        self.lcd_toggle_enable(bits_high)

        self.bus.write_byte(self.I2C_ADDR, bits_low)
        self.lcd_toggle_enable(bits_low)

    def lcd_string(self, message,line):
        message = message.ljust(self.LCD_WIDTH," ")
        self.lcd_byte(line, self.LCD_CMD)

        for i in range(self.LCD_WIDTH):
            self.lcd_byte(ord(message[i]), self.LCD_CHR)

    def display(self, message):
        print message
        self.messages.pop(0)
        self.messages.append(message)
        self.lastMessageTime = time.time()

        self.LCD_BACKLIGHT = 0x08
        for idx, msg in enumerate(self.messages):
            self.lcd_string(msg, self.LCD_LINES[idx])

    def clear(self):
        self.messages = [ "", "", "", "" ]
        self.lcd_byte(0x01, self.LCD_CMD) # 000001 Clear display

    def update(self):
        if self.LCD_BACKLIGHT != 0 and ((time.time() - self.lastMessageTime) > self.BACKLIGHT_DURATION):
            self.LCD_BACKLIGHT = 0
            self.lcd_byte(0, self.LCD_CMD)
        