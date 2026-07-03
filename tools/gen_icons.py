import struct, io
from PIL import Image

src = r"C:\Users\shawn\Downloads\icon.png"
out_dir = r"C:\.Codes\Knuckle\src\resources"
ico_path = f"{out_dir}\\icon.ico"

sizes = [16, 32, 48, 256]
img = Image.open(src).convert("RGBA")

# Generate PNGs for each size
png_data = {}
for s in sizes:
    resized = img.resize((s, s), Image.LANCZOS)
    buf = io.BytesIO()
    resized.save(buf, format="PNG")
    png_data[s] = buf.getvalue()
    resized.save(f"{out_dir}\\icon_{s}.png")

# Build ICO (direct PNG storage — modern ICO format)
count = len(sizes)
header = struct.pack("<HHH", 0, 1, count)
offset = 6 + count * 16
entries = []
for s in sizes:
    data = png_data[s]
    w = s if s < 256 else 0
    entries.append(struct.pack("<BBBBHHII", w, w, 0, 0, 1, 32, len(data), offset))
    offset += len(data)
with open(ico_path, "wb") as f:
    f.write(header)
    for e in entries:
        f.write(e)
    for s in sizes:
        f.write(png_data[s])

print(f"Generated {ico_path}")
for s in sizes:
    print(f"  icon_{s}.png ({len(png_data[s])} bytes)")
