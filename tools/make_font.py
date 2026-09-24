"""Regenerate the checked-in MyOS 8x16 font (requires Pillow and DejaVu)."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf',13)
data=bytearray()
for code in range(256):
    image=Image.new('1',(8,16));draw=ImageDraw.Draw(image)
    if code>=32 and code!=127:draw.text((0,0),bytes([code]).decode('cp437'),font=font,fill=1)
    for y in range(16):
        data.append(sum((1<<(7-x)) for x in range(8) if image.getpixel((x,y))))
(root/'assets').mkdir(exist_ok=True)
(root/'assets/myos-font.bin').write_bytes(data)
