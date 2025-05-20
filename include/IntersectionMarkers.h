#pragma once

#include "Vertex.h"
#include "Shader.h"

#include <vector>
#include <glad/glad.h>

class IntersectionMarkers {
private:
    std::vector<Vertex> points;
    unsigned int vao, vbo;
    float markerSize;
    bool buffersInitialized;

public:
    IntersectionMarkers(float size = 0.01f) :
        markerSize(size), buffersInitialized(false) {}

    ~IntersectionMarkers() {
        if (buffersInitialized) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
        }
    }

    void addPoint(const Vertex& point) {
        points.push_back(point);
        updateBuffers();
    }

    void clear() {
        points.clear();
        updateBuffers();
    }

    void updateBuffers() {
        // Initialize buffers if needed
        if (!buffersInitialized) {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            buffersInitialized = true;
        }

        // If there are no points, we're done
        if (points.empty()) return;

        // Create vertices for crosses (8 vertices per point)
        std::vector<Vertex> crossVertices;
        for (const auto& point : points) {
            // Horizontal line
            crossVertices.push_back({ point.x - markerSize, point.y });
            crossVertices.push_back({ point.x + markerSize, point.y });
            // Vertical line
            crossVertices.push_back({ point.x, point.y - markerSize });
            crossVertices.push_back({ point.x, point.y + markerSize });
        }

        // Update buffer data
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, crossVertices.size() * sizeof(Vertex),
            crossVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void draw(Shader& shader) const {
        if (points.empty() || !buffersInitialized) return;

        shader.use();
        shader.setColor("uColor", 1.0f, 0.0f, 0.0f, 1.0f);

        glBindVertexArray(vao);
        // Draw pairs of vertices as lines
        for (int i = 0; i < points.size() * 2; i++) {
            glDrawArrays(GL_LINES, i * 4, 2);     // Horizontal line
            glDrawArrays(GL_LINES, i * 4 + 2, 2); // Vertical line
        }

        glBindVertexArray(0); // Unbind to prevent side effects
    }
};