#include "Texture.h"

Texture::Texture(const char* fileName) {

	loadTexture(fileName);
	initializeTexture();
}

Texture::Texture() {
    m_TextureID = 0;
	m_loaded = false;
	printf("No Texture Data Provided.");
}

bool Texture::loadTexture(const char* texFile) {
	m_TextureID = SOIL_load_OGL_texture(texFile, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
	if (!m_TextureID) {
		printf("Failed: Could not open texture file!\n");
		return false;
	}
	return true;
}

bool Texture::initializeTexture() {

	glBindTexture(GL_TEXTURE_2D, m_TextureID);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D,
		GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,          
		GL_TEXTURE_MAG_FILTER,
		GL_LINEAR);
	
	return true;
}



