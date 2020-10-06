import bitarray

def main():

    out =''

    with open('./out.log', 'r') as f:
        data = f.readlines()

    ctx = -1

    for l in data:
        if 'addls' in l:
            ctx = 8
        if ctx == 0:
            if 'blls' in l:
                out += '0'
            else:
                out += '1'
            print(l)
        ctx -= 1

    a = bitarray.bitarray(out)
    print(a.tobytes())





if __name__ == '__main__':
    main()
