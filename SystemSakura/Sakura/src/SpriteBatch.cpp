#include "SpriteBatch.h"
#include "SakuraErrors.h"

#include <algorithm>

namespace Sakura {

Glyph::Glyph(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint Texture, float Depth, const ColorRGBA8& Color) : texture(Texture), depth(Depth){

	topLeft.color = Color;
	topLeft.setPosition(destRect.x, destRect.y + destRect.w);
	topLeft.setUV(uvRect.x, uvRect.y + uvRect.w);

	bottomLeft.color = Color;
	bottomLeft.setPosition(destRect.x, destRect.y);
	bottomLeft.setUV(uvRect.x, uvRect.y);

	bottomRight.color = Color;
	bottomRight.setPosition(destRect.x + destRect.z, destRect.y);
	bottomRight.setUV(uvRect.x + uvRect.z, uvRect.y);

	topRight.color = Color;
	topRight.setPosition(destRect.x + destRect.z, destRect.y + destRect.w);
	topRight.setUV(uvRect.x + uvRect.z, uvRect.y + uvRect.w);
}

Glyph::Glyph(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint Texture, float Depth, const ColorRGBA8& Color, float angle) : texture(Texture), depth(Depth){

	glm::vec2 halfDims(destRect.z / 2.0f, destRect.w / 2.0f);

	// Get points centered at origin
	glm::vec2 tl(-halfDims.x, halfDims.y);
	glm::vec2 bl(-halfDims.x, -halfDims.y);
	glm::vec2 br(halfDims.x, -halfDims.y);
	glm::vec2 tr(halfDims.x, halfDims.y);

	// Rotate the points
	tl = rotatePoint(tl, angle) + halfDims;
	bl = rotatePoint(bl, angle) + halfDims;
	br = rotatePoint(br, angle) + halfDims;
	tr = rotatePoint(tr, angle) + halfDims;

	topLeft.color = Color;
	topLeft.setPosition(destRect.x + tl.x, destRect.y + tl.y);
	topLeft.setUV(uvRect.x, uvRect.y + uvRect.w);

	bottomLeft.color = Color;
	bottomLeft.setPosition(destRect.x + bl.x, destRect.y + bl.y);
	bottomLeft.setUV(uvRect.x, uvRect.y);

	bottomRight.color = Color;
	bottomRight.setPosition(destRect.x + br.x, destRect.y + br.y);
	bottomRight.setUV(uvRect.x + uvRect.z, uvRect.y);

	topRight.color = Color;
	topRight.setPosition(destRect.x + tr.x, destRect.y + tr.y);
	topRight.setUV(uvRect.x + uvRect.z, uvRect.y + uvRect.w);
}

glm::vec2 Glyph::rotatePoint(const glm::vec2& pos, float angle){
	glm::vec2 newv;
	newv.x = pos.x * cos(angle) - pos.y * sin(angle);
	newv.y = pos.x * sin(angle) + pos.y * cos(angle);
	return newv;
}

SpriteBatch::SpriteBatch() : m_vbo(0), m_vao(0)
{
}


SpriteBatch::~SpriteBatch()
{
}

void SpriteBatch::dispose(){
	if (m_vao != 0){
		glDeleteVertexArrays(1, &m_vao);
		m_vao = 0;
	}
	if (m_vbo != 0){
		glDeleteBuffers(1, &m_vbo);
		m_vbo = 0;
	}
	m_glyphs.clear();
	m_glyphPointers.clear();
	m_renderBatches.clear();
}

void SpriteBatch::init() {
    createVertexArray();
}

void SpriteBatch::begin(GlyphSortType sortType /* GlyphSortType::TEXTURE */) {
    m_sortType = sortType;
    m_renderBatches.clear();
    // Have to delete any glyphs that remain so we don't have memory leaks!
    m_glyphs.clear();
}

void SpriteBatch::end() {
	//Set up all pointers for fast sorting
	m_glyphPointers.resize(m_glyphs.size());
	for (std::size_t i = 0; i < m_glyphs.size(); ++i){
		m_glyphPointers[i] = &m_glyphs[i];
	}
    sortGlyphs();
    createRenderBatches();
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

void SpriteBatch::draw(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, const ColorRGBA8& color) {
	//Create and emplace back glyphs
    m_glyphs.emplace_back(destRect, uvRect, texture, depth, color);
}

void SpriteBatch::draw(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, const ColorRGBA8& color, float angle){
	m_glyphs.emplace_back(destRect, uvRect, texture, depth, color, angle);
}

void SpriteBatch::draw(const glm::vec4& destRect, const glm::vec4& uvRect, GLuint texture, float depth, const ColorRGBA8& color, const glm::vec2& dir){
	const glm::vec2 right(1.0f, 0.0f);
	float angle = (acos(glm::dot(right, dir)));
	if (dir.y < 0.0f) angle = -angle;

	m_glyphs.emplace_back(destRect, uvRect, texture, depth, color, angle);
}

void SpriteBatch::renderBatch() {

    // Bind our VAO. This sets up the opengl state we need, including the 
    // vertex attribute pointers and it binds the VBO
	glBindVertexArray(m_vao);

	for (std::size_t i = 0; i < m_renderBatches.size(); i++) {
    glBindTexture(GL_TEXTURE_2D, m_renderBatches[i].texture);

    glDrawArrays(GL_TRIANGLES, m_renderBatches[i].offset, 
      m_renderBatches[i].numVertices);
  }

    glBindVertexArray(NULL);
}

void SpriteBatch::createRenderBatches() {
	if (m_glyphs.empty()) {
		return;
	}

    // This will store all the vertices that we need to upload
    std::vector <Vertex> vertices;
    // Resize the buffer to the exact size we need so we can treat
    // it like an array
	vertices.resize(m_glyphs.size() * 6);

    int offset = 0; // current offset
    int cv = 0; // current vertex

    //Add the first batch
	m_renderBatches.emplace_back(offset, 6, m_glyphPointers[0]->texture);
	vertices[cv++] = m_glyphPointers[0]->topLeft;
	vertices[cv++] = m_glyphPointers[0]->bottomLeft;
	vertices[cv++] = m_glyphPointers[0]->bottomRight;
	vertices[cv++] = m_glyphPointers[0]->bottomRight;
	vertices[cv++] = m_glyphPointers[0]->topRight;
	vertices[cv++] = m_glyphPointers[0]->topLeft;
    offset += 6;

    //Add all the rest of the glyphs
	for (std::size_t cg = 1; cg < m_glyphs.size(); cg++) {

        // Check if this glyph can be part of the current batch
		if (m_glyphPointers[cg]->texture != m_glyphPointers[cg - 1]->texture) {
            // Make a new batch
			m_renderBatches.emplace_back(offset, 6, m_glyphPointers[cg]->texture);
        } else {
            // If its part of the current batch, just increase numVertices
            m_renderBatches.back().numVertices += 6;
        }
		vertices[cv++] = m_glyphPointers[cg]->topLeft;
		vertices[cv++] = m_glyphPointers[cg]->bottomLeft;
		vertices[cv++] = m_glyphPointers[cg]->bottomRight;
		vertices[cv++] = m_glyphPointers[cg]->bottomRight;
		vertices[cv++] = m_glyphPointers[cg]->topRight;
		vertices[cv++] = m_glyphPointers[cg]->topLeft;
        offset += 6;
    }

    // Bind our VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	
    // Orphan the buffer (for speed)
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
	
    // Upload the data
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());

    // Unbind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	GLuint err = glGetError();
	if (err){
		printf("%i", err);
		SAKURA_THROW_FATAL("gl Error during initialization");
	}

}

void SpriteBatch::createVertexArray() {

    // Generate the VAO if it isn't already generated
	if (m_vao == 0) {
		glGenVertexArrays(1, &m_vao);
    }
    
    // Bind the VAO. All subsequent opengl calls will modify it's state.
	glBindVertexArray(m_vao);
	

    //Generate the VBO if it isn't already generated
	if (m_vbo == 0) {
		glGenBuffers(1, &m_vbo);
		
	}
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	

    //This is the position attribute pointer
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	
	glEnableVertexAttribArray(0);
    //This is the color attribute pointer
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	
	glEnableVertexAttribArray(1);
    //This is the UV attribute pointer
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	
	glEnableVertexAttribArray(2);

    glBindVertexArray(0);
	
	SAKURA_ASSERT_GL_ERROR(glGetError());
}

void SpriteBatch::sortGlyphs() {
	switch (m_sortType) {
        case GlyphSortType::BACK_TO_FRONT:
			std::stable_sort(m_glyphPointers.begin(), m_glyphPointers.end(), compareBackToFront);
            break;
        case GlyphSortType::FRONT_TO_BACK:
			std::stable_sort(m_glyphPointers.begin(), m_glyphPointers.end(), compareFrontToBack);
            break;
        case GlyphSortType::TEXTURE:
			std::stable_sort(m_glyphPointers.begin(), m_glyphPointers.end(), compareTexture);
            break;
    }
}

bool SpriteBatch::compareFrontToBack(Glyph* a, Glyph* b) {
    return (a->depth < b->depth);
}

bool SpriteBatch::compareBackToFront(Glyph* a, Glyph* b) {
    return (a->depth > b->depth);
}

bool SpriteBatch::compareTexture(Glyph* a, Glyph* b) {
    return (a->texture < b->texture);
}

}