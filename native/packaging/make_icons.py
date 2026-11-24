import sys
from pathlib import Path
from struct import pack
from zlib import compress, crc32


def inside_round_rect(x, y, left, top, right, bottom, radius):
    if not (left <= x <= right and top <= y <= bottom):
        return False
    dx = max(left + radius - x, x - (right - radius), 0)
    dy = max(top + radius - y, y - (bottom - radius), 0)
    return dx * dx + dy * dy <= radius * radius


def pixel(x, y):
    if not inside_round_rect(x, y, 4, 4, 60, 60, 13):
        return 0, 0, 0, 0
    if inside_round_rect(x, y, 13, 15, 51, 49, 6):
        if y < 24:
            if 18 <= x <= 20 and 19 <= y <= 21:
                return 211, 224, 255, 255
            return 100, 107, 245, 255
        if 18 <= x <= 26 and 27 <= y <= 45:
            return 56, 142, 233, 255
        if 30 <= x <= 46 and (27 <= y <= 34 or 38 <= y <= 45):
            return 155, 173, 250, 255
        return 27, 38, 71, 255
    return 16, 24, 43, 255


def chunk(name, data):
    return pack(">I", len(data)) + name + data + pack(">I", crc32(name + data))


def png(side):
    data = bytearray()
    for y in range(side):
        data.append(0)
        for x in range(side):
            samples = [pixel((x + sx) * 64 / side, (y + sy) * 64 / side)
                       for sx in (0.25, 0.75) for sy in (0.25, 0.75)]
            alpha = sum(p[3] for p in samples)
            if not alpha:
                data.extend((0, 0, 0, 0))
            else:
                data.extend(round(sum(p[c] * p[3] for p in samples) / alpha) for c in range(3))
                data.append(round(alpha / len(samples)))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", pack(">IIBBBBB", side, side, 8, 6, 0, 0, 0))
            + chunk(b"IDAT", compress(data, 9)) + chunk(b"IEND", b""))


def write_icons(destination):
    destination.mkdir(parents=True, exist_ok=True)
    icons = [(size, png(size)) for size in (16, 24, 32, 48, 64, 128, 256)]
    (destination / "imgui-studio.png").write_bytes(icons[-1][1])
    directory = bytearray(pack("<HHH", 0, 1, len(icons)))
    offset = 6 + 16 * len(icons)
    for size, data in icons:
        directory.extend(pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32, len(data), offset))
        offset += len(data)
    (destination / "imgui-studio.ico").write_bytes(directory + b"".join(data for _, data in icons))
    parts = b"".join(kind + pack(">I", len(data) + 8) + data for kind, data in
                     [(b"ic07", icons[-2][1]), (b"ic08", icons[-1][1]), (b"ic09", png(512))])
    (destination / "imgui-studio.icns").write_bytes(b"icns" + pack(">I", len(parts) + 8) + parts)


if __name__ == "__main__":
    write_icons(Path(sys.argv[1]))
