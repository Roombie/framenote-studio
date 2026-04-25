#include "ui/MainWindow.h"
#include "ui/CanvasPanel.h"
#include "ui/TimelinePanel.h"
#include "ui/ToolsPanel.h"
#include "ui/PalettePanel.h"
#include "io/FileManager.h"
#include "io/GifExporter.h"
#include "core/Frame.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <string>

namespace Framenote {

MainWindow::MainWindow(Document* document, Timeline* timeline,
                       ToolManager* toolManager, CanvasRenderer* canvasRenderer)
    : m_document(document)
    , m_timeline(timeline)
    , m_toolManager(toolManager)
    , m_canvasRenderer(canvasRenderer)
{}

void MainWindow::render() {
    renderMenuBar();
    ToolsPanel(m_toolManager).render();
    PalettePanel(m_document).render();
    TimelinePanel(m_document, m_timeline).render();
    CanvasPanel(m_document, m_timeline, m_toolManager, m_canvasRenderer).render();
}

void MainWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New",  "Ctrl+N")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (!m_document->filePath().empty()) {
                    std::string err;
                    FileManager::save(*m_document, m_document->filePath(), err);
                }
            }
            if (ImGui::MenuItem("Save As...")) {}
            ImGui::Separator();
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("Export as GIF...")) {
                    std::string err;
                    GifExporter::Options opts;
                    opts.fps = m_timeline->fps();
                    GifExporter::exportGif(*m_document, "output.gif", opts, err);
                    m_statusMsg = err.empty() ? "GIF exported!" : "Export failed: " + err;
                }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                SDL_Event quitEvent;
                quitEvent.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&quitEvent);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Sprite")) {
            if (ImGui::MenuItem("Canvas Size...", "C")) {
                m_showCanvasSizeDialog = true;
                m_newCanvasW = m_document->canvasSize().width;
                m_newCanvasH = m_document->canvasSize().height;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            bool onion = m_timeline->onionSkinEnabled();
            if (ImGui::MenuItem("Onion Skin", "O", &onion))
                m_timeline->setOnionSkin(onion);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Framenote Studio"))
                m_showAbout = true;
            ImGui::EndMenu();
        }

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        ImGui::TextDisabled("Frame %d/%d  |  %dx%d  |  %d fps  %s",
            m_timeline->currentFrame() + 1,
            m_document->frameCount(),
            m_document->canvasSize().width,
            m_document->canvasSize().height,
            m_timeline->fps(),
            m_document->isDirty() ? "[unsaved]" : ""
        );

        ImGui::EndMainMenuBar();
    }

    if (m_showAbout) ImGui::OpenPopup("About##modal");
    if (ImGui::BeginPopupModal("About##modal", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Framenote Studio v0.1");
        ImGui::Text("Mascot: Nibbit the Pixel Bunny :)");
        ImGui::Separator();
        ImGui::Text("Built with SDL3 + Dear ImGui");
        if (ImGui::Button("Close")) {
            m_showAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    renderCanvasSizeDialog();
}

void MainWindow::renderCanvasSizeDialog() {
    if (m_showCanvasSizeDialog) ImGui::OpenPopup("Canvas Size##dlg");

    if (ImGui::BeginPopupModal("Canvas Size##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("Current size: %dx%d",
            m_document->canvasSize().width,
            m_document->canvasSize().height);
        ImGui::Separator();
        ImGui::Text("New size:");
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Width##cs",  &m_newCanvasW);
        ImGui::SetNextItemWidth(120);
        ImGui::InputInt("Height##cs", &m_newCanvasH);

        if (m_newCanvasW <    1) m_newCanvasW =    1;
        if (m_newCanvasH <    1) m_newCanvasH =    1;
        if (m_newCanvasW > 4096) m_newCanvasW = 4096;
        if (m_newCanvasH > 4096) m_newCanvasH = 4096;

        ImGui::Separator();
        ImGui::Text("Presets:");
        ImGui::SameLine(); if (ImGui::SmallButton("16x16"))   { m_newCanvasW=16;  m_newCanvasH=16;  }
        ImGui::SameLine(); if (ImGui::SmallButton("32x32"))   { m_newCanvasW=32;  m_newCanvasH=32;  }
        ImGui::SameLine(); if (ImGui::SmallButton("64x64"))   { m_newCanvasW=64;  m_newCanvasH=64;  }
        ImGui::SameLine(); if (ImGui::SmallButton("128x128")) { m_newCanvasW=128; m_newCanvasH=128; }
        ImGui::SameLine(); if (ImGui::SmallButton("256x256")) { m_newCanvasW=256; m_newCanvasH=256; }
        ImGui::Separator();

        if (ImGui::Button("OK", {80, 0})) {
            int oldW = m_document->canvasSize().width;
            int oldH = m_document->canvasSize().height;

            if (m_newCanvasW != oldW || m_newCanvasH != oldH) {
                // Expand frame buffers if growing — pixel data outside is NEVER deleted
                for (int i = 0; i < m_document->frameCount(); ++i)
                    m_document->frame(i).expandBuffer(m_newCanvasW, m_newCanvasH);

                // Update visible canvas size
                m_document->setCanvasSize(m_newCanvasW, m_newCanvasH);

                // Renderer texture must match the buffer size (max of old/new)
                int bufW = m_newCanvasW > oldW ? m_newCanvasW : oldW;
                int bufH = m_newCanvasH > oldH ? m_newCanvasH : oldH;
                m_canvasRenderer->resize(bufW, bufH);
                m_document->markDirty();
            }

            m_showCanvasSizeDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {80, 0})) {
            m_showCanvasSizeDialog = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void MainWindow::renderDockspace() {}

} // namespace Framenote
