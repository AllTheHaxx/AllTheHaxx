
#include "shader.h"
#include <fstream>
#include <string>

CShader::CShader(const char *pShaderName) : m_pShaderName(pShaderName)
{
	try
	{
		m_ShaderID = Load();
	}
	catch(const char* error)
	{
		dbg_msg("shader/FATAL", "failed to load shader '%s'", m_pShaderName);
		dbg_msg("shader/FATAL", "error: %s", error);
		//gui_messagebox("AllTheHaxx - shader error", error);
		dbg_break();
	};
}

CShader::~CShader()
{
	glDeleteProgram(m_ShaderID);
}

GLuint CShader::Load()
{
	char *pVertSrc, *pFragSrc;
	int VertLen, FragLen;

	// load the vertex shader source
	{
		const std::string path = std::string("data/shaders/") + std::string(m_pShaderName) + std::string(".vert"); // ik sis is haxxi but well... xd
		std::ifstream file; file.open(path.c_str(), std::ios::ate);
		VertLen = file.tellg();
		if(!file.is_open() || VertLen <= 0)
			throw "failed to open vertex source file";
		file.seekg(0, file.beg);
		pVertSrc = (char*)mem_alloc(VertLen+1, 0);
		mem_zero(pVertSrc, VertLen+1);
		file.read(pVertSrc, VertLen);
		file.close();
	}

	// load the fragment shader source
	{
		const std::string path = std::string("data/shaders/") + std::string(m_pShaderName) + std::string(".frag"); // ik sis is haxxi but well... xd
		std::ifstream file; file.open(path.c_str(), std::ios::ate);
		FragLen = file.tellg();
		if(!file.is_open() || FragLen <= 0)
			throw "failed to open fragment source file";
		file.seekg(0, file.beg);
		pFragSrc = (char*)mem_alloc(FragLen+1, 0);
		mem_zero(pFragSrc, FragLen+1);
		file.read(pFragSrc, FragLen);
		file.close();
	}

	GLuint program = glCreateProgram();
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	GLint result;

	// compile the vertex shader
	glShaderSource(vert, 1, &pVertSrc, &VertLen);
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE)
	{
		GLint length; GLsizei ErrLen = 0;
		glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &length);
		//GLchar *log = (GLchar*)mem_alloc(length+1, 0);
		GLchar log[4096];
		glGetShaderInfoLog(vert, 4096, &ErrLen, log);
		dbg_msg("shader/ERROR", "–––––––––––––––––––––––– COMPILATION LOG ––––––––––––––––––––––––");
		dbg_msg("shader/ERROR", "src=%p, len=%i, ErrLen=%i/%d, GL tells '%s'\n\n%s", pVertSrc, VertLen, ErrLen, length, glewGetErrorString(glGetError()), log);
		dbg_msg("shader/ERROR", "–––––––––––––––––––––––– COMPILATION LOG ––––––––––––––––––––––––");
		//mem_free(log);
		glDeleteShader(vert);
		throw "vertex shader could not compile";
	}

	// compile the fragment shader
	glShaderSource(frag, 1, &pFragSrc, &FragLen);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &result);
	if(result == GL_FALSE)
	{
		GLint length; GLsizei ErrLen = 0;
		glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &length);
		//GLchar *log = (GLchar*)mem_alloc(length+1, 0);
		GLchar log[4096];
		glGetShaderInfoLog(frag, 4096, &ErrLen, log);
		dbg_msg("shader/ERROR", "–––––––––––––––––––––––– COMPILATION LOG ––––––––––––––––––––––––");
		dbg_msg("shader/ERROR", "src=%p, len=%i, ErrLen=%i/%d, GL tells '%s'\n\n%s", pFragSrc, FragLen, ErrLen, length, glewGetErrorString(glGetError()), log);
		dbg_msg("shader/ERROR", "–––––––––––––––––––––––– COMPILATION LOG ––––––––––––––––––––––––");
		//mem_free(log);
		glDeleteShader(frag);
		throw "fragment shader could not compile";
	}

	// attach the shaders to the program and link it
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	// cleanups
	glDeleteShader(vert);
	glDeleteShader(frag);
	mem_free(pVertSrc);
	mem_free(pFragSrc);

	return program;
}
/*
void CShader::Enable() const
{
	glUseProgram(m_ShaderID);
}

void CShader::Disable() const
{
	glUseProgram(0);
}*/
