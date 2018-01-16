import badge
import time
import random
import json


class LEDController(object):
    def __init__(self):
        self.brightness = 128

        self.color_codes = {
            "RED"      : 0Xff0000,
            "GREEN"    : 0X00FF00,
            "BLUE"     : 0x0000ff,
            "PINK"     : 0xffc0cb,
            "WHITE"    : 0xFFFFFF,
            "TEAL"     : 0x008080,
            "YELLOW"   : 0xFFFF33,
            "DC26BLUE" : 0x6A92D7,
            "DC26GREY" : 0xB3CBCF,
            "DC26RED"  : 0xDF245B,
            "DC26GREEN": 0x76C294
        }
        self.color_index = list(self.color_codes.keys())

        self.LED_SETS = {
            "TOP": 199,
            "ALL": 0xFF,
            "ARM": 56
        }
        self.locked_animations = [
            self.animation_cpt_planet,          # 4
#               self.animation_twinkle,         
            self.animation_glitch,              # 5
        ]


        self.animations = [
            self.animation_full_color_on,       # 0
            self.animation_cycle_inner_colors,  # 1
            self.animation_random,              # 2
            self.animation_flow,                # 3
            self.animation_decon_colors         # 6
        ]
        self.active_animation = 0
        self.run = False

    def start_animation_loop(self):
        while True:
            time.sleep(1)
            if self.run:
                self.clear()
                print("Starting animation loop")
                cur = self.active_animation
                while self.run: 
                    if cur != self.active_animation:
                            print("moving to new animation")
                    self.animations[self.active_animation % len(self.animations)]()


    def _LED_FLIP_CHECK(self, x):
        if x == 4:
            return 5

        elif x == 5:
            return 4
        return x

    def get_random_color(self,skip = None):
        if not skip:
            return list(self.color_codes.keys())[random.randint(0, len(list(self.color_codes.keys()))-1)]
        while True:
            color = list(self.color_codes.keys())[random.randint(0, len(list(self.color_codes.keys()))-1)]
            if color is not skip:
                return color

    def led_on(self, x, y, rcolor = "WHITE", is_hex=False, x_raw = False, dim_per_value = 2):
        if is_hex:
            color = rcolor
        else:
            if rcolor not in self.color_codes:
                return False

            color = self.color_codes[ rcolor.upper() ]


        bnib = (color & 0xFF)
        gnib = ( (color & 0xFF00) >> 8 )
        rnib = ( (color & 0xFF0000) >> 16 )

        if not x_raw:
            _x = self._LED_FLIP_CHECK(x)
            _x = 1 << _x
        else:
            _x = x

        if y == 0:    # My head hurts.  and I couldn't think of a cleaner way to make this work in the moment.  will clean up
            offset = 0
        elif y == 1:
            offset = 3
        elif y == 2:
            offset = 6
        elif y == 3:
            offset = 9
        else:
            return None
        badge.set_pwm_duty_matrix(_x, (1<< (offset + 0) ), int(rnib/dim_per_value))  # RED
        badge.set_pwm_duty_matrix(_x, (1<< (offset + 1) ), int(gnib/dim_per_value))  # GREEN
        badge.set_pwm_duty_matrix(_x, (1<< (offset + 2) ), int(bnib/dim_per_value))  # BLUE


    def led_off(self, x, y, x_raw = False):
        if x_raw:
            _x = x
        else:
            _x = self._LED_FLIP_CHECK(x)
            _x = 1 << _x

        if y == 0:
            offset = 0
        elif y == 1:
            offset = 3
        elif y == 2:
            offset = 6
        elif y == 3:
            offset = 9
        else:
            return None

        badge.set_pwm_duty_matrix(_x, (1<< (offset+0) ), 0 )  # RED
        badge.set_pwm_duty_matrix(_x, (1<< (offset+1) ), 0)  # GREEN
        badge.set_pwm_duty_matrix(_x, (1<< (offset+2) ), 0)  # BLUE

    def led_fade_in(self, x, y, color, rate=1000, x_raw=True, is_hex=False):
        for offset in range(11, 1, -1):
            self.led_on(x, y, color, x_raw=x_raw, is_hex=is_hex, dim_per_value = offset )
            time.sleep_ms( int( rate / 10) )
        self.led_on(x, y, color, x_raw=x_raw, dim_per_value=1  )

    def led_fade_out(self, x, y, color, rate=1000, x_raw=True,  is_hex=False):
        for offset in range(1, 11):
            self.led_on(x, y, color, x_raw=x_raw, is_hex=is_hex, dim_per_value=offset  )
            time.sleep_ms( int( rate / 10) )
        self.led_on(x, y, 0x00000, is_hex=True, x_raw=True  )


    def clear(self):
        badge.set_pwm_duty_matrix(0xff, 0xfff, 0)

    def boot(self):
        # Test each LED individual color
        self.clear()

        for y in range(0,3):
            self.led_on( self.LED_SETS['TOP'], y, "WHITE", x_raw = True )

        self.led_on( self.LED_SETS['ALL'], 3, "WHITE", x_raw = True)
        time.sleep_ms(500)
        self.led_off( self.LED_SETS['ALL'], 3, x_raw = True)

        for color in list(self.color_codes.keys()):
            self.led_fade_out( self.LED_SETS['ALL'], 3, color )

        for x in range(2,6):
            for y in range(0,3):
                self.led_on( x, y, self.get_random_color() )
                time.sleep_ms(200)

        time.sleep_ms(100)
