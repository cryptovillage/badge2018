import _thread
import badge
import hashlib
import machine
import network
import nvs
import ota
import time
import ujson
import urequests

import leds
from leds import LEDController

def RebootHandler():
    LEDController().animation_reboot()        
    machine.reset()

class Application:
    def __init__(self):
        self.cm = ConnectionManager()
        self.lm = LEDController()
        self.gm = LocationManager()

        self.game_mode = True

        # Need to track active press
        self.pressed = {
          2: False,
          3: False,
          4: False,
          5: False,
          6: False,
          7: False,
        }

        # Load settings here
    
    def start(self):
        _thread.start_new_thread(self.lm.start_animation_loop, ())
        self.cm.connect()
        while True:
            e = badge.get_event()
            if e.type() < 7:
                pass
            if e.type() == 7:
                pad_num = e.data()[0]
                self.pressed[pad_num] = True
                print("Pad {} pressed".format(pad_num))
                if self.pressed[3] and self.pressed[6]:
                  self.game_mode = not self.game_mode
                  self.lm.run = not self.game_mode
                  if self.game_mode:
                    self.lm.clear()
                    self.gm.draw_room()
                    self.gm.enter_room(0)
                  print("Toggle animation mode!\n")
            elif e.type() == 8:
                pad_num = e.data()[0]
                self.pressed[pad_num] = False
                print("Pad {} released".format(pad_num))
                if self.game_mode:
                  if pad_num == 2 or pad_num == 7:
                    self.gm.move_up()
                  elif pad_num == 3 or pad_num == 4:
                    self.gm.move_left()
                  elif pad_num == 5 or pad_num == 6:
                    self.gm.move_right()
                else:
                  self.lm.active_animation += 1

class UpdateThread():
    def __init__(self, cm):
        self.cm = cm
        
    def start(self):
        while not self.cm.is_online():
            # TODO: Find a different access point to try
            print("Not connected to the Internet...")
            time.sleep(60)
        # Wait for SNTP to get a valid time
        while time.time() < 1532246000:
            time.sleep(5)
        om = OTAManager()
        om.check_for_updates()
        if om.is_update_available():
            _thread.start_new_thread(LEDController().update_running, ())

            print("Updates available... Applying updates....")
            om.apply_update()
#            machine.reset()
            RebootHandler()

class ConnectionManager:
    def __init__(self):
        pass
    
    def connect(self):
        try:
            network.connect()
        except OSError:
            network.connect("badgenet", "VVQZp3NwRazmEu7Z")

    def reconnect_thread(self):
        time.sleep(30)
        self.connect()
                        
    def handle_event(self, e):
        if e.type() == 1:
            print("Connected to access point")
        elif e.type() == 2:
            print("Got IP address")
            _thread.start_new_thread(UpdateThread(self).start, ())
        elif e.type() == 3:
            print("Disconnected from access point... Will try to reconnect in 30 seconds")
            _thread.start_new_thread(self.reconnect_thread, ())
        elif e.type() == 4:
            print("WiFi scan done")

    def is_online(self):
        resp = urequests.get('http://connectivitycheck.gstatic.com/generate_204')
        return resp.status_code == 204
            
def bytes_to_hex(bytes):
    return ''.join('%02x' % b for b in bytes)

