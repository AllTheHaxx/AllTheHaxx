import io
import hashlib
import sys


def hash(f):
    data = f.read()
    datalen = len(data)
    f.close()

    result = hashlib.sha1("blob {}\0{}".format(datalen, data).encode()).hexdigest()
    return result


def main():
    infile = sys.stdin
    if len(sys.argv) >= 2:
        infile = io.open(sys.argv[1], "rb")

    print(hash(infile))
    return 0



if __name__ == '__main__':
    exit(main())
