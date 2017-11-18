#include <stdio.h>
#include <base/system.h>

static int round_to_int(float f)
{
	if(f > 0)
		return (int)(f+0.5f);
	return (int)(f-0.5f);
}


int main(int argc, char const *argv[])
{
	char aInput[9] = {0};
	if(argc < 2)
	{
		printf("Please enter a color in hex format: ");
		if(scanf("%s", aInput) != 1)
		{
			printf("error!\n");
			return 1;
		}
	}
	else
		str_copy(aInput, argv[1], sizeof(aInput));

	const char *pHexColor = aInput;
	if(pHexColor[0] == '#')
		pHexColor++;

    int HexColor = str_toint_base(pHexColor, 16);
    float r = (float)((HexColor & 0xFF0000) >> 16) / 255.0f;
    float g = (float)((HexColor & 0x00FF00) >> 8) / 255.0f;
    float b = (float)((HexColor & 0x0000FF) >> 0) / 255.0f;
    
    int ir = round_to_int(r * 35.0f);
    int ig = round_to_int(g * 35.0f);
    int ib = round_to_int(b * 35.0f);

    char cr = ir < 10 ? '0' + ir : 'A' + ir-10;
    char cg = ig < 10 ? '0' + ig : 'A' + ig-10;
    char cb = ib < 10 ? '0' + ib : 'A' + ib-10;

	printf("Converting #%06x\n", HexColor);
    printf("> float: %.3ff, %.3ff, %.3ff\n", r,g,b);
    //printf("debuginfo: %i %i %i\n", ir, ig, ib);
    printf("> ATH-ColorCode:  $$%c%c%c\n", cr, cg, cb);

    return 0;
}
