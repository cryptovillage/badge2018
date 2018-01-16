# Bling emulator

This is a bling emulator that renders the effect of playing a .RAW animation and a .RGB LED bling file on a badge. It was specifically written for the Trans-Ionospheric badge but should be adaptable to other badges which use the same base platform (which is the AND!XOR DC25 Bender Badge). TO do that you'll need to change the configuration that's defined at the top of the source file.

The emulator does not emulate the menu system or bling previews.

```
Usage:
renderbling -master=<filename> -bling=<filename> -rgb=<filename> -out=<filename> [-frames=<numframes>]

Where:    master  = filename for background image in .RAW format [REQUIRED]
            bling = filename for LCD animation in .RAW format [OPTIONAL]
              rgb = filename for LED bling in .RGB format [REQUIRED]
              out = output filename [REQUIRED]
           frames = number of frames to render (defaults to 200)
```
		   
# Example usage

```
go run renderbling.go -master=CPV.RAW -rgb=BOUNCE.RGB -out=OUTPUT.RAW
```

To view the output:
```
./playbadge ./OUTPUT.RAW
```

# To customize this for other badges

Create a single frame .RAW image of the badge, at 100 Pixels/Inch scale
Determine the x,y values for the upper left origin of the LCD display within that image.

Determine the x,y values for the center of each LED on the badge, in the order that they are serially wired 

Update the source code with these values.

You'll also need to change the playbadge script to the new dimensions of the master file.

To generate background raw image with ffmpeg/avconv:
```
avconv -i trans-ionospheric_proto_front.png -vcodec rawvideo -f rawvideo -pix_fmt rgb565be TIBASE.RAW
```
