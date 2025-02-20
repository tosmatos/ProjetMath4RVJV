#include "GUI.h"

#include <iostream>

#include "Clipper.h"

void GUI::DrawVertexInfoPanel(bool* open) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.3f);

    if (ImGui::Begin("Vertex Info", open,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (PolyBuilder::finishedPolygons.empty()) {
            ImGui::Text("No polygons.");
        }

        static ImVec4 red(1.0f, 0.0f, 0.0f, 1.0f);
        static ImVec4 green(0.0f, 1.0f, 0.0f, 1.0f);

        for (const auto& poly : PolyBuilder::finishedPolygons) {
            const auto& verts = poly.getVertices();

            // Show color swatch
            ImGui::ColorButton("##Color",
                (poly.type == PolyBuilder::POLYGON) ? red : green);
            ImGui::SameLine();
            ImGui::Text("%s:",
                (poly.type == PolyBuilder::POLYGON) ? "Polygon" : "Window");

            for (size_t i = 0; i < verts.size(); ++i) {
                ImGui::Text("  Vertex %d: (%.2f, %.2f)",
                    static_cast<int>(i + 1), verts[i].x, verts[i].y);
            }
            ImGui::Separator();
        }
    }
    ImGui::End();
}

void GUI::HandleContextMenu(bool* openContextMenu) {
    if (*openContextMenu) {
        ImGui::OpenPopup("ContextMenu");
        *openContextMenu = false;
    }

    if (ImGui::BeginPopup("ContextMenu")) {
        if (ImGui::MenuItem("Create Polygon")) {
            PolyBuilder::StartPolygon(PolyBuilder::POLYGON);
        }
        if (ImGui::MenuItem("Create Window")) {
            PolyBuilder::StartPolygon(PolyBuilder::WINDOW);
        }
        ImGui::Separator();
        if (PolyBuilder::buildingPoly) {
            if (ImGui::MenuItem("Cancel Current Build")) {
                PolyBuilder::Cancel();
            }
        }

        if (ImGui::MenuItem("Cyrus–Beck Clip All Polygons"))
        {
            Polygon windowPoly;
            bool foundWindow = false;
            for (auto& poly : PolyBuilder::finishedPolygons) {
                if (poly.type == PolyBuilder::WINDOW) {
                    windowPoly = poly;
                    foundWindow = true;
                    break;
                }
            }
            if (!foundWindow) {
                std::cout << "No window polygon to clip against!\n";
            } else {
                std::vector<Polygon> newPolygons;
                for (auto& poly : PolyBuilder::finishedPolygons) {
                    if (poly.type == PolyBuilder::POLYGON) {
                        // Clip the polygon
                        Polygon clipped = clipPolygonCyrusBeck(poly, windowPoly);
                        clipped.type = PolyBuilder::POLYGON;
                        clipped.updateBuffers();
                        newPolygons.push_back(clipped);
                    }
                    else {
                        newPolygons.push_back(poly);
                    }
                }
                // Replace old collection
                PolyBuilder::finishedPolygons = newPolygons;
            }
        }

        ImGui::EndPopup();
    }
}

void GUI::DrawHoverTooltip(GLFWwindow* window) {
    ImVec2 mousePos = ImGui::GetMousePos();
    int displayW, displayH;
    glfwGetFramebufferSize(window, &displayW, &displayH);

    // Convert mouse pos to NDC
    float ndcX = (2.0f * mousePos.x) / displayW - 1.0f;
    float ndcY = 1.0f - (2.0f * mousePos.y) / displayH;

    // Check proximity to vertices
    const float hoverRadius = 0.02f;
    for (const auto& poly : PolyBuilder::finishedPolygons) {
        for (const auto& vert : poly.getVertices()) {
            float dx = vert.x - ndcX;
            float dy = vert.y - ndcY;
            if (dx * dx + dy * dy < hoverRadius * hoverRadius) {
                ImGui::BeginTooltip();
                ImGui::Text("Position: (%.2f, %.2f)", vert.x, vert.y);
                ImGui::EndTooltip();
                return; // Only show the first match
            }
        }
    }
}