#        for y in range(12):
#            for x in range(8):
#                badge.set_pwm_duty_matrix(1 << x, 1 << y, self.brightness)
#                time.sleep_ms(100)
#                badge.set_pwm_duty_matrix(1 << x, 1 << y, 0)
#
#        # Test each sector individual color
#        for y in range(12):
#            badge.set_pwm_duty_matrix(0xff, 1 << y, self.brightness)
#            time.sleep_ms(300)
#            badge.set_pwm_duty_matrix(0xff, 1 << y, 0)

        badge.set_pwm_duty_matrix(0xff, 0xfff, int(0xFF/2) )
        self.run = True


    def update_running(self):
        self.clear()
        # Turn on outter
        self.led_on(self.LED_SETS['TOP'], 0, "RED", x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 1, "RED", x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 2, "RED", x_raw=True)
        self.led_on(0xFF, 3, "RED", x_raw=True)

        for x in range(10):
            self.led_fade_out( self.LED_SETS['ALL'], 3, "RED")
            self.led_fade_in( self.LED_SETS['ALL'], 3, "RED")

## Animations
    def animation_reboot(self):
        self.clear()
        # Make fully white
        badge.set_pwm_duty_matrix(0xff, 0xfff, int(0xFF/2) )
        time.sleep_ms(250)
        
        #Turn off center
        self.led_off(self.LED_SETS['ALL'], 3, x_raw=True)        

        #Turn off arms
        for x in range(6,2, -1):
            for y in range(0,3):
                self.led_off( x, y )
            time.sleep_ms(100)

        #Extra little sleep
        time.sleep_ms(50)
#        self.clear()

    def animation_json(self, indata):
#        self.clear()
        jdata = []
        if type(indata) == type(""):
            jdata = json.loads(indata)
        else:
            jdata = indata

        for rline in jdata:
            for index, iled in enumerate(rline): 
                self.led_off( (index % len(rline)), int(index/len(rline)) )
                self.led_on( (index % len(rline)), int(index/len(rline)) , iled, is_hex=True )

    def animation_snake(self):
        self.clear()
        start = random.randint(0,2)
        end = None
        while end is None:
            tend = random.randint(0,2)
            if tend is not start:
                end = tend

        self.led_on(self.LED_SETS['TOP'], start, "GREEN", x_raw=True)
        self.led_on(self.LED_SETS['TOP'], end, "RED", x_raw=True)
        for tled in range(3, 6):
            self.led_on(tled, start, "PINK")
            time.sleep_ms(100)

        self.led_on(self.LED_SETS['ALL'], 3, x_raw=True)
        time.sleep_ms(100)

        for tled in range(5, 2, -1):
            self.led_on(tled, end, "PINK")
            time.sleep_ms(100)

        for tval in range(3):
            self.led_off(self.LED_SETS['TOP'], end, x_raw=True)
            time.sleep_ms(50)       
            self.led_on(self.LED_SETS['TOP'], end, "RED", x_raw=True)
            time.sleep_ms(50)


    def animation_soundem(self):
        self.clear()
        self.led_on(self.LED_SETS['ALL'], 3, "WHITE", x_raw=True)
