"""Lossless diagnostic PPM->PNG conversion, no rendering or image alteration."""
from PIL import Image
import sys
picture=Image.open(sys.argv[1]).convert('RGB')
picture.save(sys.argv[2])
print(picture.size,sorted(picture.getcolors(picture.width*picture.height),reverse=True)[:8])
