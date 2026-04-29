#include "ui/MainWindow.h"
#include "ui/Theme.h"
#include "io/FileManager.h"
#include "io/GifExporter.h"
#include "io/PngExporter.h"
#include "io/FileDialog.h"
#include "core/Frame.h"
#include "core/History.h"
#include <imgui.h>
#include <SDL3/SDL.h>
#include <string>

namespace Framenote {

static const char* FILTER_FRAMENOTE = "Framenote Files\0*.framenote\0All Files\0*.*\0";
static const char* FILTER_GIF       = "GIF Files\0*.gif\0All Files\0*.*\0";
static const char* FILTER_PNG       = "PNG Files\0*.png\0All Files\0*.*\0";

// Captures all frames + canvas size as a HistoryEntry to pass into undo/redo
static HistoryEntry captureCurrentEntry(DocumentTab* tab) {
    HistoryEntry entry;
    entry.canvasWidth  = tab->document->canvasSize().width;
    entry.canvasHeight = tab->document->canvasSize().height;
    for (int i = 0; i < tab->document->frameCount(); ++i) {
        auto& f = tab->document->frame(i);
        Snapshot s;
        s.frameIndex   = i;
        s.bufferWidth  = f.bufferWidth();
        s.bufferHeight = f.bufferHeight();
        s.pixels       = f.pixels();
        entry.frames.push_back(std::move(s));
    }
    return entry;
}

// Applies a restored HistoryEntry — handles both drawing and resize entries
static void applyHistoryEntry(DocumentTab* tab, HistoryEntry& entry) {
    if (entry.canvasWidth > 0 &&
        (entry.canvasWidth  != tab->document->canvasSize().width ||
         entry.canvasHeight != tab->document->canvasSize().height)) {
        tab->document->setCanvasSize(entry.canvasWidth, entry.canvasHeight);
        tab->renderer->resize(entry.canvasWidth, entry.canvasHeight);
        for (int i = 0; i < tab->document->frameCount(); ++i)
            tab->document->frame(i).setVisibleSize(entry.canvasWidth, entry.canvasHeight);
    }
    for (auto& snap : entry.frames)
        tab->document->frame(snap.frameIndex).restoreBuffer(
            std::move(snap.pixels), snap.bufferWidth, snap.bufferHeight);
    tab->document->markDirty();
}

MainWindow::MainWindow(TabManager* tabManager, ToolManager* toolManager)
    : m_tabManager(tabManager)
    , m_toolManager(toolManager)
{}

void MainWindow::render() {
    renderMenuBar();
    m_tabManager->render(*m_toolManager);
    renderCanvasSizeDialog();
    renderExportDialog();
    renderOnionOpacityDialog();

    // -- Global keyboard shortcuts ---------------------------------------------
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantTextInput) {
        bool ctrl  = io.KeyCtrl;
        bool shift = io.KeyShift;

        // Ctrl+N -- New document
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_N, false))
            m_tabManager->showNewDialog();

        // Ctrl+O -- Open file
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            std::string path = FileDialog::openFile(
                FILTER_FRAMENOTE, "Open Framenote File");
            if (!path.empty()) {
                std::string err;
                auto doc = FileManager::load(path, err);
                if (doc) {
                    std::string name = path.substr(path.find_last_of("/\\") + 1);
                    m_tabManager->openDocument(std::move(doc), name, path);
                } else {
                    m_statusMsg = "Open failed: " + err;
                }
            }
        }

        // Ctrl+S -- Save
        if (ctrl && !shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->document->isDirty()) {
                if (tab->document->filePath().empty()) {
                    std::string path = FileDialog::saveFile(
                        FILTER_FRAMENOTE, "framenote", "Save Framenote File");
                    if (!path.empty()) {
                        std::string err;
                        if (FileManager::save(*tab->document, path, err)) {
                            tab->document->setFilePath(path);
                            tab->document->clearDirty();
                            tab->name = path.substr(path.find_last_of("/\\") + 1);
                            m_statusMsg = "Saved!";
                        }
                    }
                } else {
                    std::string err;
                    if (FileManager::save(*tab->document, tab->document->filePath(), err)) {
                        tab->document->clearDirty();
                        m_statusMsg = "Saved!";
                    }
                }
            }
        }

        // Ctrl+Shift+S -- Save As
        if (ctrl && shift && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab) {
                std::string path = FileDialog::saveFile(
                    FILTER_FRAMENOTE, "framenote", "Save As");
                if (!path.empty()) {
                    std::string err;
                    if (FileManager::save(*tab->document, path, err)) {
                        tab->document->setFilePath(path);
                        tab->document->clearDirty();
                        tab->name = path.substr(path.find_last_of("/\\") + 1);
                        m_statusMsg = "Saved!";
                    }
                }
            }
        }

        // C -- Canvas Size
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab) {
                m_showCanvasSizeDialog = true;
                m_newCanvasW = tab->document->canvasSize().width;
                m_newCanvasH = tab->document->canvasSize().height;
            }
        }

        // O -- Toggle onion skin
        if (!ctrl && ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab)
                tab->timeline->setOnionSkin(!tab->timeline->onionSkinEnabled());
        }

        // Ctrl+Z -- Undo
        if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->history->canUndo()) {
                HistoryEntry current = captureCurrentEntry(tab);
                HistoryEntry restored = tab->history->undo(std::move(current));
                applyHistoryEntry(tab, restored);
            }
        }

        // Ctrl+Y / Ctrl+Shift+Z -- Redo
        if (ctrl && (ImGui::IsKeyPressed(ImGuiKey_Y, false) ||
                     (shift && ImGui::IsKeyPressed(ImGuiKey_Z, false)))) {
            auto* tab = m_tabManager->activeTab();
            if (tab && tab->history->canRedo()) {
                HistoryEntry current = captureCurrentEntry(tab);
                HistoryEntry restored = tab->history->redo(std::move(current));
                applyHistoryEntry(tab, restored);
            }
        }
    }
}