#        for tval in range(20):
#            depth = random.randint(2,5)
        pattern = [5,5,4,5,4,4,3,2,3,4,5,4,4,5,3,3,2,2,4,5,2]

        for depth in pattern:
            for x in range(5, depth-1, -1 ):
                for y in range(3):

                    color = "WHITE"
                    if x is 4 or x is 3:
                        color = "YELLOW"
                    elif x is 5:
                        color = "GREEN"
                    elif x is 2:
                        color = "RED"

                    if x == 2:
                        self.led_on(self.LED_SETS['TOP'], y, color, x_raw=True)
                    else:
                        self.led_on(x,y, color)
                time.sleep_ms(100)
            time.sleep_ms(400)

            for x in range(depth, 6):
                for y in range(3):    
                        if x is 2:
                            self.led_off(self.LED_SETS['TOP'],y, x_raw=True)
                        else:
                            self.led_off(x,y)
            time.sleep_ms(100)
#            time.sleep_ms(400)



    def animation_cycle_inner_colors(self):
#        self.clear()

        tcolor = self.get_random_color()

        self.led_off(self.LED_SETS['TOP'], 0, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 0, tcolor, x_raw=True)

        self.led_off(self.LED_SETS['TOP'], 1, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 1, tcolor, x_raw=True)

        self.led_off(self.LED_SETS['TOP'], 2, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 2, tcolor, x_raw=True)

        tcolor = self.get_random_color(tcolor)
        for x in range(2,6):
            for y in range(0,3):
                self.led_off( x, y )
                self.led_on( x, y, tcolor )
                time.sleep_ms(200)

        self.led_off(self.LED_SETS['ALL'], 3, x_raw=True )
        self.led_on(self.LED_SETS['ALL'], 3, self.get_random_color(tcolor), x_raw=True )
        time.sleep_ms(1000)

    def animation_random(self):
