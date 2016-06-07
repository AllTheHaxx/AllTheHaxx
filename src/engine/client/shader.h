#ifndef ENGINE_CLIENT_SHADER_H
#define ENGINE_CLIENT_SHADER_H

#include <base/system.h>

#include <GL/glew.h>

class CShader
{
private:
	GLuint m_ShaderID;
	const char *m_pShaderName;

public:
	CShader(const char *pShaderName);
	~CShader();

	GLuint GetHandle() const { return m_ShaderID; }

/*	void Enable() const;
	void Disable() const;*/

private:
	GLuint Load();
};



#endif /* ENGINE_CLIENT_SHADER_H */
