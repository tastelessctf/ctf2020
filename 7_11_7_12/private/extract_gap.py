import sys

class Header:
    kEnd = 0x00
    kHeader = 0x01
    kArchiveProperties = 0x02
    kAdditionalStreamsInfo = 0x03
    kMainStreamsInfo = 0x04
    kFilesInfo = 0x05
    kPackInfo = 0x06
    kUnPackInfo = 0x07
    kSubStreamsInfo = 0x08
    kSize = 0x09
    kCRC = 0x0A
    kFolder = 0x0B
    kCodersUnPackSize = 0x0C
    kNumUnPackStream = 0x0D
    kEmptyStream = 0x0E
    kEmptyFile = 0x0F
    kAnti = 0x10
    kName = 0x11
    kCTime = 0x12
    kATime = 0x13
    kMTime = 0x14
    kWinAttributes = 0x15
    kComment = 0x16
    kEncodedHeader = 0x17
    kStartPos = 0x18
    kDummy = 0x19

    def __init__(self, data):
        self.data = data

    def get_fixed(self, num_bytes):
        fixed = self.data[:num_bytes]
        self.data = self.data[num_bytes:]
        return int.from_bytes(fixed, byteorder='little')

    def get_encoded(self):
        if not self.data:
            raise Exception("unexpected end of data")

        extra_bytes = 0
        top_byte = self.get_fixed(1)
        bit_test = 0x80

        while bit_test:
            if top_byte & bit_test == 0:
                break
            top_byte &= ~bit_test
            bit_test >>= 1
            extra_bytes += 1

        if len(self.data) < extra_bytes:
            raise Exception("unexpected end of data")

        shift = 0
        retval = 0
        while shift < 8 * extra_bytes:
            b = self.get_fixed(1)
            retval |= b << shift
            shift += 8

        if shift < 64:
            retval |= top_byte << shift

        return retval

    def get_property(self):
        return self.get_fixed(1)

    def check_marker(self, marker):
        if self.get_fixed(1) != marker:
            raise Exception("marker not expected")

    def parse_properties(self):
        while self.data:
            prop_type = self.get_property()
            if prop_type == self.kEnd:
                return
            prop_size = self.get_encoded()
            self.data = self.data[prop_size:]
        raise Exception("unexpected properties end")

    def parse_pack_info(self):
        next_header_offset = self.get_encoded()
        num_streams = self.get_encoded()

        self.check_marker(self.kSize)

        for i in range(num_streams):
            next_header_offset += self.get_encoded()

        self.check_marker(self.kEnd)

        return next_header_offset

    def parse_streams_info(self):
        self.check_marker(self.kPackInfo)
        return self.parse_pack_info()

    def parse_header(self):
        while(self.data):
            prop = self.get_property()
            if prop == self.kArchiveProperties:
                self.parse_properties()
            elif prop == self.kAdditionalStreamsInfo:
                pass
#                raise Exception(f"unexpected property {prop}")
            elif prop == self.kMainStreamsInfo:
                return self.parse_streams_info()
        raise Exception("unexpected end of header")

    def parse_next_header(self):
        prop = self.get_property()
        if prop == self.kHeader:
            return self.parse_header()
        elif prop == self.kEncodedHeader:
            return self.parse_streams_info()

        raise Exception("unexpected header property {prop}")

def ul(b):
    return int.from_bytes(b, byteorder='little')

if len(sys.argv) < 3:
    print("usage: extract_gap 7zip_file outfile")
    exit(1)

with open(sys.argv[1], "rb") as f:
    signature = f.read(6)
    if signature != b"7z\xBC\xAF\x27\x1C":
        print("not a 7zip file")
        exit(1)
    version = f.read(2)
    crc = f.read(4)
    next_header_offset = ul(f.read(8))
    next_header_size = ul(f.read(8))
    next_header_crc = f.read(4)

    f.seek(0x20 + next_header_offset)   # headers size + offset
    next_header = f.read(next_header_size)
    if len(next_header) < next_header_size:
        print("corrupted 7zip file")
        exit(1)

    original_offset = Header(next_header).parse_next_header()

    with open(sys.argv[2], "wb") as out:
        f.seek(0x20 + original_offset)  # starting headers size + offset
        gap_size = next_header_offset - original_offset
        out.write(f.read(gap_size))