#        self.clear()
        for loopc in range(16):
            tcolor = self.color_index[ ( (time.time()) % len(self.color_index) ) ] # Slower color grabbing
            x = ( random.randint(0,10) % 8 )
            y = ( random.randint(0,10) % 4 )

            self.led_on( x, y, tcolor )
            time.sleep_ms(10)

    def animation_flow(self):
        self.clear()
        tcolor = self.get_random_color()
        self.led_off(self.LED_SETS['TOP'], 0, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 0, tcolor, x_raw=True)
        self.led_off(self.LED_SETS['TOP'], 1, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 1, tcolor, x_raw=True)
        self.led_off(self.LED_SETS['TOP'], 2, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 2, tcolor, x_raw=True)
        for x in range(3,6):
            self.led_off( x, 0)
            self.led_on( x, 0, tcolor )
            self.led_off( x, 1)
            self.led_on( x, 1, tcolor )
            self.led_off( x, 2)
            self.led_on( x, 2, tcolor )
            time.sleep_ms(50)

        self.led_off(self.LED_SETS['ALL'], 3, x_raw=True )
        self.led_on(self.LED_SETS['ALL'], 3, tcolor, x_raw=True )

        time.sleep_ms(2000)
        tcolor = self.get_random_color(tcolor)
        self.led_off(self.LED_SETS['ALL'], 3, x_raw=True )
        self.led_on(self.LED_SETS['ALL'], 3, tcolor, x_raw=True )

        for x in range(5, 2, -1):
            self.led_off( x, 0)
            self.led_on( x, 0, tcolor )
            self.led_off( x, 1)
            self.led_on( x, 1, tcolor )
            self.led_off( x, 2)
            self.led_on( x, 2, tcolor )
            time.sleep_ms(50)

        self.led_off(self.LED_SETS['TOP'], 0, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 0, tcolor, x_raw=True)
        self.led_off(self.LED_SETS['TOP'], 1, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 1, tcolor, x_raw=True)
        self.led_off(self.LED_SETS['TOP'], 2, x_raw=True)
        self.led_on(self.LED_SETS['TOP'], 2, tcolor, x_raw=True)

        time.sleep_ms(2000)
    def animation_dual_color(self):
        self.clear()
        color1 = self.get_random_color()
        color2 = self.get_random_color(color1)
        for x in range(8):
            for y in range(4):
                if x % 2 == 1:
                    self.led_on(x, y, color1)
                else:
                    self.led_on(x, y, color2)

    def animation_cpt_planet(self):
        self.clear()
        #EARTH!
        self.led_on(self.LED_SETS['TOP'], 0, "GREEN", x_raw=True)
        time.sleep_ms(100)
        for x in range(3,6):
            self.led_on( x, 0, "GREEN" )
            time.sleep_ms(50)

        time.sleep_ms(250)
        # Fire!
        self.led_on(self.LED_SETS['TOP'], 1, "RED", x_raw=True)
        time.sleep_ms(100)
        for x in range(3,6):
            self.led_on( x, 1, "RED" )
            time.sleep_ms(50)
        time.sleep_ms(250)
        # WATER
        self.led_on(self.LED_SETS['TOP'], 2, "BLUE", x_raw=True)
        time.sleep_ms(100)
        for x in range(3,6):
            self.led_on( x, 2, "BLUE" )
            time.sleep_ms(50)
        time.sleep_ms(250)

        self.led_off(self.LED_SETS['ALL'], 3, x_raw=True)

        self.led_on(1, 3, "WHITE")
        self.led_on(3, 3, "WHITE")
        self.led_on(4, 3, "WHITE")

        self.led_on(6, 3, "RED")
        self.led_on(5, 3, "BLUE")
        self.led_on(2, 3, "GREEN")

        self.led_on(0, 3, "WHITE")

        for toploop in range(3):
            self.led_off(1, 3)
            time.sleep_ms(100)
            self.led_on(1, 3, "WHITE")
            self.led_off(3, 3)
            time.sleep_ms(100)
            self.led_on(3, 3, "WHITE")
            self.led_off(4, 3)
            time.sleep_ms(100)
            self.led_on(4, 3, "WHITE")
            time.sleep_ms(1000)
            for inloop in range(10):
                self.led_off(6, 3,)
                self.led_off(5, 3,)
                self.led_off(2, 3,)
                time.sleep_ms(100)
                self.led_on(6, 3, "RED")
                self.led_on(5, 3, "BLUE")
                self.led_on(2, 3, "GREEN")

        # WATER
        time.sleep_ms(100)
        for x in range(5, 2, -1):
            self.led_off( x, 2 )
            time.sleep_ms(50)
        time.sleep_ms(150)
        self.led_off(self.LED_SETS['TOP'], 2, x_raw=True)

        time.sleep_ms(100)
        for x in range(5, 2, -1):
            self.led_off( x, 1 )
            time.sleep_ms(50)
        time.sleep_ms(150)
        self.led_off(self.LED_SETS['TOP'], 1, x_raw=True)

        time.sleep_ms(100)
        for x in range(5, 2, -1):
            self.led_off( x, 0)
            time.sleep_ms(50)
        time.sleep_ms(150)
        self.led_off(self.LED_SETS['TOP'], 0, x_raw=True)

        self.clear()

    def animation_twinkle(self):
        self.clear()
        self.led_on(self.LED_SETS['TOP'], 0, 0xCF41AE, x_raw=True, is_hex=True)
        self.led_on(self.LED_SETS['TOP'], 1, 0xCF41AE, x_raw=True, is_hex=True)
        self.led_on(self.LED_SETS['TOP'], 2, 0xCF41AE, x_raw=True, is_hex=True)

        for x in range(5, 2, -1):
            self.led_fade_in( x, 0, "WHITE", x_raw=False )
            self.led_fade_in( x, 2, "WHITE" , x_raw=False )
            self.led_fade_in( x, 1, "WHITE", x_raw=False  )
        #In progress

    def animation_glitch(self, incolor = 0xCF41AE):
        self.led_off(7,2)
        time.sleep_ms(100)
        self.led_on(7,2, incolor, is_hex=True)
        self.led_off(6,2)
        time.sleep_ms(100)
        self.led_on(6,2, incolor, is_hex=True)
        self.led_off(2,2)
        self.led_off(1,2)
        time.sleep_ms(100)
        self.led_on(2,2, incolor, is_hex=True)
        self.led_on(1,2, incolor, is_hex=True)
        time.sleep_ms(100)
        self.led_off(3,2)
        self.led_off(0,2)
        time.sleep_ms(100)
        self.led_on(3,2, incolor, is_hex=True)
        self.led_on(0,2, incolor, is_hex=True)
        self.led_off(4,2)
        time.sleep_ms(100)
        self.led_on(4,2, incolor, is_hex=True)

        self.led_off(5,2)
        self.led_off(1,3)
        self.led_off(0,0)
        time.sleep_ms(100)
        self.led_on(5,2, incolor, is_hex=True)
        self.led_on(1,3, incolor, is_hex=True)
        self.led_on(0,0, incolor, is_hex=True)      

        self.led_off(2,3)
        self.led_off(1,0)
        time.sleep_ms(100)
        self.led_on(2,3, incolor, is_hex=True)
        self.led_on(1,0, incolor, is_hex=True)

        self.led_off(2,0)
        self.led_off(3,0)
        self.led_off(4,0)
        self.led_off(5,0)
        self.led_off(6,3)
        self.led_off(0,3)
        self.led_off(3,3)
        time.sleep_ms(100)
        self.led_on(2,0, incolor, is_hex=True)
        self.led_on(3,0, incolor, is_hex=True)
        self.led_on(4,0, incolor, is_hex=True)
        self.led_on(5,0, incolor, is_hex=True)        
        self.led_on(6,3, incolor, is_hex=True)
        self.led_on(0,3, incolor, is_hex=True)
        self.led_on(3,3, incolor, is_hex=True)

        self.led_off(6,0)
        self.led_off(4,3)
        self.led_off(5,1)
        self.led_off(5,3)
        time.sleep_ms(100)
        self.led_on(6,0, incolor, is_hex=True)
        self.led_on(4,3, incolor, is_hex=True)
        self.led_on(5,1, incolor, is_hex=True)
        self.led_on(5,3, incolor, is_hex=True)

        self.led_off(7,0)
        self.led_off(4,1)
        time.sleep_ms(100)
        self.led_on(7,0, incolor, is_hex=True)
        self.led_on(4,1, incolor, is_hex=True)

        self.led_off(3,1)
        self.led_off(7,1)
        time.sleep_ms(100)
        self.led_on(3,1, incolor, is_hex=True)
        self.led_on(7,1, incolor, is_hex=True)


        self.led_off(2,1)
        self.led_off(6,1)
        time.sleep_ms(100)
        self.led_on(2,1, incolor, is_hex=True)
        self.led_on(6,1, incolor, is_hex=True)


        self.led_off(1,1)
        time.sleep_ms(100)
        self.led_on(1,1, incolor, is_hex=True)


        self.led_off(0,1)
        time.sleep_ms(100)
        self.led_on(0,1, incolor, is_hex=True)

        time.sleep_ms(100)


    def animation_decon_colors(self):
        self.clear()
        color_set = ["DC26BLUE", "DC26RED", "DC26GREEN"]
        color_index = 0
        for misc in range(20):
            time.sleep_ms(250)
            for col in range(4):
                self.led_on( self.LED_SETS['ALL'], col, self.color_codes[color_set[color_index % len(color_set)]],x_raw=True,is_hex=True,dim_per_value=1 )
                color_index += 1
            time.sleep_ms(1000)

    def animation_full_color_on(self):
        tcolor = self.get_random_color()
        findex = 150
        for x in range(8):
            for y in range(4):
                self.led_on(x, y, tcolor)
                time.sleep_ms(findex)

            findex = int(findex/1.25)       