class OTAManager:
    _OTA_URL = 'https://2018.badge.cryptovillage.org/ota/'
    
    def __init__(self, url=_OTA_URL):
        self.ota_url = url
        self.update_metadata = None
        
    def check_for_updates(self):
        url = '{}?efm8app={:04x}&efm8boot={:04x}&espidf={}&app={}&partlabel={}'.format(
            self.ota_url, ota.get_efm8_app_crc(), ota.get_efm8_bootloader_crc(),
            ota.get_esp_idf_version(), ota.get_app_version(),
            ota.get_running_partition_label())
        resp = urequests.get(url)
        if resp.status_code == 200:
            self.update_metadata = resp.json()
    
    def is_update_available(self):
        try:
            return self.update_metadata['update_available']
        except:
            return False
        
    def apply_update(self):
        # TODO: Support gziped updates
        if 'efm8_bootloader' in self.update_metadata:
            data = urequests.get(self.update_metadata['efm8_bootloader']['url']).content
            hash = bytes_to_hex(hashlib.sha256(data).digest())
            if hash != self.update_metadata['efm8_bootloader']['sha256']:
                raise Exception("efm8 bootloader sha256 mismatch")
            ota.update_efm8_bootloader(data)
        if 'efm8_app' in self.update_metadata:
            data = urequests.get(self.update_metadata['efm8_app']['url']).content
            hash = bytes_to_hex(hashlib.sha256(data).digest())
            if hash != self.update_metadata['efm8_app']['sha256']:
                raise Exception("efm8 app sha256 mismatch")
            ota.update_efm8(data)
        if 'app' in self.update_metadata:
            # TODO: Support resuming interrupted downloads
            resp = urequests.get(self.update_metadata['app']['url'])
            hash = hashlib.sha256()
            total_size = self.update_metadata['app']['size']
            size_received = 0
            ota_handle = ota.begin(total_size)
            
            while size_received < total_size:
                data = resp.raw.read(2048)
                if len(data) == 0:
                    raise Exception("Unexpected end of update data")
                ota.write(ota_handle, data)
                hash.update(data)
                size_received += len(data)
            
            if bytes_to_hex(hash.digest()) != self.update_metadata['app']['sha256']:
                raise Exception("app sha256 mismatch")
            
            ota.end(ota_handle)
            resp.close()
            
            # FIXME: We should really do something more gentle here
            #machine.reset()
            # you can use RebootHandler()
    
    def update_user_files(self, url):
        resp = urequests.get(url)
        if resp.status_code != 200:
            print("Got status code {} while requesting URL {}".format(resp.status_code, url))
            return
        manifest_hash = bytes_to_hex(hashlib.sha256(resp.content).digest())
        try:
            f = open("/user/__MANIFEST_SHA256__", 'rt')
            if f.read() == manifest_hash:
                print("/user partition is up to date")
                f.close()
                return
            f.close()
        except:
            pass
        manifest = resp.json()
        resp.close()
        try:
            for filename in manifest:
                fileNeedsUpdate = True
                fullFilename = "/user/{}".format(filename)
                try:
                    f = open(fullFilename, 'rb')
                    # Read 2k at a time to minimize the amount of memory allocated
                    hash = hashlib.sha256()
                    while True:
                        data = f.read(2048)
                        if len(data) == 0:
                            if bytes_to_hex(hash.digest()) == manifest[filename]['sha256']:
                                fileNeedsUpdate = False
                            break
                        hash.update(data)
                    f.close()
                except:
                    pass
                if fileNeedsUpdate:
                    fileURL = manifest[filename]['url']
                    resp = urequests.get(fileURL)
                    if resp.status_code != 200:
                        print("Got status code {} while requesting URL {}".format(resp.status_code, fileURL))
                        return
                    f = open(fullFilename, 'wb')
                    while True:
                        data = resp.raw.read(2048)
                        if len(data) == 0:
                            break
                        f.write(data)
                    # Should we check the SHA256 here or just assume the manifest is correct?
                    f.close()
                    resp.close()
            f = open("/user/__MANIFEST_SHA256__", 'wt')
            f.write(manifest_hash)
            f.close()
        except Exception as e:
            print("An error occurred while updating the /user partition: ", e)    
        
class InventoryManager:
  def __init__(self):
    try:
      n = nvs.open("goldbug")
      self.inventory = ujson.loads(n.get_str("inventory"))
      n.close()
    except: 
      self.inventory = []
    
  def collect_key_color(self, color):
    if color not in self.inventory:
      self.inventory.append(color)
      n = nvs.open("goldbug")
      n.set_str("inventory", ujson.dumps(self.inventory))
      n.close()

  def has_key_color(self, color):
    return color in self.inventory