void MainWindow::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {

        // -- File --------------------------------------------------------------
        if (ImGui::BeginMenu("File")) {
            auto* tab    = m_tabManager->activeTab();
            bool hasTab  = tab != nullptr;
            bool isDirty = hasTab && tab->document->isDirty();

            if (ImGui::MenuItem("New", "Ctrl+N"))
                m_tabManager->showNewDialog();

            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                std::string path = FileDialog::openFile(
                    FILTER_FRAMENOTE, "Open Framenote File");
                if (!path.empty()) {
                    std::string err;
                    auto doc = FileManager::load(path, err);
                    if (doc) {
                        std::string name = path.substr(path.find_last_of("/\\") + 1);
                        m_tabManager->openDocument(std::move(doc), name, path);
                    } else {
                        m_statusMsg = "Open failed: " + err;
                    }
                }
            }

            ImGui::Separator();

            // Save -- only enabled when there are unsaved changes
            ImGui::BeginDisabled(!isDirty);
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (tab->document->filePath().empty()) {
                    std::string path = FileDialog::saveFile(
                        FILTER_FRAMENOTE, "framenote", "Save Framenote File");
                    if (!path.empty()) {
                        std::string err;
                        if (FileManager::save(*tab->document, path, err)) {
                            tab->document->setFilePath(path);
                            tab->document->clearDirty();
                            tab->name = path.substr(path.find_last_of("/\\") + 1);
                            m_statusMsg = "Saved!";
                        } else {
                            m_statusMsg = "Save failed: " + err;
                        }
                    }
                } else {
                    std::string err;
                    if (FileManager::save(*tab->document, tab->document->filePath(), err)) {
                        tab->document->clearDirty();
                        m_statusMsg = "Saved!";
                    } else {
                        m_statusMsg = "Save failed: " + err;
                    }
                }
            }
            ImGui::EndDisabled();

            // Save As -- enabled whenever a tab is open
            ImGui::BeginDisabled(!hasTab);
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                std::string path = FileDialog::saveFile(
                    FILTER_FRAMENOTE, "framenote", "Save As");
                if (!path.empty()) {
                    std::string err;
                    if (FileManager::save(*tab->document, path, err)) {
                        tab->document->setFilePath(path);
                        tab->document->clearDirty();
                        tab->name = path.substr(path.find_last_of("/\\") + 1);
                        m_statusMsg = "Saved!";
                    } else {
                        m_statusMsg = "Save failed: " + err;
                    }
                }
            }
            ImGui::EndDisabled();

            ImGui::Separator();

            // Export -- only when tab open
            ImGui::BeginDisabled(!hasTab);
            if (ImGui::BeginMenu("Export")) {
                if (ImGui::MenuItem("Export as GIF...")) {
                    m_exportType = ExportType::GIF;
                    m_showExportDialog = true;
                }
                if (ImGui::MenuItem("Export frame as PNG...")) {
                    m_exportType = ExportType::PNG;
                    m_showExportDialog = true;
                }
                if (ImGui::MenuItem("Export PNG sequence...")) {
                    m_exportType = ExportType::PNGSequence;
                    m_showExportDialog = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndDisabled();

            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                SDL_Event e; e.type = SDL_EVENT_QUIT;
                SDL_PushEvent(&e);
            }
            ImGui::EndMenu();
        }

        // -- Edit --------------------------------------------------------------
        if (ImGui::BeginMenu("Edit")) {
            auto* tab    = m_tabManager->activeTab();
            bool canUndo = tab && tab->history->canUndo();
            bool canRedo = tab && tab->history->canRedo();

            ImGui::BeginDisabled(!canUndo);
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                if (tab) {
                    HistoryEntry restored = tab->history->undo(captureCurrentEntry(tab));
                    applyHistoryEntry(tab, restored);
                }
            }
            ImGui::EndDisabled();

            ImGui::BeginDisabled(!canRedo);
            if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                if (tab) {
                    HistoryEntry restored = tab->history->redo(captureCurrentEntry(tab));
                    applyHistoryEntry(tab, restored);
                }
            }
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        // -- Sprite ------------------------------------------------------------
        if (ImGui::BeginMenu("Sprite")) {
            auto* tab   = m_tabManager->activeTab();
            bool hasTab = tab != nullptr;

            ImGui::BeginDisabled(!hasTab);
            if (ImGui::MenuItem("Canvas Size...", "C")) {
                m_showCanvasSizeDialog = true;
                m_newCanvasW = tab->document->canvasSize().width;
                m_newCanvasH = tab->document->canvasSize().height;
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        // -- View --------------------------------------------------------------
        if (ImGui::BeginMenu("View")) {
            const char* themeLabel = Theme::current() == ThemeMode::Dark
                ? "Switch to Light Mode" : "Switch to Dark Mode";
            if (ImGui::MenuItem(themeLabel)) Theme::toggle();

            ImGui::Separator();

            auto* tab   = m_tabManager->activeTab();
            bool hasTab = tab != nullptr;

            ImGui::BeginDisabled(!hasTab);
            if (hasTab) {
                bool onion = tab->timeline->onionSkinEnabled();
                if (ImGui::MenuItem("Onion Skin", "O", &onion))
                    tab->timeline->setOnionSkin(onion);
                if (ImGui::MenuItem("Onion Skin Opacity..."))
                    m_showOnionDialog = true;
            } else {
                ImGui::MenuItem("Onion Skin", "O");
                ImGui::MenuItem("Onion Skin Opacity...");
            }
            ImGui::EndDisabled();

            ImGui::EndMenu();
        }

        // -- Help --------------------------------------------------------------
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) m_showAbout = true;
            ImGui::EndMenu();
        }

        // Status bar
        auto* tab = m_tabManager->activeTab();
        if (tab) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImGui::TextDisabled("Frame %d/%d  |  %dx%d  |  %d fps  %s  %s",
                tab->timeline->currentFrame() + 1,
                tab->document->frameCount(),
                tab->document->canvasSize().width,
                tab->document->canvasSize().height,
                tab->timeline->fps(),
                tab->document->isDirty() ? "[unsaved]" : "",
                m_statusMsg.c_str());
        } else if (!m_statusMsg.empty()) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImGui::TextDisabled("%s", m_statusMsg.c_str());
        }

        ImGui::EndMainMenuBar();
    }

    // -- About modal -----------------------------------------------------------
    if (m_showAbout) ImGui::OpenPopup("About##modal");
    if (ImGui::BeginPopupModal("About##modal", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Framenote Studio v0.2.0");
        ImGui::Text("Mascot: Nibbit :)");
        ImGui::Separator();
        ImGui::Text("Built with SDL3 + Dear ImGui");
        if (ImGui::Button("Close")) {
            m_showAbout = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MainWindow::renderExportDialog() {
    if (!m_showExportDialog) return;

    auto* tab = m_tabManager->activeTab();
    if (!tab) { m_showExportDialog = false; return; }

    std::string err;

    if (m_exportType == ExportType::GIF) {
        std::string path = FileDialog::saveFile(FILTER_GIF, "gif", "Export as GIF");
        if (!path.empty()) {
            GifExporter::Options opts;
            opts.fps = tab->timeline->fps();
            if (GifExporter::exportGif(*tab->document, path, opts, err))
                m_statusMsg = "GIF exported!";
            else
                m_statusMsg = "Export failed: " + err;
        }
    }
    else if (m_exportType == ExportType::PNG) {
        std::string path = FileDialog::saveFile(FILTER_PNG, "png", "Export Frame as PNG");
        if (!path.empty()) {
            int fi = tab->timeline->currentFrame();
            if (PngExporter::exportFrame(*tab->document, fi, path, err))
                m_statusMsg = "PNG exported!";
            else
                m_statusMsg = "Export failed: " + err;
        }
    }
    else if (m_exportType == ExportType::PNGSequence) {
        std::string path = FileDialog::saveFile(FILTER_PNG, "png",
            "Export PNG Sequence (choose base name)");
        if (!path.empty()) {
            std::string base = path;
            if (base.size() > 4 && base.substr(base.size()-4) == ".png")
                base = base.substr(0, base.size()-4);
            if (PngExporter::exportSequence(*tab->document, base, err))
                m_statusMsg = "PNG sequence exported!";
            else
                m_statusMsg = "Export failed: " + err;
        }
    }

    m_showExportDialog = false;
}

void MainWindow::renderCanvasSizeDialog() {
    if (m_showCanvasSizeDialog) ImGui::OpenPopup("Canvas Size##dlg");

    if (ImGui::BeginPopupModal("Canvas Size##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        auto* tab = m_tabManager->activeTab();
        if (!tab) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

        ImGui::Text("Current: %dx%d",
            tab->document->canvasSize().width,
            tab->document->canvasSize().height);
        ImGui::Separator();
        ImGui::SetNextItemWidth(120); ImGui::InputInt("Width##cs",  &m_newCanvasW);
        ImGui::SetNextItemWidth(120); ImGui::InputInt("Height##cs", &m_newCanvasH);
        if (m_newCanvasW <    1) m_newCanvasW =    1;
        if (m_newCanvasH <    1) m_newCanvasH =    1;
        if (m_newCanvasW > 4096) m_newCanvasW = 4096;
        if (m_newCanvasH > 4096) m_newCanvasH = 4096;

        ImGui::Separator();
        ImGui::Text("Presets:");
        ImGui::SameLine(); if (ImGui::SmallButton("16"))  { m_newCanvasW=16;  m_newCanvasH=16;  }
        ImGui::SameLine(); if (ImGui::SmallButton("32"))  { m_newCanvasW=32;  m_newCanvasH=32;  }
        ImGui::SameLine(); if (ImGui::SmallButton("64"))  { m_newCanvasW=64;  m_newCanvasH=64;  }
        ImGui::SameLine(); if (ImGui::SmallButton("128")) { m_newCanvasW=128; m_newCanvasH=128; }
        ImGui::SameLine(); if (ImGui::SmallButton("256")) { m_newCanvasW=256; m_newCanvasH=256; }
        ImGui::Separator();

        if (ImGui::Button("OK", {80, 0})) {
            int oldW = tab->document->canvasSize().width;
            int oldH = tab->document->canvasSize().height;
            if (m_newCanvasW != oldW || m_newCanvasH != oldH) {
                // Snapshot all frames as a single atomic resize entry
                std::vector<Snapshot> frameSnaps;
                for (int i = 0; i < tab->document->frameCount(); ++i) {
                    auto& f = tab->document->frame(i);
                    Snapshot snap;
                    snap.frameIndex   = i;
                    snap.bufferWidth  = f.bufferWidth();
                    snap.bufferHeight = f.bufferHeight();
                    snap.pixels       = f.pixels();
                    frameSnaps.push_back(std::move(snap));
                }
                tab->history->pushResize(std::move(frameSnaps), oldW, oldH);

                for (int i = 0; i < tab->document->frameCount(); ++i) {
                    auto& frame = tab->document->frame(i);
                    frame.expandBuffer(m_newCanvasW, m_newCanvasH);
                    frame.setVisibleSize(m_newCanvasW, m_newCanvasH);
                }

                tab->document->setCanvasSize(m_newCanvasW, m_newCanvasH);
                tab->renderer->resize(m_newCanvasW, m_newCanvasH);
                tab->document->markDirty();
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

void MainWindow::renderOnionOpacityDialog() {
    if (m_showOnionDialog) ImGui::OpenPopup("Onion Skin Opacity##dlg");

    if (ImGui::BeginPopupModal("Onion Skin Opacity##dlg", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {

        auto* tab = m_tabManager->activeTab();
        if (!tab) { ImGui::CloseCurrentPopup(); ImGui::EndPopup(); return; }

        bool onion = tab->timeline->onionSkinEnabled();
        if (ImGui::Checkbox("Enabled", &onion))
            tab->timeline->setOnionSkin(onion);

        ImGui::Spacing();
        ImGui::Text("Opacity:");
        float opacityPct = tab->timeline->onionSkinOpacity() * 100.0f;
        ImGui::SetNextItemWidth(220);
        if (ImGui::SliderFloat("##onionpct", &opacityPct, 5.0f, 100.0f, "%.0f%%",
                ImGuiSliderFlags_AlwaysClamp))
            tab->timeline->setOnionSkinOpacity(opacityPct / 100.0f);

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Close", {80, 0})) {
            m_showOnionDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    m_showOnionDialog = false;
}

} // namespace Framenote