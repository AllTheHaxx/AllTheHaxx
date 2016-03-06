const char *SOURCE_POST_FRAGMENT =
"precision mediump float;\n"
"uniform float time;\n"
"uniform vec2 resolution;\n"
"uniform float u_IsTex;\n"
"uniform sampler2D u_Tex;\n"
"varying vec4 v_Color;\n"
"#define LINES 3.0\n"
"#define BRIGHTNESS 0.6\n"
"const vec3 ORANGE = vec3(0.0, 0.2, 0.8);\n" 			// left 	2
"const vec3 BLUE = vec3(0.0, 1.2, 1.2);\n" 				// right 	2
"const vec3 GREEN = vec3(0.0, 1.2, 1.2);\n" 			// left 	1
"const vec3 RED = vec3(0.0, 0.2, 0.8);\n" 				// right 	1
/*
"const vec3 ORANGE = vec3(1.4, 0.8, 0.4);\n"
"const vec3 BLUE = vec3(0.5, 0.9, 1.3);\n"
"const vec3 GREEN = vec3(0.9, 1.4, 0.4);\n"
"const vec3 RED = vec3(1.8, 0.4, 0.3);\n"
*/
"void main() {\n"
"	float x, y, xpos, ypos;\n"
"	float t = time * 10.0;\n"
"	vec3 c = vec3(0.25);\n"
"	xpos = (gl_FragCoord.x / resolution.x);\n"
"	ypos = (gl_FragCoord.y / resolution.y);\n"
"	x = xpos;\n"
"	for (float i = 0.0; i < LINES; i += 1.0) {\n"
"		for(float j = 0.0; j < 2.0; j += 1.0)\n"
"		{\n"
"			y = ypos\n"
"			+ (0.30 * sin(x * 2.000 +( i * 1.5 + j) * 0.2 + t * 0.050)\n" 
"			+ 0.300 * cos(x * 6.350 + (i  + j) * 0.2 + t * 0.050 * j)\n"
"			+ 0.024 * sin(x * 12.35 + ( i + j * 4.0 ) * 0.8 + t * 0.034 * (8.0 *  j))\n"
"			+ 0.5);\n"
"			c += vec3(1.0 - pow(clamp(abs(1.0 - y) * 5.0, 0.0,1.0), 0.25));\n"
"			}\n" 
"		}\n" 
"c *= mix(\n"
"mix(ORANGE, BLUE, xpos)\n"
", mix(GREEN, RED, xpos)\n"
",(sin(t * 0.02) + 1.0) * 0.45\n"
") * BRIGHTNESS;\n"
"gl_FragColor = vec4(c, 1.0);\n"
"}\n";