class LocationManager:
  def __init__(self):
    self.led_handler = leds.LEDController()
    self.led_handler.clear()
    self.led_handler.game_player_center()

    self.inventory_manager = InventoryManager()

    # Need to track active press
    self.pressed = {
      2: False,
      3: False,
      4: False,
      5: False,
      6: False,
      7: False,
    }

    self.go_to_home()
    
  def go_to_home(self):
    self.current_x = 5
    self.current_y = 5
    
    self.draw_room()
    self.enter_room(0)

  def current_location(self):
    return [self.current_x, self.current_y]
    
  def hallway_data_to_text(self, hall_type):
    if hall_type == 0:
      return 'Open Passage'
    elif hall_type == 1:
      return 'Brick Wall'
    elif hall_type == 2:
      return 'Key'
    elif hall_type == 3:
      return 'Door'
    else:
      return '???'

  def draw_room(self):
    location_data = self.get_location_data()

    location = self.current_location()
    print('Drawing room ' + str(location) + '\n')
    #print('Info about room: ' + str(location_data) + '\n')
    print('Straight ahead is a ' + self.hallway_data_to_text(location_data.get('t', -1)) + '\n')
    print('To the left is a ' + self.hallway_data_to_text(location_data.get('l', -1)) + '\n')
    print('To the right is a ' + self.hallway_data_to_text(location_data.get('r', -1)) + '\n')

    # TODO animate the room background and hallways
    # I think what we want to do here is clear all the leds and then draw what the room looks like.
    # Referencing docs/led_sketch.png in this repo for led numbers.
    # Hallway types:
    # 0: open passage - no leds at the end of the hall? open to ideas
    # 1: brick wall - LEDs 0, 1, 6, 7 lit up #b30000
    # 2: colored key - LED 2 lit up with the color in hallway_options (if InventoryManager has key, don't show in the hallway)
    # 3: locked door - LEDs 0, 1, 6, 7 lit up with color in hallway_options
    # 3: unlocked door - LEDs 0, 7 lit up with color in hallway_options

    ### LED Start
    self.led_handler.clear()

    self.led_handler.game_set_wall_by_type( 0, location_data['t'], location_data['to'], self.inventory_manager.has_key_color(location_data['to']) )
    self.led_handler.game_set_wall_by_type( 1, location_data['r'], location_data['ro'], self.inventory_manager.has_key_color(location_data['ro']) )
    self.led_handler.game_set_wall_by_type( 2, location_data['l'], location_data['lo'], self.inventory_manager.has_key_color(location_data['lo']) )
    #self.led_handler.game_player_center()

    ### LED STOP

    hallway_top_type = location_data['t']
    hallway_top_options = location_data['to']
    hallway_right_type = location_data['r']
    hallway_right_options = location_data['ro']
    hallway_left_type = location_data['l']
    hallway_left_options = location_data['lo']
    pass

  def enter_room(self, direction):
    # TODO animate entering current room from one of 3 directions
    print('Enter room from direction ' + str(direction) + '\n')
    ### LED Start
    #self.led_handler.clear()
    #Needed to redraw walls
    # self.draw_room()    # Causes the message to printed twice  ### FIX ###
    self.led_handler.game_enter_room_direction( direction )
    ### LED STOP


    # center group leds aren't numbered right now in docs/led_sketch.png

    # directions:
    # 0: traveling up - center group LED at 6 o'clock, then center
    # 1: traveling right - center group LED at 10 o'clock, then center
    # 2: traveling left - center group LED at 2 o'clock, then center

    # maybe your character has an animation halo that rotates around the center 3 LEDs?
    pass

  def get_location_data(self):
    location = self.current_location()
    # use location to get info about what's in this square
    # t: top hallway type. 0 = open passage, 1 = brick wall, etc
    # to: options for hallway type such as key color: "ff3366"
    # l: left hallway type
    # lo: left hallway options
    # r: right hallway type
    # ro: right hallway options

    map_tiles = {
"[5, 5]": {"l":2,"lo":0xcc00ff,"r":0,"ro":"","t":3,"to":0x3366ff},
"[5, 4]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[6, 5]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[7, 6]": {"l":0,"lo":"","r":0,"ro":"","t":3,"to":0xcc00ff},
"[7, 7]": {"l":0,"lo":"","r":0,"ro":"","t":3,"to":0xff9933},
"[6, 6]": {"l":0,"lo":"","r":0,"ro":"","t":3,"to":0xff3399},
"[5, 6]": {"l":0,"lo":"","r":2,"ro":0xff9933,"t":0,"to":""},
"[4, 4]": {"l":2,"lo":0x3366ff,"r":0,"ro":"","t":0,"to":""},
"[4, 3]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[3, 3]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[2, 3]": {"l":1,"lo":"","r":0,"ro":"","t":1,"to":""},
"[2, 4]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[3, 5]": {"l":0,"lo":"","r":2,"ro":0xff3399,"t":0,"to":""},
"[2, 5]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[3, 7]": {"l":0,"lo":"","r":2,"ro":0xff00ff,"t":0,"to":""},
"[2, 7]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[2, 6]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[4, 8]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[5, 9]": {"l":1,"lo":"","r":0,"ro":"","t":1,"to":""},
"[5, 8]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[6, 9]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[6, 8]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[7, 9]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[7, 8]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[9, 9]": {"l":3,"lo":0xff00ff,"r":1,"ro":"","t":0,"to":""},
"[6, 10]": {"l":1,"lo":"","r":1,"ro":"","t":2,"to":0xffff00},
"[7, 10]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[8, 10]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[9, 10]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[9, 8]": {"l":0,"lo":"","r":0,"ro":"","t":0,"to":""},
"[9, 7]": {"l":0,"lo":"","r":0,"ro":"","t":0,"to":""},
"[9, 6]": {"l":0,"lo":"","r":1,"ro":"","t":1,"to":""},
"[8, 6]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[8, 5]": {"l":1,"lo":"","r":0,"ro":"","t":1,"to":""},
"[7, 5]": {"l":0,"lo":"","r":0,"ro":"","t":3,"to":0x00ff0f},
"[6, 4]": {"l":0,"lo":"","r":1,"ro":"","t":1,"to":""},
"[3, 8]": {"l":1,"lo":"","r":1,"ro":"","t":0,"to":""},
"[7, 4]": {"l":0,"lo":"","r":0,"ro":"","t":2,"to":0x4f0000},
"[7, 3]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[6, 3]": {"l":1,"lo":"","r":0,"ro":"","t":1,"to":""},
"[8, 4]": {"l":0,"lo":"","r":1,"ro":"","t":1,"to":""},
"[10, 9]": {"l":0,"lo":"","r":3,"ro":0xffff00,"t":0,"to":""},
"[10, 8]": {"l":0,"lo":"","r":3,"ro":0xffff00,"t":0,"to":""},
"[10, 7]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[11, 7]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[12, 8]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[13, 9]": {"l":2,"lo":0xff0f00,"r":0,"ro":"","t":0,"to":""},
"[13, 8]": {"l":0,"lo":"","r":0,"ro":"","t":1,"to":""},
"[14, 9]": {"l":0,"lo":"","r":1,"ro":"","t":1,"to":""},
"[11, 10]": {"l":1,"lo":"","r":0,"ro":"","t":0,"to":""},
"[12, 10]": {"l":0,"lo":"","r":0,"ro":"","t":2,"to":0x00ff0f},
"[14, 10]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[12, 11]": {"l":1,"lo":"","r":1,"ro":"","t":0,"to":""},
"[13, 11]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
"[14, 11]": {"l":0,"lo":"","r":1,"ro":"","t":0,"to":""},
    }

    return map_tiles.get(str(location), {"l":0,"lo":"","r":0,"ro":"","t":0,"to":""})

  def walk_down_hall(self, direction):
    # TODO animate walk down hall in direction
    print('Walking down the hall in direction ' + str(direction) + '\n')

    ### LED Start
    self.led_handler.game_clear_player_center()
    self.led_handler.game_move_direction( direction )
    self.led_handler.game_player_set_loc(3, direction)
    ### LED STOP


    # sequentially light up LEDs 4, 5, 3, 2, then back to center. If we have a halo effect, maybe make it trail behind the main character.
    pass

  def walk_back_hall(self, direction):
    # TODO animate walking back from a wall or door
    print('Walking back the hall from direction ' + str(direction) + '\n')
    ### LED Start
    self.led_handler.game_player_set_loc(3, direction)    
    self.led_handler.game_move_direction_back( direction )
    self.led_handler.game_player_set_loc(0, 3) # back to center
    #Needed to redraw walls
    # self.draw_room() # Don't actually because room isn't changing. Flashes leds if redrawn.
    ### LED STOP

    # sequentially light up LEDs 3, 5, 4, then back to center. If we have a halo effect, maybe make it trail behind the main character.
    pass

  def move_in_dir(self, direction):
    location_data = self.get_location_data()
    hall_type = location_data['t']
    hall_options = location_data['to']

    if direction == 1:
      hall_type = location_data['r']
      hall_options = location_data['ro']
    elif direction == 2:
      hall_type = location_data['l']
      hall_options = location_data['lo']

    if hall_type == 0: # 0 is open passage
      self.walk_down_hall(direction)

      print('You walk through an open passage.\n')

      # update current room
      if direction == 0:
        self.current_y -= 1
      elif direction == 1:
        self.current_x += 1
        self.current_y += 1
      elif direction == 2:
        self.current_x -= 1

      self.draw_room()
      self.enter_room(direction)
    elif hall_type == 1: # 1 is brick wall
      self.walk_down_hall(direction)
      # don't change room
      print('You are faced with a brick wall and cannot go any further.\n')
      self.walk_back_hall(direction)
    elif hall_type == 2: # 2 is key
      self.walk_down_hall(direction)
      print('You found a key with color: ' + str(hall_options) + '\n')
      if not self.inventory_manager.has_key_color(hall_options):
        self.inventory_manager.collect_key_color(hall_options)
        self.led_handler.game_animate_collect_key(hall_options)
      
      print('You continue on through an open passage.\n')

      # update current room
      if direction == 0:
        self.current_y -= 1
      elif direction == 1:
        self.current_x += 1
        self.current_y += 1
      elif direction == 2:
        self.current_x -= 1

      self.draw_room()
      self.enter_room(direction)
      
    elif hall_type == 3: # 3 is locked door
      self.walk_down_hall(direction)
      if self.inventory_manager.has_key_color(hall_options):
        print('You come to a door and you have the key. You enter the next room\n')
        # update current room
        if direction == 0:
          self.current_y -= 1
        elif direction == 1:
          self.current_x += 1
          self.current_y += 1
        elif direction == 2:
          self.current_x -= 1

        self.draw_room()
        self.enter_room(direction)
      else:
        print('You come to a door but you don\'t have the key.\n')
        self.walk_back_hall(direction)

    pass

  def move_up(self):
    self.move_in_dir(0)

  def move_right(self):
    self.move_in_dir(1)

  def move_left(self):
    self.move_in_dir(2)
