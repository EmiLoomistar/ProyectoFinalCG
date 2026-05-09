#pragma once
#include<glew.h>
class Texture
{
public:
	Texture();
	Texture(const char* FileLoc);
	bool LoadTexture();
	bool LoadTextureA();
	bool LoadTextureFromMemory(const unsigned char* data, unsigned int dataSize);
	bool LoadTextureFromRawPixels(const unsigned char* data, int w, int h);
	void UseTexture();
	void ClearTexture();
	~Texture();
private: 
	GLuint textureID;
	int width, height, bitDepth;
	const char *fileLocation;

};

