#include <cstdio>
#include <cstdlib>
#include <base/system.h>
#include <engine/external/openssl/sha.h>

#define oups(i) { printf("oups %i\n", i); return i; }

int main(int argc, const char **argv)
{
#if !defined(CONF_PROTECT)
	printf("rly?\n");
#endif

	if(argc == 1)
	{
		printf("nope.\n");
		return 0;
	}

	IOHANDLE f = io_open(argv[1], IOFLAG_READ);
	if(!f) oups(1);
	unsigned int len = (unsigned int)io_length(f);
	char *aFile = new char[len];
	io_read(f, aFile, len);
	io_close(f);
	unsigned char md[SHA256_DIGEST_LENGTH] = {0};
	int ret = simpleSHA256(aFile, len, md);
	if(ret) oups(2);
	char aBuf[64];
	mem_zero(aBuf, sizeof(aBuf));
	str_copy(aBuf, argv[1], sizeof(aBuf));
	char dennis[] = {
			46, 100, 101, 110, 110, 105, 115, 0
	};
	str_append(aBuf, dennis, sizeof(aBuf));
	f = io_open(aBuf, IOFLAG_WRITE);
	if(!f) oups(3);
	io_write(f, md, SHA256_DIGEST_LENGTH);
	io_close(f);
	printf("yap.\n");
	return 0;
}