###### Game Animations
    # 0: open passage - no leds at the end of the hall? open to ideas
    # 1: brick wall - LEDs 0, 1, 6, 7 lit up #b30000
    # 2: colored key - LED 2 lit up with the color in hallway_options (if InventoryManager has key, don't show in the hallway)
    # 3: locked door - LEDs 0, 1, 6, 7 lit up with color in hallway_options
    # 3: unlocked door - LEDs 0, 7 lit up with color in hallway_options

    def game_set_wall(self, loc, color):
        self.led_on( self.LED_SETS['TOP'], loc, color, x_raw=True )

    def game_set_wall_by_type(self, loc, ltype, lopt, has_key=False):
        if ltype == 1:
            self.led_on( self.LED_SETS['TOP'], loc, "RED", x_raw=True )
        elif ltype == 2:
            if not has_key:
              self.led_on( 2, loc, lopt, is_hex=True )
            else:
              pass
        elif ltype == 3:
            if not has_key:
              self.led_on( self.LED_SETS['TOP'], loc, lopt, x_raw=True, is_hex=True )
            else:
              self.led_on( 0, loc, lopt, is_hex=True )
              self.led_on( 1, loc, lopt, is_hex=True )
              self.led_on( 6, loc, lopt, is_hex=True )
              self.led_on( 7, loc, lopt, is_hex=True )
        elif ltype == 4:
            self.led_on( 0, loc, lopt )
            self.led_on( 7, loc, lopt )

    def game_enter_room_direction(self, loc):
        center_x = 3
        if loc == 1:
          center_x = 1
        elif loc == 2:
          center_x = 4

        self.game_clear_player_loc(0,3)

        self.led_off( center_x, 3)
        self.led_on( center_x, 3, "WHITE" )
        time.sleep_ms(300)
        self.led_off( center_x, 3)
        
        self.game_player_set_loc(0, 3)

    def game_move_direction(self, loc):
        # led right next to center
        center_x = 6
        if loc == 1:
          center_x = 5
        elif loc == 2:
          center_x = 2

        self.led_off( center_x, 3)
        self.led_on( center_x, 3, "WHITE" )
        time.sleep_ms(300)
        self.led_off( center_x, 3)

        for x in [5,4,3]:
            self.led_off( x, loc)
            self.led_on( x, loc, "WHITE" )
            time.sleep_ms(300)
            self.led_off( x, loc)
 #           self.led_on( x, loc, "WHITE", brightness=32 )
        time.sleep_ms(100)

