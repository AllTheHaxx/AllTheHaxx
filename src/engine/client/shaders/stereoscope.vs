const char *SOURCE_STEREOSCOPE_VERTEX =
"#version 150 \n"
"in vec3 in_Position;\n"
"in vec2 in_TexCoord;\n"
"out vec2 ex_TexCoord;\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(in_Position, 1.0);\n"
"	ex_TexCoord = in_TexCoord;\n"
"}\n";