#        for x in range(3, 6):
#            self.led_off( x, loc)
#            time.sleep_ms(25)

    def game_move_direction_back(self, loc):
        for x in [3,4,5]:
            self.led_off( x, loc)
            self.led_on( x, loc, "WHITE" )
            time.sleep_ms(300)
            self.led_off( x, loc)
#            self.led_on( x, loc, "WHITE", brightness=32 )

        # led right next to center
        center_x = 6
        if loc == 1:
          center_x = 5
        elif loc == 2:
          center_x = 2

        self.led_off( center_x, 3)
        self.led_on( center_x, 3, "WHITE" )
        time.sleep_ms(300)
        self.led_off( center_x, 3)

        time.sleep_ms(100)

#        for x in range(5, 2, -1):
#            self.led_off( x, loc)
#            time.sleep_ms(25)

    def game_enter_room(self, loc):
        # self.led_off()
        pass

    def game_led_on_temp(self, x, y, color, time_ms):
      self.led_on(x, y, color, is_hex=True)
      time.sleep_ms(time_ms)
      self.led_off(x, y)

    def game_animate_collect_key(self, color):
        #self.led_off(self.LED_SETS['ALL'], 3)
        self.game_led_on_temp(1, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(3, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)
        self.game_led_on_temp(4, 3, color, 50)
        self.game_led_on_temp(6, 3, color, 50)
        self.game_led_on_temp(1, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(3, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)
        self.game_led_on_temp(4, 3, color, 50)
        self.game_led_on_temp(6, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)
        self.game_led_on_temp(6, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)
        self.game_led_on_temp(6, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)
        self.game_led_on_temp(6, 3, color, 50)
        self.game_led_on_temp(2, 3, color, 50)
        self.game_led_on_temp(5, 3, color, 50)

    def game_player_center(self):
        self.game_player_set_loc(0, 3)

    def game_clear_player_center(self):
        self.game_clear_player_loc(0,3)

    def game_player_set_loc(self, x, y):
        self.led_on(x, y, dim_per_value=1.5)

    def game_clear_player_loc(self, x, y):
        self.led_off(x, y)










